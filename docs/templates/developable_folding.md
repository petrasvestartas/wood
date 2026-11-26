# Folding Developable {#templates_developable_folding}

[TOC]

Developable [Curved Plate](@ref elements_face_curved) structures including ruled tent shells, flat-foldable curved-crease unfolding (CCU) systems with axis-shift thickness and one-degree-of-freedom kinematic deployment, composite space-curve tubular vaults with harmonic torsion, and single-crease CCF/CCU quadrant analysis.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#curved_fold_roof">
    <img src="templates_developable_folding_1_curved_fold_lamellas.webp" alt="Tent Roof">
    <span class="wood-card-title">Tent Roof</span>
  </a>
  <a class="wood-card" href="#curved_crease_arch">
    <img src="templates_developable_folding_2_curved_crease_unfolding_plates.webp" alt="Corrugated Arch">
    <span class="wood-card-title">Corrugated Arch</span>
  </a>
  <a class="wood-card" href="#unfolding_sequence">
    <img src="templates_developable_folding_3_unfolding_sequence_stages.webp" alt="Accordion Unfolding">
    <span class="wood-card-title">Accordion Unfolding</span>
  </a>
  <a class="wood-card" href="#synclastic_unfolding">
    <img src="templates_developable_folding_5_synclastic_unfolding_plates.webp" alt="Synclastic Vault">
    <span class="wood-card-title">Synclastic Vault</span>
  </a>
  <a class="wood-card" href="#cellular_tube_vault">
    <img src="templates_developable_folding_6_cellular_tube_vault_plates.webp" alt="Cellular Tube Vault">
    <span class="wood-card-title">Cellular Tube Vault</span>
  </a>
  <a class="wood-card" href="#alternating_unfolding">
    <img src="templates_developable_folding_9_alternating_unfolding_plates.webp" alt="Alternating Vault">
    <span class="wood-card-title">Alternating Vault</span>
  </a>
  <a class="wood-card" href="#corrugated_dome">
    <img src="templates_developable_folding_11_corrugated_dome_plates.webp" alt="Corrugated Dome">
    <span class="wood-card-title">Corrugated Dome</span>
  </a>
  <a class="wood-card" href="#corrugated_funnel">
    <img src="templates_developable_folding_12_corrugated_funnel_plates.webp" alt="Corrugated Funnel">
    <span class="wood-card-title">Corrugated Funnel</span>
  </a>
  <a class="wood-card" href="#crease_quadrants">
    <img src="templates_developable_folding_4_crease_quadrants_quadrant_1.webp" alt="Crease Quadrants">
    <span class="wood-card-title">Crease Quadrants</span>
  </a>
  <a class="wood-card" href="#developable_folding_assembly_sequence">
    <img src="assembly_templates_developable_folding_1_curved_fold.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

## Unfolding a Curved Plate to a Flat Piece {#development}

`Development(element, samples)` extracts the zero-strain neutral mid-surface (`ε_0 = 0`), uniform plate thickness `t`, and angled miter bevel offsets (`bevel_a`, `bevel_b`) of a 3D `FaceCurvedElement`, unfolds the strip into the `xy`-plane by integrating its intrinsic quad sector angles `Δθ_2d = π - (β_back + β_fwd)` (`cos α = k_2d / k_3d`), and reconstructs `piece`: a closed 6-patch BRep flat solid timber piece on `z ∈ [0, t]` carrying the exact 3D curved miter bevels on both edges, placed side by side in group `flat`.

```cpp
const Development development(*plate, 48);
development.distortion; // ~1e-15 on exact cones/cylinders; relative isometric error on twisted strips
development.thickness;  // recovered plate thickness t (mm)
development.bevel_a;    // unfolded miter bevel offset t * cot(alpha) on crease a (mm)
development.piece;      // closed 6-patch BRep flat solid piece on z in [0, t] (placed in group "flat")
development.rulings;    // 2D straight ruling lines (plywood stiff-grain direction)
```

![](templates/templates_developable_folding_2_curved_crease_unfolding_flat.webp)

---

## Curved Fold Tent Roof {#curved_fold_roof}

Twelve-sector freestanding folded shell (`32 m` eaves diameter, `26 m` tower height, `24` `FaceCurvedElement` lamellas, `24` contacts) ruled between star-ring interpolated NURBS meridians.

\include{lineno} templates_developable_folding_1_curved_fold.cpp

![](templates/templates_developable_folding_1_curved_fold_lamellas.webp)

---

## Curved Crease Unfolding {#curved_crease_unfolding}

Curved-crease unfolding (CCU) joins stacked planar bending-active strips along common curved creases so that a compact flat-folded stack (`deployment = 0`, thickness `n * t`, 2D crease curvature `k_2d = k_3d * cos(opening)`) unfolds with a one-degree-of-freedom accordion mechanism into a globally doubly-curved corrugated shell (`deployment = 1`, folding angle `2 * opening`). Rotation axes shift across uniform plate thickness `t` to connect intrados-to-intrados at convex synclastic creases (`Q2`) and extrados-to-extrados at concave anticlastic creases (`Q4`), with wrapped miters locking the target angle (Scheder-Bieschin, Van Mele & Block, *ACADIA 2022*; *AAG 2023*).

| Factory | Geometry | Global Shape Control |
| --- | --- | --- |
| `catenary(span, rise, points)` | Funicular arch in the `xz`-plane | Compression-only corrugated vault / shell formwork profile |
| `vault(origin, along, axis, count, spacing, step)` | Parallel osculating planes; pair step `±(m + 0.5) * step` | Monoclastic (`step = 0`), synclastic dome-vault (`step > 0`), anticlastic saddle (`step < 0`), `0 mm` floor drift |
| `zigzag(origin, along, axis, count, spacing, tilt)` | Osculating planes `spacing` apart, alternating `±tilt` about `axis` | Alternating crown/support corrugation depth (`axis = x`) or sideways articulation (`axis = z`) |
| `leaning(origin, along, axis, count, spacing, lean)` | Progressive `i * lean` inclination about `axis` | Level-crown curved-plan or saddle-crown linear-plan vaults |
| `fan(origin, axis, first, count, angle)` | Radial meridian planes rotated by `angle` around `axis` | Closed `360°` corrugated rotational dome or funnel column |

### Monoclastic Corrugated Arch on Parallel Planes {#curved_crease_arch}

![](templates/templates_developable_folding_2_curved_crease_unfolding_plates.webp)

\include{lineno} templates_developable_folding_2_curved_crease_unfolding.cpp

### Accordion Unfolding Sequence from Closed Stack {#unfolding_sequence}

![](templates/templates_developable_folding_3_unfolding_sequence_stages.webp)

\include{lineno} templates_developable_folding_3_unfolding_sequence.cpp

### Doubly-Curved Synclastic Vault by Plane Spacing {#synclastic_unfolding}

![](templates/templates_developable_folding_5_synclastic_unfolding_plates.webp)

\include{lineno} templates_developable_folding_5_synclastic_unfolding.cpp

### Alternating Inclination About the Actuation Axis {#alternating_unfolding}

![](templates/templates_developable_folding_9_alternating_unfolding_plates.webp)

\include{lineno} templates_developable_folding_9_alternating_unfolding.cpp

### Corrugated Rotational Dome (24 Sectors) {#corrugated_dome}

![](templates/templates_developable_folding_11_corrugated_dome_plates.webp)

\include{lineno} templates_developable_folding_11_corrugated_dome.cpp

### Corrugated Funnel Column (32 Sectors) {#corrugated_funnel}

![](templates/templates_developable_folding_12_corrugated_funnel_plates.webp)

\include{lineno} templates_developable_folding_12_corrugated_funnel.cpp

---

## Composite Space-Curve Tubular Vault {#cellular_tube_vault}

`CurvedCreaseTube` generalizes curved-crease folding and unfolding from planar curves to non-planar 3D space curves with constant curvature `k_0` and harmonic torsion `τ_0(s) = a * k_0 * sin(2πs / s_0)` (Tachi, *Transformables 2013*). Solving `T(0.5 * s_0) · T(0) = 0` places all support quads coplanar on the horizontal ground plane at target opening `2 * opening`. Combining all four crease quadrants (`Q1` and `Q3` single-sheet CCF knees with `Q2` and `Q4` two-sheet CCU ridge/keel creases) around a parallelogram tube stabilizes single-crease ruling drift and preserves 1-DOF rigid-foldability across `2α ∈ (0°, 180°)` via `k(s; α) = k_0 * cos(α_0) / cos(α)` and `τ(s; α) = τ_0(s) * tan(α) / tan(α_0)`, while strip widths taper inversely (`w_R(s) * w_L(s) = const`) between supports and mid-span.

![](templates/templates_developable_folding_6_cellular_tube_vault_plates.webp)

![](templates/templates_developable_folding_6_cellular_tube_vault_flat.webp)

\include{lineno} templates_developable_folding_6_cellular_tube_vault.cpp

---

## Four Quadrants of a Curved Crease {#crease_quadrants}

| Quadrant | Strip Rulings | Mechanism & Initial State (`deployment = 0`) | Vertex Sector Sum & Gaussian Curvature |
| --- | --- | --- | --- |
| `Q1` | `(+r, -Mr)` | Classic CCF single-sheet folding (opens flat into one plane, angle `180° - 2α`) | `Σβ = 360°` — developable (convex + concave crease) |
| `Q2` | `(+r, +Mr)` | CCU two-sheet mirror unfolding (closes into `2t` stack, angle `2α`, intrados-to-intrados) | `Σβ < 360°` — synclastic (convex crease on both plates) |
| `Q3` | `(+Mr, -r)` | Classic CCF single-sheet folding (opens flat into one plane, angle `180° - 2α`) | `Σβ = 360°` — developable (convex + concave crease) |
| `Q4` | `(-r, -Mr)` | CCU two-sheet mirror unfolding (closes into `2t` stack, angle `2α`, extrados-to-extrados) | `Σβ > 360°` — anticlastic (concave crease on both plates) |

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_developable_folding_4_crease_quadrants_quadrant_1.webp">
    <img src="templates_developable_folding_4_crease_quadrants_quadrant_1.webp" alt="Q1">
    <span class="wood-card-title">Q1</span>
  </a>
  <a class="wood-card" href="templates_developable_folding_4_crease_quadrants_quadrant_2.webp">
    <img src="templates_developable_folding_4_crease_quadrants_quadrant_2.webp" alt="Q2">
    <span class="wood-card-title">Q2</span>
  </a>
  <a class="wood-card" href="templates_developable_folding_4_crease_quadrants_quadrant_3.webp">
    <img src="templates_developable_folding_4_crease_quadrants_quadrant_3.webp" alt="Q3">
    <span class="wood-card-title">Q3</span>
  </a>
  <a class="wood-card" href="templates_developable_folding_4_crease_quadrants_quadrant_4.webp">
    <img src="templates_developable_folding_4_crease_quadrants_quadrant_4.webp" alt="Q4">
    <span class="wood-card-title">Q4</span>
  </a>
</div>

\include{lineno} templates_developable_folding_4_crease_quadrants.cpp

<!-- assembly:begin -->
## Assembly Sequence {#developable_folding_assembly_sequence}

Two neighbours on their curved crease make one fold; the folds go round or side by side, and a flat-packed stack lifts off plate by plate. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Curved fold tent roof**, 24 elements, 23 moves | | |
| ![fold, 2 pieces](assembly/assembly_templates_developable_folding_1_curved_fold_fold.gif) fold, 2 pieces | ![curved fold, 12 pieces](assembly/assembly_templates_developable_folding_1_curved_fold.gif) curved fold, 12 pieces |  |
| **Curved crease arch**, 4 elements, 3 moves | | |
| ![fold, 2 pieces](assembly/assembly_templates_developable_folding_2_curved_crease_unfolding_fold.gif) fold, 2 pieces | ![curved crease unfolding, 2 pieces](assembly/assembly_templates_developable_folding_2_curved_crease_unfolding.gif) curved crease unfolding, 2 pieces |  |
| **Synclastic vault**, 4 elements, 3 moves | | |
| ![fold, 2 pieces](assembly/assembly_templates_developable_folding_5_synclastic_unfolding_fold.gif) fold, 2 pieces | ![curved crease unfolding, 2 pieces](assembly/assembly_templates_developable_folding_5_synclastic_unfolding.gif) curved crease unfolding, 2 pieces |  |
| **Alternating vault**, 4 elements, 3 moves | | |
| ![fold, 2 pieces](assembly/assembly_templates_developable_folding_9_alternating_unfolding_fold.gif) fold, 2 pieces | ![curved crease unfolding, 2 pieces](assembly/assembly_templates_developable_folding_9_alternating_unfolding.gif) curved crease unfolding, 2 pieces |  |
| **Corrugated dome**, 24 elements, 23 moves | | |
| ![fold, 2 pieces](assembly/assembly_templates_developable_folding_11_corrugated_dome_fold.gif) fold, 2 pieces | ![curved crease unfolding, 12 pieces](assembly/assembly_templates_developable_folding_11_corrugated_dome.gif) curved crease unfolding, 12 pieces |  |
| **Corrugated funnel**, 32 elements, 31 moves | | |
| ![fold, 2 pieces](assembly/assembly_templates_developable_folding_12_corrugated_funnel_fold.gif) fold, 2 pieces | ![curved crease unfolding, 16 pieces](assembly/assembly_templates_developable_folding_12_corrugated_funnel.gif) curved crease unfolding, 16 pieces |  |
| **Crease quadrants**, 8 elements, 4 moves | | |
| ![quadrant 1, 2 pieces](assembly/assembly_templates_developable_folding_4_crease_quadrants_quadrant_1.gif) quadrant 1, 2 pieces | ![quadrant 2, 2 pieces](assembly/assembly_templates_developable_folding_4_crease_quadrants_quadrant_2.gif) quadrant 2, 2 pieces | ![quadrant 3, 2 pieces](assembly/assembly_templates_developable_folding_4_crease_quadrants_quadrant_3.gif) quadrant 3, 2 pieces |
| ![quadrant 4, 2 pieces](assembly/assembly_templates_developable_folding_4_crease_quadrants_quadrant_4.gif) quadrant 4, 2 pieces |  |  |

<!-- assembly:end -->
