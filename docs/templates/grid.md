# Grid {#templates_grid}

[TOC]

Planar, facade, and multi-storey timber grid generator: the inputs in `src/templates/grid/grid_guide.h`, the builders `Grid` and `Grids` in `src/templates/grid/grid.h`. The primary unit is a single planar `wood_grid::Plan` embedded on any 3D `Plane`—a single floor, an isolated bay (`Plan::bay`), a sloped roof, or a multi-layer vertical facade (`Plan::facade`)—framed by `wood_grid::Grid` and composed into shifted, tapered, twisted, or hybrid multi-storey buildings by `wood_grid::Stack`.

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
    <img src="templates_grid_4_developed_facade_stammhaus_elevation.webp" alt="Developed Timber Facade">
    <span class="wood-card-title">4 Developed Facade</span>
  </a>
  <a class="wood-card" href="#grid_example_5">
    <img src="templates_grid_5_multistorey_twisted_tower.webp" alt="Multi-Storey Topologies">
    <span class="wood-card-title">5 Multi-Storey Topologies</span>
  </a>
  <a class="wood-card" href="#grid_example_6">
    <img src="templates_grid_6_beam_grids_concentric_lantern.webp" alt="Section-Active Beam and Grillage Systems">
    <span class="wood-card-title">6 Beam &amp; Grillage Systems</span>
  </a>
  <a class="wood-card" href="#grid_example_7">
    <img src="templates_grid_7_frames_and_slabs_waffle_coffered_slab.webp" alt="Slab, Portal and Vierendeel Systems">
    <span class="wood-card-title">7 Slabs, Frames &amp; Vierendeel</span>
  </a>
  <a class="wood-card" href="#grid_assembly_sequence">
    <img src="assembly_templates_grid_1_bay_framing_slab_head.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

---

## Code Structure {#grid_architecture}

```mermaid
flowchart LR
  Pat["Pattern (u, v axes)"] --> Plan["Plan (on any 3D Plane)"]
  Plan -->|".bay(i) / .with_cantilever / .with_column_stride"| Bay["Support &amp; Overhang Plan"]
  Plan --> Grid["Grid (WoodSession + Contacts)"]
  Bay --> Grid
  Stack["Stack (Shifted / Tapered / Twisted / Vierendeel / Facades)"] --> Grid
```

1. **`Pattern`** generates 2D axis families in local `(u, v)` coordinates: `orthogonal` (including skewed grids and `compute_gradated_bays` shear-gradated spacings), `radial` (concentric glulam rings around an optional oculus), `triangular` (`60°` triaxial grillage), `hexagonal` (`120°` honeycomb), `diagonal` (`45°` half-step shifted square grillage with corner restraint), `concentric` (nested chiral or symmetric inscribed quadrangles), `ramified` (three-way branching tree girders), and `from_lines`.
2. **`Plan`** clips a `Pattern` against optional outer/opening boundary rings and core loops on any 3D `Plane`:
   - Horizontal `Plane` for a **single floor** (`Plan::from_pattern`, `Plan::from_footprint`).
   - Vertical or pitched `Plane` for a **timber facade** or **Vierendeel elevation** (`Plan::facade`, with window openings and gable silhouettes).
   - **Support &amp; overhang modifiers**: `Plan::bay(i)` / `Plan::bays({...})` isolates bays without mesh triangulation diagonals; `Plan::with_cantilever(u, v)` suppresses outer columns to create two- or four-sided cantilevers and single-column umbrellas; `Plan::with_column_stride(su, sv)` places columns on a coarser structural super-grid beneath fine grillages or waffle ribs; and `Plan::perimeter_columns_only()` clears interior columns for long-span halls.
3. **`Framing` &amp; `Grid`** build non-overlapping timber `NodeSupportElement`, `EdgeStraightElement`, `EdgeVariableElement`, `NodeBlockElement`, and `FacePlanarElement` solids with face-to-face contacts:
   - **Systems (`Framing::system`)**: `0` slab (point-supported flat or mushroom CLT slab), `1` posts and beams (`span >= 0` one-way hierarchy, `span = -1` homogeneous grillage, `span = -2` gradated ring/branch hierarchy by family index), `2` girders and flush purlins, `3` stacked purlins / one-way ribs on lowered girders, and `4` two-way coffered **waffle slab** with solid shear drop-panels around columns.
   - **Section-active shaping**: `haunch` deepens girders over standing supports and tapers cantilevered arms (`EdgeVariableElement`); `belly` adds parabolic midspan depth on simple or Gerber suspended spans; `drop_width` / `drop_depth` insert solid square shear drop-panels (`NodeBlockElement`) beneath mushroom slabs or inside waffle rib grids; `foot_scale` tapers hinged portal frame legs from base to rigid knee; and `pinned_feet` seats columns on `NodeSupportElement` base shoes.
   - **Column nodes (`Framing::node`) &amp; T-section carpentry**: `0` flush (columns stop at framing/haunch soffit), `1` through (continuous columns or 3-station vertical-knee tapered frame legs with beams and decks trimmed to column cheeks), and `2` head (**in-line** chamfered polygonal timber head block occupying `[-beam_depth, 0]`, or a flared pyramidal capital beneath a slab or drop-panel).
   - **Multi-storey topologies (`Stack`)**: `from_footprint`, `from_levels` (stepped terraces, shifted cantilevers, umbrella clusters, biaxial Vierendeel space frames, and mega-frame transfer towers), `morph` (`Xform(t)` scaling/twisting towers), and `add_facades`.
4. **Settings**: every design the examples show is a named preset of the guide (`Plan::l_shaped_floor`, `Framing::waffle_coffered_slab`, `Stack::tapered_portal_hall`, `Profiles::built_up`, ...), and **`Grids`** lays a list of `Design`s side by side along `x`, each in its group.

---

## Section-Active Element Library and Engel Taxonomy {#grid_engel_taxonomy}

Following Heino Engel (*Structure Systems / Tragsysteme*, Chapter 3: *Section-Active Structure Systems*), section-active systems resist external loads primarily through **cross-sectional bending moment and shear** ($M, V$) mobilized by axial fiber stress couples across member depth. Wood's seven-element library and joinery interactions map directly onto Engel's four section-active families:

| Wood Element / Mechanism | Geometric Representation | Engel Ch. 3 Section-Active Roles |
|---|---|---|
| **`EdgeStraightElement` + `wood_profile`** | Prismatic extrusion along axis with arbitrary outer/inner section loops (`profile_rectangle`, `profile_w`, `profile_hss`, `profile_t`, `profile_round`) and exact cylindrical BReps (`round_radius > 0`). | **3.1 Beams &amp; Columns**: solid/glulam beams, built-up timber I-joists (`profile_w`), hollow box girders (`profile_hss`), T-ribs (`profile_t`), round timber columns, Vierendeel chords/posts, and homogeneous/skewed/diagonal/concentric grillages. |
| **`EdgeVariableElement`** | Multi-station lofted solid (`sections` along `axis`) with planar side webs, constant top flange datum, and `cuts` bevel planes. | **3.1 Span Optimization &amp; 3.2 Hinged Frames**: parabolic midspan `belly` simple spans, continuous girders with support `haunch` deepening, tapered free cantilevers, articulated **Gerber** cantilever-and-suspended spans, leaning morphed columns, and 3-station **tapered portal frame legs** (`foot_scale < 1`). |
| **`FacePlanarElement`** | Thick planar plate between bottom and top polylines with interior hole loops (`BooleanPolyline` panelization and column notches). | **3.4 Slabs &amp; Core Walls**: one-way and two-way CLT slabs, point-supported flat/mushroom plates, thin top decks over ribbed and waffle floors, flush facade spandrels/cassettes, and pinwheel CLT shear cores. |
| **`NodeBlockElement`** | Lofted polyhedral solid block trimmed by boundary half-planes (`cuts`). | **3.4 Punching-Shear Capitals &amp; Drop Panels**: flared pyramidal under-slab capitals (`head`), 8-sided in-line beam capitals (`inline_head_uv`), and solid square column **drop-panels** (`drop`) under mushroom slabs (`Pilzdecke`) or flush within two-way coffered waffle grids. |
| **`NodeSupportElement`** | Parametric steel-and-pin base connector (`base_plate`, `rod`, `head_plate` as exact BRep cylinders). | **3.2 Hinged Frame Bearings**: pinned base shoes (`pinned_feet = true`) at the narrow feet of two-hinged portal frames and central-column umbrella canopies. |
| **Planar `cuts` &amp; `compute_face_contact`** | Multi-tier angular sector miter solver (`sector_miter_ccw`), girder/drop-panel face clipping (`split_outside_cores_uv`), and face-to-face contact graph (`WoodSession::graph`). | **Continuity &amp; Load-Transmission Hierarchy**: continuous primary girders with T-butted secondary beams/purlins (`span >= 0`), gradated ring/branch hierarchies (`span = -2`), symmetric 3-/4-/6-way grillage miters (`span = -1`), and waffle ribs butted flush to solid column drop-panels. |

### Mapping Engel's Four Sub-Chapters in `wood_grid`

1. **Beam Structures (Engel §3.1)**:
   - *Cross-section efficiency*: `Profiles::built_up()` pairs glulam girders with `profile_w` I-beam purlins and `profile_hss` hollow box edge beams (`Framing::built_up_profiles`).
   - *Multi-span moment shaping*: `Framing::belly_beams()` matches the positive parabolic moment of discontinuous simple spans; `Framing::haunched_cantilever()` deepens negative-moment zones over inset columns while tapering balanced end overhangs (`0.35 L`); `Plan::gerber_hall()` + `Framing::gerber_hall()` places zero-moment hinges at tapered cantilever tips carrying a parabolic drop-in span; and `Pattern::ramified()` branches six perimeter trunks into three-way Y-forks (`Framing::ramified_floor`).
2. **Rigid &amp; Multi-Panel Vierendeel Frames (Engel §3.2)**:
   - *Two-hinged &amp; cantilevered portal halls*: `Stack::tapered_portal_hall` combines `NodeSupportElement` base shoes, 3-station tapered `EdgeVariableElement` legs with vertical rigid knees, haunched main-span girders, and inward-tapering corridor cantilevers meeting at a central pin.
   - *Vierendeel shear gradation &amp; mega-frames*: `compute_gradated_bays` narrows Vierendeel panel widths where transverse chord shear peaks (`Plan::shear_vierendeel`), `Stack::concentric_vierendeel` crosses four story-deep Vierendeel frames over a column-free `24 m` hall, and `Stack::mega_transfer_tower` alternates full-span Vierendeel transfer storeys with lighter office floors.
3. **Beam Grids / Grillages (Engel §3.3)**:
   - *Homogeneous, diagonal &amp; concentric grillages*: besides `orthogonal`, `skewed`, `triangular`, and `hexagonal`, `Pattern::diagonal` rotates a square grillage `45°` inside a continuous orthogonal perimeter rim so short corner triangles restrain midspan deflection (`Framing::diagonal_floor`), while `Pattern::concentric` (`span = -2`) nests four chiral rotated rings of decreasing depth on eight inset perimeter fin columns with cantilevered corners (`Framing::concentric_floor`).
4. **Slab Structures (Engel §3.4)**:
   - *Mushroom, ribbed, waffle &amp; umbrella slabs*: `Framing::mushroom_drop_slab` stacks flared capitals and rectangular shear drop-panels under a cantilevered two-way slab; `Framing::ribbed_cantilever_slab` cantilevers closely spaced one-way ribs over inset girders; `Framing::waffle_coffered_slab` (`system = 4`) forms a four-sided cantilevered two-way coffered rib grid whose ribs terminate flush against solid `2 x 2`-coffer drop-panels over four inset columns; and `Stack::umbrella_canopies` steps four single-column cantilevered umbrellas at alternating clerestory heights.

---

<!-- assembly:begin -->
## Assembly Sequence {#grid_assembly_sequence}

Storey by storey, in each the base supports, columns and walls, their heads and shear drop-panels, girders, beams, purlins, braces and decks. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Bay framing**, 113 elements, 105 moves | | |
| ![beam head, 19 pieces](assembly/assembly_templates_grid_1_bay_framing_beam_head.gif) beam head, 19 pieces | ![floor l plan, 31 pieces](assembly/assembly_templates_grid_1_bay_framing_floor_l_plan.gif) floor l plan, 31 pieces | ![isolated bay, 9 pieces](assembly/assembly_templates_grid_1_bay_framing_isolated_bay.gif) isolated bay, 9 pieces |
| ![node flush, 13 pieces](assembly/assembly_templates_grid_1_bay_framing_node_flush.gif) node flush, 13 pieces | ![node through, 10 pieces](assembly/assembly_templates_grid_1_bay_framing_node_through.gif) node through, 10 pieces | ![purlin stacked, 10 pieces](assembly/assembly_templates_grid_1_bay_framing_purlin_stacked.gif) purlin stacked, 10 pieces |
| ![round columns, 11 pieces](assembly/assembly_templates_grid_1_bay_framing_round_columns.gif) round columns, 11 pieces | ![slab head, 10 pieces](assembly/assembly_templates_grid_1_bay_framing_slab_head.gif) slab head, 10 pieces |  |
| **Footprints**, 457 elements, 450 moves | | |
| ![courtyard, 70 pieces](assembly/assembly_templates_grid_2_footprints_courtyard.gif) courtyard, 70 pieces | ![hexagonal, 49 pieces](assembly/assembly_templates_grid_2_footprints_hexagonal.gif) hexagonal, 49 pieces | ![irregular, 53 pieces](assembly/assembly_templates_grid_2_footprints_irregular.gif) irregular, 53 pieces |
| ![orthogonal, 51 pieces](assembly/assembly_templates_grid_2_footprints_orthogonal.gif) orthogonal, 51 pieces | ![radial, 96 pieces](assembly/assembly_templates_grid_2_footprints_radial.gif) radial, 96 pieces | ![skewed, 75 pieces](assembly/assembly_templates_grid_2_footprints_skewed.gif) skewed, 75 pieces |
| ![triangular, 63 pieces](assembly/assembly_templates_grid_2_footprints_triangular.gif) triangular, 63 pieces |  |  |
| **Facades**, 224 elements, 220 moves | | |
| ![braced facade, 49 pieces](assembly/assembly_templates_grid_3_facades_braced_facade.gif) braced facade, 49 pieces | ![curtain mullion, 31 pieces](assembly/assembly_templates_grid_3_facades_curtain_mullion.gif) curtain mullion, 31 pieces | ![gable facade, 66 pieces](assembly/assembly_templates_grid_3_facades_gable_facade.gif) gable facade, 66 pieces |
| ![punched openings, 78 pieces](assembly/assembly_templates_grid_3_facades_punched_openings.gif) punched openings, 78 pieces |  |  |
| **Multi-storey**, 4101 elements, 4093 moves | | |
| ![core and facade, 504 pieces](assembly/assembly_templates_grid_5_multistorey_core_and_facade.gif) core and facade, 504 pieces | ![cylindrical rotunda, 640 pieces](assembly/assembly_templates_grid_5_multistorey_cylindrical_rotunda.gif) cylindrical rotunda, 640 pieces | ![five bay, 84 pieces](assembly/assembly_templates_grid_5_multistorey_five_bay.gif) five bay, 84 pieces |
| ![oblique flatiron, 424 pieces](assembly/assembly_templates_grid_5_multistorey_oblique_flatiron.gif) oblique flatiron, 424 pieces | ![setback tower, 948 pieces](assembly/assembly_templates_grid_5_multistorey_setback_tower.gif) setback tower, 948 pieces | ![shifted floors, 666 pieces](assembly/assembly_templates_grid_5_multistorey_shifted_floors.gif) shifted floors, 666 pieces |
| ![tapered tower, 429 pieces](assembly/assembly_templates_grid_5_multistorey_tapered_tower.gif) tapered tower, 429 pieces | ![twisted tower, 406 pieces](assembly/assembly_templates_grid_5_multistorey_twisted_tower.gif) twisted tower, 406 pieces |  |
| **Beam and grillage systems**, 347 elements, 340 moves | | |
| ![belly spans, 38 pieces](assembly/assembly_templates_grid_6_beam_grids_belly_spans.gif) belly spans, 38 pieces | ![built up profiles, 17 pieces](assembly/assembly_templates_grid_6_beam_grids_built_up_profiles.gif) built up profiles, 17 pieces | ![concentric lantern, 32 pieces](assembly/assembly_templates_grid_6_beam_grids_concentric_lantern.gif) concentric lantern, 32 pieces |
| ![diagonal grillage, 120 pieces](assembly/assembly_templates_grid_6_beam_grids_diagonal_grillage.gif) diagonal grillage, 120 pieces | ![gerber hall, 65 pieces](assembly/assembly_templates_grid_6_beam_grids_gerber_hall.gif) gerber hall, 65 pieces | ![haunched cantilever, 43 pieces](assembly/assembly_templates_grid_6_beam_grids_haunched_cantilever.gif) haunched cantilever, 43 pieces |
| ![ramified floor, 32 pieces](assembly/assembly_templates_grid_6_beam_grids_ramified_floor.gif) ramified floor, 32 pieces |  |  |
| **Slabs, portal and Vierendeel frames**, 1454 elements, 1446 moves | | |
| ![concentric vierendeel, 252 pieces](assembly/assembly_templates_grid_7_frames_and_slabs_concentric_vierendeel.gif) concentric vierendeel, 252 pieces | ![mega transfer tower, 422 pieces](assembly/assembly_templates_grid_7_frames_and_slabs_mega_transfer_tower.gif) mega transfer tower, 422 pieces | ![mushroom drop slab, 51 pieces](assembly/assembly_templates_grid_7_frames_and_slabs_mushroom_drop_slab.gif) mushroom drop slab, 51 pieces |
| ![ribbed cantilever slab, 83 pieces](assembly/assembly_templates_grid_7_frames_and_slabs_ribbed_cantilever_slab.gif) ribbed cantilever slab, 83 pieces | ![shear vierendeel, 88 pieces](assembly/assembly_templates_grid_7_frames_and_slabs_shear_vierendeel.gif) shear vierendeel, 88 pieces | ![tapered portal hall, 198 pieces](assembly/assembly_templates_grid_7_frames_and_slabs_tapered_portal_hall.gif) tapered portal hall, 198 pieces |
| ![umbrella canopies, 76 pieces](assembly/assembly_templates_grid_7_frames_and_slabs_umbrella_canopies.gif) umbrella canopies, 76 pieces | ![waffle coffered slab, 284 pieces](assembly/assembly_templates_grid_7_frames_and_slabs_waffle_coffered_slab.gif) waffle coffered slab, 284 pieces |  |

<!-- assembly:end -->

## Examples {#grid_examples}

### 1 Bay Isolation and In-Line Column Heads {#grid_example_1}

Isolates bay `0` from a five-bay L-shaped floor plan via `Plan::bay(0)`, then compares pure-timber framing and node details—including **in-line** chamfered timber column heads (`node = 2` sitting in `[-beam_depth, 0]` between square-ended beams), under-slab chamfered heads, flush girders and purlins, continuous through-columns, stacked purlins, and round timber columns.

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

Uses `Plan::facade` (`height = 0.0`) to frame vertical timber curtain walls, punched window/spandrel facades (`deck_flush = true`), co-planar X-braced heavy-timber frames (`brace = 2`), and pitched pentagonal gable walls (`brace = 3`).

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

Develops a five-storey timber headquarters inspired by the Blumer-Lehmann Stammhaus in Gossau: **(1)** primary glulam structure with an eight-bay recessed ground colonnade, four cantilevered upper CLT slabs, and a projecting flat timber roof cornice; **(2)** recessed ground storefront glazing and upper ribbon window mullions; **(3)** the isolated two-layer developed timber screen (`Framing::fin_screen`, `system = 3` on `Plan::facade`) pairing `760 mm` solid horizontal timber spandrel fascia bands in the inner layer with continuous syncopated vertical glulam fins (`{950, 680, 1240, 880, 750} mm` rhythm) standing proud in the outer layer; and **(4–6)** isometric, front-elevation, and corner-detail views of the complete headquarters.

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="templates_grid_4_developed_facade_stage_1_colonnade.webp">
    <img src="templates_grid_4_developed_facade_stage_1_colonnade.webp" alt="Stage 1 Colonnade and Cantilevered Slabs">
    <span class="wood-card-title">1 Colonnade &amp; Slabs</span>
  </a>
  <a class="wood-card" href="templates_grid_4_developed_facade_stage_2_spandrels.webp">
    <img src="templates_grid_4_developed_facade_stage_2_spandrels.webp" alt="Stage 2 Inner Window Envelope">
    <span class="wood-card-title">2 Window Envelope</span>
  </a>
  <a class="wood-card" href="templates_grid_4_developed_facade_stage_3_fin_screen.webp">
    <img src="templates_grid_4_developed_facade_stage_3_fin_screen.webp" alt="Stage 3 Two-Layer Fin Screen">
    <span class="wood-card-title">3 Two-Layer Fin Screen</span>
  </a>
  <a class="wood-card" href="templates_grid_4_developed_facade_stammhaus_headquarters.webp">
    <img src="templates_grid_4_developed_facade_stammhaus_headquarters.webp" alt="Stage 4 Complete Headquarters">
    <span class="wood-card-title">4 Complete Headquarters</span>
  </a>
  <a class="wood-card" href="templates_grid_4_developed_facade_stammhaus_elevation.webp">
    <img src="templates_grid_4_developed_facade_stammhaus_elevation.webp" alt="Front Elevation">
    <span class="wood-card-title">5 Front Elevation</span>
  </a>
  <a class="wood-card" href="templates_grid_4_developed_facade_stammhaus_detail.webp">
    <img src="templates_grid_4_developed_facade_stammhaus_detail.webp" alt="Corner Tectonic Detail">
    <span class="wood-card-title">6 Corner Tectonic Detail</span>
  </a>
</div>

\include{lineno} templates_grid_4_developed_facade.cpp

---

### 5 Multi-Storey Building Topologies {#grid_example_5}

Eight multi-storey timber building topologies composed via `Stack`: a two-storey L-frame with **in-line** chamfered column heads and 45°-notched flush CLT cassette decks (`five_bay`), five storeys of **shifted** cantilevered floor boxes over a plumb column grid (`Stack::from_levels`), a six-storey **tapered** tower with seamless continuous leaning columns (`Stack::morph`), a seven-storey `42°` **twisting** tower with seamless helicoidal round timber columns (`Stack::morph`), a six-storey cascading **stepped-terrace** building around an open central atrium (`Stack::from_levels`), a four-storey 16-sector **cylindrical** ring-and-radial rotunda (`Pattern::radial`), a four-storey chamfered pentagonal **flatiron** building with `45°` diagonal facade cuts, and a three-storey **core-and-braced** hybrid frame (`Stack::add_facades`).

<div class="wood-card-grid cols-4">
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
    <span class="wood-card-title">Twisted Tower (42°)</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_setback_tower.webp">
    <img src="templates_grid_5_multistorey_setback_tower.webp" alt="Stepped Atrium Terraces">
    <span class="wood-card-title">Stepped Atrium</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_cylindrical_rotunda.webp">
    <img src="templates_grid_5_multistorey_cylindrical_rotunda.webp" alt="Cylindrical Ring Rotunda">
    <span class="wood-card-title">Cylindrical Rotunda</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_oblique_flatiron.webp">
    <img src="templates_grid_5_multistorey_oblique_flatiron.webp" alt="Oblique Flatiron Prism">
    <span class="wood-card-title">Oblique Flatiron</span>
  </a>
  <a class="wood-card" href="templates_grid_5_multistorey_core_and_facade.webp">
    <img src="templates_grid_5_multistorey_core_and_facade.webp" alt="Core Walls and Braced Facades">
    <span class="wood-card-title">Core &amp; Braced Facades</span>
  </a>
</div>

\include{lineno} templates_grid_5_multistorey.cpp

---

### 6 Section-Active Beam and Grillage Systems {#grid_example_6}

Seven beam and grillage systems after Heino Engel (*Structure Systems*, §§3.1 &amp; 3.3): **(1)** built-up timber I-beam purlins (`profile_w`) and hollow box edge beams (`profile_hss`) on glulam girders; **(2)** five simple spans with parabolic midspan `belly` depth increase under stacked purlins; **(3)** continuous multi-span girders with support `haunch` deepening over inset columns and tapered balanced cantilevers; **(4)** an articulated **Gerber** hall whose paired-column girders taper to zero-moment hinges supporting a parabolic drop-in span; **(5)** a homogeneous `45°` **diagonal** square grillage restrained at the corners by a continuous orthogonal perimeter rim; **(6)** a gradated four-ring chiral **concentric** lantern grillage (`span = -2`) with cantilevered corners on perimeter fin columns; and **(7)** a three-way **ramified** branching beam floor on six perimeter round timber columns.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_6_beam_grids_built_up_profiles.webp">
    <img src="templates_grid_6_beam_grids_built_up_profiles.webp" alt="Built-Up I and Box Timber Profiles">
    <span class="wood-card-title">Built-Up I &amp; Box</span>
  </a>
  <a class="wood-card" href="templates_grid_6_beam_grids_belly_spans.webp">
    <img src="templates_grid_6_beam_grids_belly_spans.webp" alt="Simple Spans with Parabolic Belly">
    <span class="wood-card-title">Parabolic Belly Spans</span>
  </a>
  <a class="wood-card" href="templates_grid_6_beam_grids_haunched_cantilever.webp">
    <img src="templates_grid_6_beam_grids_haunched_cantilever.webp" alt="Continuous Haunched Cantilever Girders">
    <span class="wood-card-title">Haunched Cantilever</span>
  </a>
  <a class="wood-card" href="templates_grid_6_beam_grids_gerber_hall.webp">
    <img src="templates_grid_6_beam_grids_gerber_hall.webp" alt="Articulated Gerber Hall">
    <span class="wood-card-title">Articulated Gerber</span>
  </a>
  <a class="wood-card" href="templates_grid_6_beam_grids_diagonal_grillage.webp">
    <img src="templates_grid_6_beam_grids_diagonal_grillage.webp" alt="45-Degree Diagonal Square Grillage">
    <span class="wood-card-title">Diagonal Grillage (45°)</span>
  </a>
  <a class="wood-card" href="templates_grid_6_beam_grids_concentric_lantern.webp">
    <img src="templates_grid_6_beam_grids_concentric_lantern.webp" alt="Gradated Concentric Lantern Grid">
    <span class="wood-card-title">Concentric Lantern</span>
  </a>
  <a class="wood-card" href="templates_grid_6_beam_grids_ramified_floor.webp">
    <img src="templates_grid_6_beam_grids_ramified_floor.webp" alt="Ramified Branching Beam Floor">
    <span class="wood-card-title">Ramified Tree Floor</span>
  </a>
</div>

\include{lineno} templates_grid_6_beam_grids.cpp

---

### 7 Slab, Rigid Portal and Vierendeel Frame Systems {#grid_example_7}

Eight slab, rigid portal frame, and multi-panel Vierendeel systems after Heino Engel (*Structure Systems*, §§3.2 &amp; 3.4): **(1)** a cantilevered two-way CLT **mushroom slab** (`Pilzdecke`) on inset columns with two-stage flared capitals and square shear drop-panels; **(2)** a one-way closely **ribbed slab** cantilevering past two inset primary girder lines; **(3)** a four-sided cantilevered two-way coffered **waffle slab** (`system = 4`) on four inset columns with solid `2 x 2`-coffer shear drop-panels; **(4)** four staggered single-column cantilevered **umbrella canopies** on `NodeSupportElement` base shoes, tapered shafts, and chamfered capitals; **(5)** a three-storey coupled two-hinged **portal hall** with base shoes, tapered legs, rigid knees, and tapered corridor cantilevers; **(6)** a double-cantilevered eight-panel **Vierendeel** frame with shear-gradated bay widths; **(7)** four biaxial intersecting story-deep **concentric Vierendeel** frames spanning a `24 m` hall on eight perimeter columns; and **(8)** a six-storey **mega-frame** alternating full-span five-panel Vierendeel transfer storeys with lighter office floors.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_grid_7_frames_and_slabs_mushroom_drop_slab.webp">
    <img src="templates_grid_7_frames_and_slabs_mushroom_drop_slab.webp" alt="Mushroom Slab with Drop Panels">
    <span class="wood-card-title">Mushroom Drop Slab</span>
  </a>
  <a class="wood-card" href="templates_grid_7_frames_and_slabs_ribbed_cantilever_slab.webp">
    <img src="templates_grid_7_frames_and_slabs_ribbed_cantilever_slab.webp" alt="One-Way Ribbed Cantilever Slab">
    <span class="wood-card-title">Ribbed Cantilever Slab</span>
  </a>
  <a class="wood-card" href="templates_grid_7_frames_and_slabs_waffle_coffered_slab.webp">
    <img src="templates_grid_7_frames_and_slabs_waffle_coffered_slab.webp" alt="Two-Way Waffle Coffered Slab">
    <span class="wood-card-title">Waffle Coffered Slab</span>
  </a>
  <a class="wood-card" href="templates_grid_7_frames_and_slabs_umbrella_canopies.webp">
    <img src="templates_grid_7_frames_and_slabs_umbrella_canopies.webp" alt="Staggered Single-Column Umbrella Canopies">
    <span class="wood-card-title">Umbrella Canopies</span>
  </a>
  <a class="wood-card" href="templates_grid_7_frames_and_slabs_tapered_portal_hall.webp">
    <img src="templates_grid_7_frames_and_slabs_tapered_portal_hall.webp" alt="Tapered Hinged Portal Frame Hall">
    <span class="wood-card-title">Tapered Portal Hall</span>
  </a>
  <a class="wood-card" href="templates_grid_7_frames_and_slabs_shear_vierendeel.webp">
    <img src="templates_grid_7_frames_and_slabs_shear_vierendeel.webp" alt="Shear-Gradated Vierendeel Frame">
    <span class="wood-card-title">Shear-Gradated Vierendeel</span>
  </a>
  <a class="wood-card" href="templates_grid_7_frames_and_slabs_concentric_vierendeel.webp">
    <img src="templates_grid_7_frames_and_slabs_concentric_vierendeel.webp" alt="Biaxial Concentric Vierendeel Space Frame">
    <span class="wood-card-title">Concentric Vierendeel</span>
  </a>
  <a class="wood-card" href="templates_grid_7_frames_and_slabs_mega_transfer_tower.webp">
    <img src="templates_grid_7_frames_and_slabs_mega_transfer_tower.webp" alt="Vierendeel Mega-Frame Transfer Tower">
    <span class="wood-card-title">Mega-Frame Transfer</span>
  </a>
</div>

\include{lineno} templates_grid_7_frames_and_slabs.cpp
