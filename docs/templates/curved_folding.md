# Folding curved {#templates_curved_folding}

[TOC]

Folded structures whose creases are curves: every lamella is a curved plate, a [`PlateCurved`](@ref elements_plate_curved), the strip of a developable surface between two curves, bent from a flat panel about one axis and never twisted, as glulam and curved cross-laminated timber are made. Three constructions, each a `WoodSession` whose constructor is its recipe, one group per step, and every picture below one group of the example's scene:

| kind | template | header | source | examples |
| --- | --- | --- | --- | --- |
| cylindrical | [`ReflectionFold`](#reflection_fold) | `curved_folding/reflection_fold.h` | the reflection principle; the IBOIS curved folded plate and Mendrisio pavilion; the linear systems of folded cylindrical surfaces | `templates_curved_folding_{2_reflection,3_curved_folded_plate,4_mendrisio_pavilion,13_two_hinged_frame,15_two_hinged_arch,16_arch_with_top_hinge,17_a_frame}` |
| cylindrical | [`CylinderVault`](#cylinder_vault) | `curved_folding/cylinder_vault.h` | equal cylinders over a regular polygon meeting on planar creases | `templates_curved_folding_{5_groin_vault,8_pavilion_vault,9_pentagon_vault,10_octagon_vault,11_hexagon_vault}` |
| cylindrical | [`CurvedFoldedBeam`](#curved_folded_beam) | `curved_folding/curved_folded_beam.h` | a closed box of four curved plates, flat at the supports and deep at mid-span, the top and bottom bulging out, the sides sagging in, webs across the span | `templates_curved_folding_31_curved_folded_beam` |
| cylindrical | [`CornerFan`](#corner_fan) | `curved_folding/corner_fan.h` | cylinder strips folded from one corner of a square on parabolic creases to its two far sides | `templates_curved_folding_{32_corner_fan,33_corner_fan_subdivisions}` |
| cylindrical | [`CylinderCarving`](#cylinder_carving) | `curved_folding/cylinder_carving.h` | a cylinder carved by tilted cylinders in sequence, one concave plate per carved patch | `templates_curved_folding_{34_cylinder_carving,35_cylinder_carving_rings}` |
| developable | [`CurvedFold`](#curved_fold_roof) | `curved_folding/curved_fold.h` | a tent roof of curved folds with interpolated meridians | `templates_curved_folding_1_curved_fold` |
| developable | [`CurvedCreaseUnfolding`](#curved_crease_unfolding) | `curved_folding/curved_crease_unfolding.h` | flat plates with curved edges hinged into a corrugated vault | `templates_curved_folding_{20_curved_crease_unfolding,21_catenary_unfolding,23_synclastic_unfolding,..,30_corrugated_funnel}` |
| developable | [`CreaseQuadrants`](#crease_quadrants) | `curved_folding/crease_quadrants.h` | the four ways two strips join on one planar curved crease | `templates_curved_folding_22_crease_quadrants` |
| both | [`Development`](#development) | `curved_folding/development.h` | the unfolding of a ruled strip, the check that every lamella is cut flat | every example prints it |

Every example prints the number of lamellas or panels, how many are closed solids, how many contacts the session's contact detection finds between them, and the development distortion and ruling twist of the worst one; on every model below the distortion is at the rounding floor (1e-14), the lamellas being exact cylinders. The contacts are the seams: every two lamellas that share a face are found, so the count below each example is the proof that its mitres meet, creases times lamellas per crease plus the joints between neighbouring barrels of a row.

## Why cylinders {#why_cylinders}

A curved crease between two smooth developable patches is planar when the two patches are each other's mirror image in the crease's plane: the generating lines on both sides make equal angles with that plane. This is the construction: cut a developable sheet by a plane and mirror the part beyond it; the crease is the plane's section of the sheet and the sheet still unfolds flat, because the mirrored part keeps its development. The same fact makes interpenetrating cylinders curved foldings: two equal cylinders mirrored in a vertical plane meet in that plane along an ellipse, straight in plan and curved in elevation.

For timber the cylinder is the surface to use. Its generatrices are straight and parallel, so a lamella between two of them is bent about one axis only, from a flat panel whose outline is the lamella's development; the circle is an exact rational NURBS arc, its sections by planes are exact ellipse arcs (the arc projected along the generatrices onto the plane, an affine map that keeps the arc's weights), and the ruled surface between two such sections, joining the points of equal parameter, is the exact cylinder, since both are projections of the same arc along the same direction. A `PlateCurved` built on them is therefore an exact solid: its outer and inner faces the two coaxial cylinders, its side faces the planar sections (shared with the neighbour across the crease, which is their mirror image), its caps the generatrices (shared with the next lamella along the segment).

## Development {#development}

`Development(a, b, samples)` unfolds the ruled strip between two curves into the plane, ruling by ruling: every quad between two rulings is split along one diagonal into two triangles laid flat with their edge lengths kept. On a developable strip, a cylinder or a cone, the quads are planar, so the other diagonal keeps its length too: its largest relative change is the `distortion`, the error of cutting the lamella from a flat panel, 0 for an exact cylinder. The `twist` measures the same on the smooth strip: the sine of the angle by which a ruling leaves the plane of the two curves' tangents. The `outline` is the flat cutting outline, drawn in every model's group `flat` beside it, every lamella where it lies on the unfolded sheet or the unrolled segment.

```cpp
const Development development(lamella->outer_a, lamella->outer_b, 32);
development.distortion; // 0 on a cylinder, the relative length error of a twisted strip
development.outline; // the flat outline in the xy plane, the first ruling on the y axis
```

## Cylindrical folding {#cylindrical_folding}

Templates whose every plate is a cylinder: straight parallel rulings over one profile, bent from a flat panel about one axis.

### Reflection fold {#reflection_fold}

A cylindrical sheet folded at planar creases (`curved_folding/reflection_fold.h`). The sheet is a profile, planar curves one after another sharing their ends (`profile`), swept `length` along the unit `direction`. Every `Fold` turns the rulings about the folds' `axis` by its `angle` at its `station` along the rulings, and the crease is the fold plane's section of the sheet, a planar curve, an ellipse arc when the profile is an arc. A fold is made in one of two ways:

| `Fold::reflected` | construction | the crease | the bulge | the development |
| --- | --- | --- | --- | --- |
| `true` | the reflection: the part of the sheet beyond the plane is mirrored in it, the plane bisecting the angle between the old rulings and the new | the plane's oblique section, the long oval of a sharp kink | flips: the sheet is turned over, a vault folds into a trough, as the end elevations show at the knees | the whole sheet stays one development |
| `false` | the mirror fold: the next segment is the mirror image of the one before in the plane bisecting the fold's angle | the bisecting plane's section | kept: a vault folds onto a vault, the capsules of the two-hinged arch | every segment is cut flat on its own; the two sides of the crease develop as mirror images, not as one sheet |

An optional `symmetry` plane is the other planar crease, a frame's ridge or an arch's crown: the sheet ends on the plane and the folded half is mirrored in it, the two halves meeting along the plane's section, straight in plan. Where the last segment's rulings cross the plane square, a level crown, the segment and its mirror image are one cylinder, so its plates run on across the plane to the mirror image of their first crease, one plate each, and the plane is no seam. A column is the sheet too: a reflection whose plane crosses the first segment higher in the middle of a barrel than at its cusps (a reflection's crease runs `rise / tan(angle / 2)` further along the rulings at the bulge than at the cusps) leaves a leg under every barrel rising as an arch to the crease.

The thickness is the sheet's own: the profile's section square to the rulings is offset once to each side, every curve along its normal, two curves meeting at an angle ending where their offsets cross (the seam between the two rows is the plane through the rulings there, vertical for symmetric barrels), two meeting tangent sharing their end, the sheet's free edges cut square to the section's chord; both offsets are carried from plane to plane along the rulings exactly as the creases are. The material lies under the sheet in the first segment; a reflection turns the sheet over, so beyond it the material lies on the other offset, while a mirror fold keeps it. At a mirror fold, a sheet end and the symmetry plane the two plates are each other's mirror image on either side of the plane and meet on it in one planar curve. At a reflection both plates lie on one side of the plane, so every inner ruling of the plate before runs on to the inner cylinder of the plate beyond, the mirror image of the other offset's: the mitre is the ruled face between the crease and that crossing. Every seam is one face the two plates share, and the session's contact detection finds one contact per seam.

```cpp
 // segments: the sheet's straight runs between the folds, each with its rulings and its two planes
 const Plane first = compute_profile_plane();
 const std::vector<Segment> segments = compute_segments(first);

 // creases: every fold plane's section of the sheet, the profile carried along the rulings from plane to plane
 const std::vector<std::vector<NurbsCurve>> creases = compute_creases(segments);

 // profiles: the profile's section offset the thickness under and over the sheet, seams through the cusps, free edges square
 const std::array<std::vector<NurbsCurve>, 2> profiles = {
     compute_inner_profile(first, 1.0),
     compute_inner_profile(first, -1.0)
 };

 // inners: the offsets carried from plane to plane along the rulings, on every cut the one curve the two plates there share
 const std::vector<std::vector<NurbsCurve>> inners = compute_inners(segments, profiles);

 // creases drawn, lamellas, flat, the mirrored half when there is a symmetry plane, and the contacts
```

| step | what it checks or makes |
| --- | --- |
| segments | the fold plane through the station on the ruling from the profile's first point, its normal `rulings - next` for a reflection and `rulings + next` for a mirror fold; the end the symmetry plane or the profile's plane moved by `length`; at a crown, the rulings square to the symmetry plane, the last segment's creases and inner meridians carried by the mirror in it instead, one plate across the plane |
| creases | each crease the one before projected along its segment's rulings onto the next plane (exact: an affine map keeps the arcs' weights); every ruling must run forward from crease to crease, and a reflection's plane must not stand on the sheet, where its fold would be flat and the sheet would cross itself |
| offsets | the crease moved the thickness along the sheet's normal to the underside, its ends mitred where two curves of the profile meet at a kink |
| inners | the shared inner meridian of every cut, as above |
| lamellas | between two creases and two profile curves one `PlateCurved`, `lamella_<segment>_<row>`: its outer meridians the two creases, its inner meridians the two shared curves |
| flat | the cutting pattern beside the model: every lamella's `Development` where it lies on the unrolled sheet, the rows along x and the rulings along y |
| contacts | `compute_face_contacts(0)`: every shared face, the proof the seams meet |

Three factories build the usual profiles: `ReflectionFold::arcs(start, across, up, count, width, rise)`, circular arcs side by side, rows of barrels, and `ReflectionFold::waves(..)`, half sine waves.

#### The reflection principle {#reflection_principle}

The principle: a vertical strip with a circular arc section e(t), 120 degrees of a 1 m circle, runs down from s0 and is cut at s1 by the plane R inclined at (pi - phi) / 2 to its axis; the part beyond R is mirrored in R, so the strip bends by phi = 120 degrees toward s2r, and the crease a(t) is R's section of the arc. Two lamellas 120 mm thick, one crease, 1 contact: the exact curve where their offset cylinders cross (example `templates_curved_folding_2_reflection`).

![the reflection principle](templates/templates_curved_folding_2_reflection_lamellas.webp)

```cpp
ReflectionFold fold(
 {Primitives::arc(Point(500.0, -866.0, 3000.0), Point(1000.0, 0.0, 3000.0), Point(500.0, 866.0, 3000.0))},
 Vector(0.0, 0.0, -1.0), // the strip's rulings, down
 Vector(0.0, -1.0, 0.0), // the fold's axis
 3500.0,
 {{1500.0, 60.0 * Tolerance::TO_RADIANS}} // the plane R 1.5 m down, the strip bent by 120 degrees
);
```

<details><summary>Example code: templates_curved_folding_2_reflection.cpp</summary>

\include{lineno} templates_curved_folding_2_reflection.cpp

</details>

#### Curved folded plate {#curved_folded_plate}

A wave corrugation, three sine waves of 3 m wavelength and 400 mm amplitude across a 9 m wide sheet, swept along a polygonal arch of five segments of 3.62 m that turns 30 degrees at each of four reverse folds, from 60 degrees up at one foot to 60 degrees down at the other: 13.5 m span, 4.9 m rise. Every reverse fold is a reflection in the plane bisecting the kink, so every crease is a planar wave and the sheet still unrolls flat; every half wave of every segment is one lamella, 120 mm thick, mitred on the creases. 30 lamellas, 30 closed solids, 49 contacts (example `templates_curved_folding_3_curved_folded_plate`).

![curved folded plate](templates/templates_curved_folding_3_curved_folded_plate_lamellas.webp)

```cpp
const double climb = 60.0 * Tolerance::TO_RADIANS;
const Vector along(0.0, std::cos(climb), std::sin(climb));
const Vector up(0.0, -std::sin(climb), std::cos(climb));
ReflectionFold fold(
    ReflectionFold::waves(Point(-4500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), up, 6, 1500.0, 400.0),
    along,
    Vector(1.0, 0.0, 0.0),
    18100.0,
    {{3620.0, -30.0 * Tolerance::TO_RADIANS}, {7240.0, -30.0 * Tolerance::TO_RADIANS}, {10860.0, -30.0 * Tolerance::TO_RADIANS}, {14480.0, -30.0 * Tolerance::TO_RADIANS}}
);
```

<details><summary>Example code: templates_curved_folding_3_curved_folded_plate.cpp</summary>

\include{lineno} templates_curved_folding_3_curved_folded_plate.cpp

</details>

#### Mendrisio pavilion {#mendrisio_pavilion}

The Mendrisio pavilion of IBOIS: 13 m long, 4.5 m wide, 3.4 m high, 77 mm curved cross-laminated timber. One shallow barrel, an arc 4.5 m wide and 400 mm high, stands as the end wall leaning out, is reflected at the eave onto the roof rising 10 degrees, a trough, and at the ridge crease onto the level middle, a vault whose rulings cross the plane of symmetry square, one plate from ridge to ridge. The eave crease is the short oval where the wall meets the roof, the ridge crease the long oval reaching 5 m in from the end. Five plates, four contacts, one per crease (example `templates_curved_folding_4_mendrisio_pavilion`).

![mendrisio pavilion](templates/templates_curved_folding_4_mendrisio_pavilion_lamellas.webp)

```cpp
const Vector along(0.1719, 0.0, 0.9851); // the wall, leaning out 0.5 m over 2.9 m
ReflectionFold fold(
 {Primitives::arc(Point(6000.0, -2250.0, 0.0), Point(6400.0, 0.0, 0.0), Point(6000.0, 2250.0, 0.0))},
 along,
 Vector(0.0, -1.0, 0.0),
 12000.0,
 {{2923.0, 89.9 * Tolerance::TO_RADIANS, true}, {3623.0, 10.0 * Tolerance::TO_RADIANS, true}},
 Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
 77.0
);
```

<details><summary>Example code: templates_curved_folding_4_mendrisio_pavilion.cpp</summary>

\include{lineno} templates_curved_folding_4_mendrisio_pavilion.cpp

</details>

#### Folded cylindrical surfaces {#folded_cylinders}

Linear structure systems of folded cylindrical surfaces: a row of four barrels, each 6 m wide, folded at the kinks of a frame's or an arch's section, read from their sections, plans and end elevations. Every system is the same `ReflectionFold` with the profile `arcs(..)` across y and the folds along x, the first segment's climb setting the ruling direction. The end elevations say which kind every fold is: where the barrels flip from vaults (the columns, rounded arches at the feet) to troughs (the roof, hanging scallops at the top) the fold is a reflection; where the barrels stay rounded all the way, the two-hinged arch's capsules, the folds are mirror folds. A ridge or crown with a straight line in the plan is the `symmetry` plane, the ovals of the plans the sections of the barrels by the fold planes. Every system's contacts are its creases times four plus three cusp joints per segment.

| page | system | climb | folds: station, turn, kind | symmetry at x | barrels | lamellas, contacts | example |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 242 | two-hinged frame | 90, column 6 m | 6000, -76, reflection | 20000 | 600 high, inward | 16, 24 | `templates_curved_folding_13_two_hinged_frame` |
| 243 | two-hinged arch | 105, leg leaning out | 11000, -50; 22000, -50, mirror folds | 11000 | 2400 high | 24, 38 | `templates_curved_folding_15_two_hinged_arch` |
| 243 | arch with top hinge | 90, legs | 300, -15; 18300, -40, reflections; 25300, -35, mirror fold | 13000 | 1600 deep | 28, 45 | `templates_curved_folding_16_arch_with_top_hinge` |
| 244 | two-hinged A-frame | 110, foot leaning out | 5600, -60, reflection | 12500 | 1400 high | 16, 24 | `templates_curved_folding_17_a_frame` |

```cpp
const double climb = 90.0 * Tolerance::TO_RADIANS;
const Vector along(std::cos(climb), 0.0, std::sin(climb));
const Vector up(-std::sin(climb), 0.0, std::cos(climb));
const Point foot(0.0, 12000.0, 0.0);
ReflectionFold fold(
 ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 600.0),
 along,
 Vector(0.0, -1.0, 0.0),
 30000.0,
 {{6000.0, -76.0 * Tolerance::TO_RADIANS, true}},
 Plane::from_point_normal(Point(20000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
 120.0
);
```

##### Two-hinged frame {#two_hinged_frame}

A low wide hall: 6 m columns bulging inward, reflected 76 degrees at the knee onto a roof of troughs rising 14 degrees to the ridge, the plane of symmetry 20 m from the columns; span 40 m, ridge 11 m (example `templates_curved_folding_13_two_hinged_frame`).

![two-hinged frame](templates/templates_curved_folding_13_two_hinged_frame_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 600.0), along, Vector(0.0, -1.0, 0.0), 30000.0, {{6000.0, -76.0 * Tolerance::TO_RADIANS, true}}, Plane::from_point_normal(Point(20000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

<details><summary>Example code: templates_curved_folding_13_two_hinged_frame.cpp</summary>

\include{lineno} templates_curved_folding_13_two_hinged_frame.cpp

</details>

##### Two-hinged arch {#two_hinged_arch}

Deep barrels of 2400 rise from hinged feet as legs leaning 15 degrees outward, mirror-folded twice by 50 degrees into a tall polygonal arch with a flat crown, every segment a vault, the capsules of the end elevation; span 22 m, crown 20 m (example `templates_curved_folding_15_two_hinged_arch`).

![two-hinged arch](templates/templates_curved_folding_15_two_hinged_arch_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 2400.0), along, Vector(0.0, -1.0, 0.0), 40000.0, {{11000.0, -50.0 * Tolerance::TO_RADIANS, false}, {22000.0, -50.0 * Tolerance::TO_RADIANS, false}}, Plane::from_point_normal(Point(11000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

<details><summary>Example code: templates_curved_folding_15_two_hinged_arch.cpp</summary>

\include{lineno} templates_curved_folding_15_two_hinged_arch.cpp

</details>

##### Arch with top hinge {#arch_with_top_hinge}

The columns are the sheet itself: four barrels of 1600 depth stand on the ground as a vertical leg and are reflected 15 degrees at the eaves onto a roof rising 75 degrees. The reflection plane runs through a station 300 mm up the ruling at the barrels' cusps, and on the bulging legs it crosses the middle of every barrel 12 m higher than its cusps, so every crease is a tall arch and every leg a column rising under its trough to the crease, the roof's cusps coming down to the ground between the legs (the hatched notches of the plan) and the roof hanging as troughs between the cusps (the wine glasses of the end elevation). The steep roof, turned over by the reflection into a vault, is reflected again 40 degrees onto a shoulder at 35 degrees, a trough, and mirror-folded 35 degrees onto the level crown, which keeps the trough; its rulings cross the plane of symmetry square, so every crown plate is one piece across it. Span 26 m, crown 21.7 m (example `templates_curved_folding_16_arch_with_top_hinge`).

![arch with top hinge](templates/templates_curved_folding_16_arch_with_top_hinge_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1600.0), along, Vector(0.0, -1.0, 0.0), 30000.0, {{300.0, -15.0 * Tolerance::TO_RADIANS, true}, {18300.0, -40.0 * Tolerance::TO_RADIANS, true}, {25300.0, -35.0 * Tolerance::TO_RADIANS, false}}, Plane::from_point_normal(Point(13000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

<details><summary>Example code: templates_curved_folding_16_arch_with_top_hinge.cpp</summary>

\include{lineno} templates_curved_folding_16_arch_with_top_hinge.cpp

</details>

##### Two-hinged A-frame {#a_frame}

Deep barrels from splayed feet, reflected 60 degrees at the knee into two steep legs of troughs hanging between the cusps that run up to the apex, the plane of symmetry; span 25 m, apex 22.4 m (example `templates_curved_folding_17_a_frame`).

![two-hinged a-frame](templates/templates_curved_folding_17_a_frame_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1400.0), along, Vector(0.0, -1.0, 0.0), 40000.0, {{5600.0, -60.0 * Tolerance::TO_RADIANS, true}}, Plane::from_point_normal(Point(12500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

<details><summary>Example code: templates_curved_folding_17_a_frame.cpp</summary>

\include{lineno} templates_curved_folding_17_a_frame.cpp

</details>

### Cylinder vault {#cylinder_vault}

Equal cylinders over a regular polygon, one segment per side, meeting their neighbours in the vertical planes through the corners and the centre (`curved_folding/cylinder_vault.h`). Two equal cylinders mirrored in a plane meet in that plane, so every crease is a planar ellipse arc. `CylinderVault(sides, edge, radius, kind, wall, thickness)`:

| kind | generatrices | profile | creases |
| --- | --- | --- | --- |
| `Groin` | level, from the side to the centre | the arch over the side, springing at the corners; the crown at its top | half ellipses from the corners to the crown |
| `Cloister` | level, along the side, on a `wall` | the arc from the wall's top to the peak over the centre | quarter ellipses from the corners to the peak |

Every segment is one `PlateCurved`, `lamella_<segment>`: its outer face the exact cylinder between the two vertical planes its generatrices run between, its inner face the cylinder `thickness` smaller cut by the same planes, so two panels at a crease share their side face. Each panel unfolds flat (`add_flat()`), and `compute_face_contacts(0)` finds one contact per crease.

#### Groin vault {#groin_vault}

Two half barrels of 12 m span crossing over a square (example `templates_curved_folding_5_groin_vault`).

![groin vault](templates/templates_curved_folding_5_groin_vault_lamellas.webp)

```cpp
CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Groin);
```

<details><summary>Example code: templates_curved_folding_5_groin_vault.cpp</summary>

\include{lineno} templates_curved_folding_5_groin_vault.cpp

</details>

#### Pavilion vault on walls {#pavilion_vault}

Four cylinder segments on 4 m walls around a 12 m square, rising to the peak over the centre (example `templates_curved_folding_8_pavilion_vault`).

![pavilion vault on walls](templates/templates_curved_folding_8_pavilion_vault_lamellas.webp)

```cpp
CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Cloister, 4000.0);
```

<details><summary>Example code: templates_curved_folding_8_pavilion_vault.cpp</summary>

\include{lineno} templates_curved_folding_8_pavilion_vault.cpp

</details>

#### Pentagon vault {#pentagon_vault}

Five half barrels of 8 m span (example `templates_curved_folding_9_pentagon_vault`).

![pentagon vault](templates/templates_curved_folding_9_pentagon_vault_lamellas.webp)

```cpp
CylinderVault vault(5, 8000.0, 4000.0, CylinderVault::Kind::Groin);
```

<details><summary>Example code: templates_curved_folding_9_pentagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_9_pentagon_vault.cpp

</details>

#### Hexagon vault {#hexagon_vault}

Six half barrels of 8 m span (example `templates_curved_folding_11_hexagon_vault`).

![hexagon vault](templates/templates_curved_folding_11_hexagon_vault_lamellas.webp)

```cpp
CylinderVault vault(6, 8000.0, 4000.0, CylinderVault::Kind::Groin);
```

<details><summary>Example code: templates_curved_folding_11_hexagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_11_hexagon_vault.cpp

</details>

#### Octagon vault {#octagon_vault}

Eight half barrels of 6 m span (example `templates_curved_folding_10_octagon_vault`).

![octagon vault](templates/templates_curved_folding_10_octagon_vault_lamellas.webp)

```cpp
CylinderVault vault(8, 6000.0, 3000.0, CylinderVault::Kind::Groin);
```

<details><summary>Example code: templates_curved_folding_10_octagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_10_octagon_vault.cpp

</details>

### Curved-folded beam {#curved_folded_beam}

`CurvedFoldedBeam(span, width, rise, sag, thickness, webs, count)` (`curved_folding/curved_folded_beam.h`): a closed box spanning along x, `width` wide and half as high at the supports, of four cylinder plates with their rulings straight across. The top and bottom bulge outward by `rise` toward mid-span, the sides sag inward by `sag`, so the flat section at the supports deepens to mid-span. Every corner is a curved crease, the two plates mitred on the ruled face between the crease and the inner crease. `webs` flat webs across the span divide the box, each filling its inner section, its edges on the four plates' inner faces.

Six beams 12 m long, 800 by 400 mm at the supports, 640 by 900 mm at mid-span, 27 mm plates and five webs each: 54 plates, 144 contacts, the mitres and the webs' edges (example `templates_curved_folding_31_curved_folded_beam`).

![curved-folded beam](templates/templates_curved_folding_31_curved_folded_beam_all.webp)

```cpp
CurvedFoldedBeam beams;
```

<details><summary>Example code: templates_curved_folding_31_curved_folded_beam.cpp</summary>

\include{lineno} templates_curved_folding_31_curved_folded_beam.cpp

</details>

### Corner fan {#corner_fan}

`CornerFan(size, height, strips, lift, drop, foot, thickness, corner)` (`curved_folding/corner_fan.h`): a fan inscribed in a `size` square, folded from one bottom corner to the square's two far sides, `strips` strips, half on each side. Every fold is a parabola from three control points laid out in plan, the corner, the middle and the end on the square, the end raised to `height`, every second one `drop` lower, and the middle lifted `lift` above the chord. All folds lifted alike, two neighbours differ by their ends' difference times the parameter, so every strip is a cylinder of straight rulings parallel to its top edge on the square, developable. Two strips meet at every fold on the bisector of their inner faces; every strip ends on the square's side and is cut at `foot` of the way up, both ends planar.

Six strips in a 6 m square, folds 4 m high, every second 800 mm lower, lifted 1200 mm, 40 mm plates: 6 strips, 5 contacts, one per fold (example `templates_curved_folding_32_corner_fan`).

![corner fan](templates/templates_curved_folding_32_corner_fan_strips.webp)

![corner fan, plan](templates/templates_curved_folding_32_corner_fan_plan.webp)

```cpp
CornerFan fan(6000.0, 4000.0, 6, 1200.0, 800.0, 0.15, 40.0);
```

<details><summary>Example code: templates_curved_folding_32_corner_fan.cpp</summary>

\include{lineno} templates_curved_folding_32_corner_fan.cpp

</details>

#### Subdivisions {#corner_fan_subdivisions}

The same fan divided into 2, 4, 8 and 12 strips, each placed by its `corner` and grafted into one scene; every fan has one contact per fold (example `templates_curved_folding_33_corner_fan_subdivisions`).

![corner fan subdivisions](templates/templates_curved_folding_33_corner_fan_subdivisions_all.webp)

<details><summary>Example code: templates_curved_folding_33_corner_fan_subdivisions.cpp</summary>

\include{lineno} templates_curved_folding_33_corner_fan_subdivisions.cpp

</details>

### Cylinder carving {#cylinder_carving}

`CylinderCarving(stock, carvers, length, thickness, min_angle)` (`curved_folding/cylinder_carving.h`): a cylinder `stock` carved by any number of cylinders in sequence, ((a - b) - c) - d, every one of its own axis and radius. Only the carved patches are made: every carver's surface inside the stock and outside the other carvers, curving inward, one plate a `thickness` behind it in the stock. Every generator of a carver is clipped exactly, so every plate is a cylinder; two patches meet on the curve where their cylinders cross, both plates ending on the crossing of their grown cylinders, a patch's free edge is cut square at the stock's surface and its ends on the stock's end planes. Every boundary is cut where what ends it changes, so its corners stay sharp. The inputs are validated first: every carver must leave a patch, none may cut another's patch in two, and two patches must meet at `min_angle` or more, else the carving throws naming the carvers.

A stock of 2 m radius, 12 m long, carved by three cylinders of 1.5, 1.8 and 1.3 m radius, their axes 40 degrees apart over its top, cutting 700 mm deep and turned a few degrees: a - b, (a - b) - c and ((a - b) - c) - d side by side (example `templates_curved_folding_34_cylinder_carving`).

![cylinder carving](templates/templates_curved_folding_34_cylinder_carving_all.webp)

<details><summary>Example code: templates_curved_folding_34_cylinder_carving.cpp</summary>

\include{lineno} templates_curved_folding_34_cylinder_carving.cpp

</details>

#### Closed rings {#cylinder_carving_rings}

The stock carved all round by 4 cylinders of 2 m radius cutting 1100 mm deep, by 6 of 1.6 m cutting 800 mm and by 8 of 1.2 m cutting 500 mm, every axis turned off the stock's one way and the other in turn: every patch meets both neighbours, the plates close round the stock into a fluted tube, one contact per crease (example `templates_curved_folding_35_cylinder_carving_rings`).

![cylinder carving rings](templates/templates_curved_folding_35_cylinder_carving_rings_all.webp)

<details><summary>Example code: templates_curved_folding_35_cylinder_carving_rings.cpp</summary>

\include{lineno} templates_curved_folding_35_cylinder_carving_rings.cpp

</details>

## Developable folding {#developable_folding}

Templates whose plates are other developables, cones and the strips of rulings leaving a curved crease.

### Curved fold {#curved_fold_roof}

A folded tent roof: a freestanding folded shell of 32 m diameter rising into a tower 26 m high with a round top of 7 m, its valleys running down to concrete columns, its ridges ending in a gable over the eaves between them (`curved_folding/curved_fold.h`, example `templates_curved_folding_1_curved_fold`). Every ridge and valley is a smooth NURBS curve, a meridian interpolated through rings, each ring the same star of ridge and valley points scaled by the trumpet curve's radius, flat at the eaves and vertical at the tower, and lifted to its height. Every fold strip between two neighbouring meridians is one lamella, a [`PlateCurved`](@ref elements_plate_curved) in the group `lamellas`, named `lamella_<j>`: a closed curved solid of six exact NURBS patches, with no facet and no triangle, as the 5 cm sheathing on the uniaxially curved glulam of the built roof. Its outer face is the ruled surface between the two meridians, its inner face the one between the inner meridians, where the two neighbouring strips offset by the thickness meet; its side faces run along the folds, so two neighbours share their side face as mitered planar plates do, and a cap closes it at the eaves and at the top. Unlike the templates below, its strips are ruled between interpolated meridians, not cylinders: the free-form reference the exact constructions are measured against.

```cpp
CurvedFold fold(
 12, // sectors, a ridge and a valley each
 16000.0, // eaves' radius, 32 m diameter
 3500.0, // tower's top radius
 4000.0, // valleys at the eaves, on the columns
 26000.0, // tower's top
 4000.0, // gable: a ridge over its valleys at the eaves
 0.1, // valleys' inset
 14, // rings the meridians are interpolated through
 120.0 // thickness
);
```

24 lamellas, 24 contacts, one on every fold.

![curved fold](templates/templates_curved_folding_1_curved_fold_lamellas.webp)

<details><summary>Example code: templates_curved_folding_1_curved_fold.cpp</summary>

\include{lineno} templates_curved_folding_1_curved_fold.cpp

</details>

### Curved-crease unfolding {#curved_crease_unfolding}

A mechanism, not a fixed shape: flat plates cut as bands between two curved edges are stacked and hinged to their neighbours along those edges, one plate's edge on the top of the stack and the next's at the bottom. Pulled apart like an accordion the stack can only open if every plate bends about its rulings, because a curved hinge cannot stay flat while the plates on either side of it turn: the curvature of the plan becomes the curvature of the profile. Each crease stays in its plane and keeps its length, its curvature in space is the cut edge's curvature over the cosine of the opening (cos a = k2D / k3D), the rulings leave it perpendicular to it at the opening to the crease plane (a circular crease gives exact cones), and each plate is the mirror image of the one before it in the plane of their shared crease. The hinges alternate between the two faces of the stack, the axis shift of width sin a + thickness cos a between the crease planes, so the plates fold completely closed into a stack of count times thickness. A straight edge would only hinge.

The template draws the open state: the given crease lies on one of the `planes`; the plate after it is the strip of rulings leaving it square to it at the `opening`, toward its concave side, cut by the next plane, the plate before it its mirror image, and every further plate the mirror image of its neighbour in their shared crease plane. Every crease stays planar and every plate is a developable cut from a flat band, a `PlateCurved` mitred on the two planes of its creases, so it shares one face with each neighbour; the session's contact detection finds every one. On parallel planes the stack folds flat; with inclined planes it closes only by bending its rulings.

| factory | planes | figure |
| --- | --- | --- |
| `zigzag(origin, along, axis, count, spacing, tilt)` | `spacing` apart, every second one turned by `tilt` and `-tilt` in turn about `axis` through its point, so the mirror images' turns cancel and the vault runs straight; with `tilt` 0 parallel, the unfolding on parallel planes | 7-9; 11, 12a |
| `leaning(origin, along, axis, count, spacing, lean)` | standing on the floor `spacing` apart, the i-th from the middle turned by i `lean` about the span: drawn in at the apex (synclastic) or spread (anticlastic) | 10b, 10c |
| `fan(origin, axis, first, count, angle)` | through one axis, `angle` apart: around a vertical axis a dome or a funnel | 12b, 12c |

Two mirrors compose to a turn about the line where their planes meet, so planes that lean about the span lift the supports toward the ends (`drift()`); vertical planes keep them on the floor. Each example shows the principle on four plates, three creases and three contacts.

#### Arch {#curved_crease_arch}

Four 40 mm plates on five parallel planes 1000 mm apart, the crease a semicircle 9 m wide and 4.5 m high, opened 50 degrees; every plate an exact cone (example `templates_curved_folding_20_curved_crease_unfolding`).

![arch](templates/templates_curved_folding_20_curved_crease_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0), 40.0);
```

<details><summary>Example code: templates_curved_folding_20_curved_crease_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_20_curved_crease_unfolding.cpp

</details>

#### Catenary {#curved_crease_catenary}

Five parallel planes 1000 mm apart, the crease a catenary 10 m wide and 6 m high (`CurvedCreaseUnfolding::catenary`), opened 45 degrees (example `templates_curved_folding_21_catenary_unfolding`).

![catenary](templates/templates_curved_folding_21_catenary_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(CurvedCreaseUnfolding::catenary(10000.0, 6000.0), 45.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0), 40.0);
```

<details><summary>Example code: templates_curved_folding_21_catenary_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_21_catenary_unfolding.cpp

</details>

#### Synclastic vault {#synclastic_unfolding}

Five planes 1000 mm apart on the floor, each leaning 5 degrees more toward the middle: the apex drawn in (example `templates_curved_folding_23_synclastic_unfolding`).

![synclastic vault](templates/templates_curved_folding_23_synclastic_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 5.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_23_synclastic_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_23_synclastic_unfolding.cpp

</details>

#### Anticlastic vault {#anticlastic_unfolding}

The planes leaning 5 degrees more away from the middle: the apex spread, a saddle (example `templates_curved_folding_24_anticlastic_unfolding`).

![anticlastic vault](templates/templates_curved_folding_24_anticlastic_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, -5.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_24_anticlastic_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_24_anticlastic_unfolding.cpp

</details>

#### Inclined about the span at the apex {#apex_level_unfolding}

Every second plane turned 8 degrees, alternately each way, about a line along the span at the apex: the apex spacing kept, the fold articulated at the feet (example `templates_curved_folding_25_apex_level_unfolding`).

![inclined about the span at the apex](templates/templates_curved_folding_25_apex_level_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 4500.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_25_apex_level_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_25_apex_level_unfolding.cpp

</details>

#### Inclined about the span at the floor {#support_level_unfolding}

The same turned about a line on the floor: the supports kept, the fold articulated at the crown (example `templates_curved_folding_26_support_level_unfolding`).

![inclined about the span at the floor](templates/templates_curved_folding_26_support_level_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_26_support_level_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_26_support_level_unfolding.cpp

</details>

#### Inclined about the span at half height {#alternating_unfolding}

Every second plane turned 8 degrees, alternately each way, about a line at half height: each plate wide at one end and narrow at the other (example `templates_curved_folding_27_alternating_unfolding`).

![inclined about the span at half height](templates/templates_curved_folding_27_alternating_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 2250.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_27_alternating_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_27_alternating_unfolding.cpp

</details>

#### Inclined about a vertical axis {#sideways_unfolding}

Every second plane turned 8 degrees, alternately each way, about a vertical axis through the crown: the plane distance shrinks toward one foot and grows toward the other (example `templates_curved_folding_28_sideways_unfolding`).

![inclined about a vertical axis](templates/templates_curved_folding_28_sideways_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_28_sideways_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_28_sideways_unfolding.cpp

</details>

#### Corrugated dome {#corrugated_dome}

A meridian arc of a 6 m sphere up to 80 degrees, 25 meridian planes 15 degrees apart, the rulings at 60 degrees toward the centre: ridges the arc turned by 30 degrees, valleys its mirror images, a star at the base and an oculus of 2.1 m; 24 contacts (example `templates_curved_folding_29_corrugated_dome`).

![corrugated dome](templates/templates_curved_folding_29_corrugated_dome_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(6000.0, 0.0, 0.0), Point(6000.0 * std::cos(0.7), 0.0, 6000.0 * std::sin(0.7)), Point(6000.0 * std::cos(1.4), 0.0, 6000.0 * std::sin(1.4))), 60.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 24, 15.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_29_corrugated_dome.cpp</summary>

\include{lineno} templates_curved_folding_29_corrugated_dome.cpp

</details>

#### Corrugated funnel {#corrugated_funnel}

A quarter circle of 5 m radius flaring from a stem 0.8 m off the axis to a rim 5.8 m out and 5 m up, 33 meridian planes 11.25 degrees apart, the rulings at 75 degrees: a funnel column with a star at the rim; 32 contacts (example `templates_curved_folding_30_corrugated_funnel`).

![corrugated funnel](templates/templates_curved_folding_30_corrugated_funnel_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(800.0, 0.0, 0.0), Point(5800.0 - 5000.0 * std::cos(0.25 * Tolerance::PI), 0.0, 5000.0 * std::sin(0.25 * Tolerance::PI)), Point(5800.0, 0.0, 5000.0)), 75.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 32, 11.25 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_30_corrugated_funnel.cpp</summary>

\include{lineno} templates_curved_folding_30_corrugated_funnel.cpp

</details>

### Four quadrants of a crease {#crease_quadrants}

`CreaseQuadrants` (`curved_folding/crease_quadrants.h`, the four ways two strips join on one planar curved crease): a developable through a planar curved crease has its rulings r on one side of the crease plane and -r on the other, and their mirror images Mr and -Mr. Two strips joined on the crease take one ruling each, which gives the four ways a curved crease can join two plates: folded from one sheet or unfolded from two mirror sheets.

| quadrant | rulings | kind | at the crease |
| --- | --- | --- | --- |
| Q1 | r, -Mr | folding: one sheet, both strips in front of the plane | convex for one strip, concave for the other; sector angles summing to 360, developable |
| Q2 | r, Mr | unfolding: two mirror sheets | convex for both: synclastic, sector angles summing to less than 360 |
| Q3 | Mr, -r | folding the other way, both strips behind the plane | as Q1 |
| Q4 | -r, -Mr | unfolding | concave for both: anticlastic, sector angles summing to more than 360 |

Every unfolding of this page alternates Q2 and Q4 creases. A folding is mitred on the plane through the crease square to it, an unfolding on the crease plane.

#### Q1 to Q4 on one arc {#crease_quadrants_example}

An arc 4 m wide and 1.5 m high, strips 1200 mm wide and 40 mm thick, the rulings 40 degrees off the crease plane; from the left Q1, Q2, Q3, Q4; 8 strips, 4 contacts, one on each crease (example `templates_curved_folding_22_crease_quadrants`).

![q1 to q4 on one arc](templates/templates_curved_folding_22_crease_quadrants_all.webp)

```cpp
CreaseQuadrants quadrants(Primitives::arc(Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)), 40.0 * Tolerance::TO_RADIANS, 1200.0, 40.0);
```

<details><summary>Example code: templates_curved_folding_22_crease_quadrants.cpp</summary>

\include{lineno} templates_curved_folding_22_crease_quadrants.cpp

</details>

## Not built {#curved_folding_not_built}

segments with tilted generatrices over a corner (the diagonal vault's generatrices sloping or rising) break the mirror symmetry of the midline planes, so their creases are not planar and they are not made. A cone reflected in planes across its axis, concentric circular creases, a corrugated round roof of conical lamellas, is the one developable base besides the cylinder the reflection would take exactly, and is left for a cone variant of the fold.
