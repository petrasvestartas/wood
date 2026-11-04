# Grid {#templates_grid}

[TOC]

Multi-storey timber building generator in `src/templates/grid/*.h`. `wood_grid::Grid` frames a building level by level from a `Building` guide and a `Framing` specification.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#grid_ex_tree">
    <img src="templates_grid_6_five_bay_all.webp" alt="Five-Bay Building">
    <span class="wood-card-title">Five-Bay Building</span>
  </a>
  <a class="wood-card" href="#grid_ex_footprint">
    <img src="templates_grid_1_footprint_orthogonal.webp" alt="Footprints and Patterns">
    <span class="wood-card-title">Footprints and Patterns</span>
  </a>
  <a class="wood-card" href="#grid_ex_solid">
    <img src="templates_grid_2_solid_setback.webp" alt="Massing Solids">
    <span class="wood-card-title">Massing Solids</span>
  </a>
  <a class="wood-card" href="#grid_ex_lines">
    <img src="templates_grid_3_lines_braced.webp" alt="Lines and Braced Frames">
    <span class="wood-card-title">Lines and Braced Frames</span>
  </a>
  <a class="wood-card" href="#grid_ex_reference">
    <img src="templates_grid_4_reference_timber_bay_v1.webp" alt="Reference Buildings">
    <span class="wood-card-title">Reference Buildings</span>
  </a>
  <a class="wood-card" href="#grid_ex_framing">
    <img src="templates_grid_5_framing_head_section.webp" alt="Framing Systems">
    <span class="wood-card-title">Framing Systems</span>
  </a>
</div>

```cpp
const wood_grid::Building building = wood_grid::Building::from_footprint(
    FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)
);
wood_grid::Grid grid(building, FRAMING, "five_bay");
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
│   └── decks_0                    FacePlanarElement deck_<i>_0 (ground slabs)
└── level_1                        level_2 .. level_N (each capping the storey below)
    ├── plan_1
    ├── columns_1                  Column column_<i>_1
    ├── walls_1, core_walls_1      FacePlanarElement wall_<i>_1, core_wall_<i>_1
    ├── heads_1, drop_panels_1     NodeBlockElement head_<i>_1, drop_panel_<i>_1
    ├── girders_1, beams_1         Beam girder_<i>_1, beam_<i>_1
    ├── purlins_1                  Beam purlin_<i>_1
    ├── decks_1                    FacePlanarElement deck_<i>_1
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

![The plans](templates/templates_grid_6_five_bay_plan_2.webp)

```cpp
for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
    const Point start = compute_lift(*level.plan.vertex_point(edge.first), level.z);
    const Point end = compute_lift(*level.plan.vertex_point(edge.second), level.z);
    add_line(Line::from_points(start, end), plan);
}
```

### Columns

A `Column` stands on every stacked column vertex from the lower slab top (`foot`) to the upper column head underside or level datum (`top`):

![The columns](templates/templates_grid_6_five_bay_columns_1.webp)

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

![The heads](templates/templates_grid_6_five_bay_heads_1.webp)

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

![The beams](templates/templates_grid_6_five_bay_beams_1.webp)

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

![The decks](templates/templates_grid_6_five_bay_decks_1.webp)

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

An L of five bays, two storeys of post and beam:

![templates_grid_6_five_bay](templates/templates_grid_6_five_bay_all.webp)

\include{lineno} templates_grid_6_five_bay.cpp

### Footprints & Grid Patterns {#grid_ex_footprint}

Nine footprints and grid patterns (L with purlins/walls, skewed grid, radial, triangular, hexagonal, drawn axes, courtyard, pentagon, U with two cores):

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_1_footprint_orthogonal.webp">
    <img src="templates_grid_1_footprint_orthogonal.webp" alt="Orthogonal">
    <span class="wood-card-title">Orthogonal</span>
  </a>
  <a class="wood-card" href="templates_grid_1_footprint_skewed.webp">
    <img src="templates_grid_1_footprint_skewed.webp" alt="Skewed">
    <span class="wood-card-title">Skewed</span>
  </a>
  <a class="wood-card" href="templates_grid_1_footprint_radial.webp">
    <img src="templates_grid_1_footprint_radial.webp" alt="Radial">
    <span class="wood-card-title">Radial</span>
  </a>
  <a class="wood-card" href="templates_grid_1_footprint_triangular.webp">
    <img src="templates_grid_1_footprint_triangular.webp" alt="Triangular">
    <span class="wood-card-title">Triangular</span>
  </a>
  <a class="wood-card" href="templates_grid_1_footprint_hexagonal.webp">
    <img src="templates_grid_1_footprint_hexagonal.webp" alt="Hexagonal">
    <span class="wood-card-title">Hexagonal</span>
  </a>
  <a class="wood-card" href="templates_grid_1_footprint_irregular.webp">
    <img src="templates_grid_1_footprint_irregular.webp" alt="Irregular">
    <span class="wood-card-title">Irregular</span>
  </a>
  <a class="wood-card" href="templates_grid_1_footprint_courtyard.webp">
    <img src="templates_grid_1_footprint_courtyard.webp" alt="Courtyard">
    <span class="wood-card-title">Courtyard</span>
  </a>
  <a class="wood-card" href="templates_grid_1_footprint_pentagon.webp">
    <img src="templates_grid_1_footprint_pentagon.webp" alt="Pentagon">
    <span class="wood-card-title">Pentagon</span>
  </a>
  <a class="wood-card" href="templates_grid_1_footprint_cores.webp">
    <img src="templates_grid_1_footprint_cores.webp" alt="Cores">
    <span class="wood-card-title">Cores</span>
  </a>
</div>

\include{lineno} templates_grid_1_footprint.cpp

### Sliced Massing Solids {#grid_ex_solid}

Six 3D massing solids sliced at storey elevations (box, pentagonal prism, tapered loft, podium + tower, atrium, cylinder):

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_2_solid_box.webp">
    <img src="templates_grid_2_solid_box.webp" alt="Box">
    <span class="wood-card-title">Box</span>
  </a>
  <a class="wood-card" href="templates_grid_2_solid_prism.webp">
    <img src="templates_grid_2_solid_prism.webp" alt="Prism">
    <span class="wood-card-title">Prism</span>
  </a>
  <a class="wood-card" href="templates_grid_2_solid_taper.webp">
    <img src="templates_grid_2_solid_taper.webp" alt="Taper">
    <span class="wood-card-title">Taper</span>
  </a>
  <a class="wood-card" href="templates_grid_2_solid_setback.webp">
    <img src="templates_grid_2_solid_setback.webp" alt="Setback">
    <span class="wood-card-title">Setback</span>
  </a>
  <a class="wood-card" href="templates_grid_2_solid_atrium.webp">
    <img src="templates_grid_2_solid_atrium.webp" alt="Atrium">
    <span class="wood-card-title">Atrium</span>
  </a>
  <a class="wood-card" href="templates_grid_2_solid_curved.webp">
    <img src="templates_grid_2_solid_curved.webp" alt="Curved">
    <span class="wood-card-title">Curved</span>
  </a>
</div>

\include{lineno} templates_grid_2_solid.cpp

### Explicit Lines & Braced Frames {#grid_ex_lines}

Direct line-and-surface input (`compas_grid` dataset and braced timber frame):

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_3_lines_crea_4x4.webp">
    <img src="templates_grid_3_lines_crea_4x4.webp" alt="Crea 4 x 4">
    <span class="wood-card-title">Crea 4 x 4</span>
  </a>
  <a class="wood-card" href="templates_grid_3_lines_braced.webp">
    <img src="templates_grid_3_lines_braced.webp" alt="Braced">
    <span class="wood-card-title">Braced</span>
  </a>
</div>

\include{lineno} templates_grid_3_lines.cpp

### Reference Building Configurations {#grid_ex_reference}

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_4_reference_branch_plate.webp">
    <img src="templates_grid_4_reference_branch_plate.webp" alt="Branch Plate">
    <span class="wood-card-title">Branch Plate</span>
  </a>
  <a class="wood-card" href="templates_grid_4_reference_branch_post_and_beam.webp">
    <img src="templates_grid_4_reference_branch_post_and_beam.webp" alt="Branch Post and Beam">
    <span class="wood-card-title">Branch Post and Beam</span>
  </a>
  <a class="wood-card" href="templates_grid_4_reference_branch_purlin_on_girder.webp">
    <img src="templates_grid_4_reference_branch_purlin_on_girder.webp" alt="Branch Purlin on Girder">
    <span class="wood-card-title">Branch Purlin on Girder</span>
  </a>
  <a class="wood-card" href="templates_grid_4_reference_branch_residential.webp">
    <img src="templates_grid_4_reference_branch_residential.webp" alt="Branch Residential">
    <span class="wood-card-title">Branch Residential</span>
  </a>
  <a class="wood-card" href="templates_grid_4_reference_timber_bay_v1.webp">
    <img src="templates_grid_4_reference_timber_bay_v1.webp" alt="Timber Bay 1">
    <span class="wood-card-title">Timber Bay 1</span>
  </a>
  <a class="wood-card" href="templates_grid_4_reference_timber_bay_v2.webp">
    <img src="templates_grid_4_reference_timber_bay_v2.webp" alt="Timber Bay 2">
    <span class="wood-card-title">Timber Bay 2</span>
  </a>
  <a class="wood-card" href="templates_grid_4_reference_timber_bay_v3.webp">
    <img src="templates_grid_4_reference_timber_bay_v3.webp" alt="Timber Bay 3">
    <span class="wood-card-title">Timber Bay 3</span>
  </a>
  <a class="wood-card" href="templates_grid_4_reference_timber_bay_v4.webp">
    <img src="templates_grid_4_reference_timber_bay_v4.webp" alt="Timber Bay 4">
    <span class="wood-card-title">Timber Bay 4</span>
  </a>
</div>

\include{lineno} templates_grid_4_reference.cpp

### Framing Systems, Nodes & Profiles {#grid_ex_framing}

Fourteen comparative bays testing all `system`, `node`, `drop`, `capital`, and `wood_profile.h` combinations with zero-overlap verification:

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_5_framing_head_section.webp">
    <img src="templates_grid_5_framing_head_section.webp" alt="Head Section">
    <span class="wood-card-title">Head Section</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_head_conical.webp">
    <img src="templates_grid_5_framing_head_conical.webp" alt="Head Conical">
    <span class="wood-card-title">Head Conical</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_head_stepped.webp">
    <img src="templates_grid_5_framing_head_stepped.webp" alt="Head Stepped">
    <span class="wood-card-title">Head Stepped</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_node_flush.webp">
    <img src="templates_grid_5_framing_node_flush.webp" alt="Node Flush">
    <span class="wood-card-title">Node Flush</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_node_through.webp">
    <img src="templates_grid_5_framing_node_through.webp" alt="Node Through">
    <span class="wood-card-title">Node Through</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_purlin_flush.webp">
    <img src="templates_grid_5_framing_purlin_flush.webp" alt="Purlin Flush">
    <span class="wood-card-title">Purlin Flush</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_purlin_hung.webp">
    <img src="templates_grid_5_framing_purlin_hung.webp" alt="Purlin Hung">
    <span class="wood-card-title">Purlin Hung</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_purlin_stacked.webp">
    <img src="templates_grid_5_framing_purlin_stacked.webp" alt="Purlin Stacked">
    <span class="wood-card-title">Purlin Stacked</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_profile_rectangle.webp">
    <img src="templates_grid_5_framing_profile_rectangle.webp" alt="Profile Rectangle">
    <span class="wood-card-title">Profile Rectangle</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_profile_round.webp">
    <img src="templates_grid_5_framing_profile_round.webp" alt="Profile Round">
    <span class="wood-card-title">Profile Round</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_profile_w.webp">
    <img src="templates_grid_5_framing_profile_w.webp" alt="Profile W">
    <span class="wood-card-title">Profile W</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_profile_hss.webp">
    <img src="templates_grid_5_framing_profile_hss.webp" alt="Profile HSS">
    <span class="wood-card-title">Profile HSS</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_profile_slab_band.webp">
    <img src="templates_grid_5_framing_profile_slab_band.webp" alt="Profile Slab Band">
    <span class="wood-card-title">Profile Slab Band</span>
  </a>
  <a class="wood-card" href="templates_grid_5_framing_profile_t.webp">
    <img src="templates_grid_5_framing_profile_t.webp" alt="Profile T">
    <span class="wood-card-title">Profile T</span>
  </a>
</div>

\include{lineno} templates_grid_5_framing.cpp
