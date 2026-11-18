# Vidy {#templates_vidy}

[TOC]

`src/templates/vidy/vidy.h` — `28.7 × 19.1 × 10.3 m`, `11` bays × `2609.6 mm`.
`45 mm` inner CLT + `210 mm` cavity + `45 mm` outer CLT = `300 mm` (`308` shell + `154` border = `462` plates).

<div class="wood-card-grid cols-4">
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
</div>

## Parameters

| Parameter | `VidyGuide` | `VidyPavilion` | Dimensions |
| --- | --- | --- | --- |
| `bays`, `bay_length` | `11`, `2609.62` | `guide` | Bay count, pitch (`mm`) |
| `span_mid`, `span_end` | `19115.80`, `16285.82` | `guide` | Mid span, end span (`mm`) |
| `wall_pleat` | `656.0` | `guide` | Wall V-pleat (`mm`) |
| `eave_height_mid`, `eave_height_end` | `7482.78`, `6512.31` | `guide` | Mid eave, end eave (`mm`) |
| `crown_height_mid`, `crown_height_end` | `10347.67`, `9723.09` | `guide` | Mid crown, end crown (`mm`) |
| `plate_thickness`, `total_offset` | `45.0`, `300.0` | `guide` | CLT thickness, shell depth (`mm`) |
| `with_joints` | — | `true` | Tenons and mortises |
| `corner_scale`, `pin_pitch` | — | `1.0`, `500.0` | Saddle chamfer scale, pitch (`mm`) |

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

`176` seams, `3187` pins (`954` `Double_Pin` + `2233` `Single_Pin`), `4141` mortise slots (`>= 44.6 mm` edge clearance), `500 mm` pitch, `1` insertion vector / plate:
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
