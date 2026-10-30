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

```cpp
#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

/// One building of the row: its footprint rings (empty for every bounded cell), pattern, cores and framing.
struct Case {
    std::string name;
    std::vector<Polyline> footprint;
    wood_grid::Pattern pattern;
    std::vector<Polyline> cores;
    wood_grid::Framing framing;
};

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double GAP = 8000.0;
const std::vector<double> ELEVATIONS = {0.0, 4000.0, 8000.0, 12000.0};
const wood_grid::Profiles GLULAM{
    .column = profile_rectangle(360.0, 360.0),
    .girder = profile_rectangle(320.0, 800.0),
    .purlin = profile_rectangle(260.0, 640.0)
};
const std::vector<Case> CASES = {
    {"orthogonal",
     {Polyline(
         {Point(0.0, 0.0, 0.0),
          Point(26810.0, 0.0, 0.0),
          Point(26810.0, 7600.0, 0.0),
          Point(17670.0, 7600.0, 0.0),
          Point(17670.0, 15200.0, 0.0),
          Point(0.0, 15200.0, 0.0),
          Point(0.0, 0.0, 0.0)}
     )},
     wood_grid::Pattern::orthogonal({9140.0, 8530.0, 9140.0}, {7600.0, 7600.0}),
     {},
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 3000.0,
         .node = 0,
         .deck = 175.0,
         .wall = 175.0,
         .facade = true,
         .profiles =
             {.column = profile_rectangle(365.0, 365.0),
              .girder = profile_rectangle(365.0, 365.0),
              .purlin = profile_rectangle(265.0, 265.0)}
     }},
    {"skewed",
     {},
     wood_grid::Pattern::orthogonal(
         std::vector<double>(4, 6000.0), std::vector<double>(3, 6000.0), 30.0
     ),
     {},
     wood_grid::Framing{.system = 2, .span = 0, .spacing = 2500.0, .node = 1}},
    {"radial",
     {},
     wood_grid::Pattern::radial({4000.0, 8000.0, 12000.0}, 12),
     {},
     wood_grid::Framing{
         .system = 1,
         .span = 0,
         .node = 0,
         .profiles =
             {.column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 240.0)}
     }},
    {"triangular",
     {},
     wood_grid::Pattern::triangular(6000.0, 4, 3),
     {},
     wood_grid::Framing{.system = 1, .span = -1, .node = 0}},
    {"hexagonal",
     {},
     wood_grid::Pattern::hexagonal(4000.0, 3, 2),
     {},
     wood_grid::Framing{
         .system = 1,
         .span = -1,
         .node = 0,
         .profiles =
             {.column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 400.0)}
     }},
    {"irregular",
     {Polyline(
         {Point(0.0, 0.0, 0.0),
          Point(28000.0, 1500.0, 0.0),
          Point(30000.0, 14000.0, 0.0),
          Point(15000.0, 19000.0, 0.0),
          Point(-1000.0, 12000.0, 0.0),
          Point(0.0, 0.0, 0.0)}
     )},
     wood_grid::Pattern::from_lines(
         {Line::from_points(Point(-2000.0, 5000.0, 0.0), Point(32000.0, 7000.0, 0.0)),
          Line::from_points(Point(-2000.0, 11000.0, 0.0), Point(32000.0, 12500.0, 0.0)),
          Line::from_points(Point(7000.0, -2000.0, 0.0), Point(5000.0, 21000.0, 0.0)),
          Line::from_points(Point(15000.0, -2000.0, 0.0), Point(16000.0, 21000.0, 0.0)),
          Line::from_points(Point(22000.0, -2000.0, 0.0), Point(24000.0, 21000.0, 0.0))}
     ),
     {},
     wood_grid::Framing{.system = 1, .span = -1, .node = 0}},
    {"courtyard",
     {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 36576.0, 27432.0),
      Polyline(
          {Point(12192.0, 9144.0, 0.0),
           Point(12192.0, 18288.0, 0.0),
           Point(24384.0, 18288.0, 0.0),
           Point(24384.0, 9144.0, 0.0),
           Point(12192.0, 9144.0, 0.0)}
      )},
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0)
     ),
     {},
     wood_grid::Framing{.system = 1, .span = 0, .node = 2, .facade = true}},
    {"pentagon",
     {Polyline(
         {Point(0.0, 0.0, 0.0),
          Point(13716.0, 13716.0, 0.0),
          Point(36576.0, 13716.0, 0.0),
          Point(36576.0, 27432.0, 0.0),
          Point(0.0, 27432.0, 0.0),
          Point(0.0, 0.0, 0.0)}
     )},
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0)
     ),
     {},
     wood_grid::Framing{
         .system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .profiles = GLULAM
     }},
    {"cores",
     {Polyline(
         {Point(0.0, 0.0, 0.0),
          Point(45720.0, 0.0, 0.0),
          Point(45720.0, 21336.0, 0.0),
          Point(18288.0, 21336.0, 0.0),
          Point(18288.0, 48768.0, 0.0),
          Point(45720.0, 48768.0, 0.0),
          Point(45720.0, 70104.0, 0.0),
          Point(0.0, 70104.0, 0.0),
          Point(0.0, 0.0, 0.0)}
     )},
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(45720.0, 9144.0), wood_grid::compute_bays(70104.0, 9144.0)
     ),
     {Polyline::rectangle(Point(12192.0, 48768.0, 0.0), X, Y, 6096.0, 9144.0),
      Polyline::rectangle(Point(15240.0, 15240.0, 0.0), X, Y, 3048.0, 6096.0)},
     wood_grid::Framing{
         .system = 2,
         .span = 1,
         .spacing = 3048.0,
         .node = 2,
         .drop = 203.2,
         .wall = 250.0,
         .profiles = GLULAM
     }},
};
const bool INSTANCES = false;

/// The x range of a case: its pattern lines and footprint corners.
std::pair<double, double> compute_extent(const Case& item) {

    std::pair<double, double> extent(
        std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()
    );
    for (const Line& line : item.pattern.lines)
        for (const Point& point : {line.start(), line.end()}) {
            extent.first = std::min(extent.first, point[0]);
            extent.second = std::max(extent.second, point[0]);
        }
    for (const Polyline& ring : item.footprint)
        for (const Point& point : ring.get_points()) {
            extent.first = std::min(extent.first, point[0]);
            extent.second = std::max(extent.second, point[0]);
        }

    return extent;
}

int main() {

    WoodSession wood_session("templates_grid_1_footprint");
    double offset = 0.0;

    for (const Case& item : CASES) {
        const std::pair<double, double> extent = compute_extent(item);
        const Xform shift = Xform::translation(offset - extent.first, 0.0, 0.0);
        std::vector<Polyline> footprint;
        std::vector<Polyline> cores;
        for (const Polyline& ring : item.footprint)
            footprint.push_back(ring.transformed(shift));
        for (const Polyline& ring : item.cores)
            cores.push_back(ring.transformed(shift));

        const wood_grid::Building building = wood_grid::Building::from_footprint(
            footprint, ELEVATIONS, item.pattern.transformed(shift), cores
        );
        const wood_grid::Grid grid(building, item.framing, item.name);
        wood_session.graft(grid, wood_session.add_group(item.name));
        offset += extent.second - extent.first + GAP;
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    std::cout << wood_session << std::endl;
    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

### Sliced Massing Solids {#grid_ex_solid}

Six 3D massing solids sliced at storey elevations (box, pentagonal prism, tapered loft, podium + tower, atrium, cylinder):

![templates_grid_2_solid](templates/templates_grid_2_solid_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

/// One building of the row: a closed massing, the pattern and framing it is filled with.
struct Case {
    std::string name;
    Mesh massing;
    wood_grid::Pattern pattern;
    std::vector<double> elevations;
    wood_grid::Framing framing;
};

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double GAP = 8000.0;
const Polyline PENTAGON(
    {Point(0.0, 0.0, 0.0),
     Point(13716.0, 13716.0, 0.0),
     Point(36576.0, 13716.0, 0.0),
     Point(36576.0, 27432.0, 0.0),
     Point(0.0, 27432.0, 0.0),
     Point(0.0, 0.0, 0.0)}
);
const Polyline PODIUM = Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 40000.0, 30000.0);
const Polyline TOWER = Polyline::rectangle(Point(10000.0, 5000.0, 8000.0), X, Y, 20000.0, 20000.0);
const Polyline OUTER = Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 36000.0, 36000.0);
const Polyline ATRIUM = Polyline::rectangle(Point(12000.0, 12000.0, 0.0), X, Y, 12000.0, 12000.0);
const bool INSTANCES = false;

/// A closed shell through the faces of several open lofts and their caps.
Mesh compute_shell(const std::vector<Mesh>& parts, const std::vector<Polyline>& caps) {

    std::vector<Polyline> faces = caps;
    for (const Mesh& part : parts)
        for (const Polyline& face : part.face_outlines())
            faces.push_back(face);

    return Mesh::from_polylines(faces, 1.0);
}

const std::vector<Case> CASES = {
    {"box",
     Mesh::create_box(30000.0, 18000.0, 12000.0)
         .transformed(Xform::translation(15000.0, 9000.0, 6000.0)),
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(30000.0, 6000.0), wood_grid::compute_bays(18000.0, 6000.0)
     ),
     {0.0, 4000.0, 8000.0, 12000.0},
     wood_grid::Framing{.system = 1, .span = 1, .node = 1}},
    {"prism",
     Mesh::loft({PENTAGON}, {PENTAGON.transformed(Xform::translation(0.0, 0.0, 10972.8))}),
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0)
     ),
     {0.0, 3657.6, 7315.2, 10972.8},
     wood_grid::Framing{.system = 2, .span = 0, .spacing = 3048.0, .node = 0}},
    {"taper",
     Mesh::loft(
         {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 30000.0, 20000.0)},
         {Polyline::rectangle(Point(1500.0, 1500.0, 14400.0), X, Y, 27000.0, 17000.0)}
     ),
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(30000.0, 6000.0), wood_grid::compute_bays(20000.0, 5000.0)
     ),
     {0.0, 3600.0, 7200.0, 10800.0, 14400.0},
     wood_grid::Framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 0, .taper = 30.0}},
    {"setback",
     compute_shell(
         {Mesh::loft({PODIUM}, {PODIUM.transformed(Xform::translation(0.0, 0.0, 8000.0))}, false),
          Mesh::loft({PODIUM.transformed(Xform::translation(0.0, 0.0, 8000.0))}, {TOWER}, false),
          Mesh::loft({TOWER}, {TOWER.transformed(Xform::translation(0.0, 0.0, 14400.0))}, false)},
         {PODIUM, TOWER.transformed(Xform::translation(0.0, 0.0, 14400.0))}
     ),
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(40000.0, 5000.0), wood_grid::compute_bays(30000.0, 5000.0)
     ),
     {0.0, 4000.0, 8000.0, 11600.0, 15200.0, 18800.0, 22400.0},
     wood_grid::Framing{.system = 1, .span = 1, .node = 1}},
    {"atrium",
     Mesh::loft(
         {OUTER, ATRIUM},
         {OUTER.transformed(Xform::translation(0.0, 0.0, 20000.0)),
          ATRIUM.transformed(Xform::translation(0.0, 0.0, 20000.0))}
     ),
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(36000.0, 6000.0), wood_grid::compute_bays(36000.0, 6000.0)
     ),
     {0.0, 4000.0, 8000.0, 12000.0, 16000.0, 20000.0},
     wood_grid::Framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 2}},
    {"curved",
     BRep::create_cylinder(15000.0, 15200.0).mesh(),
     wood_grid::Pattern::radial({5000.0, 10000.0, 15000.0}, 16),
     {0.0, 3800.0, 7600.0, 11400.0, 15200.0},
     wood_grid::Framing{.system = 2, .span = 0, .spacing = 2500.0, .node = 0}},
};

/// The x range of a massing.
std::pair<double, double> compute_extent(const Mesh& massing) {

    std::pair<double, double> extent(
        std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()
    );
    for (const size_t vertex : massing.vertices()) {
        extent.first = std::min(extent.first, (*massing.vertex_point(vertex))[0]);
        extent.second = std::max(extent.second, (*massing.vertex_point(vertex))[0]);
    }

    return extent;
}

int main() {

    WoodSession wood_session("templates_grid_2_solid");
    double offset = 0.0;

    for (const Case& item : CASES) {
        const std::pair<double, double> extent = compute_extent(item.massing);
        const Xform shift = Xform::translation(offset - extent.first, 0.0, 0.0);
        const wood_grid::Building building = wood_grid::Building::from_solid(
            item.massing.transformed(shift), item.elevations, item.pattern.transformed(shift)
        );
        const wood_grid::Grid grid(building, item.framing, item.name);
        wood_session.graft(grid, wood_session.add_group(item.name));
        offset += extent.second - extent.first + GAP;
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    std::cout << wood_session << std::endl;
    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

### Explicit Lines & Braced Frames {#grid_ex_lines}

Direct line-and-surface input (`compas_grid` dataset and braced timber frame):

![templates_grid_3_lines](templates/templates_grid_3_lines_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::string INPUT = "crea_4x4";
const double GAP = 8000.0;
const double BAY = 6000.0;
const double STOREY = 3800.0;
const int BAYS = 2;
const int STOREYS = 2;
const wood_grid::Framing CREA{
    .system = 1,
    .span = -1,
    .node = 0,
    .deck = 200.0,
    .wall = 200.0,
    .head = 300.0,
    .reach = 360.0,
    .profiles =
        {.column = profile_rectangle(300.0, 300.0), .girder = profile_rectangle(300.0, 300.0)}
};
const wood_grid::Framing BRACED{
    .span = -1,
    .node = 1,
    .profiles =
        {.column = profile_rectangle(300.0, 300.0),
         .girder = profile_rectangle(200.0, 500.0),
         .brace = profile_rectangle(150.0, 150.0)}
};
const bool INSTANCES = false;

int main() {

    const Session input =
        Session::pb_load(config::dataset_path("crea/" + INPUT + "_input", ".pb").string());
    std::vector<Line> lines;
    std::vector<Polyline> surfaces;
    double right = -std::numeric_limits<double>::max();
    for (const std::vector<Polyline>& group : input.select_by_type<Polyline>())
        for (const Polyline& polyline : group) {
            lines.push_back(
                Line::from_points(polyline.get_points().front(), polyline.get_points().back())
            );
            right = std::max({right, lines.back().start()[0], lines.back().end()[0]});
        }
    for (const std::vector<Mesh>& group : input.select_by_type<Mesh>())
        for (const Mesh& mesh : group)
            for (const size_t face : mesh.faces()) {
                surfaces.push_back(*mesh.face_polygon(face));
                surfaces.back().name =
                    mesh.name.find("Core") == std::string::npos ? "surface" : "core";
            }

    WoodSession wood_session("templates_grid_3_lines");
    const wood_grid::Grid crea(wood_grid::Building::from_lines(lines, surfaces), CREA, INPUT);
    wood_session.graft(crea, wood_session.add_group(INPUT));

    std::vector<Line> frame;
    std::vector<Polyline> floors;
    for (int k = 0; k <= STOREYS; k++)
        for (int i = 0; i <= BAYS; i++)
            for (int j = 0; j <= BAYS; j++) {
                const Point node(right + GAP + i * BAY, j * BAY, k * STOREY);
                if (k > 0 && i < BAYS)
                    frame.push_back(Line::from_points(node, node + X * BAY));
                if (k > 0 && j < BAYS)
                    frame.push_back(Line::from_points(node, node + Y * BAY));
                if (k < STOREYS)
                    frame.push_back(Line::from_points(node, node + Vector(0.0, 0.0, STOREY)));
                if (k > 0 && i < BAYS && j < BAYS)
                    floors.push_back(Polyline::rectangle(node, X, Y, BAY, BAY));
                if (k < STOREYS && j < BAYS && (i == 0 || i == BAYS))
                    frame.push_back(
                        Line::from_points(node, node + Y * BAY + Vector(0.0, 0.0, STOREY))
                    );
            }
    const wood_grid::Grid braced(wood_grid::Building::from_lines(frame, floors), BRACED, "braced");
    wood_session.graft(braced, wood_session.add_group("braced"));

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    std::cout << wood_session << std::endl;
    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

### Reference Building Configurations {#grid_ex_reference}

![templates_grid_4_reference](templates/templates_grid_4_reference_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

/// One building of the row: a reference configuration as its footprint, pattern, cores, levels and framing.
struct Case {
    std::string name;
    std::vector<Polyline> footprint;
    wood_grid::Pattern pattern;
    std::vector<Polyline> cores;
    std::vector<double> elevations;
    wood_grid::Framing framing;
};

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double GAP = 8000.0;
const std::vector<Polyline> SQUARE = {
    Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 18288.0, 18288.0)
};
const wood_grid::Pattern SQUARE_BAYS = wood_grid::Pattern::orthogonal(
    wood_grid::compute_bays(18288.0, 4572.0), wood_grid::compute_bays(18288.0, 4572.0)
);
const std::vector<double> BRANCH = {0.0, 3657.6, 7315.2, 10972.8};
const wood_grid::Profiles GLULAM{
    .column = profile_rectangle(360.0, 360.0),
    .girder = profile_rectangle(320.0, 800.0),
    .purlin = profile_rectangle(260.0, 640.0)
};
const std::vector<Case> CASES = {
    {"branch_plate",
     SQUARE,
     SQUARE_BAYS,
     {},
     BRANCH,
     wood_grid::Framing{
         .system = 0,
         .node = 0,
         .deck = 365.1,
         .head = 300.0,
         .reach = 600.0,
         .capital = 1,
         .panel = 3505.2
     }},
    {"branch_post_and_beam",
     SQUARE,
     SQUARE_BAYS,
     {},
     BRANCH,
     wood_grid::Framing{
         .system = 1,
         .span = 0,
         .node = 2,
         .deck = 189.8,
         .profiles = {.girder = profile_rectangle(220.0, 520.0)}
     }},
    {"branch_purlin_on_girder",
     SQUARE,
     SQUARE_BAYS,
     {},
     BRANCH,
     wood_grid::Framing{
         .system = 2,
         .span = 1,
         .spacing = 3048.0,
         .node = 2,
         .drop = 203.2,
         .deck = 189.8,
         .profiles =
             {.girder = profile_rectangle(220.0, 520.0), .purlin = profile_rectangle(220.0, 400.0)}
     }},
    {"branch_residential",
     {Polyline(
         {Point(0.0, 0.0, 0.0),
          Point(21945.6, 0.0, 0.0),
          Point(21945.6, 45720.0, 0.0),
          Point(43891.2, 45720.0, 0.0),
          Point(43891.2, 67056.0, 0.0),
          Point(0.0, 67056.0, 0.0),
          Point(0.0, 0.0, 0.0)}
     )},
     wood_grid::Pattern::orthogonal(
         wood_grid::compute_bays(43891.2, 9144.0), wood_grid::compute_bays(67056.0, 9144.0)
     ),
     {Polyline::rectangle(Point(7467.6, 19354.8, 0.0), X, Y, 7010.4, 7010.4)},
     BRANCH,
     wood_grid::Framing{
         .system = 2,
         .span = 1,
         .spacing = 3048.0,
         .node = 2,
         .drop = 203.2,
         .deck = 189.8,
         .wall = 250.0,
         .profiles = GLULAM
     }},
    {"timber_bay_v1",
     {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 9000.0, 9000.0)},
     wood_grid::Pattern::orthogonal({9000.0}, {9000.0}),
     {},
     {0.0, 4500.0},
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 2250.0,
         .node = 1,
         .deck = 87.0,
         .panel = 3114.0,
         .profiles =
             {.column = profile_rectangle(315.0, 342.0),
              .girder = profile_rectangle(265.0, 608.0),
              .purlin = profile_rectangle(215.0, 456.0)}
     }},
    {"timber_bay_v2",
     {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 6000.0, 12000.0)},
     wood_grid::Pattern::orthogonal({6000.0}, {12000.0}),
     {},
     {0.0, 4500.0},
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 1500.0,
         .node = 1,
         .deck = 87.0,
         .panel = 3095.0,
         .profiles =
             {.column = profile_rectangle(265.0, 380.0),
              .girder = profile_rectangle(265.0, 532.0),
              .purlin = profile_rectangle(215.0, 532.0)}
     }},
    {"timber_bay_v3",
     {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 6000.0, 9000.0)},
     wood_grid::Pattern::orthogonal({6000.0}, {9000.0}),
     {},
     {0.0, 4500.0},
     wood_grid::Framing{
         .system = 1,
         .span = 1,
         .node = 1,
         .deck = 243.0,
         .panel = 3126.67,
         .profiles =
             {.column = profile_rectangle(265.0, 380.0),
              .girder = profile_rectangle(265.0, 836.0),
              .beam = profile_rectangle(215.0, 836.0)}
     }},
    {"timber_bay_v4",
     {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 8000.0, 12000.0)},
     wood_grid::Pattern::orthogonal({8000.0}, {12000.0}),
     {},
     {0.0, 4500.0},
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 8000.0 / 6.0,
         .node = 1,
         .deck = 87.0,
         .panel = 3104.5,
         .profiles =
             {.column = profile_rectangle(365.0, 418.0),
              .girder = profile_rectangle(265.0, 684.0),
              .purlin = profile_rectangle(215.0, 494.0)}
     }},
};
const bool INSTANCES = false;

/// The x range of a footprint.
std::pair<double, double> compute_extent(const std::vector<Polyline>& footprint) {

    std::pair<double, double> extent(
        std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()
    );
    for (const Polyline& ring : footprint)
        for (const Point& point : ring.get_points()) {
            extent.first = std::min(extent.first, point[0]);
            extent.second = std::max(extent.second, point[0]);
        }

    return extent;
}

int main() {

    WoodSession wood_session("templates_grid_4_reference");
    double offset = 0.0;

    for (const Case& item : CASES) {
        const std::pair<double, double> extent = compute_extent(item.footprint);
        const Xform shift = Xform::translation(offset - extent.first, 0.0, 0.0);
        std::vector<Polyline> footprint;
        std::vector<Polyline> cores;
        for (const Polyline& ring : item.footprint)
            footprint.push_back(ring.transformed(shift));
        for (const Polyline& ring : item.cores)
            cores.push_back(ring.transformed(shift));

        const wood_grid::Building building = wood_grid::Building::from_footprint(
            footprint, item.elevations, item.pattern.transformed(shift), cores
        );
        const wood_grid::Grid grid(building, item.framing, item.name);
        wood_session.graft(grid, wood_session.add_group(item.name));
        offset += extent.second - extent.first + GAP;
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    std::cout << wood_session << std::endl;
    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

### Framing Systems, Nodes & Profiles {#grid_ex_framing}

Fifteen comparative bays testing all `system`, `node`, `drop`, `capital`, and `wood_profile.h` combinations with zero-overlap verification:

![templates_grid_5_framing](templates/templates_grid_5_framing_all.webp)

```cpp
#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

/// One bay of the row: its pattern and framing.
struct Case {
    std::string name;
    wood_grid::Pattern pattern;
    wood_grid::Framing framing;
};

const double BAY = 6000.0;
const double GAP = 3000.0;
const std::vector<double> ELEVATIONS = {0.0, 4500.0, 9000.0};
const double CLEARANCE = 1.0;
const wood_grid::Pattern ONE = wood_grid::Pattern::orthogonal({BAY}, {BAY});
const wood_grid::Framing PURLINS{
    .system = 2,
    .span = 0,
    .spacing = 2000.0,
    .node = 1,
    .deck = 87.0,
    .profiles = {.purlin = profile_rectangle(215.0, 456.0)}
};
const std::vector<Case> CASES = {
    {"head_section",
     wood_grid::Pattern::orthogonal({BAY, BAY}, {BAY}),
     wood_grid::Framing{.system = 1, .span = 0, .node = 0}},
    {"head_conical",
     ONE,
     wood_grid::Framing{.system = 0, .node = 0, .reach = 600.0, .capital = 0, .panel = 3000.0}},
    {"head_stepped",
     ONE,
     wood_grid::Framing{.system = 0, .node = 0, .reach = 600.0, .capital = 1, .panel = 3000.0}},
    {"node_flush", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1}},
    {"node_through", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 2}},
    {"purlin_flush",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 3000.0,
         .node = 1,
         .drop = 0.0,
         .profiles =
             {.column = profile_rectangle(360.0, 360.0),
              .girder = profile_rectangle(320.0, 800.0),
              .purlin = profile_rectangle(260.0, 640.0)}
     }},
    {"purlin_hung",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 3000.0,
         .node = 1,
         .drop = 203.2,
         .profiles =
             {.column = profile_rectangle(360.0, 360.0),
              .girder = profile_rectangle(320.0, 800.0),
              .purlin = profile_rectangle(260.0, 640.0)}
     }},
    {"purlin_stacked",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 3000.0,
         .node = 1,
         .drop = 640.0,
         .profiles =
             {.column = profile_rectangle(360.0, 360.0),
              .girder = profile_rectangle(320.0, 800.0),
              .purlin = profile_rectangle(260.0, 640.0)}
     }},
    {"profile_rectangle",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 2000.0,
         .node = 1,
         .deck = 87.0,
         .profiles =
             {.column = profile_rectangle(315.0, 342.0),
              .girder = profile_rectangle(265.0, 608.0),
              .purlin = profile_rectangle(215.0, 456.0)}
     }},
    {"profile_round",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 2000.0,
         .node = 1,
         .deck = 87.0,
         .profiles =
             {.column = profile_round(360.0),
              .girder = profile_round(300.0),
              .purlin = profile_rectangle(215.0, 456.0)}
     }},
    {"profile_w",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 2000.0,
         .node = 1,
         .deck = 87.0,
         .profiles =
             {.column = profile_w(206.0, 210.0, 14.2, 10.2),
              .girder = profile_w(250.0, 250.0, 15.0, 10.0),
              .purlin = profile_rectangle(215.0, 456.0)}
     }},
    {"profile_hss",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 2000.0,
         .node = 1,
         .deck = 87.0,
         .profiles =
             {.column = profile_hss(178.0, 178.0, 12.7),
              .girder = profile_hss(250.0, 250.0, 10.0),
              .purlin = profile_rectangle(215.0, 456.0)}
     }},
    {"profile_slab_band",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 2000.0,
         .node = 1,
         .deck = 87.0,
         .profiles =
             {.column = profile_rectangle(315.0, 342.0),
              .girder = profile_slab_band(1200.0, 300.0),
              .purlin = profile_rectangle(215.0, 456.0)}
     }},
    {"profile_t",
     ONE,
     wood_grid::Framing{
         .system = 2,
         .span = 0,
         .spacing = 2000.0,
         .node = 1,
         .deck = 87.0,
         .profiles =
             {.column = profile_rectangle(315.0, 342.0),
              .girder = profile_t(300.0, 500.0, 100.0, 80.0),
              .purlin = profile_rectangle(215.0, 456.0)}
     }},
};
const bool INSTANCES = false;

/// The face planes of a closed mesh, normals out.
std::vector<Plane> compute_planes(const Mesh& mesh) {

    const Point centre = mesh.centroid();
    std::vector<Plane> planes;
    for (const size_t face : mesh.faces()) {
        const std::vector<Point> points = mesh.face_polygon(face)->get_points();
        const Vector normal = compute_newell(points).normalized();
        const Point origin = Point::centroid(points);
        planes.push_back(
            Plane::from_point_normal(origin, (origin - centre).dot(normal) < 0.0 ? -normal : normal)
        );
    }

    return planes;
}

/// True when every vertex of a mesh lies behind every one of its face planes.
bool is_convex(const Mesh& mesh, const std::vector<Plane>& planes) {

    for (const Plane& plane : planes)
        for (const size_t vertex : mesh.vertices())
            if ((*mesh.vertex_point(vertex) - plane.origin()).dot(plane.z_axis()) > CLEARANCE)
                return false;

    return true;
}

/// Volume of other inside a convex mesh, its face planes pulled in by the tolerance so touching faces count for nothing.
double compute_overlap(const Mesh& convex, const Mesh& other) {

    Mesh part = other;
    for (const Plane& plane : compute_planes(convex)) {
        part = part.cut_by_plane(
            Plane::from_point_normal(plane.origin() - plane.z_axis() * CLEARANCE, -plane.z_axis())
        );
        if (part.faces().empty())
            return 0.0;
    }

    return std::abs(part.volume());
}

/// The lowest and highest corner of a mesh.
std::pair<Point, Point> compute_box(const Mesh& mesh) {

    std::pair<Point, Point> box(Point(1e18, 1e18, 1e18), Point(-1e18, -1e18, -1e18));
    for (const size_t vertex : mesh.vertices())
        for (int axis = 0; axis < 3; axis++) {
            box.first[axis] = std::min(box.first[axis], (*mesh.vertex_point(vertex))[axis]);
            box.second[axis] = std::max(box.second[axis], (*mesh.vertex_point(vertex))[axis]);
        }

    return box;
}

/// True when two meshes' boxes overlap by more than the tolerance on every axis.
bool is_near(const Mesh& a, const Mesh& b) {

    const std::pair<Point, Point> box_a = compute_box(a);
    const std::pair<Point, Point> box_b = compute_box(b);
    for (int axis = 0; axis < 3; axis++)
        if (box_a.second[axis] <= box_b.first[axis] + CLEARANCE ||
            box_b.second[axis] <= box_a.first[axis] + CLEARANCE)
            return false;

    return true;
}

/// The largest overlap volume over every pair of elements whose boxes overlap, a convex one of the pair cutting the other.
double compute_clash(const WoodSession& session) {

    double worst = 0.0;
    const Collection<std::shared_ptr<Element>>& elements = session.elements();
    for (size_t i = 0; i < elements.size(); i++)
        for (size_t j = i + 1; j < elements.size(); j++) {
            const Mesh& a = elements[i]->model_geometry_mesh();
            const Mesh& b = elements[j]->model_geometry_mesh();
            if (!is_near(a, b))
                continue;

            const bool convex = is_convex(a, compute_planes(a));
            worst = std::max(worst, convex ? compute_overlap(a, b) : compute_overlap(b, a));
        }

    return worst;
}

int main() {

    WoodSession wood_session("templates_grid_5_framing");
    double offset = 0.0;

    for (const Case& item : CASES) {
        const wood_grid::Building building = wood_grid::Building::from_footprint(
            {}, ELEVATIONS, item.pattern.transformed(Xform::translation(offset, 0.0, 0.0))
        );
        const wood_grid::Grid grid(building, item.framing, item.name);
        wood_session.graft(grid, wood_session.add_group(item.name));
        offset += item.pattern.lines.front().length() + GAP;
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.pb_dump(pb_path("live"));

    const double clash = compute_clash(wood_session);
    std::cout << "largest overlap " << clash << " mm3" << std::endl;

    return clash > CLEARANCE ? 1 : 0;
}
```
