# Folding Developable {#templates_developable_folding}

[TOC]

Developable [Curved Plate](@ref elements_plate_curved) structures including ruled tent shells, curved-crease accordion unfoldings, and quad-diagonal flat development checks.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#curved_fold_roof">
    <img src="templates_developable_folding_1_curved_fold_lamellas.webp" alt="Tent Roof">
    <span class="wood-card-title">Tent Roof</span>
  </a>
  <a class="wood-card" href="#curved_crease_arch">
    <img src="templates_developable_folding_2_curved_crease_unfolding_plates.webp" alt="Semicircular Arch">
    <span class="wood-card-title">Semicircular Arch</span>
  </a>
  <a class="wood-card" href="#synclastic_unfolding">
    <img src="templates_developable_folding_5_synclastic_unfolding_plates.webp" alt="Synclastic Vault">
    <span class="wood-card-title">Synclastic Vault</span>
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
</div>

## Flat Unfolding Check {#development}

`Development(a, b, samples)` unfolds the ruled strip between curves `a` and `b` into the `xy`-plane triangle-by-triangle and measures the relative length change of each quad's second diagonal (`distortion = 0` on exact cylinders and cones).

```cpp
const Development development(lamella->outer_a, lamella->outer_b, 32);
development.distortion; // 0 (~1e-14) on exact cylinders/cones; relative diagonal error on twisted strips
development.outline;    // 2D cutting pattern in the xy-plane (placed in group "flat")
```

---

## Curved Fold Tent Roof {#curved_fold_roof}

Twelve-sector freestanding folded shell (`32 m` eaves diameter, `26 m` tower height, `24` `PlateCurved` lamellas, `24` contacts) ruled between star-ring interpolated NURBS meridians.

\include{lineno} templates_developable_folding_1_curved_fold.cpp

![](templates/templates_developable_folding_1_curved_fold_lamellas.webp)

---

## Curved Crease Unfolding {#curved_crease_unfolding}

Curved-crease mechanism: a planar crease with 3D curvature `k_3d = k_2d / cos(opening)` generates developable strips whose rulings leave perpendicular to the crease at angle `opening` and mirror successively across a sequence of crease `planes`:

| Plane Factory | Plane Sequence Geometry | Resulting Shell |
| --- | --- | --- |
| `zigzag(origin, along, axis, count, spacing, tilt)` | `spacing` apart, alternating `±tilt` about `axis` (`tilt = 0` → parallel planes) | Straight corrugated arch / articulated vault |
| `leaning(origin, along, axis, count, spacing, lean)` | Progressive `i * lean` rotation about span axis | Synclastic (`lean > 0`) or anticlastic saddle (`lean < 0`) |
| `fan(origin, axis, first, count, angle)` | Radial planes rotated by `angle` around `axis` | Corrugated rotational dome or funnel column |

### Semicircular Arch on Parallel Planes {#curved_crease_arch}

![](templates/templates_developable_folding_2_curved_crease_unfolding_plates.webp)

\include{lineno} templates_developable_folding_2_curved_crease_unfolding.cpp

### Synclastic Vault (Leaning +5 Degrees) {#synclastic_unfolding}

![](templates/templates_developable_folding_5_synclastic_unfolding_plates.webp)

\include{lineno} templates_developable_folding_5_synclastic_unfolding.cpp

### Zigzag Inclined About Mid-Height {#alternating_unfolding}

![](templates/templates_developable_folding_9_alternating_unfolding_plates.webp)

\include{lineno} templates_developable_folding_9_alternating_unfolding.cpp

### Corrugated Rotational Dome (24 Sectors) {#corrugated_dome}

![](templates/templates_developable_folding_11_corrugated_dome_plates.webp)

\include{lineno} templates_developable_folding_11_corrugated_dome.cpp

### Corrugated Funnel Column (32 Sectors) {#corrugated_funnel}

![](templates/templates_developable_folding_12_corrugated_funnel_plates.webp)

\include{lineno} templates_developable_folding_12_corrugated_funnel.cpp

---

## Four Quadrants of a Curved Crease {#crease_quadrants}

| Quadrant | Strip Rulings | Mechanism Type | Curvature Behavior |
| --- | --- | --- | --- |
| `Q1` | `(+r, -Mr)` | Single-sheet folding (both strips in front of crease plane) | Convex + concave; sector angles sum to `360°` |
| `Q2` | `(+r, +Mr)` | Two-sheet mirror unfolding | Both convex (synclastic); sector angles `< 360°` |
| `Q3` | `(+Mr, -r)` | Single-sheet folding (both strips behind crease plane) | Convex + concave; sector angles sum to `360°` |
| `Q4` | `(-r, -Mr)` | Two-sheet mirror unfolding | Both concave (anticlastic); sector angles `> 360°` |

### Q1–Q4 {#crease_quadrants_example}

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

```cpp
CreaseQuadrants quadrants(Primitives::arc(Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)), 40.0 * Tolerance::TO_RADIANS, 1200.0, 40.0);
```

## Examples {#developable_folding_examples}

### 4 Crease Quadrants {#developable_folding_example_4}

Step 4 of the developable folding templates: the four quadrants of a planar curved crease. An arc 4 m wide and 1.5 m high is the crease; a developable through it has its rulings r on one side of the arc's plane and -r on the other, and their mirror images Mr and -Mr. Two strips 1200 mm wide and 40 mm thick, each a PlateCurved cone, take one ruling each: Q1 (r, -Mr) and Q3 (Mr, -r) are one sheet folded along the crease, the crease convex for one strip and concave for the other; Q2 (r, Mr) are two mirror sheets unfolded, the crease convex for both, synclastic; Q4 (-r, -Mr) the same with the crease concave for both, anticlastic. The rulings stand 40 degrees off the crease plane, square to the crease; an unfolding is mitred on the crease plane, a folding on the plane through the crease square to it. Groups: quadrant_\<q\> (quadrant_\<q\>_\<k\>), creases.

\include{lineno} templates_developable_folding_4_crease_quadrants.cpp

