# Folding cylindrical {#templates_cylindrical_folding}

[TOC]

Folded structures of exact cylindrical [`PlateCurved`](@ref elements_plate_curved) lamellas (parallel straight generatrices, zero Gaussian curvature, exact rational NURBS ellipse creases, `Development::distortion < 1e-14`).

![Folding Geometry](diagrams/wood_folding_geometry.svg)

| Template | Header | Geometric Construction | Examples |
| --- | --- | --- | --- |
| [`ReflectionFold`](#reflection_fold) | `cylindrical_folding/reflection_fold.h` | Cylindrical profile swept along rulings and reflected/mirrored across planar creases | `templates_cylindrical_folding_{1..7}_*` |
| [`CylinderVault`](#cylinder_vault) | `cylindrical_folding/cylinder_vault.h` | Equal cylinders over a regular $n$-gon meeting on vertical diagonal planes (`Groin` / `Cloister`) | `templates_cylindrical_folding_{8..12}_*` |
| [`CurvedFoldedBeam`](#curved_folded_beam) | `cylindrical_folding/curved_folded_beam.h` | Closed 4-cylinder box girder deepening toward mid-span with transverse planar webs | `templates_cylindrical_folding_13_curved_folded_beam` |
| [`CornerFan`](#corner_fan) | `cylindrical_folding/corner_fan.h` | Cylindrical strips radiating from one corner of a square on parabolic creases | `templates_cylindrical_folding_{14,15}_*` |
| [`CylinderCarving`](#cylinder_carving) | `cylindrical_folding/cylinder_carving.h` | Sequential boolean cylinder carving $((a - b) - c) - d$, one concave `PlateCurved` per patch | `templates_cylindrical_folding_{16,17}_*` |

## Cylindrical Crease Invariants {#why_cylinders}

- **Planar Crease Theorem:** Two smooth developable patches meet along a planar curve when one is the reflection or mirror image of the other across the crease plane (generatrices make equal angles with the plane).
- **Exact Rational NURBS Solids:** Projecting a circular NURBS arc along parallel rulings onto an oblique fold plane is an affine map that preserves rational weights, producing exact ellipse arcs and an exact 6-patch `PlateCurved` solid (`distortion = 0`).

---

## ReflectionFold {#reflection_fold}

| `Fold::reflected` | Ruling Update | Curvature / Bulge | Unfolded Development |
| --- | --- | --- | --- |
| `true` (Reflection) | Sheet beyond fold plane $R$ is mirrored across $R$ ($n_R \parallel r_{\text{prev}} - r_{\text{next}}$) | Flips: vault $\leftrightarrow$ trough | Single continuous flat sheet |
| `false` (Mirror fold) | Next segment is mirror image in angle bisector ($n_R \parallel r_{\text{prev}} + r_{\text{next}}$) | Preserved: vault $\leftrightarrow$ vault | Separate mirror-image developments per segment |

```cpp
const Plane first = compute_profile_plane();
const std::vector<Segment> segments = compute_segments(first);
const std::vector<std::vector<NurbsCurve>> creases = compute_creases(segments);
const std::array<std::vector<NurbsCurve>, 2> profiles = {
    compute_inner_profile(first, 1.0),
    compute_inner_profile(first, -1.0)
};
const std::vector<std::vector<NurbsCurve>> inners = compute_inners(segments, profiles);
```

| Step | Construction |
| --- | --- |
| `segments` | Fold planes from station + turn angle; optional `symmetry` plane at ridge/crown |
| `creases` | Affine projection of `profile` curves along segment rulings onto each fold plane |
| `inners` | Normal offsets (`±thickness`) projected along rulings to form shared inner miter curves |
| `lamellas` | `PlateCurved(outer_a, outer_b, inner_a, inner_b)` per segment and profile arc |
| `flat` / `contacts` | Unrolled 2D `Development` cutting patterns + `compute_face_contacts(0)` verification |

### 1. Reflection Principle {#reflection_principle}

Circular arc strip (`120°`, `R = 1000 mm`, `t = 120 mm`) cut by an oblique plane `1500 mm` down and reflected by `120°` (`2` lamellas, `1` contact).

![the reflection principle](templates/templates_cylindrical_folding_1_reflection_lamellas.webp)

```cpp
ReflectionFold fold(
    {Primitives::arc(Point(500.0, -866.0, 3000.0), Point(1000.0, 0.0, 3000.0), Point(500.0, 866.0, 3000.0))},
    Vector(0.0, 0.0, -1.0),
    Vector(0.0, -1.0, 0.0),
    3500.0,
    {{1500.0, 60.0 * Tolerance::TO_RADIANS}}
);
```

<details><summary>Example code: templates_cylindrical_folding_1_reflection.cpp</summary>

\include{lineno} templates_cylindrical_folding_1_reflection.cpp

</details>

### 2. Curved Folded Plate Arch {#curved_folded_plate}

Six half-sine waves (`9 m` wide, `400 mm` amplitude) swept along a 5-segment polygonal arch with four `-30°` reverse folds (`13.5 m` span, `4.9 m` rise, `30` lamellas, `49` contacts).

![curved folded plate](templates/templates_cylindrical_folding_2_curved_folded_plate_lamellas.webp)

```cpp
const double climb = 60.0 * Tolerance::TO_RADIANS;
const Vector along(0.0, std::cos(climb), std::sin(climb));
const Vector up(0.0, -std::sin(climb), std::cos(climb));
ReflectionFold fold(
    ReflectionFold::waves(Point(-4500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), up, 6, 1500.0, 400.0),
    along, Vector(1.0, 0.0, 0.0), 18100.0,
    {{3620.0, -30.0 * Tolerance::TO_RADIANS}, {7240.0, -30.0 * Tolerance::TO_RADIANS},
     {10860.0, -30.0 * Tolerance::TO_RADIANS}, {14480.0, -30.0 * Tolerance::TO_RADIANS}}
);
```

<details><summary>Example code: templates_cylindrical_folding_2_curved_folded_plate.cpp</summary>

\include{lineno} templates_cylindrical_folding_2_curved_folded_plate.cpp

</details>

### 3. Mendrisio Pavilion {#mendrisio_pavilion}

IBOIS Mendrisio pavilion (`13 × 4.5 × 3.4 m`, `77 mm` curved CLT): shallow barrel reflected at the eave (`89.9°`) and ridge (`10°`) across the symmetry plane (`5` plates, `4` contacts).

![mendrisio pavilion](templates/templates_cylindrical_folding_3_mendrisio_pavilion_lamellas.webp)

```cpp
const Vector along(0.1719, 0.0, 0.9851);
ReflectionFold fold(
    {Primitives::arc(Point(6000.0, -2250.0, 0.0), Point(6400.0, 0.0, 0.0), Point(6000.0, 2250.0, 0.0))},
    along, Vector(0.0, -1.0, 0.0), 12000.0,
    {{2923.0, 89.9 * Tolerance::TO_RADIANS, true}, {3623.0, 10.0 * Tolerance::TO_RADIANS, true}},
    Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
    77.0
);
```

<details><summary>Example code: templates_cylindrical_folding_3_mendrisio_pavilion.cpp</summary>

\include{lineno} templates_cylindrical_folding_3_mendrisio_pavilion.cpp

</details>

### 4–7. Linear Folded Cylindrical Hall Systems {#folded_cylinders}

Four-barrel rows (`4 × 6000 mm` width, `120 mm` thick) folded into halls and arches:

| System | Climb | Folds `(station, angle, reflected)` | Symmetry $x$ | Barrel Rise | Lamellas / Contacts | Example |
| --- | --- | --- | --- | --- | --- | --- |
| Two-hinged frame | `90°` | `(6000, -76°, true)` | `20000` | `600 mm` | `16` / `24` | `templates_cylindrical_folding_4_two_hinged_frame` |
| Two-hinged arch | `105°` | `(11000, -50°, false)`, `(22000, -50°, false)` | `11000` | `2400 mm` | `24` / `38` | `templates_cylindrical_folding_5_two_hinged_arch` |
| Arch with top hinge | `90°` | `(300, -15°, true)`, `(18300, -40°, true)`, `(25300, -35°, false)` | `13000` | `1600 mm` | `28` / `45` | `templates_cylindrical_folding_6_arch_with_top_hinge` |
| Two-hinged A-frame | `110°` | `(5600, -60°, true)` | `12500` | `1400 mm` | `16` / `24` | `templates_cylindrical_folding_7_a_frame` |

#### 4. Two-Hinged Frame {#two_hinged_frame}

![two-hinged frame](templates/templates_cylindrical_folding_4_two_hinged_frame_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 600.0), along, Vector(0.0, -1.0, 0.0), 30000.0, {{6000.0, -76.0 * Tolerance::TO_RADIANS, true}}, Plane::from_point_normal(Point(20000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

<details><summary>Example code: templates_cylindrical_folding_4_two_hinged_frame.cpp</summary>

\include{lineno} templates_cylindrical_folding_4_two_hinged_frame.cpp

</details>

#### 5. Two-Hinged Arch {#two_hinged_arch}

![two-hinged arch](templates/templates_cylindrical_folding_5_two_hinged_arch_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 2400.0), along, Vector(0.0, -1.0, 0.0), 40000.0, {{11000.0, -50.0 * Tolerance::TO_RADIANS, false}, {22000.0, -50.0 * Tolerance::TO_RADIANS, false}}, Plane::from_point_normal(Point(11000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

<details><summary>Example code: templates_cylindrical_folding_5_two_hinged_arch.cpp</summary>

\include{lineno} templates_cylindrical_folding_5_two_hinged_arch.cpp

</details>

#### 6. Arch with Top Hinge {#arch_with_top_hinge}

![arch with top hinge](templates/templates_cylindrical_folding_6_arch_with_top_hinge_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1600.0), along, Vector(0.0, -1.0, 0.0), 30000.0, {{300.0, -15.0 * Tolerance::TO_RADIANS, true}, {18300.0, -40.0 * Tolerance::TO_RADIANS, true}, {25300.0, -35.0 * Tolerance::TO_RADIANS, false}}, Plane::from_point_normal(Point(13000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

<details><summary>Example code: templates_cylindrical_folding_6_arch_with_top_hinge.cpp</summary>

\include{lineno} templates_cylindrical_folding_6_arch_with_top_hinge.cpp

</details>

#### 7. Two-Hinged A-Frame {#a_frame}

![two-hinged a-frame](templates/templates_cylindrical_folding_7_a_frame_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1400.0), along, Vector(0.0, -1.0, 0.0), 40000.0, {{5600.0, -60.0 * Tolerance::TO_RADIANS, true}}, Plane::from_point_normal(Point(12500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

<details><summary>Example code: templates_cylindrical_folding_7_a_frame.cpp</summary>

\include{lineno} templates_cylindrical_folding_7_a_frame.cpp

</details>

---

## CylinderVault (`8–12`) {#cylinder_vault}

`CylinderVault(sides, edge, radius, kind, wall, thickness)`: `sides` equal cylinder sectors over a regular polygon meeting on vertical corner-to-centre planes.

| `Kind` | Generatrices | Profile | Creases |
| --- | --- | --- | --- |
| `Groin` | Radial (side $\to$ centre) | Arch over polygon side, springing at corners | Half-ellipses from corners to crown |
| `Cloister` | Tangential (parallel to polygon side, on `wall`) | Arc from wall top to central peak | Quarter-ellipses from corners to peak |

### 8. Groin Vault (`sides = 4`) {#groin_vault}

![groin vault](templates/templates_cylindrical_folding_8_groin_vault_lamellas.webp)

```cpp
CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Groin);
```

<details><summary>Example code: templates_cylindrical_folding_8_groin_vault.cpp</summary>

\include{lineno} templates_cylindrical_folding_8_groin_vault.cpp

</details>

### 9. Pavilion Cloister Vault on Walls (`sides = 4`) {#pavilion_vault}

![pavilion vault on walls](templates/templates_cylindrical_folding_9_pavilion_vault_lamellas.webp)

```cpp
CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Cloister, 4000.0);
```

<details><summary>Example code: templates_cylindrical_folding_9_pavilion_vault.cpp</summary>

\include{lineno} templates_cylindrical_folding_9_pavilion_vault.cpp

</details>

### 10. Pentagon Vault (`sides = 5`) {#pentagon_vault}

![pentagon vault](templates/templates_cylindrical_folding_10_pentagon_vault_lamellas.webp)

```cpp
CylinderVault vault(5, 8000.0, 4000.0, CylinderVault::Kind::Groin);
```

<details><summary>Example code: templates_cylindrical_folding_10_pentagon_vault.cpp</summary>

\include{lineno} templates_cylindrical_folding_10_pentagon_vault.cpp

</details>

### 11. Hexagon Vault (`sides = 6`) {#hexagon_vault}

![hexagon vault](templates/templates_cylindrical_folding_11_hexagon_vault_lamellas.webp)

```cpp
CylinderVault vault(6, 8000.0, 4000.0, CylinderVault::Kind::Groin);
```

<details><summary>Example code: templates_cylindrical_folding_11_hexagon_vault.cpp</summary>

\include{lineno} templates_cylindrical_folding_11_hexagon_vault.cpp

</details>

### 12. Octagon Vault (`sides = 8`) {#octagon_vault}

![octagon vault](templates/templates_cylindrical_folding_12_octagon_vault_lamellas.webp)

```cpp
CylinderVault vault(8, 6000.0, 3000.0, CylinderVault::Kind::Groin);
```

<details><summary>Example code: templates_cylindrical_folding_12_octagon_vault.cpp</summary>

\include{lineno} templates_cylindrical_folding_12_octagon_vault.cpp

</details>

---

## 13. CurvedFoldedBeam {#curved_folded_beam}

`CurvedFoldedBeam(span, width, rise, sag, thickness, webs, count)`: closed box girder of `4` cylindrical `PlateCurved` flanges/webs (`top`/`bottom` arched outward by `rise`, `sides` dished inward by `sag`) stiffened by `webs` planar cross-diaphragms (`54` plates, `144` contacts across `6` beams).

![curved-folded beam](templates/templates_cylindrical_folding_13_curved_folded_beam_all.webp)

```cpp
CurvedFoldedBeam beams;
```

<details><summary>Example code: templates_cylindrical_folding_13_curved_folded_beam.cpp</summary>

\include{lineno} templates_cylindrical_folding_13_curved_folded_beam.cpp

</details>

---

## 14–15. CornerFan {#corner_fan}

`CornerFan(size, height, strips, lift, drop, foot, thickness, corner)`: `strips` cylindrical `PlateCurved` bands radiating from one corner of a `size × size` square to its two far sides along 3-point parabolic creases (`lift` mid-chord rise, alternating `drop` height modulation).

### 14. Six-Strip Corner Fan

![corner fan](templates/templates_cylindrical_folding_14_corner_fan_strips.webp)

![corner fan, plan](templates/templates_cylindrical_folding_14_corner_fan_plan.webp)

```cpp
CornerFan fan(6000.0, 4000.0, 6, 1200.0, 800.0, 0.15, 40.0);
```

<details><summary>Example code: templates_cylindrical_folding_14_corner_fan.cpp</summary>

\include{lineno} templates_cylindrical_folding_14_corner_fan.cpp

</details>

### 15. Corner Fan Subdivisions (`2`, `4`, `8`, `12` Strips) {#corner_fan_subdivisions}

![corner fan subdivisions](templates/templates_cylindrical_folding_15_corner_fan_subdivisions_all.webp)

<details><summary>Example code: templates_cylindrical_folding_15_corner_fan_subdivisions.cpp</summary>

\include{lineno} templates_cylindrical_folding_15_corner_fan_subdivisions.cpp

</details>

---

## 16–17. CylinderCarving {#cylinder_carving}

`CylinderCarving(stock, carvers, length, thickness, min_angle, closed)`: sequential boolean subtraction $((a - b) - c) - d$ of cylindrical `carvers` from a `stock` cylinder, converting each concave carved patch into a `PlateCurved` shell of `thickness`.

### 16. Sequential Three-Carver Column

![cylinder carving](templates/templates_cylindrical_folding_16_cylinder_carving_all.webp)

Progressive carving stages `a - b`, `(a - b) - c`, and `((a - b) - c) - d` on a `R = 2000 mm`, `12 m` column.

<details><summary>Example code: templates_cylindrical_folding_16_cylinder_carving.cpp</summary>

\include{lineno} templates_cylindrical_folding_16_cylinder_carving.cpp

</details>

### 17. Closed Fluted Rings (`4`, `6`, `8` Carvers) {#cylinder_carving_rings}

![cylinder carving rings](templates/templates_cylindrical_folding_17_cylinder_carving_rings_all.webp)

Closed fluted columns (`closed = true`) carved all round by `4`, `6`, and `8` alternating-tilt cylinders.

<details><summary>Example code: templates_cylindrical_folding_17_cylinder_carving_rings.cpp</summary>

\include{lineno} templates_cylindrical_folding_17_cylinder_carving_rings.cpp

</details>
