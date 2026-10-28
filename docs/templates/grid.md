# Grid {#templates_grid}

[TOC]

Multi-storey timber building generator (`src/templates/grid/*.h`). `wood_grid::Grid` subclasses `WoodSession` and frames a building level by level from two primary inputs: a `Building` guide (constructed from a massing solid, footprint rings, or explicit lines) and a `Framing` specification.

![Multistorey Timber Grid Pipeline](diagrams/template_grid_pipeline.webp)

![The L of five bays over two storeys](templates/3_elements_tree_all.webp)

```cpp
const wood_grid::Building building = wood_grid::Building::from_footprint(
    FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)
);
wood_grid::Grid grid(building, FRAMING, "elements_tree");
```

---

## Primary Inputs (`Pattern`, `Building`, `Framing`)

| Primary Input | Factories / Fields | Geometric Role |
| --- | --- | --- |
| **`Pattern`** | `orthogonal(xs, ys, skew)` · `radial(radii, sectors, sweep)` · `triangular` · `hexagonal` · `from_lines` | Defines 2D plan grid axes clipped against each level boundary |
| **`Building`** | **A:** `from_solid(massing, elevations, pattern)`<br>**B:** `from_footprint(rings, elevations, pattern)`<br>**C:** `from_lines(lines, surfaces)` | Produces `levels`, where each level has datum `z`, `cores`, and a planar `plan` `Mesh` (faces = bays, edges = member axes, vertices = column nodes) |
| **`Framing`** | `system` (`0` point-supported, `1` post-and-beam, `2` purlin-on-girder) · `span` · `node` (`0` head, `1` flush, `2` column-through) · `drop` · `deck` · `wall` · `head` · `reach` · `capital` · `profiles` | Controls structural hierarchy, node priority/head geometry, and cross-section `Polyline` profiles (`wood_profile.h`) per member role |

### Plan Mesh Attributes (Customizable per Bay, Edge, or Node)

| Mesh Entity | Attribute | Values & Meaning |
| --- | --- | --- |
| **Face (bay)** | `system`, `span`, `floor` | Local framing system (`0..2`), girder direction, and slab presence (`1.0` = decked bay) |
| **Edge (axis)** | `role` | `0.0` none · `1.0` girder · `2.0` beam · `3.0` purlin · `4.0` brace |
| **Edge (axis)** | `wall` | `0.0` none · `1.0` perimeter facade wall · `2.0` shear-core wall |
| **Vertex (node)** | `column` | `0.0` none · `1.0` standard column · `2.0` transfer column |

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

### 1. Level Plans (`add_plans`)

Every level's `plan` mesh edges and column vertices are lifted to datum `level.z` and placed in `plan_<level>`:

![The plans](templates/3_elements_tree_plan_2.webp)

```cpp
for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
    const Point start = compute_lift(*level.plan.vertex_point(edge.first), level.z);
    const Point end = compute_lift(*level.plan.vertex_point(edge.second), level.z);
    add_line(Line::from_points(start, end), plan);
}
```

### 2. Columns (`columns_<s>`)

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

### 3. Facade & Core Walls (`walls_<s>`, `core_walls_<s>`)

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

### 4. Column Heads & Drop Panels (`heads_<s>`, `drop_panels_<s>`)

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

### 5. Girders, Beams & Bay Purlins (`girders_<s>`, `beams_<s>`, `purlins_<s>`)

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

### 6. Bay Decks (`decks_<s>`)

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

### 7. Storey Braces & Global Contacts (`braces_<s>`, `compute_face_contacts(0)`)

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

### 1. Five-Bay Two-Storey L-Building (`examples/3_elements_tree.cpp`)

\include{lineno} 3_elements_tree.cpp

### 2. Workflow B — Footprints & Grid Patterns (`examples/templates_grid_1_footprint.cpp`)

Nine footprints and grid patterns (L with purlins/walls, skewed grid, radial, triangular, hexagonal, drawn axes, courtyard, pentagon, U with two cores):

![templates_grid_1_footprint](templates/templates_grid_1_footprint_all.webp)

\include{lineno} templates_grid_1_footprint.cpp

### 3. Workflow A — Sliced Massing Solids (`examples/templates_grid_2_solid.cpp`)

Six 3D massing solids sliced at storey elevations (box, pentagonal prism, tapered loft, podium + tower, atrium, cylinder):

![templates_grid_2_solid](templates/templates_grid_2_solid_all.webp)

\include{lineno} templates_grid_2_solid.cpp

### 4. Workflow C — Explicit Lines & Braced Frames (`examples/templates_grid_3_lines.cpp`)

Direct line-and-surface input (`compas_grid` dataset and braced timber frame):

![templates_grid_3_lines](templates/templates_grid_3_lines_all.webp)

\include{lineno} templates_grid_3_lines.cpp

### 5. Reference Building Configurations (`examples/templates_grid_4_reference.cpp`)

![templates_grid_4_reference](templates/templates_grid_4_reference_all.webp)

\include{lineno} templates_grid_4_reference.cpp

### 6. Framing Systems, Nodes & Profiles (`examples/templates_grid_5_framing.cpp`)

Fifteen comparative bays testing all `system`, `node`, `drop`, `capital`, and `wood_profile.h` combinations with zero-overlap verification:

![templates_grid_5_framing](templates/templates_grid_5_framing_all.webp)

\include{lineno} templates_grid_5_framing.cpp
