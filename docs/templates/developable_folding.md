# Folding developable {#templates_developable_folding}

[TOC]

Developable `PlateCurved` structures beyond cylinders: free-form ruled tent shells, curved-crease accordion unfoldings (cones, synclastic/anticlastic vaults, domes, funnels), and quad-diagonal flat development verification.

![Folding Geometry](diagrams/wood_folding_geometry.svg)

| Template | Header | Geometric Construction | Examples |
| --- | --- | --- | --- |
| [`Development`](#development) | `development.h` | Ruled-strip 2D unfolding + quad diagonal `distortion` & `twist` check | Printed by all examples |
| [`CurvedFold`](#curved_fold_roof) | `developable_folding/curved_fold.h` | 12-sector trumpet tent roof (`32 m` span, `26 m` tower) ruled between interpolated NURBS meridians | `templates_developable_folding_1_curved_fold` |
| [`CurvedCreaseUnfolding`](#curved_crease_unfolding) | `developable_folding/curved_crease_unfolding.h` | Flat curved-edge bands hinged across `zigzag`, `leaning`, or `fan` crease planes | `templates_developable_folding_{2,3,5..12}_*` |
| [`CreaseQuadrants`](#crease_quadrants) | `developable_folding/crease_quadrants.h` | Four ruling combinations (`Q1..Q4`) joining two developable strips at one planar curved crease | `templates_developable_folding_4_crease_quadrants` |

## 1. Flat Unfolding Check (`Development`) {#development}

`Development(a, b, samples)` unfolds the ruled strip between curves `a` and `b` into the $xy$-plane triangle-by-triangle and measures the relative length change of each quad's second diagonal (`distortion = 0` on exact cylinders and cones).

```cpp
const Development development(lamella->outer_a, lamella->outer_b, 32);
development.distortion; // 0 (~1e-14) on exact cylinders/cones; relative diagonal error on twisted strips
development.outline;    // 2D cutting pattern in the xy-plane (placed in group "flat")
```

---

## 2. CurvedFold Tent Roof {#curved_fold_roof}

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

![curved fold](templates/templates_developable_folding_1_curved_fold_lamellas.webp)

<details><summary>Example code: templates_developable_folding_1_curved_fold.cpp</summary>

\include{lineno} templates_developable_folding_1_curved_fold.cpp

</details>

---

## 3. CurvedCreaseUnfolding (`2, 3, 5–12`) {#curved_crease_unfolding}

Curved-crease mechanism: a planar crease of curvature $\kappa_{3\text{D}} = \kappa_{2\text{D}} / \cos\alpha$ generates developable strips whose rulings leave perpendicular to the crease at opening angle $\alpha$, mirrored successively across a sequence of crease `planes`:

| Plane Factory | Plane Sequence Geometry | Resulting Shell |
| --- | --- | --- |
| `zigzag(origin, along, axis, count, spacing, tilt)` | `spacing` apart, alternating `±tilt` about `axis` (`tilt = 0` $\to$ parallel planes) | Straight corrugated arch / articulated vault |
| `leaning(origin, along, axis, count, spacing, lean)` | Progressive `i * lean` rotation about span axis | Synclastic (`lean > 0`) or anticlastic saddle (`lean < 0`) |
| `fan(origin, axis, first, count, angle)` | Radial planes rotated by `angle` around `axis` | Corrugated rotational dome or funnel column |

### 2. Semicircular Arch on Parallel Planes {#curved_crease_arch}

![arch](templates/templates_developable_folding_2_curved_crease_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0), 40.0);
```

<details><summary>Example code: templates_developable_folding_2_curved_crease_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_2_curved_crease_unfolding.cpp

</details>

### 3. Catenary Arch on Parallel Planes {#curved_crease_catenary}

![catenary](templates/templates_developable_folding_3_catenary_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(CurvedCreaseUnfolding::catenary(10000.0, 6000.0), 45.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0), 40.0);
```

<details><summary>Example code: templates_developable_folding_3_catenary_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_3_catenary_unfolding.cpp

</details>

### 5. Synclastic Vault (`leaning = +5°`) {#synclastic_unfolding}

![synclastic vault](templates/templates_developable_folding_5_synclastic_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 5.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_5_synclastic_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_5_synclastic_unfolding.cpp

</details>

### 6. Anticlastic Saddle Vault (`leaning = -5°`) {#anticlastic_unfolding}

![anticlastic vault](templates/templates_developable_folding_6_anticlastic_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, -5.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_6_anticlastic_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_6_anticlastic_unfolding.cpp

</details>

### 7. Zigzag Inclined About Apex (`z = 4500 mm`) {#apex_level_unfolding}

![inclined about the span at the apex](templates/templates_developable_folding_7_apex_level_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 4500.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_7_apex_level_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_7_apex_level_unfolding.cpp

</details>

### 8. Zigzag Inclined About Support Floor (`z = 0`) {#support_level_unfolding}

![inclined about the span at the floor](templates/templates_developable_folding_8_support_level_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_8_support_level_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_8_support_level_unfolding.cpp

</details>

### 9. Zigzag Inclined About Mid-Height (`z = 2250 mm`) {#alternating_unfolding}

![inclined about the span at half height](templates/templates_developable_folding_9_alternating_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 2250.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_9_alternating_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_9_alternating_unfolding.cpp

</details>

### 10. Zigzag Inclined About Vertical Axis {#sideways_unfolding}

![inclined about a vertical axis](templates/templates_developable_folding_10_sideways_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_10_sideways_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_10_sideways_unfolding.cpp

</details>

### 11. Corrugated Rotational Dome (`fan`, `24` Sectors) {#corrugated_dome}

![corrugated dome](templates/templates_developable_folding_11_corrugated_dome_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(6000.0, 0.0, 0.0), Point(6000.0 * std::cos(0.7), 0.0, 6000.0 * std::sin(0.7)), Point(6000.0 * std::cos(1.4), 0.0, 6000.0 * std::sin(1.4))), 60.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 24, 15.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_11_corrugated_dome.cpp</summary>

\include{lineno} templates_developable_folding_11_corrugated_dome.cpp

</details>

### 12. Corrugated Funnel Column (`fan`, `32` Sectors) {#corrugated_funnel}

![corrugated funnel](templates/templates_developable_folding_12_corrugated_funnel_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(800.0, 0.0, 0.0), Point(5800.0 - 5000.0 * std::cos(0.25 * Tolerance::PI), 0.0, 5000.0 * std::sin(0.25 * Tolerance::PI)), Point(5800.0, 0.0, 5000.0)), 75.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 32, 11.25 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_12_corrugated_funnel.cpp</summary>

\include{lineno} templates_developable_folding_12_corrugated_funnel.cpp

</details>

---

## 4. Four Quadrants of a Curved Crease (`CreaseQuadrants`) {#crease_quadrants}

| Quadrant | Strip Rulings | Mechanism Type | Gaussian / Sector Curvature Behavior |
| --- | --- | --- | --- |
| `Q1` | $(+r, -Mr)$ | Single-sheet folding (both strips in front of crease plane) | Convex + concave; sector angles sum to $360^\circ$ |
| `Q2` | $(+r, +Mr)$ | Two-sheet mirror unfolding | Both convex (synclastic); sector angles $< 360^\circ$ |
| `Q3` | $(+Mr, -r)$ | Single-sheet folding (both strips behind crease plane) | Convex + concave; sector angles sum to $360^\circ$ |
| `Q4` | $(-r, -Mr)$ | Two-sheet mirror unfolding | Both concave (anticlastic); sector angles $> 360^\circ$ |

### Q1–Q4 Side-by-Side (`templates_developable_folding_4_crease_quadrants`) {#crease_quadrants_example}

![q1 to q4 on one arc](templates/templates_developable_folding_4_crease_quadrants_all.webp)

```cpp
CreaseQuadrants quadrants(Primitives::arc(Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)), 40.0 * Tolerance::TO_RADIANS, 1200.0, 40.0);
```

<details><summary>Example code: templates_developable_folding_4_crease_quadrants.cpp</summary>

\include{lineno} templates_developable_folding_4_crease_quadrants.cpp

</details>
