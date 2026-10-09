# Grid {#templates_grid}

[TOC]

`wood_grid::Grid` is a multistorey timber building, a `WoodSession` framed level by level from a `Building` guide and a `Framing`.

![The L of five bays over two storeys](templates/3_elements_tree_all.webp)

A guide and a framing make a grid (`examples/3_elements_tree.cpp`):

```cpp
const wood_grid::Building building = wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS));
wood_grid::Grid grid(building, FRAMING, "elements_tree");
```

The constructor is the recipe:

```cpp
Grid::Grid(
    const Building& guide,
    const Framing& framing,
    const std::string& name
)
    : WoodSession(name),
      framing(compute_framing(framing)),
      guide(compute_guide(guide, this->framing)) {

    // plans: every level's member lines and column points at its datum
    add_plans();

    // storeys: the columns, walls, heads, members, purlins, decks and braces of every storey, under the level it caps
    for (size_t storey = 0; storey + 1 < this->guide.levels.size(); storey++)
        add_storey(storey);

    // contacts: an interaction between every two elements that touch, across the levels
    compute_face_contacts(0);
}
```

<details>
<summary><b>Tree</b></summary>

Every element is in the session, found by name.

```
grid
├── level_0
│   ├── plan_0                     the plan lines and column points at the ground
│   └── decks_0                    Plate deck_<i>_0, the ground decks
└── level_1                        level_2 .. alike, each capping the storey below it
    ├── plan_1
    ├── columns_1                  Column column_<i>_1
    ├── walls_1, core_walls_1      Plate wall_<i>_1, core_wall_<i>_1
    ├── heads_1, drop_panels_1     Block head_<i>_1, drop_panel_<i>_1
    ├── girders_1, beams_1         Beam girder_<i>_1, beam_<i>_1
    ├── purlins_1                  Beam purlin_<i>_1
    ├── decks_1                    Plate deck_<i>_1
    └── braces_1                   Beam brace_<i>_1
```

</details>

## Guide

- `Pattern`: the plan lines, `orthogonal(xs, ys, skew)`, `radial(radii, sectors, sweep)`, `triangular`, `hexagonal` or `from_lines`.
- `Building`: the levels, `from_solid(massing, elevations, pattern)` (A), `from_footprint(rings, elevations, pattern)` (B) or `from_lines(lines, surfaces)` (C); each level a datum, its cores and a `plan` mesh whose faces are bays, edges member lines and vertices column points.
- `Framing`: `system` 0 point supported, 1 post and beam, 2 purlin on girder; `span` the girder family; `node` 0 head, 1 flush, 2 through; `drop`, `deck`, `wall`, `head`, `reach`, `capital`, `panel`, `taper`, `facade` and a profile per role.

The plan attributes a user may set before building a Grid, all doubles:

| On | Attribute | Meaning |
|---|---|---|
| face | `system`, `span`, `floor` | the framing field for this bay; 1 under a deck |
| edge | `role` | 0 none, 1 girder, 2 beam, 3 purlin, 4 brace |
| edge | `wall` | 1 facade, 2 core wall |
| vertex | `column` | 0 none, 1 a column, 2 a transfer |

## The constructor, step by step

### Plans

Every level's plan is drawn at its datum: the member lines and the column points.

![The plans](templates/3_elements_tree_plan_2.webp)

```cpp
for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
    const Point start = compute_lift(*level.plan.vertex_point(edge.first), level.z);
    const Point end = compute_lift(*level.plan.vertex_point(edge.second), level.z);
    add_line(Line::from_points(start, end), plan);
}
```

### Columns

A column stands on every column point, from the deck top below to the head bottom or the datum.

![The columns](templates/3_elements_tree_columns_1.webp)

```cpp
for (const build::Stack& stack : stacks) {
    const double foot = lower.z + (framing.node == 2 ? 0.0 : build::compute_floor(lower.plan, stack.lower, framing));
    const double top = level.z + (framing.node == 0 ? build::compute_head_bottom(upper, stack.upper) : 0.0);
    const std::vector<Point>& section = upper.standing.at(stack.upper);
    columns.push_back(
        build::to_column(
            stack,
            section,
            foot,
            top
        )
    );
}

add_numbered(columns, storey + 1);
```

### Walls

Facade walls stand under the perimeter members, core walls in a pinwheel round every core.

![The facade walls](templates/templates_grid_1_footprint_walls_1.webp)

```cpp
for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
    const double wall = level.plan.edge_attribute(edge, "wall").value_or(0.0);
    if (wall == 0.0)
        continue;

    walls.push_back(
        build::to_wall(
            upper,
            edge,
            bottom,
            level.z,
            wall == 2.0 ? "core_wall" : "wall"
        )
    );
}
```

### Heads

Under node 0 every column gets a head, a pyramid each member rests on.

![The heads](templates/3_elements_tree_heads_1.webp)

```cpp
for (const build::Stack& stack : stacks) {
    const double bottom = level.z + build::compute_head_bottom(upper, stack.upper);
    const double top = level.z + build::compute_head_top(upper, stack.upper);
    const std::vector<std::shared_ptr<Element>> head = build::to_head(
        upper,
        level,
        stack.upper,
        bottom,
        top
    );
    heads[stack.upper] = head[0];
    blocks.insert(blocks.end(), head.begin(), head.end());
}
```

### Members

Girders and beams run on every plan edge with a role, their ends cut by the joint rules.

![The beams](templates/3_elements_tree_beams_1.webp)

```cpp
for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
    if (level.plan.edge_attribute(edge, "role").value_or(0.0) <= 0.0)
        continue;

    const std::vector<std::shared_ptr<Element>> beams = build::to_beam(
        upper,
        edge,
        level.z,
        outer
    );
    members.insert(members.end(), beams.begin(), beams.end());
}
```

Under system 2 purlins fill every bay at `spacing`, each end cut on what it lands on.

![The girders and purlins](templates/templates_grid_1_footprint_purlins_1.webp)

```cpp
for (const build::Station& station : build::compute_stations(upper, face, level.cores)) {
    const std::vector<std::shared_ptr<Element>> purlins = build::to_member(
        station.line.start(),
        station.line.end(),
        level.z - size.second / 2.0,
        profile,
        station.cuts,
        "purlin",
        size.first
    );
    members.insert(members.end(), purlins.begin(), purlins.end());
}
```

### Decks

A deck covers every bay, and a flush deck takes each corner head away through `add_interaction`.

![The decks](templates/3_elements_tree_decks_1.webp)

```cpp
for (const size_t vertex : build::compute_loop(level.plan, outline.first)) {
    if (!heads.count(vertex))
        continue;

    const Mesh pyramid = build::compute_pyramid(
        context,
        vertex,
        bottom,
        top,
        level.z
    );
    for (const std::shared_ptr<Element>& deck : decks) {
        const std::shared_ptr<wood_session::InteractionFeatureSolid> cut = std::make_shared<wood_session::InteractionFeatureSolid>(pyramid, wood_session::SolidOperation::subtract);
        add_interaction(heads.at(vertex), deck, cut);
    }
}
```

### Braces

A drawn brace runs through its storey, cut by the column faces, the deck at its foot and the members at its head.

![The braced frame](templates/templates_grid_3_lines_braced.webp)

```cpp
const std::vector<std::shared_ptr<Element>> brace = build::to_brace(
    line,
    ground,
    upper,
    z_lower,
    z_upper
);
braces.insert(braces.end(), brace.begin(), brace.end());
```

### Contacts

`compute_face_contacts(0)` pairs every two elements that touch, across the levels.

```cpp
compute_face_contacts(0);
```

## Joints

Every column point ranks its members: the highest runs through, the rest butt into its side or the column face, and only a pure corner is mitred.

![Heads the girders rest on](templates/templates_grid_5_framing_head_section.webp)

Under a point supported deck the head is conical or stepped under a drop panel (`capital`).

![A stepped head](templates/templates_grid_5_framing_head_stepped.webp)

Under node 2 the column runs through the levels, the decks notched round it.

![Columns through](templates/templates_grid_5_framing_node_through.webp)

`drop` hangs the girders below the purlins.

![Purlins hung](templates/templates_grid_5_framing_purlin_hung.webp)

Every role takes a profile of `wood_profile.h`.

![A W profile](templates/templates_grid_5_framing_profile_w.webp)

## Examples

Each gallery builds one Grid per case and grafts it under its own group.

### templates_grid_1_footprint

Workflow B, nine footprints and patterns: an L with purlins and facade walls, a skewed grid, radial, triangular and hexagonal cells, drawn axes, a courtyard, Branch3D's pentagon and its U with two cores.

![templates_grid_1_footprint](templates/templates_grid_1_footprint_all.webp)

<details><summary>Example code: templates_grid_1_footprint.cpp</summary>

\include{lineno} templates_grid_1_footprint.cpp

</details>

### templates_grid_2_solid

Workflow A, six massings sliced at their levels: a box, a pentagonal prism, a tapered loft, a podium with a tower, an atrium and a cylinder.

![templates_grid_2_solid](templates/templates_grid_2_solid_all.webp)

<details><summary>Example code: templates_grid_2_solid.cpp</summary>

\include{lineno} templates_grid_2_solid.cpp

</details>

### templates_grid_3_lines

Workflow C, line by line: compas_grid's crea dataset as drawn, and a braced frame.

![templates_grid_3_lines](templates/templates_grid_3_lines_all.webp)

<details><summary>Example code: templates_grid_3_lines.cpp</summary>

\include{lineno} templates_grid_3_lines.cpp

</details>

### templates_grid_4_reference

The reference configurations: Branch3D's square in its three methods, its residential L with a core, and the four FAST+EPP bays.

![templates_grid_4_reference](templates/templates_grid_4_reference_all.webp)

<details><summary>Example code: templates_grid_4_reference.cpp</summary>

\include{lineno} templates_grid_4_reference.cpp

</details>

### templates_grid_5_framing

The joints and the sections, fifteen bays side by side, failing on any overlap between two elements.

![templates_grid_5_framing](templates/templates_grid_5_framing_all.webp)

<details><summary>Example code: templates_grid_5_framing.cpp</summary>

\include{lineno} templates_grid_5_framing.cpp

</details>

### 3_elements_tree

The L of five bays over two storeys, a beam on every grid line, the picture at the top of this page.

<details><summary>Example code: 3_elements_tree.cpp</summary>

\include{lineno} 3_elements_tree.cpp

</details>
