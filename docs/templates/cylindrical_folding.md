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

```cpp
ReflectionFold fold(
    {Primitives::arc(Point(500.0, -866.0, 3000.0), Point(1000.0, 0.0, 3000.0), Point(500.0, 866.0, 3000.0))},
    Vector(0.0, 0.0, -1.0),
    Vector(0.0, -1.0, 0.0),
    3500.0,
    {{1500.0, 60.0 * Tolerance::TO_RADIANS}}
);
```

### Curved Folded Plate Arch {#curved_folded_plate}

Six half-sine waves (`9 m` wide, `400 mm` amplitude) swept along a 5-segment polygonal arch with four `-30°` reverse folds (`13.5 m` span, `4.9 m` rise, `30` lamellas, `49` contacts).

![](templates/templates_cylindrical_folding_2_curved_folded_plate_lamellas.webp)

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

### Mendrisio Pavilion {#mendrisio_pavilion}

IBOIS Mendrisio pavilion (`13 × 4.5 × 3.4 m`, `77 mm` curved CLT): shallow barrel reflected at the eave (`89.9°`) and ridge (`10°`) across the symmetry plane (`5` plates, `4` contacts).

![](templates/templates_cylindrical_folding_3_mendrisio_pavilion_lamellas.webp)

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

### Linear Folded Cylindrical Hall Systems {#folded_cylinders}

Four-barrel rows (`4 × 6000 mm` width, `120 mm` thick) folded into halls and arches:

| System | Climb | Folds `(station, angle, reflected)` | Symmetry `x` | Barrel Rise | Lamellas / Contacts | Example |
| --- | --- | --- | --- | --- | --- | --- |
| Two-hinged arch | `105°` | `(11000, -50°, false)`, `(22000, -50°, false)` | `11000` | `2400 mm` | `24` / `38` | `templates_cylindrical_folding_5_two_hinged_arch` |
| Two-hinged A-frame | `110°` | `(5600, -60°, true)` | `12500` | `1400 mm` | `16` / `24` | `templates_cylindrical_folding_7_a_frame` |

#### Two-Hinged Arch {#two_hinged_arch}

![](templates/templates_cylindrical_folding_5_two_hinged_arch_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 2400.0), along, Vector(0.0, -1.0, 0.0), 40000.0, {{11000.0, -50.0 * Tolerance::TO_RADIANS, false}, {22000.0, -50.0 * Tolerance::TO_RADIANS, false}}, Plane::from_point_normal(Point(11000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

#### Two-Hinged A-Frame {#a_frame}

![](templates/templates_cylindrical_folding_7_a_frame_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1400.0), along, Vector(0.0, -1.0, 0.0), 40000.0, {{5600.0, -60.0 * Tolerance::TO_RADIANS, true}}, Plane::from_point_normal(Point(12500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

---

## Cylinder Vault {#cylinder_vault}

`CylinderVault(sides, edge, radius, kind, wall, thickness)`: `sides` equal cylinder sectors over a regular polygon meeting on vertical corner-to-centre planes.

| `Kind` | Generatrices | Profile | Creases |
| --- | --- | --- | --- |
| `Groin` | Radial (side → centre) | Arch over polygon side, springing at corners | Half-ellipses from corners to crown |
| `Cloister` | Tangential (parallel to polygon side, on `wall`) | Arc from wall top to central peak | Quarter-ellipses from corners to peak |

### Groin Vault (4 Sides) {#groin_vault}

![](templates/templates_cylindrical_folding_8_groin_vault_lamellas.webp)

```cpp
CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Groin);
```

### Pavilion Cloister Vault on Walls (4 Sides) {#pavilion_vault}

![](templates/templates_cylindrical_folding_9_pavilion_vault_lamellas.webp)

```cpp
CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Cloister, 4000.0);
```

### Octagon Vault (8 Sides) {#octagon_vault}

![](templates/templates_cylindrical_folding_12_octagon_vault_lamellas.webp)

```cpp
CylinderVault vault(8, 6000.0, 3000.0, CylinderVault::Kind::Groin);
```

---

## Curved Folded Beam {#curved_folded_beam}

`CurvedFoldedBeam(span, width, rise, sag, thickness, webs, count)`: closed box girder of `4` cylindrical `PlateCurved` flanges and webs (`top` and `bottom` arched outward by `rise`, `sides` dished inward by `sag`) stiffened by `webs` planar cross-diaphragms (`54` plates, `144` contacts across `6` beams).

![](templates/templates_cylindrical_folding_13_curved_folded_beam_all.webp)

```cpp
CurvedFoldedBeam beams;
```

---

## Corner Fan {#corner_fan}

`CornerFan(size, height, strips, lift, drop, foot, thickness, corner)`: `strips` cylindrical `PlateCurved` bands radiating from one corner of a `size × size` square to its two far sides along 3-point parabolic creases (`lift` mid-chord rise, alternating `drop` height modulation).

### Six-Strip Corner Fan

![](templates/templates_cylindrical_folding_14_corner_fan_strips.webp)

![](templates/templates_cylindrical_folding_14_corner_fan_plan.webp)

```cpp
CornerFan fan(6000.0, 4000.0, 6, 1200.0, 800.0, 0.15, 40.0);
```

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

Full source: @ref cylindrical_folding_example_16

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

Full source: @ref cylindrical_folding_example_17

## Examples {#cylindrical_folding_examples}

### 1 Reflection {#cylindrical_folding_example_1}

Step 1 of the cylindrical folding templates: the reflection principle. A vertical strip with a circular arc section e(t), 120 degrees of a circle of 1 m radius, runs down from s0 and is cut at s1 by the plane R inclined at (pi - phi) / 2 to its axis; the part beyond R is mirrored in R, so the strip bends by phi = 120 degrees toward s2r, and the crease a(t) is R's section of the arc, a planar curve. Two lamellas 120 mm thick meet on the crease along one mitre, the exact curve where their two offset cylinders cross. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_\<segment\>_\<row\>), flat.

\include{lineno} templates_cylindrical_folding_1_reflection.cpp

### 2 Curved Folded Plate {#cylindrical_folding_example_2}

Step 2 of the cylindrical folding templates: a curved folded plate. A wave corrugation, three sine waves of 3 m wavelength and 400 mm amplitude across a 9 m wide sheet, is swept along a polygonal arch of five segments of 3.62 m turning 30 degrees at each of four reverse folds, from 60 degrees up at one foot to 60 degrees down at the other, spanning 13.5 m and rising 4.9 m. Every reverse fold is a reflection of the sheet in the plane bisecting the kink, so every crease is that plane's section of the corrugation, a planar wave, and the sheet still unrolls flat. Every half wave of every segment is one lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick, mitred where it meets the next lamella across a crease, its caps the straight rulings at the waves' cusps and the sheet's edges. Groups: creases, lamellas (lamella_\<segment\>_\<row\>), flat.

\include{lineno} templates_cylindrical_folding_2_curved_folded_plate.cpp

### 3 Mendrisio Pavilion {#cylindrical_folding_example_3}

Step 3 of the cylindrical folding templates: the Mendrisio pavilion of IBOIS, 13 m long at the top, 4.5 m wide and 3.4 m high, 77 mm curved cross-laminated timber. One shallow barrel, an arc 4.5 m wide and 400 mm high, stands as the end wall leaning out, is reflected at the eave onto the roof rising 10 degrees, a trough, and 700 mm further at the ridge crease onto the level middle, a vault whose rulings cross the plane of symmetry at mid-length square, so it is one plate from ridge to ridge. Five curved plates, wall, trough, middle, trough, wall, each an exact cylinder, mitred on the four creases. Groups: creases, lamellas, flat.

\include{lineno} templates_cylindrical_folding_3_mendrisio_pavilion.cpp

### 5 Two Hinged Arch {#cylindrical_folding_example_5}

Step 5 of the cylindrical folding templates: the two-hinged arch, high and narrow. The sheet of four deep barrels, each 6 m wide and 2400 high, the capsules of the end elevation, rises from the hinge at its foot as a leg leaning 15 degrees outward, is folded 50 degrees onto a segment at 55 degrees and 50 degrees again onto the crown segment at 5 degrees; the crown at mid-span is the plane of symmetry, the folded half mirrored in it, meeting itself along the plane's section of the barrels. Both folds are the mirror folds: the next segment is the mirror image of the one before in the plane bisecting the fold, so every segment is a vault, as the end elevation shows the barrels rounded all the way up; the creases are the bisecting planes' sections, the ovals of the plan at the quarter points, the crown line is straight. Span 22 m, crown 20 m. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_\<segment\>_\<row\>), flat.

\include{lineno} templates_cylindrical_folding_5_two_hinged_arch.cpp

### 7 A Frame {#cylindrical_folding_example_7}

Step 7 of the cylindrical folding templates: the two-hinged A-frame. The sheet of four barrels, each 6 m wide and 1400 high, rises from the hinge at its foot leaning 20 degrees outward, is reflected 60 degrees at the knee into a leg at 50 degrees, and the apex at mid-span is the plane of symmetry: the leg and its mirror image meet there along the plane's section of the barrels, the straight ridge of the plan, the knee creases its pointed ovals near the feet; the feet are vaults and the legs troughs hanging between the cusps that run up to the apex, as in the end elevation. Span 25 m, apex 22.4 m. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_\<segment\>_\<row\>), flat.

\include{lineno} templates_cylindrical_folding_7_a_frame.cpp

### 8 Groin Vault {#cylindrical_folding_example_8}

Step 8 of the cylindrical folding templates: the groin vault. Two half barrels of 12 m span and 6 m radius cross over a 12 m square; every quarter is a level cylinder from its side to the centre, and neighbours meet on half ellipses in the diagonal planes. Every segment is one 120 mm PlateCurved: its outer face the exact cylinder, its inner face the cylinder 120 mm smaller, its side faces shared with its neighbours across the creases; every panel unfolds flat. Groups: plan, lamellas, flat.

\include{lineno} templates_cylindrical_folding_8_groin_vault.cpp

### 9 Pavilion Vault {#cylindrical_folding_example_9}

Step 9 of the cylindrical folding templates: the pavilion vault. Four walls 4 m high on a 12 m square, on each a cylinder segment of 6 m radius along the wall rising to the peak over the centre; the segments meet on quarter ellipses in the diagonal planes. Every segment is one 120 mm PlateCurved: its outer face the exact cylinder, its inner face the cylinder 120 mm smaller, its side faces shared with its neighbours across the creases; every panel unfolds flat. The walls are flat 120 mm panels mitred at the corners. Groups: plan, lamellas, walls, flat.

\include{lineno} templates_cylindrical_folding_9_pavilion_vault.cpp

### 12 Octagon Vault {#cylindrical_folding_example_12}

Step 12 of the cylindrical folding templates: the octagon groin vault. Eight half barrels of 6 m span and 3 m radius over a regular octagon, every one a level cylinder from its side to the centre, so the crown is at the height of the arches' top; neighbours meet on half ellipses in the vertical planes through the corners and the centre. Every segment is one 120 mm PlateCurved: its outer face the exact cylinder, its inner face the cylinder 120 mm smaller, its side faces shared with its neighbours across the creases; every panel unfolds flat. Groups: plan, lamellas, flat.

\include{lineno} templates_cylindrical_folding_12_octagon_vault.cpp

### 13 Curved Folded Beam {#cylindrical_folding_example_13}

Step 13 of the cylindrical folding templates: a closed curved-folded beam. A box spans 12 m along x, 800 mm wide and 400 mm high at the supports; its four plates are cylinders bent along the span with their rulings straight across: the top and bottom bulge 250 mm outward toward mid-span, the sides sag 80 mm inward, so the flat section at the supports becomes 640 mm wide and 900 mm deep at mid-span. Every corner is a curved crease, the two plates mitred on the ruled face between the crease and the inner crease. Five flat webs across the span divide the box, each filling its inner section, its four edges on the plates' inner faces. Six beams of 27 mm plates side by side; the contacts are the four mitres and the four edges of every web. Groups: beam_\<b\>.

\include{lineno} templates_cylindrical_folding_13_curved_folded_beam.cpp

### 14 Corner Fan {#cylindrical_folding_example_14}

Step 14 of the cylindrical folding templates: a corner fan inscribed in a 6 m square. Every fold is a parabola from three control points laid out in plan, the corner, the middle and the end on one of the square's two far sides, the end raised to 4 m, every second one 800 mm lower, and the middle lifted 1200 mm above the chord. All folds lifted alike, two neighbours differ by their ends' difference times the parameter, so every strip is a cylinder of straight rulings parallel to its top edge on the square, developable. 40 mm plates meeting at every fold on the bisector of their inner faces, cut at 15 percent of the way up. Groups: strips.

\include{lineno} templates_cylindrical_folding_14_corner_fan.cpp

### 16 Cylinder Carving {#cylindrical_folding_example_16}

Step 16 of the cylindrical folding templates: a cylinder carved by cylinders in sequence. The stock a, 2 m radius and 12 m high, standing on its axis, is carved by b of 1.5 m radius, then by c of 1.8 m, then by d of 1.3 m, their axes 40 degrees apart round one side, every one cutting 700 mm deep and turned a few degrees off a's axis; the three stages a - b, (a - b) - c and ((a - b) - c) - d stand side by side. Only the carved patches are made: every carver's surface inside the stock and outside the other carvers, curving inward, one 60 mm plate behind it. The carving is validated first: every carver must leave a patch, none may cut another's patch in two, and two patches must meet at 10 degrees or more. Groups: stage_\<carvers\>.

\include{lineno} templates_cylindrical_folding_16_cylinder_carving.cpp

### 17 Cylinder Carving Rings {#cylindrical_folding_example_17}

Step 17 of the cylindrical folding templates: closed rings of carved patches. The stock, 2 m radius and 12 m high, standing on its axis, is carved all round by 4 cylinders of 2 m radius cutting 1600 mm deep, by 6 of 1.6 m cutting 1000 mm and by 8 of 1.2 m cutting 800 mm, every carver's axis turned off the stock's one way and the other in turn; the three rings stand side by side. Every patch meets both neighbours, so the plates close round the stock into a fluted tube, one contact per crease; the carving is validated closed, every point of the stock's surface carved, so the plates leave no gap. Groups: ring_\<carvers\>.

\include{lineno} templates_cylindrical_folding_17_cylinder_carving_rings.cpp
