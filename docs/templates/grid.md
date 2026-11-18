# Grid {#templates_grid}

[TOC]

Planar and multi-storey timber grid generator in `src/templates/grid/grid.h`. The primary unit is a single planar `wood_grid::Plan` embedded on any 3D `Plane`—a single floor, an isolated bay (`Plan::bay`), a sloped roof, or a vertical facade (`Plan::facade`)—framed by `wood_grid::Grid` and composed into multi-storey buildings by `wood_grid::Stack`.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#grid_example_1">
    <img src="templates_grid_1_bay_framing_isolated_bay.webp" alt="Bay Isolation and Framing">
    <span class="wood-card-title">1 Bay &amp; Framing</span>
  </a>
  <a class="wood-card" href="#grid_example_2">
    <img src="templates_grid_2_footprints_radial.webp" alt="Planar Floor Footprints">
    <span class="wood-card-title">2 Floor Footprints</span>
  </a>
  <a class="wood-card" href="#grid_example_3">
    <img src="templates_grid_3_facades_braced_facade.webp" alt="Timber Facade Grids">
    <span class="wood-card-title">3 Facade Grids</span>
  </a>
  <a class="wood-card" href="#grid_example_4">
    <img src="templates_grid_4_multistorey_five_bay.webp" alt="Multi-Storey Stack">
    <span class="wood-card-title">4 Multi-Storey Stack</span>
  </a>
</div>

---

## Architecture {#grid_architecture}

```mermaid
flowchart LR
  Pat["Pattern (u, v axes)"] --> Plan["Plan (on any 3D Plane)"]
  Plan -->|".bay(i) / .bays(...)"| Bay["Isolated Bay Plan"]
  Plan --> Grid["Grid (WoodSession + Contacts)"]
  Bay --> Grid
  Stack["Stack (Floor + Facade Plans)"] --> Grid
```

Instead of coupling every model to a horizontal multi-storey stack, `wood_grid` separates three clean layers:

1. **`Pattern`** generates 2D axis families in local `(u, v)` coordinates: `orthogonal` (including skewed grids), `radial` (concentric glulam rings around an optional oculus), `triangular` (`60°` diagrid), `hexagonal` (`120°` honeycomb), and `from_lines`.
2. **`Plan`** clips a `Pattern` against optional outer/opening boundary rings and core loops on any 3D `Plane`:
   - Horizontal `Plane` for a **single floor** (`Plan::from_pattern`, `Plan::from_footprint`).
   - Vertical or pitched `Plane` for a **timber facade** (`Plan::facade`, with window openings and gable silhouettes).
   - **`Plan::bay(i)` / `Plan::bays({...})`** isolates one bay or a subset of bays as its own standalone `Plan` without mesh triangulation diagonals.
3. **`Framing` & `Grid`** turn one `Plan` (or a multi-storey `Stack`) into non-overlapping timber `EdgeStraightElement`, `NodeBlockElement`, and `FacePlanarElement` solids with face-to-face contacts:
   - **Systems (`System`)**: `Slab` (point-supported CLT), `PostAndBeam` (girders and transverse/ring beams on grid axes), and `GirderPurlin` (`PurlinMode::Flush` or `PurlinMode::Stacked`).
   - **Column nodes (`NodeMode`)**: `Flush` (columns stop at framing soffit), `Through` (continuous columns with beams and decks trimmed to column faces), and `Head` (one clean chamfered timber head block beneath the slab or girders, clipped flush on perimeter/corner columns).
   - **In-plane bracing (`BraceMode`)**: `Diagonal`, `Cross` (co-planar split X-bracing), and `Chevron`, trimmed cleanly inside quadrilateral bays.

```cpp
const Plane floor_plane(Point(0.0, 0.0, 3600.0), Vector(1, 0, 0), Vector(0, 1, 0));
const wood_grid::Plan floor = wood_grid::Plan::from_footprint(
    {footprint}, wood_grid::Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {4000.0, 4000.0}), {}, floor_plane
);

// Frame the entire floor, or isolate bay 0 alone:
wood_grid::Grid whole_floor(floor, framing, "floor");
wood_grid::Grid single_bay(floor.bay(0), framing, "bay_0");
```

---

## Examples {#grid_examples}

### 1 Bay Isolation and Timber Framing Modes {#grid_example_1}

Isolates bay `0` from a five-bay L-shaped floor plan via `Plan::bay(0)`, then compares pure-timber column, head, girder, purlin, and CLT deck configurations on a single bay (`0.000 mm³` pairwise overlap across all elements).

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_1_bay_framing_floor_l_plan.webp">
    <img src="templates_grid_1_bay_framing_floor_l_plan.webp" alt="Five-Bay L Floor">
    <span class="wood-card-title">L-Plan Floor</span>
  </a>
  <a class="wood-card" href="templates_grid_1_bay_framing_isolated_bay.webp">
    <img src="templates_grid_1_bay_framing_isolated_bay.webp" alt="Isolated Bay 0">
    <span class="wood-card-title">Isolated Bay (bay(0))</span>
  </a>
  <a class="wood-card" href="templates_grid_1_bay_framing_slab_head.webp">
    <img src="templates_grid_1_bay_framing_slab_head.webp" alt="CLT Slab on Chamfered Head">
    <span class="wood-card-title">Slab + Chamfered Head</span>
  </a>
  <a class="wood-card" href="templates_grid_1_bay_framing_beam_head.webp">
    <img src="templates_grid_1_bay_framing_beam_head.webp" alt="Post-and-Beam on Chamfered Head">
    <span class="wood-card-title">Beams + Chamfered Head</span>
  </a>
  <a class="wood-card" href="templates_grid_1_bay_framing_node_flush.webp">
    <img src="templates_grid_1_bay_framing_node_flush.webp" alt="Flush Girders and Purlins">
    <span class="wood-card-title">Flush Node &amp; Purlins</span>
  </a>
  <a class="wood-card" href="templates_grid_1_bay_framing_node_through.webp">
    <img src="templates_grid_1_bay_framing_node_through.webp" alt="Continuous Through-Columns">
    <span class="wood-card-title">Through-Columns</span>
  </a>
  <a class="wood-card" href="templates_grid_1_bay_framing_purlin_stacked.webp">
    <img src="templates_grid_1_bay_framing_purlin_stacked.webp" alt="Stacked Purlins on Girders">
    <span class="wood-card-title">Stacked Purlins</span>
  </a>
  <a class="wood-card" href="templates_grid_1_bay_framing_round_columns.webp">
    <img src="templates_grid_1_bay_framing_round_columns.webp" alt="Round Timber Columns">
    <span class="wood-card-title">Round Timber Columns</span>
  </a>
</div>

\include{lineno} templates_grid_1_bay_framing.cpp

---

### 2 Planar Floor Patterns and Footprints {#grid_example_2}

Frames single-floor footprints across seven planar grid patterns: orthogonal L-floor, `30°` skewed parallelogram, `12`-sector concentric glulam ring pavilion with an open central oculus and full ring beams, equilateral `60°` triangular diagrid, `120°` hexagonal honeycomb, five-sided irregular polygon with hand-drawn axes, and a courtyard floor plate.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_2_footprints_orthogonal.webp">
    <img src="templates_grid_2_footprints_orthogonal.webp" alt="Orthogonal L-Footprint">
    <span class="wood-card-title">Orthogonal</span>
  </a>
  <a class="wood-card" href="templates_grid_2_footprints_skewed.webp">
    <img src="templates_grid_2_footprints_skewed.webp" alt="Skewed 30 deg">
    <span class="wood-card-title">Skewed (30°)</span>
  </a>
  <a class="wood-card" href="templates_grid_2_footprints_radial.webp">
    <img src="templates_grid_2_footprints_radial.webp" alt="Radial Ring Pavilion">
    <span class="wood-card-title">Radial Ring</span>
  </a>
  <a class="wood-card" href="templates_grid_2_footprints_triangular.webp">
    <img src="templates_grid_2_footprints_triangular.webp" alt="Triangular Diagrid">
    <span class="wood-card-title">Triangular (60°)</span>
  </a>
  <a class="wood-card" href="templates_grid_2_footprints_hexagonal.webp">
    <img src="templates_grid_2_footprints_hexagonal.webp" alt="Hexagonal Honeycomb">
    <span class="wood-card-title">Hexagonal (120°)</span>
  </a>
  <a class="wood-card" href="templates_grid_2_footprints_irregular.webp">
    <img src="templates_grid_2_footprints_irregular.webp" alt="Irregular Axes">
    <span class="wood-card-title">Irregular</span>
  </a>
  <a class="wood-card" href="templates_grid_2_footprints_courtyard.webp">
    <img src="templates_grid_2_footprints_courtyard.webp" alt="Courtyard">
    <span class="wood-card-title">Courtyard</span>
  </a>
</div>

\include{lineno} templates_grid_2_footprints.cpp

---

### 3 Timber Facade Grids {#grid_example_3}

Uses `Plan::facade` (`post_height = 0.0`) to frame vertical timber curtain walls, punched window/spandrel facades (`deck_flush = true`), co-planar X-braced heavy-timber frames (`BraceMode::Cross`), and pitched pentagonal gable walls (`BraceMode::Chevron`).

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_3_facades_curtain_mullion.webp">
    <img src="templates_grid_3_facades_curtain_mullion.webp" alt="Mullion and Transom Curtain Wall">
    <span class="wood-card-title">Mullion &amp; Transom</span>
  </a>
  <a class="wood-card" href="templates_grid_3_facades_punched_openings.webp">
    <img src="templates_grid_3_facades_punched_openings.webp" alt="Punched Window Openings">
    <span class="wood-card-title">Punched Openings</span>
  </a>
  <a class="wood-card" href="templates_grid_3_facades_braced_facade.webp">
    <img src="templates_grid_3_facades_braced_facade.webp" alt="X-Braced Timber Facade">
    <span class="wood-card-title">X-Braced Facade</span>
  </a>
  <a class="wood-card" href="templates_grid_3_facades_gable_facade.webp">
    <img src="templates_grid_3_facades_gable_facade.webp" alt="Pitched Gable Facade">
    <span class="wood-card-title">Gable Facade</span>
  </a>
</div>

\include{lineno} templates_grid_3_facades.cpp

---

### 4 Multi-Storey Composition {#grid_example_4}

Composes single-floor and facade `Plan`s across storeys with `wood_grid::Stack`: a two-storey L-shaped five-bay building with chamfered timber heads, a five-storey podium and setback tower (`Stack::from_levels`) with continuous through-columns and notched CLT floor plates, and a three-storey hybrid timber building with pinwheel CLT core walls and braced perimeter facades (`Stack::add_perimeter_facades`).

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="templates_grid_4_multistorey_five_bay.webp">
    <img src="templates_grid_4_multistorey_five_bay.webp" alt="Five-Bay L Building">
    <span class="wood-card-title">Five-Bay L Frame</span>
  </a>
  <a class="wood-card" href="templates_grid_4_multistorey_setback_tower.webp">
    <img src="templates_grid_4_multistorey_setback_tower.webp" alt="Podium and Setback Tower">
    <span class="wood-card-title">Setback Tower</span>
  </a>
  <a class="wood-card" href="templates_grid_4_multistorey_core_and_facade.webp">
    <img src="templates_grid_4_multistorey_core_and_facade.webp" alt="Core Walls and Perimeter Facades">
    <span class="wood-card-title">Core &amp; Braced Facades</span>
  </a>
</div>

\include{lineno} templates_grid_4_multistorey.cpp
