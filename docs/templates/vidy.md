# Vidy {#templates_vidy}

[TOC]

Double-layer folded timber pavilion (`VidyGuide` and `VidyPavilion`) in `src/templates/vidy/vidy.h`, after Christopher Robeller and Yves Weinand's *Théâtre de Vidy* pavilion in Lausanne (IBOIS / EPFL, 2017). Each longitudinal bay combines **four planar vertical V-pleated wall quads** with **ten planar antiprismatic roof triangles** (`154` base facets across `11` bays, `308` unique `45 mm` 5-ply CLT plates separated by a `210 mm` insulated cavity for a `300 mm` structural shell depth). Angle-proportional corner chamfers and transverse station clearance snips prevent multi-valent 3D corner collisions (`0` CSG overlaps across all `308` plates), while integral stepped chamfered **Double Through-Tenons (`Double_Pin`)**, interleaved **Single Through-Tenons (`Single_Pin` / `Extend_Pins`)**, and vertical `+Z` on-site eave tenons (`situ`) lock all `308` plates across all `11` bays.

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
| `with_joints` | — | `true` | Generates `990` `Double_Pin`s, `2,132` `Single/Extend_Pin`s (`3,122` pins) and `4,112` mortise slots |
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

With `with_joints = false`, `VidyPavilion` lofts all `154` inner and `154` outer `45 mm` `FacePlanarElement` panels (`308` panels total) with Robeller's angle-proportional corner chamfers (`(180 - delta) * factor`: `{0.40, 0.75, 1.08, 1.25}` across `h = {0, 45, 255, 300} mm`) and half-bay station clearance snips, producing `12`-vertex roof triangles and `16`-vertex wall quads with zero 3D solid collisions at five- and six-valent saddle nodes.

<div class="wood-pair-grid">
  <img src="templates_vidy_3_panels_pavilion.webp" alt="Full 11-bay Théâtre de Vidy pavilion with all 308 chamfered double-layer CLT panels">
  <img src="templates_vidy_3_panels_two_bay_panels.webp" alt="Complete 56-panel two-bay double-layer CLT module with both layers intact">
</div>

\include{lineno} templates_vidy_3_panels.cpp

---

## 4. Full-Pavilion Double-Layer Joinery and Module Assembly {#vidy_section_joints}

With `with_joints = true`, `VidyPavilion` generates production Vidy joinery across **all `308` panels of all `11` bays** (`176` jointed double-layer seams, `3,122` integral through-tenon pins, and `4,112` rectangular through-mortise slots): `12`-vertex stepped chamfered `Double_Pin`s (`270 mm` base shaft stepping by `35 mm` shoulders to a `180 mm` outer shaft and `140 mm` chamfered tip at `500 mm` pitch), `6`-vertex `Single_Pin`s, cavity-bridging `Extend_Pins` / `Extend_NoPins` shelves, and vertical `+Z` `situ` eave tenons.

| Metric | 3D Rhino Fabrication Model | `VidyPavilion` (`wood`) |
| --- | --- | --- |
| Double-layer CLT plates (`1` bay / `11` bays) | `28` / `308` | `28` / `308` |
| Jointed double-layer Regular seams (`1` bay / `11` bays) | `16` / `176` | `16` / `176` |
| Stepped `Double_Pin`s (`1` bay roof + wall / `11` bays) | `90` (`58` + `32`) / `992` | `90` (`58` + `32`) / `990` |
| `Single_Pin`s + `Extend_Pin`s (`11` bays) | `2,128` | `2,132` |
| Total integral through-tenon pins (`11` bays) | `3,120` | `3,122` |
| Rectangular through-mortise slots (`11` bays) | `4,111` | `4,112` |
| 3D Manifold CSG solid collisions (`308` plates) | `0` | `0` (`0.0 mm^3`) |

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_first_row_section.webp" alt="First-row 28-plate verification arch section matching the Rhino production joinery polylines">
  <img src="templates_vidy_4_section_joints_full_pavilion.webp" alt="Full 11-bay 308-plate Théâtre de Vidy pavilion with through-tenon joinery on every panel">
</div>

![Close-up of the assembled V-pleated left wall, vertical +Z situ eave tenon mortises, haunch joints and non-colliding corner chamfers](templates_vidy_4_section_joints_eave_joint_node.webp)

### Six-Step Module Assembly Sequence

Each `28`-plate prefabricated bay module follows the six-step IBOIS *Design for Assembly* sequence (`add_module_assembly_groups`):

1. **`module_1_haunch_wings`**: The left (`{5, 10}`) and right (`{7, 12}`) `4`-plate double-layer haunch wings close across their negative-fold valley creases (`11` outer `Double_Pin`s piercing into the inner valley plates).
2. **`module_2_haunches_joined`**: The two `4`-plate double-layer haunch wings slide inward to meet at the central crown apex.
3. **`module_3_inner_crowns_eaves`**: The two inner crown triangles (`6_in, 11_in`, carrying `6 + 5 = 11` stepped `Double_Pin`s along their crown bisectors) and four inner eave triangles (`4_in, 9_in` with `3` `Double_Pin`s; `8_in, 13_in` with `4` `Double_Pin`s) slide in-plane into the haunches.
4. **`module_4_outer_covers`**: The six pure-female (`0`-pin) outer cover triangles (`4_out, 6_out, 8_out, 9_out, 11_out, 13_out`) close along `-normal` onto the exposed `Single_Pin`s and `Extend_Pin`s, completing the `20`-plate double-layer roof arch.
5. **`module_5_vertical_walls`**: Beneath the completed `20`-plate roof arch, the two vertical V-pleated double-layer walls (`{0, 1}` and `{2, 3}`, `8` plates with `12` horizontal `Double_Pin`s + `12` `Extend_Pin`s + `12` `Single_Pin`s per V-pleat) slide together horizontally.
6. **`module_6_roof_onto_walls`**: The `20`-plate double-layer roof arch lowers vertically (`-Z`) onto the upward `+Z` `situ` tenons (`2` `Double_Pin`s on each inner wall quad + `2` `Single_Pin`s on each outer wall quad).

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_module_1_haunch_wings.webp" alt="Step 1: Left and right 4-plate double-layer haunch wings closing across their valley creases">
  <img src="templates_vidy_4_section_joints_module_2_haunches_joined.webp" alt="Step 2: Left and right double-layer haunch wings sliding inward to meet at the crown apex">
</div>

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_module_3_inner_crowns_eaves.webp" alt="Step 3: Inner crown and eave triangles sliding in-plane with stepped Double_Pins into the haunches">
  <img src="templates_vidy_4_section_joints_module_4_outer_covers.webp" alt="Step 4: Six pure-female outer cover triangles closing onto exposed Single_Pins and Extend_Pins">
</div>

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_module_5_vertical_walls.webp" alt="Step 5: Two vertical V-pleated double-layer walls assembling horizontally beneath the completed roof">
  <img src="templates_vidy_4_section_joints_module_6_roof_onto_walls.webp" alt="Step 6: Complete 20-plate double-layer roof arch lowering vertically onto the vertical walls">
</div>

\include{lineno} templates_vidy_4_section_joints.cpp
