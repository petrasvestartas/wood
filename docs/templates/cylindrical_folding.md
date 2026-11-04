# Folding Cylindrical {#templates_cylindrical_folding}

[TOC]

Folded structures of exact cylindrical [Curved Plate](@ref elements_plate_curved) lamellas with parallel straight rulings, zero Gaussian curvature, rational NURBS ellipse creases, and zero flat-unfolding distortion.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#reflection_principle">
    <img src="templates_cylindrical_folding_1_reflection_lamellas.webp" alt="Reflection Principle">
    <span class="wood-card-title">Reflection Principle</span>
  </a>
  <a class="wood-card" href="#curved_folded_plate">
    <img src="templates_cylindrical_folding_2_curved_folded_plate_lamellas.webp" alt="Curved Folded Arch">
    <span class="wood-card-title">Curved Folded Arch</span>
  </a>
  <a class="wood-card" href="#mendrisio_pavilion">
    <img src="templates_cylindrical_folding_3_mendrisio_pavilion_lamellas.webp" alt="Mendrisio Pavilion">
    <span class="wood-card-title">Mendrisio Pavilion</span>
  </a>
  <a class="wood-card" href="#folded_cylinders">
    <img src="templates_cylindrical_folding_5_two_hinged_arch_lamellas.webp" alt="Cylindrical Halls">
    <span class="wood-card-title">Cylindrical Halls</span>
  </a>
  <a class="wood-card" href="#cylinder_vault">
    <img src="templates_cylindrical_folding_8_groin_vault_lamellas.webp" alt="Cylinder Vault">
    <span class="wood-card-title">Cylinder Vault</span>
  </a>
  <a class="wood-card" href="#curved_folded_beam">
    <img src="templates_cylindrical_folding_13_curved_folded_beam_all.webp" alt="Curved Folded Beam">
    <span class="wood-card-title">Curved Folded Beam</span>
  </a>
  <a class="wood-card" href="#corner_fan">
    <img src="templates_cylindrical_folding_14_corner_fan_strips.webp" alt="Corner Fan">
    <span class="wood-card-title">Corner Fan</span>
  </a>
  <a class="wood-card" href="#cylinder_carving">
    <img src="templates_cylindrical_folding_17_cylinder_carving_rings_ring_8.webp" alt="Cylinder Carving">
    <span class="wood-card-title">Cylinder Carving</span>
  </a>
</div>

## Cylindrical Crease Invariants {#why_cylinders}

- Planar Crease Theorem: Two smooth developable patches meet along a planar curve when one is the reflection or mirror image of the other across the crease plane (rulings make equal angles with the plane).
- Exact Rational NURBS Solids: Projecting a circular NURBS arc along parallel rulings onto an oblique fold plane is an affine map that preserves rational weights, producing exact ellipse arcs and an exact 6-patch `PlateCurved` solid (`distortion = 0`).

---

## Reflection Fold {#reflection_fold}

| `Fold::reflected` | Ruling Update | Curvature / Bulge | Unfolded Development |
| --- | --- | --- | --- |
| `true` (Reflection) | Sheet beyond fold plane `R` is mirrored across `R` (`normal` parallel to `r_prev - r_next`) | Flips: vault ↔ trough | Single continuous flat sheet |
| `false` (Mirror fold) | Next segment is mirrored in the angle bisector (`normal` parallel to `r_prev + r_next`) | Preserved: vault ↔ vault | Separate mirror-image developments per segment |

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

### Reflection Principle {#reflection_principle}

Circular arc strip (`120°`, `R = 1000 mm`, `t = 120 mm`) cut by an oblique plane `1500 mm` down and reflected by `120°` (`2` lamellas, `1` contact).

![](templates/templates_cylindrical_folding_1_reflection_lamellas.webp)

\include{lineno} templates_cylindrical_folding_1_reflection.cpp

### Curved Folded Plate Arch {#curved_folded_plate}

Six half-sine waves (`9 m` wide, `400 mm` amplitude) swept along a 5-segment polygonal arch with four `-30°` reverse folds (`13.5 m` span, `4.9 m` rise, `30` lamellas, `49` contacts).

![](templates/templates_cylindrical_folding_2_curved_folded_plate_lamellas.webp)

\include{lineno} templates_cylindrical_folding_2_curved_folded_plate.cpp

### Mendrisio Pavilion {#mendrisio_pavilion}

IBOIS Mendrisio pavilion (`13 × 4.5 × 3.4 m`, `77 mm` curved CLT): shallow barrel reflected at the eave (`89.9°`) and ridge (`10°`) across the symmetry plane (`5` plates, `4` contacts).

![](templates/templates_cylindrical_folding_3_mendrisio_pavilion_lamellas.webp)

\include{lineno} templates_cylindrical_folding_3_mendrisio_pavilion.cpp

### Linear Folded Cylindrical Hall Systems {#folded_cylinders}

Four-barrel rows (`4 × 6000 mm` width, `120 mm` thick) folded into halls and arches:

| System | Climb | Folds `(station, angle, reflected)` | Symmetry `x` | Barrel Rise | Lamellas / Contacts | Example |
| --- | --- | --- | --- | --- | --- | --- |
| Two-hinged arch | `105°` | `(11000, -50°, false)`, `(22000, -50°, false)` | `11000` | `2400 mm` | `24` / `38` | `templates_cylindrical_folding_5_two_hinged_arch` |
| Two-hinged A-frame | `110°` | `(5600, -60°, true)` | `12500` | `1400 mm` | `16` / `24` | `templates_cylindrical_folding_7_a_frame` |

#### Two-Hinged Arch {#two_hinged_arch}

![](templates/templates_cylindrical_folding_5_two_hinged_arch_lamellas.webp)

\include{lineno} templates_cylindrical_folding_5_two_hinged_arch.cpp

#### Two-Hinged A-Frame {#a_frame}

![](templates/templates_cylindrical_folding_7_a_frame_lamellas.webp)

\include{lineno} templates_cylindrical_folding_7_a_frame.cpp

---

## Cylinder Vault {#cylinder_vault}

`CylinderVault(sides, edge, radius, kind, wall, thickness)`: `sides` equal cylinder sectors over a regular polygon meeting on vertical corner-to-centre planes.

| `Kind` | Generatrices | Profile | Creases |
| --- | --- | --- | --- |
| `Groin` | Radial (side → centre) | Arch over polygon side, springing at corners | Half-ellipses from corners to crown |
| `Cloister` | Tangential (parallel to polygon side, on `wall`) | Arc from wall top to central peak | Quarter-ellipses from corners to peak |

### Groin Vault (4 Sides) {#groin_vault}

![](templates/templates_cylindrical_folding_8_groin_vault_lamellas.webp)

\include{lineno} templates_cylindrical_folding_8_groin_vault.cpp

### Pavilion Cloister Vault on Walls (4 Sides) {#pavilion_vault}

![](templates/templates_cylindrical_folding_9_pavilion_vault_lamellas.webp)

\include{lineno} templates_cylindrical_folding_9_pavilion_vault.cpp

### Octagon Vault (8 Sides) {#octagon_vault}

![](templates/templates_cylindrical_folding_12_octagon_vault_lamellas.webp)

\include{lineno} templates_cylindrical_folding_12_octagon_vault.cpp

---

## Curved Folded Beam {#curved_folded_beam}

`CurvedFoldedBeam(span, width, rise, sag, thickness, webs, count)`: closed box girder of `4` cylindrical `PlateCurved` flanges and webs (`top` and `bottom` arched outward by `rise`, `sides` dished inward by `sag`) stiffened by `webs` planar cross-diaphragms (`54` plates, `144` contacts across `6` beams).

![](templates/templates_cylindrical_folding_13_curved_folded_beam_all.webp)

\include{lineno} templates_cylindrical_folding_13_curved_folded_beam.cpp

---

## Corner Fan {#corner_fan}

`CornerFan(size, height, strips, lift, drop, foot, thickness, corner)`: `strips` cylindrical `PlateCurved` bands radiating from one corner of a `size × size` square to its two far sides along 3-point parabolic creases (`lift` mid-chord rise, alternating `drop` height modulation).

### Six-Strip Corner Fan

![](templates/templates_cylindrical_folding_14_corner_fan_strips.webp)

![](templates/templates_cylindrical_folding_14_corner_fan_plan.webp)

\include{lineno} templates_cylindrical_folding_14_corner_fan.cpp

---

## Cylinder Carving {#cylinder_carving}

`CylinderCarving(stock, carvers, length, thickness, min_angle, closed)`: sequential boolean subtraction `((a - b) - c) - d` of cylindrical `carvers` from a `stock` cylinder, converting each concave carved patch into a `PlateCurved` shell of `thickness`.

### Sequential Three-Carver Column

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_cylindrical_folding_16_cylinder_carving_stage_1.webp">
    <img src="templates_cylindrical_folding_16_cylinder_carving_stage_1.webp" alt="a - b">
    <span class="wood-card-title">a - b</span>
  </a>
  <a class="wood-card" href="templates_cylindrical_folding_16_cylinder_carving_stage_2.webp">
    <img src="templates_cylindrical_folding_16_cylinder_carving_stage_2.webp" alt="(a - b) - c">
    <span class="wood-card-title">(a - b) - c</span>
  </a>
  <a class="wood-card" href="templates_cylindrical_folding_16_cylinder_carving_stage_3.webp">
    <img src="templates_cylindrical_folding_16_cylinder_carving_stage_3.webp" alt="((a - b) - c) - d">
    <span class="wood-card-title">((a - b) - c) - d</span>
  </a>
</div>

Progressive carving stages `a - b`, `(a - b) - c`, and `((a - b) - c) - d` on a `R = 2000 mm`, `12 m` column.

\include{lineno} templates_cylindrical_folding_16_cylinder_carving.cpp

### Closed Fluted Rings (4, 6, 8 Carvers) {#cylinder_carving_rings}

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_cylindrical_folding_17_cylinder_carving_rings_ring_4.webp">
    <img src="templates_cylindrical_folding_17_cylinder_carving_rings_ring_4.webp" alt="4 Carvers">
    <span class="wood-card-title">4 Carvers</span>
  </a>
  <a class="wood-card" href="templates_cylindrical_folding_17_cylinder_carving_rings_ring_6.webp">
    <img src="templates_cylindrical_folding_17_cylinder_carving_rings_ring_6.webp" alt="6 Carvers">
    <span class="wood-card-title">6 Carvers</span>
  </a>
  <a class="wood-card" href="templates_cylindrical_folding_17_cylinder_carving_rings_ring_8.webp">
    <img src="templates_cylindrical_folding_17_cylinder_carving_rings_ring_8.webp" alt="8 Carvers">
    <span class="wood-card-title">8 Carvers</span>
  </a>
</div>

Closed fluted columns (`closed = true`) carved all round by `4`, `6`, and `8` alternating-tilt cylinders.

\include{lineno} templates_cylindrical_folding_17_cylinder_carving_rings.cpp
