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

## Examples {#developable_folding_examples}

### 1 Curved Fold {#developable_folding_example_1}

Step 1 of the developable folding templates: a curved folded roof, a freestanding folded shell roof of 32 m diameter rising into a tower 26 m high with a round top of 7 m. Twelve sectors, their valleys running down to the columns at 4000 and their ridges ending in a gable 4000 over the eaves, the folds following a trumpet curve, flat at the eaves and vertical at the tower, their depth shrinking with the radius into the tower, the top cut level at 26 m. Every ridge and valley is a NURBS curve interpolated through fourteen rings, each the same star scaled, and every fold strip between two of them one lamella, a PlateCurved: a closed curved solid of six exact NURBS patches, its outer face the ruled surface between the two meridians, its inner face 120 mm in between the inner meridians, where the neighbouring strips offset by the thickness meet, its side faces along the folds shared with the neighbours as miters, and a cap at the eaves and at the top: 24 curved solids, no facets. Each step is a group: meridians, lamellas, the lamellas named lamella_\<j\>.

\include{lineno} templates_developable_folding_1_curved_fold.cpp

### 2 Curved Crease Unfolding {#developable_folding_example_2}

Step 2 of the developable folding templates: curved-crease unfolding. Four 40 mm plates, flat bands with circular edges, hinged on their curved edge and opened: the curved hinge forces each plate to bend about its rulings, so the stack rises into an arch. Five parallel planes 1000 mm apart, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees; every plate an exact cone. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

\include{lineno} templates_developable_folding_2_curved_crease_unfolding.cpp

### 3 Catenary Unfolding {#developable_folding_example_3}

Step 3 of the developable folding templates: curved-crease unfolding to a catenary. Five parallel planes 1000 mm apart, the crease a catenary 10 m wide and 6 m high, the plates opened 45 degrees. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

\include{lineno} templates_developable_folding_3_catenary_unfolding.cpp

### 4 Crease Quadrants {#developable_folding_example_4}

Step 4 of the developable folding templates: the four quadrants of a planar curved crease. An arc 4 m wide and 1.5 m high is the crease; a developable through it has its rulings r on one side of the arc's plane and -r on the other, and their mirror images Mr and -Mr. Two strips 1200 mm wide and 40 mm thick, each a PlateCurved cone, take one ruling each: Q1 (r, -Mr) and Q3 (Mr, -r) are one sheet folded along the crease, the crease convex for one strip and concave for the other; Q2 (r, Mr) are two mirror sheets unfolded, the crease convex for both, synclastic; Q4 (-r, -Mr) the same with the crease concave for both, anticlastic. The rulings stand 40 degrees off the crease plane, square to the crease; an unfolding is mitred on the crease plane, a folding on the plane through the crease square to it. Groups: quadrant_\<q\> (quadrant_\<q\>_\<k\>), creases.

\include{lineno} templates_developable_folding_4_crease_quadrants.cpp

### 5 Synclastic Unfolding {#developable_folding_example_5}

Step 5 of the developable folding templates: a synclastic vault by plane inclination. Five planes 1000 mm apart on the floor, each leaning 5 degrees more toward the middle, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees: the apex is drawn in. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

\include{lineno} templates_developable_folding_5_synclastic_unfolding.cpp

### 6 Anticlastic Unfolding {#developable_folding_example_6}

Step 6 of the developable folding templates: an anticlastic vault by plane inclination. Five planes 1000 mm apart on the floor, each leaning 5 degrees more away from the middle, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees: the apex is spread, a saddle. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

\include{lineno} templates_developable_folding_6_anticlastic_unfolding.cpp

### 7 Apex Level Unfolding {#developable_folding_example_7}

Step 7 of the developable folding templates: plane inclination about the span at the apex. Five planes 1000 mm apart, every second one turned 8 degrees, alternately each way, about a line along the span at the apex, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees: the apex spacing is kept and the supports move, the fold articulated at the feet. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

\include{lineno} templates_developable_folding_7_apex_level_unfolding.cpp

### 8 Support Level Unfolding {#developable_folding_example_8}

Step 8 of the developable folding templates: plane inclination about the span at the floor. Five planes 1000 mm apart, every second one turned 8 degrees, alternately each way, about a line along the span on the floor, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees: the supports keep their spacing and the apex moves, the fold articulated at the crown. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

\include{lineno} templates_developable_folding_8_support_level_unfolding.cpp

### 9 Alternating Unfolding {#developable_folding_example_9}

Step 9 of the developable folding templates: plane inclination about the span at half height. Five planes 1000 mm apart, every second one turned 8 degrees, alternately each way, about a line along the span at half height, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees: each plate wide at one end and narrow at the other. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

\include{lineno} templates_developable_folding_9_alternating_unfolding.cpp

### 10 Sideways Unfolding {#developable_folding_example_10}

Step 10 of the developable folding templates: plane inclination about a vertical axis. Five planes 1000 mm apart, every second one turned 8 degrees, alternately each way, about a vertical axis through the crown, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees: the plane distance shrinks toward one foot and grows toward the other. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

\include{lineno} templates_developable_folding_10_sideways_unfolding.cpp

### 11 Corrugated Dome {#developable_folding_example_11}

Step 11 of the developable folding templates: a corrugated dome by consecutive plane inclinations in a radial layout. The crease is a meridian arc of a 6 m sphere from the floor up to 80 degrees, an oculus of 2.1 m left open; the 25 crease planes are the meridian planes 15 degrees apart around the vertical axis. The rulings leave the arc square to it at 60 degrees to its plane toward the centre; every plate is the mirror image of its neighbour in their meridian plane, so the ridges are the arc turned by 30 degrees and the valleys its mirror images, a star in plan. 24 plates 40 mm thick, each a developable mitred on its two meridian planes. Groups: plates (plate_\<k\>), creases.

\include{lineno} templates_developable_folding_11_corrugated_dome.cpp

### 12 Corrugated Funnel {#developable_folding_example_12}

Step 12 of the developable folding templates: a corrugated funnel column by consecutive plane inclinations in a radial layout. The crease is a quarter circle of 5 m radius flaring from a stem 0.8 m off the axis, vertical on the floor, to a rim 5.8 m out and 5 m up, horizontal there; the 33 crease planes are the meridian planes 11.25 degrees apart. The rulings leave the arc square to it at 75 degrees to its plane toward its concave side, outward and down; every plate is the mirror image of its neighbour in their meridian plane, a star in plan at the rim. 32 plates 40 mm thick, each a developable mitred on its two meridian planes. Groups: plates (plate_\<k\>), creases.

\include{lineno} templates_developable_folding_12_corrugated_funnel.cpp
