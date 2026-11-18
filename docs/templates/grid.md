# Grid {#templates_grid}

[TOC]

Planar, facade, and multi-storey timber grid generator in `src/templates/grid/grid.h`. The primary unit is a single planar `wood_grid::Plan` embedded on any 3D `Plane`—a single floor, an isolated bay (`Plan::bay`), a sloped roof, or a multi-layer vertical facade (`Plan::facade`)—framed by `wood_grid::Grid` and composed into shifted, tapered, twisted, or hybrid multi-storey buildings by `wood_grid::Stack`.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#grid_example_1">
    <img src="templates_grid_1_bay_framing_beam_head.webp" alt="Bay Isolation and In-Line Heads">
    <span class="wood-card-title">1 Bay &amp; In-Line Heads</span>
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
    <img src="templates_grid_4_developed_facade_stammhaus_headquarters.webp" alt="Developed Timber Facade">
    <span class="wood-card-title">4 Developed Facade</span>
  </a>
  <a class="wood-card" href="#grid_example_5">
    <img src="templates_grid_5_multistorey_twisted_tower.webp" alt="Multi-Storey Topologies">
    <span class="wood-card-title">5 Multi-Storey Topologies</span>
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
  Stack["Stack (Shifted / Tapered / Twisted / Facades)"] --> Grid
```

1. **`Pattern`** generates 2D axis families in local `(u, v)` coordinates: `orthogonal` (including skewed grids), `radial` (concentric glulam rings around an optional oculus), `triangular` (`60°` diagrid), `hexagonal` (`120°` honeycomb), and `from_lines`.
2. **`Plan`** clips a `Pattern` against optional outer/opening boundary rings and core loops on any 3D `Plane`:
   - Horizontal `Plane` for a **single floor** (`Plan::from_pattern`, `Plan::from_footprint`).
   - Vertical or pitched `Plane` for a **timber facade** (`Plan::facade`, with window openings and gable silhouettes).
   - **`Plan::bay(i)` / `Plan::bays({...})`** isolates one bay or a subset of bays as its own standalone `Plan` without mesh triangulation diagonals.
3. **`Framing` & `Grid`** build non-overlapping timber `EdgeStraightElement`, `NodeBlockElement`, and `FacePlanarElement` solids with face-to-face contacts:
   - **Systems (`System`)**: `Slab` (point-supported CLT), `PostAndBeam` (girders and transverse/ring beams), and `GirderPurlin` (`PurlinMode::Flush` or `PurlinMode::Stacked`—which on a vertical facade plane places continuous outer vertical timber fins proud of inner horizontal spandrel ledgers).
   - **Column nodes (`NodeMode`)**: `Flush` (columns stop at framing soffit), `Through` (continuous columns with beams and decks trimmed to column faces), and `Head` (**in-line** chamfered polygonal timber head block occupying `[-beam_depth, 0]` flush with beam tops and soffits, or beneath a beamless `Slab`).
   - **Multi-storey topologies (`Stack`)**: `from_footprint`, `from_levels` (setbacks/cantilevers), `shifted` (staggered cantilevered floor plates over a plumb column grid), `tapered` (scaling floors with inclined perimeter columns), and `twisted` (progressively rotated floors with helicoidal columns).

---

## Examples {#grid_examples}

### 1 Bay Isolation and In-Line Column Heads {#grid_example_1}

Isolates bay `0` from a five-bay L-shaped floor plan via `Plan::bay(0)`, then compares pure-timber framing and node details—including **in-line** chamfered timber column heads (`NodeMode::Head` sitting in `[-beam_depth, 0]` between square-ended beams), under-slab chamfered heads, flush girders and purlins, continuous through-columns, stacked purlins, and round timber columns.

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
    <img src="templates_grid_1_bay_framing_beam_head.webp" alt="In-Line Column Heads">
    <span class="wood-card-title">In-Line Column Heads</span>
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

Frames single-floor footprints across seven planar grid patterns: orthogonal L-floor, `30°` skewed parallelogram with `60°`-beveled girder ends coplanar with the skewed perimeter, `12`-sector concentric glulam ring pavilion, equilateral `60°` triangular diagrid, `120°` hexagonal honeycomb with symmetric three-way Y-miters at every node, five-sided irregular polygon, and a courtyard floor plate.

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

### 4 Developed Timber Facade (Stammhaus Progression) {#grid_example_4}

Develops a five-storey timber headquarters facade inspired by the Blumer-Lehmann Stammhaus in Gossau across four stages: **(1)** recessed ground-floor glulam colonnade, cantilevered upper CLT floor slabs, and projecting roof cornice; **(2)** recessed ground storefront glazing and upper-storey punched timber spandrel bands; **(3)** the isolated two-layer brise-soleil screen where syncopated vertical glulam fins (`{1150, 850, 1450, 1350} mm` rhythm per `4800 mm` bay) run continuously in front of horizontal loggia ledgers via `PurlinMode::Stacked` on a vertical `Plane`; and **(4)** the complete four-sided headquarters assembly.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_4_developed_facade_stage_1_colonnade.webp">
    <img src="templates_grid_4_developed_facade_stage_1_colonnade.webp" alt="Stage 1 Colonnade and Cantilevered Slabs">
    <span class="wood-card-title">1 Colonnade &amp; Slabs</span>
  </a>
  <a class="wood-card" href="templates_grid_4_developed_facade_stage_2_spandrels.webp">
    <img src="templates_grid_4_developed_facade_stage_2_spandrels.webp" alt="Stage 2 Inner Spandrel Envelope">
    <span class="wood-card-title">2 Spandrel Envelope</span>
  </a>
  <a class="wood-card" href="templates_grid_4_developed_facade_stage_3_fin_screen.webp">
    <img src="templates_grid_4_developed_facade_stage_3_fin_screen.webp" alt="Stage 3 Modulated Vertical Fin Screen">
    <span class="wood-card-title">3 Modulated Fin Screen</span>
  </a>
  <a class="wood-card" href="templates_grid_4_developed_facade_stammhaus_headquarters.webp">
    <img src="templates_grid_4_developed_facade_stammhaus_headquarters.webp" alt="Stage 4 Complete Headquarters">
    <span class="wood-card-title">4 Complete Headquarters</span>
  </a>
</div>

\include{lineno} templates_grid_4_developed_facade.cpp

---

### 5 Multi-Storey Building Topologies {#grid_example_5}

Six multi-storey timber building topologies composed via `Stack`: a two-storey L-frame with **in-line** chamfered column heads and 45°-notched flush CLT cassette decks (`five_bay`), five storeys of **shifted** cantilevered floor boxes over a plumb column grid (`Stack::shifted`), a six-storey **tapered** tower with inclined perimeter columns (`Stack::tapered`), a six-storey `30°` **twisting** tower with helicoidal round timber columns (`Stack::twisted`), a five-storey podium and **setback** tower (`Stack::from_levels`), and a three-storey **core-and-braced** hybrid frame (`Stack::add_perimeter_facades`).

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="templates_grid_5_multistorey_five_bay.webp">
    <img src="templates_grid_5_multistorey_five_bay.webp" alt="Five-Bay L Frame with In-Line Heads">
    <span class="wood-card-title">Five-Bay (In-Line Heads)</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_shifted_floors.webp">
    <img src="templates_grid_5_multistorey_shifted_floors.webp" alt="Shifted Cantilevered Floors">
    <span class="wood-card-title">Shifted Cantilevers</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_tapered_tower.webp">
    <img src="templates_grid_5_multistorey_tapered_tower.webp" alt="Tapered Timber Tower">
    <span class="wood-card-title">Tapered Tower</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_twisted_tower.webp">
    <img src="templates_grid_5_multistorey_twisted_tower.webp" alt="Twisted Timber Tower">
    <span class="wood-card-title">Twisted Tower (30°)</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_setback_tower.webp">
    <img src="templates_grid_5_multistorey_setback_tower.webp" alt="Podium and Setback Tower">
    <span class="wood-card-title">Setback Tower</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_core_and_facade.webp">
    <img src="templates_grid_5_multistorey_core_and_facade.webp" alt="Core Walls and Braced Facades">
    <span class="wood-card-title">Core &amp; Braced Facades</span>
  </a>
</div>

\include{lineno} templates_grid_5_multistorey.cpp
