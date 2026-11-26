# Folding Developable {#templates_developable_folding}

[TOC]

Developable [Curved Plate](@ref elements_face_curved) structures: a ruled tent roof, flat-foldable curved-crease unfolding (CCU) with one-degree-of-freedom deployment, composite space-curve tube vaults with harmonic torsion, the four CCF/CCU quadrants of a single crease (`src/templates/developable_folding/`, after Tachi, *Transformables 2013*), and one-DOF rigid foldable curved folds and Miura tessellations from a space curve (after Tachi, *IASS 2011*).

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
  <a class="wood-card" href="#space_curve_fold">
    <img src="templates_developable_folding_13_space_curve_fold_stages.webp" alt="Space Curve Fold">
    <span class="wood-card-title">Space Curve Fold</span>
  </a>
  <a class="wood-card" href="#curved_miura">
    <img src="templates_developable_folding_14_curved_miura_stages.webp" alt="Curved Miura">
    <span class="wood-card-title">Curved Miura</span>
  </a>
  <a class="wood-card" href="#developable_folding_assembly_sequence">
    <img src="assembly_templates_developable_folding_1_curved_fold.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

## Surface First, Thickness Last {#developable_folding_stages}

Folding needs no thickness. Every template builds its geometry on the surface first: the creases, the rulings, every strip as a zero-thickness quad mesh (`Development::strip`), the 2D crease pattern (every strip developed flat, `Development::add_pattern`, group `pattern`) and the deployment stages. The distortion and the drift are measured there. Only then, as the last step, a thickness turns every strip into a plate and every plate into its flat piece for cutting; with `thickness = 0` the template stops after the surfaces and makes no element.

- `CurvedFold`: the surface is the outer face between the meridians (`compute_rings`, `add_meridians`, `add_strips`); the lamellas go under it, their inner faces where the neighbours offset by `t` meet (`compute_inner_rings`, `add_lamellas`, `add_flat`).
- `CurvedCreaseUnfolding`: the mid-surface strips are traced at the target and deployed hinged on their shared creases (`compute_target`, `compute_stages` with `compute_reaches(target, 0)`, `add_strips`); the plates are deployed again with their hinges moved onto the faces, the axis shift of paper 5.3, so a closed stack is `n * t` thick (`compute_reaches(target, t)`, `add_plates`, `add_flat`). At `w = 1` the plates' mid-surface is the strips'.
- `CreaseQuadrants`: the surface is every strip's hinge face (`compute_frame`, `compute_strips`, `add_strips`); the body goes `t` to its side, mitered at the crease (`add_plates`, `add_flat`).
- `CurvedCreaseTube`: the walls run between spine, ridge and keel (`compute_profile`, `compute_stage`, `add_walls`); every plate is `t` thick centred on its wall, mitered where the walls meet (`add_plates`, `add_flat`).
- `SpaceCurveFold` and `CurvedMiura`: the quads are folded from their crease pattern (`SpaceCurveRidge`, `folded`, `add_quads`); every quad of every stage becomes a plate last, mitered at its folds (`add_plates`).

Every template is a `WoodSession` built from its guide in `developable_folding_guide.h`, a struct of fields with defaults and named presets:

```cpp
CurvedCreaseUnfoldingGuide guide = CurvedCreaseUnfoldingGuide::synclastic();
guide.thickness = 0.0;                      // the strips, creases and crease pattern alone
CurvedCreaseUnfolding unfolding(guide);
unfolding.distortion();                     // the same with and without thickness
```

## Unfolding a Curved Plate to a Flat Piece {#development}

`Development(a, b, samples)` unfolds the ruled strip between two curves into the `xy`-plane by its quad sector angles (`cos α = k_2d / k_3d`); its `outline` is the strip's cut in the crease pattern. `Development(plate, samples)` does the same for the zero-strain neutral mid-surface (`ε_0 = 0`) of a `FaceCurvedElement`, recovers its thickness `t` and miter bevels (`bevel_a`, `bevel_b`), and rebuilds `piece`: a closed 6-patch BRep flat solid on `z ∈ [0, t]` carrying the exact 3D curved miters, placed side by side in group `flat`.

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

Twelve-sector freestanding folded shell (`32 m` eaves diameter, `26 m` tower height, `24` `FaceCurvedElement` lamellas, `24` contacts) ruled between star-ring interpolated NURBS meridians. `CurvedFoldGuide::tent_roof()` is the default:

| Field | Default | Description |
| --- | --- | --- |
| `sectors` | `12` | Folds around the axis, a ridge and a valley each |
| `radius`, `top_radius` | `16000.0`, `3500.0` | The eaves' radius at the ridges, the tower's at the top |
| `eave`, `height`, `gable` | `4000.0`, `26000.0`, `4000.0` | The valleys at the eaves, the tower's top, the ridges' rise over the eaves |
| `inset` | `0.1` | The valleys' radius inside the ridges', a fraction of the radius |
| `rings`, `samples` | `14`, `32` | Rings the meridians run through, rulings per strip |
| `thickness` | `120.0` | Every lamella under the outer surface, `0` for the surface alone |

\include{lineno} templates_developable_folding_1_curved_fold.cpp

![](templates/templates_developable_folding_1_curved_fold_lamellas.webp)

---

## Curved Crease Unfolding {#curved_crease_unfolding}

Curved-crease unfolding (CCU) joins stacked planar bending-active strips along common curved creases so that a compact flat-folded stack (`deployment = 0`, 2D crease curvature `k_2d = k_3d * cos(opening)`) unfolds with a one-degree-of-freedom accordion mechanism into a globally doubly-curved corrugated shell (`deployment = 1`, folding angle `2 * opening`). The strips deploy on their mid-surface, hinged on their shared creases; the thickness comes last, its rotation axes shifted across `t` to connect intrados to intrados at convex synclastic creases (`Q2`) and extrados to extrados at concave anticlastic creases (`Q4`), with miters locking the target angle and a closed stack `n * t` thick.

| Field | Default | Description |
| --- | --- | --- |
| `crease` | arc of `9 m` span, `4.5 m` rise | The target crease, on one of the planes |
| `opening` | `55°` | Target angle between the rulings and the crease's plane, in `(0°, 90°)` |
| `planes` | `vault(..., 12, 450.0, 0.0)` | The reflection planes in order, a plate between every two |
| `deployments` | `{1.0}` | Per stage `w = alpha / opening`; two or more side by side in `stage_<s>` |
| `thickness` | `30.0` | Every plate, `0` for the surfaces alone |
| `samples` | `48` | Rulings per plate |

| Factory | Geometry | Global Shape Control |
| --- | --- | --- |
| `catenary(span, rise, points)` | Funicular arch in the `xz`-plane | Compression-only corrugated vault / shell formwork profile |
| `vault(origin, along, axis, count, spacing, step)` | Parallel planes; pair step `±(m + 0.5) * step` | Monoclastic (`step = 0`), synclastic (`step > 0`), anticlastic (`step < 0`), `0 mm` floor drift |
| `zigzag(origin, along, axis, count, spacing, tilt)` | Planes `spacing` apart, every other pair `±tilt` about `axis` | Alternating crown/support corrugation (`axis = x`) or sideways articulation (`axis = z`) |
| `leaning(origin, along, axis, count, spacing, lean)` | Progressive `i * lean` inclination about `axis` | Level-crown curved-plan or saddle-crown linear-plan vaults |
| `fan(origin, axis, first, count, angle)` | Radial meridian planes rotated by `angle` around `axis` | Closed `360°` corrugated dome or funnel column |

The presets shape the vault by the crease's profile, the distance between the planes and their inclination:

| Preset | Rule | Example |
| --- | --- | --- |
| `monoclastic()` | Constant plane distance, the default | 2 |
| `synclastic()` | Plane distance decreasing towards the middle | 5 |
| `anticlastic()` | Plane distance decreasing away from the middle | |
| `catenary_vault()` | Catenary crease profile | |
| `unfolding_sequence()` | The catenary vault at `w = 0, 0.35, 0.70, 1` | 3 |
| `alternating()` | Inclination around the actuation axis, alternating | 9 |
| `level_crown()` | Inclination around the actuation axis, progressive, the crown level | |
| `asymmetric()` | Inclination perpendicular to the actuation axis, sideways | |
| `dome()` | Consecutive radial inclinations, a dome | 11 |
| `funnel()` | Consecutive radial inclinations, a funnel column | 12 |

Arc and catenary creases are there; the Elastica, the funicular under asymmetric loads and the freeform crease with inflection points are not (no factory solves them; a freeform crease can be passed as any planar `NurbsCurve`, untested).

Every unfolding is shown at its design angle, the shape it is designed for and fixed in (the formwork's or the vault's), and closed: the plates stacked flat, n x t thick, as they are cut, joined and transported. Its examples build both with `guide.deployments = {0.0, 1.0}`.

### Monoclastic Corrugated Arch on Parallel Planes {#curved_crease_arch}

| unfolded to its design angle | closed for transport |
| --- | --- |
| ![](templates/templates_developable_folding_2_curved_crease_unfolding_plates.webp) | ![](templates/templates_developable_folding_2_curved_crease_unfolding_closed.webp) |

\include{lineno} templates_developable_folding_2_curved_crease_unfolding.cpp

### Accordion Unfolding Sequence from Closed Stack {#unfolding_sequence}

![](templates/templates_developable_folding_3_unfolding_sequence_stages.webp)

\include{lineno} templates_developable_folding_3_unfolding_sequence.cpp

### Doubly-Curved Synclastic Vault by Plane Spacing {#synclastic_unfolding}

| unfolded to its design angle | closed for transport |
| --- | --- |
| ![](templates/templates_developable_folding_5_synclastic_unfolding_plates.webp) | ![](templates/templates_developable_folding_5_synclastic_unfolding_closed.webp) |

\include{lineno} templates_developable_folding_5_synclastic_unfolding.cpp

### Alternating Inclination About the Actuation Axis {#alternating_unfolding}

| unfolded to its design angle | closed for transport |
| --- | --- |
| ![](templates/templates_developable_folding_9_alternating_unfolding_plates.webp) | ![](templates/templates_developable_folding_9_alternating_unfolding_closed.webp) |

\include{lineno} templates_developable_folding_9_alternating_unfolding.cpp

### Corrugated Rotational Dome (24 Sectors) {#corrugated_dome}

| unfolded to its design angle | closed for transport |
| --- | --- |
| ![](templates/templates_developable_folding_11_corrugated_dome_plates.webp) | ![](templates/templates_developable_folding_11_corrugated_dome_closed.webp) |

\include{lineno} templates_developable_folding_11_corrugated_dome.cpp

### Corrugated Funnel Column (32 Sectors) {#corrugated_funnel}

| unfolded to its design angle | closed for transport |
| --- | --- |
| ![](templates/templates_developable_folding_12_corrugated_funnel_plates.webp) | ![](templates/templates_developable_folding_12_corrugated_funnel_closed.webp) |

\include{lineno} templates_developable_folding_12_corrugated_funnel.cpp

---

## Composite Space-Curve Tubular Vault {#cellular_tube_vault}

`CurvedCreaseTube` generalizes curved-crease folding and unfolding from planar curves to non-planar 3D space curves with constant curvature `k_0` and harmonic torsion `τ_0(s) = a * k_0 * sin(2πs / s_0)` (Tachi, *Transformables 2013*). Solving `T(0.5 * s_0) · T(0) = 0` places all support quads coplanar on the horizontal ground plane at target opening `2 * opening`. Combining all four crease quadrants (`Q1` and `Q3` single-sheet CCF knees with `Q2` and `Q4` two-sheet CCU ridge/keel creases) around a parallelogram tube stabilizes single-crease ruling drift and preserves 1-DOF rigid-foldability across `2α ∈ (0°, 180°)` via `k(s; α) = k_0 * cos(α_0) / cos(α)` and `τ(s; α) = τ_0(s) * tan(α) / tan(α_0)`, while strip widths taper inversely (`w_R(s) * w_L(s) = const`) between supports and mid-span. The walls are surfaces first (group `walls`); the plates come last, `t` thick centred on them. `CurvedCreaseTubeGuide::cellular_vault()` is the default:

| Field | Default | Description |
| --- | --- | --- |
| `span` | `8000.0` | Horizontal chord of the reference space curve at the target opening |
| `torsion_ratio` | `0.30` | Harmonic torsion amplitude `a = tau_max / kappa_0` |
| `opening` | `45°` | Target half folding angle `alpha_0` |
| `width_left`, `width_right` | `480.0`, `360.0` | Wall widths `w_L(0)`, `w_R(0)` at the supports |
| `tubes` | `6` | Tubes chained from corner `(b, b)` to `(b + 1, b + 1)` |
| `deployments` | `{1.0}` | Per stage `w = alpha / opening > 0`; two or more side by side |
| `thickness`, `samples` | `24.0`, `48` | Every wall, `0` for the surfaces alone; stations along the arch |

![](templates/templates_developable_folding_6_cellular_tube_vault_plates.webp)

![](templates/templates_developable_folding_6_cellular_tube_vault_flat.webp)

\include{lineno} templates_developable_folding_6_cellular_tube_vault.cpp

---

## Four Quadrants of a Curved Crease {#crease_quadrants}

Every strip is first its hinge face, rulings of `width` from the crease (group `strips`); with a thickness its body goes `t` to one side: on the outer fold face for CCF, so `Q1` and `Q3` open flat, on the shared contact face for CCU, so `Q2` and `Q4` close into a `2t` stack. `CreaseQuadrantsGuide::arc_crease()` is the default: an arc `4 m` wide and `1.5 m` high, `opening = 40°`, `width = 1200.0`, `deployment = 1.0`, `thickness = 40.0`, `samples = 48`.

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

---

## Curved Folds from Space Curves {#space_curve_folds}

A space curve is the folded crease of a curved fold of constant angle; discretized, the fold is a strip of planar quads that folds rigidly with one degree of freedom (Tachi, *One-DOF Rigid Foldable Structures from Space Curves*, IASS 2011, after the equations of Fuchs and Tabachnikov, *More on Paperfolding*, 1999). `SpaceCurveRidge` builds it in three steps, without thickness:

- ribbon: a strip `w` wide whose centre line is the polyline, quad `Q_i` normal to `N_i`; the ruling at the next vertex is normal to `N_i`, at equal angles to `T_i` and `T_i+1`, and `w / 2` along `B_i` (Eq. 6); the next frame is `N_i+1 = r_i+1 x T_i+1`, `B_i+1 = T_i+1 x N_i+1`, started at the middle edge from its neighbours' tangents.
- ridge: per edge two half-planes at `rho / 2` to `Q_i`, one on either side, trimmed where neighbouring half-planes meet into a continuous V; every quad is planar and every vertex satisfies Kawasaki (alternating sum of its sector angles 0), so the ridge is developable and flat-foldable.
- folding: the ridge developed flat is the crease pattern; folded with its rulings fixed, every quad stays rigid, each vertex's crease ray solved from its two rulings and its flat sector angles, and the crease keeps one opening `w * rho` per stage, `w = 0` folded flat.

### Space Curve Fold {#space_curve_fold}

One crease, a `2 x n` strip. A smooth curve rolls up as it folds and its quads collide before it is flat; the zig-zagged curve, every other vertex moved along its curvature vector, folds flat without a crossing. `SpaceCurveFoldGuide::helix_fold()` is the default:

| Field | Default | Description |
| --- | --- | --- |
| `curve` | `helix(2000.0, 3000.0, 1.5, 36)` | The space curve the crease follows, a ruling at every vertex |
| `width` | `600.0` | The ribbon across, `w`, each arm `w / 2` from the crease |
| `angle` | `60°` | `rho`, the opening between the arms, `0°` folded flat to `180°` a flat sheet |
| `zigzag` | `0.0` | Every other vertex moved this far along its curvature vector, the others against it |
| `deployments` | `{1.0}` | Per stage the opening as a fraction of `angle`; two or more side by side in `stage_<s>` |
| `thickness` | `18.0` | Every plate, mitered at its folds, `0` for the quads alone |
| `smooth` | `false` | Every arm one curved plate over its interpolated crease and edge, offset away from the other arm, not a mitered plate per quad |

| Preset | Curve |
| --- | --- |
| `helix_fold()` | A helix of `2 m` radius rising `3 m` per turn, `1.5` turns |
| `wave_fold()` | An arc of `3 m` radius over `172°` rising `0.8 m` and falling again, `wave(3000.0, 3.0, 800.0, 1.0, 40)` |
| `zigzag_fold()` | The helix zig-zagged by `150`, from `60°` to `1.2°`, all but flat, without plates |
| `smooth_helix_fold()` | The helix in `180` segments, from `60°` to `6°`, one curved plate per arm |

| at its design angle | folded nearly flat in five stages |
| --- | --- |
| ![](templates/templates_developable_folding_13_space_curve_fold_design.webp) | ![](templates/templates_developable_folding_13_space_curve_fold_stages.webp) |

\include{lineno} templates_developable_folding_13_space_curve_fold.cpp

The zig-zag is polygonal on purpose: its alternating curvature is what lets it fold flat (Tachi 2011, 4.1). The same helix sampled densely and not zig-zagged is the smooth curved fold, its arms one curved plate each; folding raises its curvature and torsion until it rolls into itself, here a little below `6°`.

| smooth, at its design angle | folded from `60°` to `6°` |
| --- | --- |
| ![](templates/templates_developable_folding_15_smooth_space_curve_fold_design.webp) | ![](templates/templates_developable_folding_15_smooth_space_curve_fold_stages.webp) |

\include{lineno} templates_developable_folding_15_smooth_space_curve_fold.cpp

### Curved Miura Tessellation {#curved_miura}

The ridge's arms trimmed so that every far edge is parallel to its crease edge, then strips added across, every other strip on the same arm's rulings: a generalized Miura-ori bent and twisted along the curve. Every vertex copies the ridge's, so the `m x n` quads fold with the same one degree of freedom. `CurvedMiuraGuide::helix_miura()` is the default, the fields of `SpaceCurveFoldGuide` with `width = 300.0` every strip across at the middle of the curve and `strips = 6`; `wave_miura()` and `zigzag_miura()` are the other presets.

| at its design angle | three stages |
| --- | --- |
| ![](templates/templates_developable_folding_14_curved_miura_design.webp) | ![](templates/templates_developable_folding_14_curved_miura_stages.webp) |

\include{lineno} templates_developable_folding_14_curved_miura.cpp

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
