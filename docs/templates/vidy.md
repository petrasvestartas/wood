# Vidy {#templates_vidy}

[TOC]

Double-layer folded timber pavilion (`VidyGuide` and `VidyPavilion`) in `src/templates/vidy/vidy.h`, after Christopher Robeller and Yves Weinand's *Théâtre de Vidy* pavilion in Lausanne (IBOIS / EPFL, 2017). Each longitudinal bay combines **four planar vertical V-pleated wall quads** with **ten planar antiprismatic roof triangles** (`154` base facets across `11` bays, `308` unique `45 mm` 5-ply CLT plates separated by a `210 mm` insulated cavity for a `300 mm` structural shell depth). Native Vidy linked-tenon joints (`side_side_outplane_linked_tenons` on roof and eave creases, `side_side_outplane_linked_tenons_wall` on V-pleated wall and bay seams) and cross-layer `WoodSession::three_valence` shadow joints couple the two layers across the hollow cavity.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#vidy_main_mesh">
    <img src="templates_vidy_1_main_mesh_mesh_axes.webp" alt="1. Main Mesh Geometry">
    <span class="wood-card-title">1. Main Mesh Geometry</span>
  </a>
  <a class="wood-card" href="#vidy_offset_mesh">
    <img src="templates_vidy_2_offset_mesh_two_bay_shells.webp" alt="2. Offset Mesh">
    <span class="wood-card-title">2. Offset Mesh</span>
  </a>
  <a class="wood-card" href="#vidy_panels">
    <img src="templates_vidy_3_panels_two_bay_cutaway.webp" alt="3. Offset Mesh with Panels">
    <span class="wood-card-title">3. Offset Mesh with Panels</span>
  </a>
  <a class="wood-card" href="#vidy_section_joints">
    <img src="templates_vidy_4_section_joints_eave_joint_node.webp" alt="4. Double-Layer Vidy Joinery">
    <span class="wood-card-title">4. Double-Layer Vidy Joinery</span>
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
| `with_joints` | — | `true` | Runs native Vidy linked-tenon (`15`, `16`) and cross-layer shadow-joint solver |
| `corner_chamfer`, `division_length` | — | `0.0`, `350.0` | Optional corner relief setback and linked-tenon division length along fold creases in mm |

---

## 1. Main Mesh Geometry {#vidy_main_mesh}

`VidyGuide` constructs the doubly curved polygonal base mesh (`h = 0`, room-facing surface) over `2 * bays + 1` alternating transverse stations: six-vertex even bay-axis frames (`L_foot, L_eave, L_knee, R_knee, R_eave, R_foot`) and five-vertex odd mid-bay V-pleat frames (`L_foot_mid, L_eave_mid, Crown_mid, R_eave_mid, R_foot_mid`). Every wall facet is an exact vertical planar quad (`n_z = 0`) and every roof facet is an exact planar triangle whose interior fold bevels stay within the 5-axis CNC spindle tilt limit (`beta <= 58.6 deg`).

<div class="wood-pair-grid">
  <img src="templates_vidy_1_main_mesh_mesh_axes.webp" alt="Vidy 11-bay base polygonal mesh and transverse bay axes">
  <img src="templates_vidy_1_main_mesh_plan.webp" alt="Vidy barrel-curved plan with V-pleated walls and antiprismatic roof triangulation">
</div>

\include{lineno} templates_vidy_1_main_mesh.cpp

---

## 2. Offset Mesh {#vidy_offset_mesh}

Each topology edge receives its exact fabrication miter plane: angle-bisector planes on regular interior folds (`Regular`), vertical `XZ` separation planes between prefabricated bays (`BaySeparation`) and at gable ends (`GableEnd`), and the horizontal `z = 0` slab plane along wall feet (`Ground`). Three-plane corner intersections at `h = {0, 45, 255, 300} mm` produce the four concentric shell surfaces bounding both `45 mm` CLT layers and the `210 mm` insulation cavity.

<div class="wood-pair-grid">
  <img src="templates_vidy_2_offset_mesh_offset_shells.webp" alt="Inner and outer Vidy offset shells across all 11 bays">
  <img src="templates_vidy_2_offset_mesh_two_bay_shells.webp" alt="Two-bay cutaway of the four concentric Vidy offset surfaces">
</div>

\include{lineno} templates_vidy_2_offset_mesh.cpp

---

## 3. Offset Mesh with Panels {#vidy_panels}

With `with_joints = false`, `VidyPavilion` lofts each facet's paired offset rings into `308` solid `45 mm` `FacePlanarElement` panels (`88` vertical V-pleated wall plates and `220` antiprismatic roof plates) meeting flush along all mitered fold and bay-separation bevels.

<div class="wood-pair-grid">
  <img src="templates_vidy_3_panels_pavilion.webp" alt="Full 11-bay Théâtre de Vidy pavilion with 308 double-layer CLT panels">
  <img src="templates_vidy_3_panels_two_bay_cutaway.webp" alt="Two-bay cutaway revealing inner and outer 45 mm CLT panels and the 210 mm hollow cavity">
</div>

\include{lineno} templates_vidy_3_panels.cpp

---

## 4. Joinery of the Double-Layer Vidy System {#vidy_section_joints}

With `with_joints = true`, `VidyPavilion` populates `WoodSession::adjacency`, `WoodSession::three_valence = {{1}, {concave_a, concave_b, convex_b, convex_a}, ...}`, per-face joint codes `15` (`side_side_outplane_linked_tenons`) on roof/eave folds and `16` (`side_side_outplane_linked_tenons_wall`) on V-pleated wall seams, and corner-angle clearance setbacks in `settings.joint_volume_extension`, then runs `WoodSession::compute_features()` to merge paired linked tenons and cross-layer shadow mortises into `FacePlanarElement::features`.

<div class="wood-pair-grid">
  <img src="templates_vidy_4_section_joints_section.webp" alt="Full-scale 28-plate double-layer jointed Vidy bay arch section">
  <img src="templates_vidy_4_section_joints_cavity_cutaway.webp" alt="Cutaway exposing linked tenons and cross-layer shadow joints across the 210 mm cavity">
</div>

![Close-up of the four-plate Vidy eave node with paired chamfered linked tenon blades and wall lock tenons](templates_vidy_4_section_joints_eave_joint_node.webp)

\include{lineno} templates_vidy_4_section_joints.cpp
