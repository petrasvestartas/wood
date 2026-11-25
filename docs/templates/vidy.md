# Vidy {#templates_vidy}

[TOC]

`src/templates/vidy/vidy.h` — `VidyPavilion` (`28.7 × 19.1 × 10.3 m`, `11` bays, `45 / 300 mm`, `462` plates) + `OrigamiShell` (arbitrary planar origami mesh, `4` miter shells, selective chamfers, corner borders, stepped tenons).

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="#vidy_main_mesh">
    <img src="templates_vidy_1_main_mesh_mesh_axes.webp" alt="1. Main Mesh">
    <span class="wood-card-title">1. Main Mesh</span>
  </a>
  <a class="wood-card" href="#vidy_offset_mesh">
    <img src="templates_vidy_2_offset_mesh_two_bay_shells.webp" alt="2. Offset Shells">
    <span class="wood-card-title">2. Offset Shells</span>
  </a>
  <a class="wood-card" href="#vidy_panels">
    <img src="templates_vidy_3_panels_two_bay_panels.webp" alt="3. Two-Layer Panels">
    <span class="wood-card-title">3. Two-Layer Panels</span>
  </a>
  <a class="wood-card" href="#vidy_section_joints">
    <img src="templates_vidy_4_section_joints_full_pavilion.webp" alt="4. Joinery">
    <span class="wood-card-title">4. Joinery</span>
  </a>
  <a class="wood-card" href="#vidy_parametric">
    <img src="templates_vidy_5_parametric_parametric_family.webp" alt="5. Parametric">
    <span class="wood-card-title">5. Parametric</span>
  </a>
  <a class="wood-card" href="#vidy_origami_shapes">
    <img src="templates_vidy_6_origami_shapes_counter_fold.webp" alt="6. Deep Folded Halls">
    <span class="wood-card-title">6. Deep Folded Halls</span>
  </a>
  <a class="wood-card" href="#vidy_assembly_sequence">
    <img src="assembly_templates_vidy_4_section_joints.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

## Parameters

| Parameter | `VidyGuide` / `VidyPavilion` | `OrigamiShell` | Dimensions |
| --- | --- | --- | --- |
| `input_mesh` | `guide` (`11` × `2609.62`) | `Mesh` | Base planar origami mesh (`mm`) |
| `span_mid`, `span_end` | `19115.80`, `16285.82` | — | Mid span, end span (`mm`) |
| `wall_pleat` | `656.0` | — | Wall V-pleat (`mm`) |
| `eave_height_mid`, `eave_height_end` | `7482.78`, `6512.31` | — | Mid eave, end eave (`mm`) |
| `crown_height_mid`, `crown_height_end` | `10347.67`, `9723.09` | — | Mid crown, end crown (`mm`) |
| `plate_thickness`, `total_offset` | `45.0`, `300.0` | `45.0`, `300.0` | CLT thickness, shell depth (`mm`) |
| `with_joints`, `with_borders` | `true`, `true` | `true`, `true` | Stepped tenons, corner borders |
| `corner_scale`, `pin_pitch` | `1.0`, `500.0` | `1.0`, `500.0` | Saddle chamfer scale, pitch (`mm`) |

---

## 1. Main Mesh {#vidy_main_mesh}

`127` vertices, `23` stations (`6`-pt even / `5`-pt odd), `154` planar facets (`110` roof triangles, `44` wall quads), `12` bay axes.

<div class="wood-pair-grid">
  <img src="templates_vidy_1_main_mesh_mesh_axes.webp" alt="11-bay base mesh and bay axes">
  <img src="templates_vidy_1_main_mesh_plan.webp" alt="Top plan">
</div>

\include{lineno} templates_vidy_1_main_mesh.cpp

---

## 2. Offset Shells {#vidy_offset_mesh}

`4` shells at `h = {0, 45, 255, 300} mm` (`210 mm` cavity), `236` miter planes (`176` fold, `50` bay `XZ`, `10` gable `XZ`, `44` ground `z = 0`).

<div class="wood-pair-grid">
  <img src="templates_vidy_2_offset_mesh_offset_shells.webp" alt="11-bay offset shells and miter planes">
  <img src="templates_vidy_2_offset_mesh_two_bay_shells.webp" alt="2-bay offset shells">
</div>

\include{lineno} templates_vidy_2_offset_mesh.cpp

---

## 3. Two-Layer Panels {#vidy_panels}

`308` shell plates (`45 mm`) + `154` corner-to-corner perpendicular border boards (`45 × 210 mm`, `6` roof + `4` wall + `4` sill / bay, `1.0 mm` corner gap) = `462` plates (`0` collisions).

<div class="wood-pair-grid">
  <img src="templates_vidy_3_panels_pavilion.webp" alt="11-bay double-layer panels">
  <img src="templates_vidy_3_panels_two_bay_panels.webp" alt="2-bay 84-plate module">
</div>

\include{lineno} templates_vidy_3_panels.cpp

---

## 4. Joinery and Assembly {#vidy_section_joints}

`176` seams, `3181` pins (`951` `Double_Pin` + `2230` `Single_Pin`), `4132` mortise slots (`>= 44.6 mm` edge clearance), `500 mm` pitch, `1` insertion vector / plate:
- `Double_Pin`: `270 mm` base, `35 mm` shoulder, `180 mm` shaft, `140 mm` tip (`20 mm` bevel).
- `Single_Pin`: `180 mm` shaft, `140 mm` tip (`20 mm` bevel).
- `situ` eave: `2` `Double_Pin` + `2` `Single_Pin` / wall (`+Z`).

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_first_row_section.webp" alt="1-bay 42-plate jointed arch">
  <img src="templates_vidy_4_section_joints_full_pavilion.webp" alt="11-bay 462-plate jointed pavilion">
</div>

### Assembly (`42` plates / bay)

1. `module_1_haunch_wings` — `{5, 10}`, `{7, 12}` (`8` plates)
2. `module_2_haunches_joined` — left + right haunches (`8` plates)
3. `module_3_inner_crowns_eaves` — `6, 11` + `4, 8, 9, 13` inner (`14` plates)
4. `module_4_outer_covers` — outer covers + `6` roof borders (`26` plates)
5. `module_5_vertical_walls` — `{0, 1}`, `{2, 3}` + `8` wall/sill borders (`42` plates)
6. `module_6_roof_onto_walls` — `26` roof onto `16` walls (`-Z`)

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_module_1_haunch_wings.webp" alt="Step 1: Haunch wings">
  <img src="templates_vidy_4_section_joints_module_2_haunches_joined.webp" alt="Step 2: Haunches joined">
</div>

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_module_3_inner_crowns_eaves.webp" alt="Step 3: Inner crowns and eaves">
  <img src="templates_vidy_4_section_joints_module_4_outer_covers.webp" alt="Step 4: Outer covers and roof borders">
</div>

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_module_5_vertical_walls.webp" alt="Step 5: Vertical walls and borders">
  <img src="templates_vidy_4_section_joints_module_6_roof_onto_walls.webp" alt="Step 6: Roof onto walls">
</div>

\include{lineno} templates_vidy_4_section_joints.cpp

---

## 5. Parametric Variations {#vidy_parametric}

- `compact_symmetric`: `3` × `2400 mm`, `12.8 × 7.2 m`, `40 / 240 mm`, `126` plates (`84` + `42`), `744` pins (`0` collisions).
- `tapered_hall`: `5` × `2800 mm`, `21.0 -> 14.8 × 11.6 -> 9.0 m`, `45 / 320 mm`, `210` plates (`140` + `70`), `1450` pins (`0` collisions).
- `wide_shallow`: `4` × `2500 mm`, `22.0 -> 19.5 × 8.4 -> 7.8 m`, `45 / 280 mm`, `168` plates (`112` + `56`), `1132` pins (`0` collisions).

<div class="wood-pair-grid">
  <img src="templates_vidy_5_parametric_parametric_family.webp" alt="Parametric Vidy family">
  <img src="templates_vidy_5_parametric_compact_symmetric.webp" alt="Compact symmetric 3-bay">
</div>

<div class="wood-pair-grid">
  <img src="templates_vidy_5_parametric_tapered_hall.webp" alt="Tapered 5-bay hall">
  <img src="templates_vidy_5_parametric_wide_shallow.webp" alt="Wide shallow 4-bay">
</div>

\include{lineno} templates_vidy_5_parametric.cpp

---

## 6. Deep Folded Halls {#vidy_origami_shapes}

`OrigamiShell` (`45 mm` CLT + `210 mm` cavity + `45 mm` CLT = `300 mm`, `500 mm` pitch, `250 mm` row stagger, `180 / 270 mm` tenons, `|θ| <= 35°`, `0` unjoined creases, `0` collisions):
- `prismatic_fold`: `14.0 × 13.2 × 2.2 m`, `6` facets, `26` plates (`12` + `14`), `5` seams, `255` pins (`125` + `130`), `380` slots, `0.0°` skew.
- `counter_fold`: `14.0 × 13.2 × 4.4 m`, `18` facets, `50` plates (`36` + `14`), `20` seams, `444` pins (`212` + `232`), `656` slots, `24.0°` max skew.

<div class="wood-pair-grid">
  <img src="templates_vidy_6_origami_shapes_prismatic_fold.webp" alt="PrismaticFold 14 m hall">
  <img src="templates_vidy_6_origami_shapes_counter_fold.webp" alt="CounterFold 14 m hall">
</div>

\include{lineno} templates_vidy_6_origami_shapes.cpp

<!-- assembly:begin -->
## Assembly Sequence {#vidy_assembly_sequence}

A bay is two walls and one roof. Each of the three goes together plate by plate, its tenons into the plates before it, a shell plate along its one insertion vector where that is free. The roof is lowered on the walls, and the bays slide one after another along the hall. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Pavilion: wall, roof, bay, eleven bays**, 462 elements, 461 moves | | |
| ![wall left, 8 pieces](assembly/assembly_templates_vidy_4_section_joints_wall_left.gif) wall left, 8 pieces | ![wall right, 8 pieces](assembly/assembly_templates_vidy_4_section_joints_wall_right.gif) wall right, 8 pieces | ![roof, 26 pieces](assembly/assembly_templates_vidy_4_section_joints_roof.gif) roof, 26 pieces |
| ![bay, 3 pieces](assembly/assembly_templates_vidy_4_section_joints_bay.gif) bay, 3 pieces | ![vidy pavilion, 11 pieces](assembly/assembly_templates_vidy_4_section_joints.gif) vidy pavilion, 11 pieces |  |
| **Parametric family**, 504 elements, 501 moves | | |
| ![bay, 3 pieces](assembly/assembly_templates_vidy_5_parametric_compact_symmetric_bay.gif) bay, 3 pieces | ![compact symmetric, 3 pieces](assembly/assembly_templates_vidy_5_parametric_compact_symmetric.gif) compact symmetric, 3 pieces | ![bay, 3 pieces](assembly/assembly_templates_vidy_5_parametric_tapered_hall_bay.gif) bay, 3 pieces |
| ![tapered hall, 5 pieces](assembly/assembly_templates_vidy_5_parametric_tapered_hall.gif) tapered hall, 5 pieces | ![bay, 3 pieces](assembly/assembly_templates_vidy_5_parametric_wide_shallow_bay.gif) bay, 3 pieces | ![wide shallow, 4 pieces](assembly/assembly_templates_vidy_5_parametric_wide_shallow.gif) wide shallow, 4 pieces |
| **Deep folded halls**, 76 elements, 74 moves | | |
| ![counter fold, 50 pieces](assembly/assembly_templates_vidy_6_origami_shapes_counter_fold.gif) counter fold, 50 pieces | ![prismatic fold, 26 pieces](assembly/assembly_templates_vidy_6_origami_shapes_prismatic_fold.gif) prismatic fold, 26 pieces |  |

<!-- assembly:end -->
