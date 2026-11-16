# Vidy {#templates_vidy}

[TOC]

Double-layer folded timber pavilion (`VidyGuide` and `VidyPavilion`) in `src/templates/vidy/vidy.h`, after Christopher Robeller and Yves Weinand's *Théâtre de Vidy* pavilion in Lausanne (IBOIS / EPFL, 2017). Each longitudinal bay combines **four planar vertical V-pleated wall quads** with **ten planar antiprismatic roof triangles** (`154` base facets across `11` bays, `308` unique `45 mm` 5-ply CLT plates separated by a `210 mm` insulated cavity for a `300 mm` structural shell depth). Angle-proportional corner chamfers prevent multi-valent corner collisions, while integral stepped chamfered **Double Through-Tenons (`Double_Pin`)**, interleaved **Single Through-Tenons (`Single_Pin`)**, and vertical `+Z` on-site eave tenons (`situ`) lock all `308` plates across all `11` bays.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#vidy_main_mesh">
    <img src="templates_vidy_1_main_mesh_mesh_axes.webp" alt="1. Main Mesh Geometry">
    <span class="wood-card-title">1. Main Mesh Geometry</span>
  </a>
  <a class="wood-card" href="#vidy_offset_mesh">
    <img src="templates_vidy_2_offset_mesh_two_bay_shells.webp" alt="2. Offset Mesh and Miter Planes">
    <span class="wood-card-title">2. Offset Mesh &amp; Planes</span>
  </a>
  <a class="wood-card" href="#vidy_panels">
    <img src="templates_vidy_3_panels_two_bay_panels.webp" alt="3. Chamfered Two-Layer Panels">
    <span class="wood-card-title">3. Two-Layer Panels</span>
  </a>
  <a class="wood-card" href="#vidy_section_joints">
    <img src="templates_vidy_4_section_joints_full_pavilion.webp" alt="4. Full Pavilion Joinery">
    <span class="wood-card-title">4. Full Pavilion Joinery</span>
  </a>
</div>

## Parameters

| Parameter | `VidyGuide` | `VidyPavilion` | Description |
| --- | --- | --- | --- |
| `bays`, `bay_length` | `11`, `2609.62` | via `guide` | Prefabricated bay count and axial pitch in mm (`28.71 m` total length) |
| `span_mid`, `span_end` | `19115.80`, `16285.82` | via `guide` | Barrel-curved clear span at mid-pavilion (`y = 0`) and gable ends in mm |
| `wall_pleat` | `656.0` | via `guide` | Outward V-pleat amplitude of odd mid-bay vertical wall stations in mm |
| `eave_height_mid`, `eave_height_end` | `7482.78`, `6512.31` | via `guide` | Vertical pleated wall eave heights from centre to gable ends in mm |
| `crown_height_mid`, `crown_height_end` | `10347.67`, `9723.09` | via `guide` | Antiprismatic roof ridge heights in mm |
| `plate_thickness`, `total_offset` | `45.0`, `300.0` | via `guide` | Single-layer CLT thickness `t_plate` and total shell depth `t_offs` (`210 mm` cavity) |
| `with_joints` | — | `true` | Generates stepped `Double_Pin`s, `Single_Pin`s, `+Z` `situ` eave tenons and `3,618` mortise slots |
| `corner_scale`, `pin_pitch` | — | `1.0`, `500.0` | Angle-proportional multi-valent corner chamfer scale and tenon pitch in mm |

---

## 1. Main Mesh Geometry {#vidy_main_mesh}

`VidyGuide` constructs the doubly curved polygonal base mesh (`h = 0`, room-facing surface) over `2 * bays + 1` alternating transverse stations: six-vertex even bay-axis frames (`L_foot, L_eave, L_knee, R_knee, R_eave, R_foot`) and five-vertex odd mid-bay V-pleat frames (`L_foot_mid, L_eave_mid, Crown_mid, R_eave_mid, R_foot_mid`). Every wall facet is an exact vertical planar quad (`n_z = 0`) and every roof facet is an exact planar triangle whose interior fold bevels stay within the 5-axis CNC spindle tilt limit (`beta < 60 deg`).

<div class="wood-pair-grid">
  <img src="templates_vidy_1_main_mesh_mesh_axes.webp" alt="Vidy 11-bay base polygonal mesh and transverse bay axes">
  <img src="templates_vidy_1_main_mesh_plan.webp" alt="Vidy barrel-curved plan with V-pleated walls and antiprismatic roof triangulation">
</div>

\include{lineno} templates_vidy_1_main_mesh.cpp

---

## 2. Offset Mesh and Miter Planes {#vidy_offset_mesh}

Each topology edge carries its exact fabrication miter plane: angle-bisector planes on the `176` regular interior folds (`Regular`), vertical `XZ` separation planes on the `50` inter-bay axes (`BaySeparation`) and `10` gable edges (`GableEnd`), and the horizontal `z = 0` slab plane along the `44` wall feet (`Ground`). Three-plane corner intersections at `h = {0, 45, 255, 300} mm` produce all four complete concentric offset shells together with all `236` miter connection ribbons (`miter_planes_mesh()`).

<div class="wood-pair-grid">
  <img src="templates_vidy_2_offset_mesh_offset_shells.webp" alt="Complete 11-bay inner and outer Vidy offset shells with 236 miter connection planes">
  <img src="templates_vidy_2_offset_mesh_two_bay_shells.webp" alt="Complete two-bay four-surface offset shells and angle-bisector miter planes">
</div>

\include{lineno} templates_vidy_2_offset_mesh.cpp

---

## 3. Offset Mesh with Two-Layer Panels {#vidy_panels}

With `with_joints = false`, `VidyPavilion` lofts all `154` inner and `154` outer `45 mm` `FacePlanarElement` panels (`308` panels total) with Robeller's angle-proportional corner chamfers (`(180 - delta) * factor`: `0.30` on `h = 0`, `0.70` on `h = 45, 255, 300`), producing `12`-vertex roof triangles and `16`-vertex wall quads that prevent 3D corner collisions at five- and six-valent nodes.

<div class="wood-pair-grid">
  <img src="templates_vidy_3_panels_pavilion.webp" alt="Full 11-bay Théâtre de Vidy pavilion with all 308 chamfered double-layer CLT panels">
  <img src="templates_vidy_3_panels_two_bay_panels.webp" alt="Complete 56-panel two-bay double-layer CLT module with both layers intact">
</div>

\include{lineno} templates_vidy_3_panels.cpp

---

## 4. Full-Pavilion Double-Layer Joinery and Erection Sequence {#vidy_section_joints}

With `with_joints = true`, `VidyPavilion` generates production Vidy joinery across **all `308` panels of all `11` bays** (`3,618` rectangular through-mortise slots): `12`-vertex stepped chamfered `Double_Pin`s (`270 mm` base shaft stepping by `35 mm` shoulders to a `180 mm` outer shaft and `140 mm` chamfered tip at `500 mm` pitch), `6`-vertex `Single_Pin`s, cavity-bridging `Extend_Pins` / `Extend_NoPins` tabs, and vertical `+Z` `situ` eave tenons.

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_first_row_section.webp" alt="First-row 28-plate verification arch section matching the Rhino production joinery polylines">
  <img src="templates_vidy_4_section_joints_full_pavilion.webp" alt="Full 11-bay 308-plate Théâtre de Vidy pavilion with through-tenon joinery on every panel">
</div>

![Close-up of the V-pleated left wall, vertical +Z situ eave tenons, stepped haunch Double_Pins and non-colliding corner chamfers](templates_vidy_4_section_joints_eave_joint_node.webp)

### Real-Building Four-Stage Insertion Sequence

1. **Vertical walls assembled first** (`erection_1_walls`): inner and outer V-pleated wall quads slide horizontally along their in-plane insertion vectors.
2. **Roof bottom layer assembled separately** (`erection_2_roof_bottom`): the ten inner roof triangles insert gradually one after another from valley haunches to crown bisectors to eaves.
3. **Roof top layer assembled onto bottom layer** (`erection_3_roof_top`): the ten outer roof triangles slide gradually onto the stepped `Double_Pin` spacers across the `210 mm` cavity.
4. **Whole double-layer roof lowered onto vertical walls** (`erection_4_roof_onto_walls`): the completed `20`-plate double-layer roof drops vertically along `-Z` onto the walls' `+Z` `situ` eave tenons.

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_erection_1_walls.webp" alt="Step 1: Vertical V-pleated double-layer walls assembled first">
  <img src="templates_vidy_4_section_joints_erection_2_roof_bottom.webp" alt="Step 2: Roof bottom (inner) layer plates inserted gradually one after another">
</div>

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_erection_3_roof_top.webp" alt="Step 3: Roof top (outer) layer plates inserted gradually onto the bottom roof layer">
  <img src="templates_vidy_4_section_joints_erection_4_roof_onto_walls.webp" alt="Step 4: Complete double-layer roof lowered vertically onto the vertical walls">
</div>

\include{lineno} templates_vidy_4_section_joints.cpp
