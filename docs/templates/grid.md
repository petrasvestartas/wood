# Grid {#templates_grid}

[TOC]

Multi-storey timber building generator in `src/templates/grid/*.h`. `wood_grid::Grid` frames a building level by level from a `Building` guide and a `Framing` specification.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#grid_ex_tree">
    <img src="3_elements_tree_all.webp" alt="Five-Bay Building">
    <span class="wood-card-title">Five-Bay Building</span>
  </a>
  <a class="wood-card" href="#grid_ex_footprint">
    <img src="templates_grid_1_footprint_all.webp" alt="Footprints and Patterns">
    <span class="wood-card-title">Footprints and Patterns</span>
  </a>
  <a class="wood-card" href="#grid_ex_solid">
    <img src="templates_grid_2_solid_all.webp" alt="Massing Solids">
    <span class="wood-card-title">Massing Solids</span>
  </a>
  <a class="wood-card" href="#grid_ex_lines">
    <img src="templates_grid_3_lines_all.webp" alt="Lines and Braced Frames">
    <span class="wood-card-title">Lines and Braced Frames</span>
  </a>
  <a class="wood-card" href="#grid_ex_reference">
    <img src="templates_grid_4_reference_all.webp" alt="Reference Buildings">
    <span class="wood-card-title">Reference Buildings</span>
  </a>
  <a class="wood-card" href="#grid_ex_framing">
    <img src="templates_grid_5_framing_all.webp" alt="Framing Systems">
    <span class="wood-card-title">Framing Systems</span>
  </a>
</div>

```cpp
const wood_grid::Building building = wood_grid::Building::from_footprint(
    FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)
);
wood_grid::Grid grid(building, FRAMING, "elements_tree");
```

---

## Primary Inputs

| Primary Input | Factories / Fields | Geometric Role |
| --- | --- | --- |
| `Pattern` | `orthogonal(xs, ys, skew)` · `radial(radii, sectors, sweep)` · `triangular` · `hexagonal` · `from_lines` | Defines 2D plan grid axes clipped against each level boundary |
| `Building` | A: `from_solid(massing, elevations, pattern)`<br>B: `from_footprint(rings, elevations, pattern)`<br>C: `from_lines(lines, surfaces)` | Produces `levels`, where each level has datum `z`, `cores`, and a planar `plan` `Mesh` (faces = bays, edges = member axes, vertices = column nodes) |
| `Framing` | `system` (`0` point-supported, `1` post-and-beam, `2` purlin-on-girder) · `span` · `node` (`0` head, `1` flush, `2` column-through) · `drop` · `deck` · `wall` · `head` · `reach` · `capital` · `profiles` | Controls structural hierarchy, node priority/head geometry, and cross-section `Polyline` profiles (`wood_profile.h`) per member role |

### Plan Mesh Attributes

| Mesh Entity | Attribute | Values & Meaning |
| --- | --- | --- |
| Face (bay) | `system`, `span`, `floor` | Local framing system (`0..2`), girder direction, and slab presence (`1.0` = decked bay) |
| Edge (axis) | `role` | `0.0` none · `1.0` girder · `2.0` beam · `3.0` purlin · `4.0` brace |
| Edge (axis) | `wall` | `0.0` none · `1.0` perimeter facade wall · `2.0` shear-core wall |
| Vertex (node) | `column` | `0.0` none · `1.0` standard column · `2.0` transfer column |

```
grid
├── level_0
│   ├── plan_0                     plan lines and column points at ground datum
│   └── decks_0                    Plate deck_<i>_0 (ground slabs)
└── level_1                        level_2 .. level_N (each capping the storey below)
    ├── plan_1
    ├── columns_1                  Column column_<i>_1
    ├── walls_1, core_walls_1      Plate wall_<i>_1, core_wall_<i>_1
    ├── heads_1, drop_panels_1     Block head_<i>_1, drop_panel_<i>_1
    ├── girders_1, beams_1         Beam girder_<i>_1, beam_<i>_1
    ├── purlins_1                  Beam purlin_<i>_1
    ├── decks_1                    Plate deck_<i>_1
    └── braces_1                   Beam brace_<i>_1
```

---

## Geometry Generation Pipeline

```cpp
Grid::Grid(const Building& guide, const Framing& framing, const std::string& name)
    : WoodSession(name),
      framing(compute_framing(framing)),
      guide(compute_guide(guide, this->framing)) {

    add_plans();
    for (size_t storey = 0; storey + 1 < this->guide.levels.size(); storey++)
        add_storey(storey);
    compute_face_contacts(0);
}
```

### Level Plans

Every level's `plan` mesh edges and column vertices are lifted to datum `level.z` and placed in `plan_<level>`:

![The plans](templates/3_elements_tree_plan_2.webp)

```cpp
for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
    const Point start = compute_lift(*level.plan.vertex_point(edge.first), level.z);
    const Point end = compute_lift(*level.plan.vertex_point(edge.second), level.z);
    add_line(Line::from_points(start, end), plan);
}
```

### Columns

A `Column` stands on every stacked column vertex from the lower slab top (`foot`) to the upper column head underside or level datum (`top`):

![The columns](templates/3_elements_tree_columns_1.webp)

```cpp
for (const build::Stack& stack : stacks) {
    const double foot = lower.z + (framing.node == 2 ? 0.0 : build::compute_floor(lower.plan, stack.lower, framing));
    const double top = level.z + (framing.node == 0 ? build::compute_head_bottom(upper, stack.upper) : 0.0);
    const std::vector<Point>& section = upper.standing.at(stack.upper);
    columns.push_back(build::to_column(stack, section, foot, top));
}
add_numbered(columns, storey + 1);
```

### Facade & Core Walls

Edges carrying `wall == 1.0` generate perimeter `wall` plates under the edge beams; edges carrying `wall == 2.0` generate pinwheel `core_wall` plates around core openings:

![The facade walls](templates/templates_grid_1_footprint_walls_1.webp)

```cpp
for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
    const double wall = level.plan.edge_attribute(edge, "wall").value_or(0.0);
    if (wall == 0.0)
        continue;
    walls.push_back(build::to_wall(upper, edge, bottom, level.z, wall == 2.0 ? "core_wall" : "wall"));
}
```

### Column Heads & Drop Panels

When `framing.node == 0`, each column top receives a chamfered pyramidal, conical, or stepped `head` block supporting the incoming girders/beams or point-supported slab:

![The heads](templates/3_elements_tree_heads_1.webp)

```cpp
for (const build::Stack& stack : stacks) {
    const double bottom = level.z + build::compute_head_bottom(upper, stack.upper);
    const double top = level.z + build::compute_head_top(upper, stack.upper);
    const std::vector<std::shared_ptr<Element>> head = build::to_head(upper, level, stack.upper, bottom, top);
    heads[stack.upper] = head[0];
    blocks.insert(blocks.end(), head.begin(), head.end());
}
```

### Girders, Beams & Bay Purlins

Plan edges with `role > 0` become `girder` or `beam` members with end cuts determined by node priority. In `system == 2` bays, `compute_stations` populates secondary `purlin` members across the bay at `framing.spacing`:

![The beams](templates/3_elements_tree_beams_1.webp)

```cpp
for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
    if (level.plan.edge_attribute(edge, "role").value_or(0.0) <= 0.0)
        continue;
    const std::vector<std::shared_ptr<Element>> beams = build::to_beam(upper, edge, level.z, outer);
    members.insert(members.end(), beams.begin(), beams.end());
}
```

![The girders and purlins](templates/templates_grid_1_footprint_purlins_1.webp)

```cpp
for (const build::Station& station : build::compute_stations(upper, face, level.cores)) {
    const std::vector<std::shared_ptr<Element>> purlins = build::to_member(
        station.line.start(), station.line.end(), level.z - size.second / 2.0,
        profile, station.cuts, "purlin", size.first
    );
    members.insert(members.end(), purlins.begin(), purlins.end());
}
```

### Bay Decks

Every floored bay receives a `deck` plate; flush decks boolean-subtract corner column head pyramids via `InteractionFeatureSolid`:

![The decks](templates/3_elements_tree_decks_1.webp)

```cpp
for (const size_t vertex : build::compute_loop(level.plan, outline.first)) {
    if (!heads.count(vertex))
        continue;
    const Mesh pyramid = build::compute_pyramid(context, vertex, bottom, top, level.z);
    for (const std::shared_ptr<Element>& deck : decks) {
        const auto cut = std::make_shared<wood_session::InteractionFeatureSolid>(pyramid, wood_session::SolidOperation::subtract);
        add_interaction(heads.at(vertex), deck, cut);
    }
}
```

### Storey Braces & Contacts

Diagonal braces are clipped against column faces, lower decks, and upper beams, and `compute_face_contacts(0)` registers all face-to-face contacts across the building:

![The braced frame](templates/templates_grid_3_lines_braced.webp)

```cpp
const std::vector<std::shared_ptr<Element>> brace = build::to_brace(line, ground, upper, z_lower, z_upper);
braces.insert(braces.end(), brace.begin(), brace.end());
```

---

## Node & Section Variations

![Heads the girders rest on](templates/templates_grid_5_framing_head_section.webp)

![A stepped head](templates/templates_grid_5_framing_head_stepped.webp)

![Columns through](templates/templates_grid_5_framing_node_through.webp)

![Purlins hung](templates/templates_grid_5_framing_purlin_hung.webp)

![A W profile](templates/templates_grid_5_framing_profile_w.webp)

---

## Full C++ Examples

### Five-Bay Building {#grid_ex_tree}

![](templates/3_elements_tree_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> XS = {4000.0, 4000.0, 4000.0};
const std::vector<double> YS = {3000.0, 3000.0};
const std::vector<Polyline> FOOTPRINT = {Polyline(
    {Point(0.0, 0.0, 0.0),
     Point(12000.0, 0.0, 0.0),
     Point(12000.0, 3000.0, 0.0),
     Point(8000.0, 3000.0, 0.0),
     Point(8000.0, 6000.0, 0.0),
     Point(0.0, 6000.0, 0.0),
     Point(0.0, 0.0, 0.0)}
)};
const std::vector<double> ELEVATIONS = {0.0, 3000.0, 6000.0};
const wood_grid::Framing FRAMING{
    .system = 1,
    .span = -1,
    .node = 0,
    .deck = 200.0,
    .reach = 200.0,
    .profiles =
        {.column = profile_rectangle(200.0, 200.0), .girder = profile_rectangle(200.0, 200.0)}
};
const bool INSTANCES = false;

/// The L of five bays, two storeys of post and beam, its tree a level per storey and a group per kind.
int main() {

    const wood_grid::Building building = wood_grid::Building::from_footprint(
        FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)
    );
    wood_grid::Grid grid(building, FRAMING, "elements_tree");

    if constexpr (INSTANCES)
        grid.instance_by_key();

    std::cout << grid;
    grid.pb_dump(pb_path("live"));

    return 0;
}
```

### Footprints & Grid Patterns {#grid_ex_footprint}

Nine footprints and grid patterns (L with purlins/walls, skewed grid, radial, triangular, hexagonal, drawn axes, courtyard, pentagon, U with two cores):

![templates_grid_1_footprint](templates/templates_grid_1_footprint_all.webp)

Full source: @ref grid_example_1

### Sliced Massing Solids {#grid_ex_solid}

Six 3D massing solids sliced at storey elevations (box, pentagonal prism, tapered loft, podium + tower, atrium, cylinder):

![templates_grid_2_solid](templates/templates_grid_2_solid_all.webp)

Full source: @ref grid_example_2

### Explicit Lines & Braced Frames {#grid_ex_lines}

Direct line-and-surface input (`compas_grid` dataset and braced timber frame):

![templates_grid_3_lines](templates/templates_grid_3_lines_all.webp)

Full source: @ref grid_example_3

### Reference Building Configurations {#grid_ex_reference}

![templates_grid_4_reference](templates/templates_grid_4_reference_all.webp)

Full source: @ref grid_example_4

### Framing Systems, Nodes & Profiles {#grid_ex_framing}

Fifteen comparative bays testing all `system`, `node`, `drop`, `capital`, and `wood_profile.h` combinations with zero-overlap verification:

![templates_grid_5_framing](templates/templates_grid_5_framing_all.webp)

Full source: @ref grid_example_5

## Examples {#grid_examples}

### 1 Footprint {#grid_example_1}

Workflow B, a footprint and a pattern, nine buildings side by side over three storeys, one group each: an L with uneven bays, purlins on girders and facade walls; a grid whose y lines lean 30 degrees with flush columns; a radial plan with girders on the rays; triangular cells and hexagonal cells with every line a beam resting on the sloped sides of the pyramid heads at the nodes; five hand-drawn axes clipped to a five-sided footprint; a courtyard ring with columns through the levels; pentagon with girders hung 8 in and purlins at 10 ft; its institutional U with two cores as pinwheel walls. Every building is a Grid, its contacts found inside it, grafted under its own group. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

\include{lineno} templates_grid_1_footprint.cpp

### 2 Solid {#grid_example_2}

Workflow A, a massing sliced at its levels, six buildings side by side, one group each: a box whose every section is the same rectangle; a pentagonal prism whose diagonal side cuts every girder, purlin and deck obliquely; a tapered loft whose perimeter columns lean to follow the moving section; a podium with a tower, the tower ring added to the roof plan so every tower column stands on a podium column or girder; a block with an atrium through every level, the pattern crossing in the hole getting no column; a cylinder whose facet corners fall on the sixteen rays. Every building is a Grid, its contacts found inside it, grafted under its own group. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

\include{lineno} templates_grid_2_solid.cpp

### 3 Lines {#grid_example_3}

Workflow C, line by line, two buildings side by side, one group each: the crea dataset compas_grid ships, read from data/crea/\<INPUT\>_input.pb, every column, beam, floor, facade and core quad as drawn, the vertical lines as column points, the horizontal lines and floor edges as the plan of their level, the vertical quads as the facade and core walls compas_grid drops, every line a beam resting on a sloped side of a pyramid head at its own height; and a braced frame of two by two bays over two storeys drawn as lines and floor quads, every beam cut by the column faces and every brace by the column faces, the deck top at its foot and the beam bottom at its head. Every building is a Grid, its contacts found inside it, grafted under its own group. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

\include{lineno} templates_grid_3_lines.cpp

### 4 Reference {#grid_example_4}

The reference configurations side by side, one group each: 60 ft square in its three structural methods, plate on columns with stepped heads under CLT strips, post and beam with girders on the x lines, purlin on girder with the girders running y hung 8 in and purlin rows at 10 ft, the columns through the levels with the decks notched round them; residential L over 30 ft bays with its core as pinwheel walls, every girder and purlin reaching it cut at the wall face; the four timber bay variants with the datum at the framing top, columns flush with the datum, girders cut by the column faces, purlins cut by the girder sides and CLT strips over the outer column faces. Every building is a Grid, its contacts found inside it, grafted under its own group. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

\include{lineno} templates_grid_4_reference.cpp

### 5 Framing {#grid_example_5}

The joints and sections of the grid template, fifteen bays side by side over two storeys, one group each: pyramid heads as tall as the girders, each girder resting on a sloped side, then conical and stepped (a capital under a drop panel) under a point supported deck in strips; columns flush with the datum and running through the levels with the decks notched round them; purlins flush with their girders, hung with the girder top 8 in lower, and stacked over a girder a whole purlin depth lower; then the seven profiles of the library as girders with a matching column: rectangle, round, W, HSS with its hole, double as two members with the same cuts, slab band and T. Every bay is a Grid, its contacts found inside it, grafted under its own group. The example then cuts every pair of overlapping elements by the convex one's faces, prints the largest overlap volume and fails when it is more than the tolerance: every element touches its neighbours face to face and none overlap. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

\include{lineno} templates_grid_5_framing.cpp
