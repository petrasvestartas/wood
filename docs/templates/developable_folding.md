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
  <a class="wood-card" href="#curved_crease_catenary">
    <img src="templates_developable_folding_3_catenary_unfolding_plates.webp" alt="Catenary Arch">
    <span class="wood-card-title">Catenary Arch</span>
  </a>
  <a class="wood-card" href="#synclastic_unfolding">
    <img src="templates_developable_folding_5_synclastic_unfolding_plates.webp" alt="Synclastic Vault">
    <span class="wood-card-title">Synclastic Vault</span>
  </a>
  <a class="wood-card" href="#anticlastic_unfolding">
    <img src="templates_developable_folding_6_anticlastic_unfolding_plates.webp" alt="Anticlastic Saddle">
    <span class="wood-card-title">Anticlastic Saddle</span>
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
    <img src="templates_developable_folding_4_crease_quadrants_all.webp" alt="Crease Quadrants">
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

```cpp
CurvedFold fold(
    12,      // sectors (1 ridge + 1 valley per sector = 24 lamellas)
    16000.0, // eaves radius
    3500.0,  // tower top radius
    4000.0,  // valley height at eaves
    26000.0, // tower height
    4000.0,  // gable rise above eaves valleys
    0.1,     // valley radial inset
    14,      // interpolation rings
    120.0    // lamella thickness
);
```

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

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0), 40.0);
```

### Catenary Arch on Parallel Planes {#curved_crease_catenary}

![](templates/templates_developable_folding_3_catenary_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(CurvedCreaseUnfolding::catenary(10000.0, 6000.0), 45.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0), 40.0);
```

### Synclastic Vault (Leaning +5 Degrees) {#synclastic_unfolding}

![](templates/templates_developable_folding_5_synclastic_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 5.0 * Tolerance::TO_RADIANS), 40.0);
```

### Anticlastic Saddle Vault (Leaning -5 Degrees) {#anticlastic_unfolding}

![](templates/templates_developable_folding_6_anticlastic_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, -5.0 * Tolerance::TO_RADIANS), 40.0);
```

### Zigzag Inclined About Apex {#apex_level_unfolding}

![](templates/templates_developable_folding_7_apex_level_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 4500.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

### Zigzag Inclined About Support Floor {#support_level_unfolding}

![](templates/templates_developable_folding_8_support_level_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

### Zigzag Inclined About Mid-Height {#alternating_unfolding}

![](templates/templates_developable_folding_9_alternating_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 2250.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

### Zigzag Inclined About Vertical Axis {#sideways_unfolding}

![](templates/templates_developable_folding_10_sideways_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

### Corrugated Rotational Dome (24 Sectors) {#corrugated_dome}

![](templates/templates_developable_folding_11_corrugated_dome_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(6000.0, 0.0, 0.0), Point(6000.0 * std::cos(0.7), 0.0, 6000.0 * std::sin(0.7)), Point(6000.0 * std::cos(1.4), 0.0, 6000.0 * std::sin(1.4))), 60.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 24, 15.0 * Tolerance::TO_RADIANS), 40.0);
```

### Corrugated Funnel Column (32 Sectors) {#corrugated_funnel}

![](templates/templates_developable_folding_12_corrugated_funnel_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(800.0, 0.0, 0.0), Point(5800.0 - 5000.0 * std::cos(0.25 * Tolerance::PI), 0.0, 5000.0 * std::sin(0.25 * Tolerance::PI)), Point(5800.0, 0.0, 5000.0)), 75.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 32, 11.25 * Tolerance::TO_RADIANS), 40.0);
```

---

## Four Quadrants of a Curved Crease {#crease_quadrants}

| Quadrant | Strip Rulings | Mechanism Type | Curvature Behavior |
| --- | --- | --- | --- |
| `Q1` | `(+r, -Mr)` | Single-sheet folding (both strips in front of crease plane) | Convex + concave; sector angles sum to `360°` |
| `Q2` | `(+r, +Mr)` | Two-sheet mirror unfolding | Both convex (synclastic); sector angles `< 360°` |
| `Q3` | `(+Mr, -r)` | Single-sheet folding (both strips behind crease plane) | Convex + concave; sector angles sum to `360°` |
| `Q4` | `(-r, -Mr)` | Two-sheet mirror unfolding | Both concave (anticlastic); sector angles `> 360°` |

### Q1–Q4 Side-by-Side {#crease_quadrants_example}

![](templates/templates_developable_folding_4_crease_quadrants_all.webp)

```cpp
CreaseQuadrants quadrants(Primitives::arc(Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)), 40.0 * Tolerance::TO_RADIANS, 1200.0, 40.0);
```
