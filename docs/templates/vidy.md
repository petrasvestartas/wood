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
    <img src="templates_vidy_6_origami_shapes_miura_arch.webp" alt="6. Origami Shapes">
    <span class="wood-card-title">6. Origami Shapes</span>
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
| `plate_thickness`, `total_offset` | `45.0`, `300.0` | `40.0`, `240.0` | CLT thickness, shell depth (`mm`) |
| `with_joints`, `with_borders` | `true`, `true` | `true`, `true` | Stepped tenons, corner borders |
| `corner_scale`, `pin_pitch` | `1.0`, `500.0` | `1.0`, `450.0` | Saddle chamfer scale, pitch (`mm`) |

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

## 6. Origami Shapes {#vidy_origami_shapes}

`OrigamiShell` (`40 mm` CLT + `160 mm` cavity + `40 mm` CLT = `240 mm`, `1` insertion vector / plate, `0` collisions):
- `miura_arch`: `12.0 × 9.6 × 4.4 m`, `36` quads, `96` plates (`72` + `24`), `52` seams, `209` pins, `291` slots.
- `yoshimura_vault`: `12.0 × 8.4 × 4.6 m`, `60` triangles, `136` plates (`120` + `16`), `58` seams, `286` pins, `394` slots.
- `counter_fold`: `12.8 × 9.6 × 3.0 m`, `18` facets, `50` plates (`36` + `14`), `20` seams, `478` pins, `720` slots.
- `cross_fold`: `14.4 × 14.4 × 5.6 m`, `16` facets, `40` plates (`32` + `8`), `20` seams, `528` pins, `783` slots.

<div class="wood-pair-grid">
  <img src="templates_vidy_6_origami_shapes_miura_arch.webp" alt="Miura-ori quad arch">
  <img src="templates_vidy_6_origami_shapes_yoshimura_vault.webp" alt="Yoshimura diamond vault">
</div>

<div class="wood-pair-grid">
  <img src="templates_vidy_6_origami_shapes_counter_fold.webp" alt="CounterFold span">
  <img src="templates_vidy_6_origami_shapes_cross_fold.webp" alt="CrossFold dome">
</div>

\include{lineno} templates_vidy_6_origami_shapes.cpp
