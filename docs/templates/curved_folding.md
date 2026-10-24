# Curved folding {#templates_curved_folding}

[TOC]

Folded structures whose creases are curves: every lamella is a curved plate, a [`PlateCurved`](@ref elements_plate_curved), the strip of a developable surface between two curves, bent from a flat panel about one axis and never twisted, as glulam and curved cross-laminated timber are made. Three constructions, each a `WoodSession` whose constructor is its recipe, one group per step, and every picture below one group of the example's scene:

| template | header | source | examples |
| --- | --- | --- | --- |
| [`CurvedFold`](#curved_fold_zauberhut) | `curved_folding/curved_fold.h` | Knie's Zauberhut, Blumer Lehmann 2020: a tent roof of curved folds, the meridians interpolated | `templates_curved_folding_1_curved_fold` |
| [`ReflectionFold`](#reflection_fold) | `curved_folding/reflection_fold.h` | Mitani and Igarashi, *Interactive Design of Planar Curved Folding by Reflection*, Pacific Graphics 2011; Huffman 1976; Buri, Stotz and Weinand, *Curved Folded Plate Timber Structures*, IABSE-IASS 2011; Engel, *Structure Systems* pp. 242-244 | `templates_curved_folding_{2_reflection_fold,3_curved_folded_plate,14_two_hinged_frame,...,19_cantilever}` |
| [`CylinderVault`](#cylinder_vault) | `curved_folding/cylinder_vault.h` | Engel pp. 238-241, structure systems through interpenetration of cylindrical surfaces and their composition over a square grid | `templates_curved_folding_{4_groin_vault,...,13_cross_vault,20_vault_field}` |
| [`Development`](#development) | `curved_folding/development.h` | the unfolding of a ruled strip, the check that every lamella is cut flat | every example prints it |

Every example prints the number of lamellas, how many are closed solids, and the development distortion and ruling twist of the worst lamella; on every model below both are at the rounding floor (1e-15 and 1e-8), the lamellas being exact cylinders.

## Why cylinders {#why_cylinders}

A curved crease between two smooth developable patches is planar when the two patches are each other's mirror image in the crease's plane (Huffman 1976: the generating lines on both sides make equal angles with that plane). Mitani and Igarashi make this the construction: cut a developable sheet by a plane and mirror the part beyond it; the crease is the plane's section of the sheet and the sheet still unfolds flat, because the mirrored part keeps its development. The same fact makes Engel's interpenetrating cylinders curved foldings: two equal cylinders mirrored in a vertical plane meet in that plane along an ellipse, straight in plan and curved in elevation.

For timber the cylinder is the surface to use. Its generatrices are straight and parallel, so a lamella between two of them is bent about one axis only, from a flat panel whose outline is the lamella's development; the circle is an exact rational NURBS arc, its sections by planes are exact ellipse arcs (the arc projected along the generatrices onto the plane, an affine map that keeps the arc's weights), and the ruled surface between two such sections, joining the points of equal parameter, is the exact cylinder, since both are projections of the same arc along the same direction. A `PlateCurved` built on them is therefore an exact solid: its outer and inner faces the two coaxial cylinders, its side faces the planar sections (shared with the neighbour across the crease, which is their mirror image), its caps the generatrices (shared with the next lamella along the segment).

## Development {#development}

`Development(a, b, samples)` unfolds the ruled strip between two curves into the plane, ruling by ruling: every quad between two rulings is split along one diagonal into two triangles laid flat with their edge lengths kept. On a developable strip, a cylinder or a cone, the quads are planar, so the other diagonal keeps its length too: its largest relative change is the `distortion`, the error of cutting the lamella from a flat panel, 0 for an exact cylinder. The `twist` measures the same on the smooth strip: the sine of the angle by which a ruling leaves the plane of the two curves' tangents. The `outline` is the flat cutting outline, drawn in every model's group `flat` beside it, every lamella where it lies on the unfolded sheet or the unrolled segment.

```cpp
const Development development(lamella->outer_a, lamella->outer_b, 32);
development.distortion;   // 0 on a cylinder, the relative length error of a twisted strip
development.outline;      // the flat outline in the xy plane, the first ruling on the y axis
```

## Curved fold {#curved_fold_zauberhut}

The folded tent roof of Knie's Zauberhut in Rapperswil (Blumer Lehmann with Carlos Martinez Architekten, 2020; [project page](https://www.blumer-lehmann.com/bauprojekte-realisieren/bauen/free-form/knies-zauberhut.html)): a freestanding folded shell of 32 m diameter rising into a tower 26 m high with a round top of 7 m, its valleys running down to concrete columns, its ridges ending in a gable over the eaves between them (`curved_folding/curved_fold.h`, example `templates_curved_folding_1_curved_fold`). Every ridge and valley is a smooth NURBS curve, a meridian interpolated through rings, each ring the same star of ridge and valley points scaled by the trumpet curve's radius, flat at the eaves and vertical at the tower, and lifted to its height. Every fold strip between two neighbouring meridians is one lamella, a [`PlateCurved`](@ref elements_plate_curved) in the group `lamellas`, named `lamella_<j>`: a closed curved solid of six exact NURBS patches, with no facet and no triangle, as the 5 cm sheathing on the uniaxially curved glulam of the built roof. Its outer face is the ruled surface between the two meridians, its inner face the one between the inner meridians, where the two neighbouring strips offset by the thickness meet; its side faces run along the folds, so two neighbours share their side face as mitered planar plates do, and a cap closes it at the eaves and at the top. Unlike the templates below, its strips are ruled between interpolated meridians, not cylinders: the free-form reference the exact constructions are measured against.

```cpp
CurvedFold fold(
    12,        // sectors, a ridge and a valley each
    16000.0,   // eaves' radius, 32 m diameter
    3500.0,    // tower's top radius
    4000.0,    // valleys at the eaves, on the columns
    26000.0,   // tower's top
    4000.0,    // gable: a ridge over its valleys at the eaves
    0.1,       // valleys' inset
    14,        // rings the meridians are interpolated through
    120.0      // thickness
);
```

![curved fold](templates/templates_curved_folding_1_curved_fold_lamellas.webp)

![curved fold elevation](templates/templates_curved_folding_1_curved_fold_elevation.webp)

<details><summary>Example code: templates_curved_folding_1_curved_fold.cpp</summary>

\include{lineno} templates_curved_folding_1_curved_fold.cpp

</details>

## Reflection fold {#reflection_fold}

Mitani and Igarashi's sequential reflection (`curved_folding/reflection_fold.h`). The sheet is a generalized cylinder: a profile, planar curves one after another sharing their ends (`profile`), swept `length` along the unit `direction`. Mirror `planes` are given on the unfolded sheet, in order along the rulings; plane j folds the part of the sheet beyond it, as it lies after the folds before it, so the fold of that part is the composition of the reflections 1 to j, and every crease is the plane's section of the sheet, a planar curve. An optional `symmetry` plane makes the other planar crease, a frame's ridge: the sheet ends on the plane and the folded half is mirrored in it, the two halves meeting along the plane's section of the sheet, straight in plan (the reflection beyond a plane turns the sheet over and gives the oval creases of Engel's plans; the symmetry gives his straight ridge lines). The constructor is the recipe:

```cpp
    // cuts: the sheet's end planes with the mirror planes between them, every one across all the rulings, in order
    const std::vector<Plane> cuts = compute_cuts();

    // folds: the sheet beyond mirror plane j carried by the reflections 1 to j, Mitani's sequential reflection
    const std::vector<Xform> folds = compute_folds();

    // offsets: the profile the thickness under the sheet and over it, the plates' inner faces on the unfolded sheet, since every reflection swaps the sheet's faces
    const std::vector<NurbsCurve> under = compute_offset(-1.0);
    const std::vector<NurbsCurve> over = compute_offset(1.0);

    // sheet, creases, lamellas, flat, and the mirrored half when there is a symmetry plane
```

| step | what it checks or makes |
| --- | --- |
| cuts | every plane cuts every ruling between the sheet's ends and after the plane before it; no mirror plane stands on the sheet, where its fold would be flat and the two sides would collide (a barrel vault cannot be turned in plan by a vertical mirror: at the crown the sheet is perpendicular to it) |
| folds | the reflection in a plane as an `Xform`, the composition for every part |
| offsets | the profile moved the thickness to each side, its ends mitred where two curves of the profile meet at a kink; a reflection turns the sheet over, so the physical underside of the part beyond fold j is the other side of the unfolded sheet |
| lamellas | between two cuts and two profile curves one `PlateCurved`, `lamella_<fold>_<row>`: its outer meridians the profile curve projected along the rulings onto the two cuts (exact), its inner meridians the offset cut flush by the sheet's end planes and, at a crease, by the fold's bisector surface: the surface through the crease perpendicular to the mirror plane, which the two plates at the crease lie on either side of, so they neither overlap nor leave a gap and each keeps its exact thickness |
| flat | the cutting pattern beside the model: every lamella's `Development` where it lies on the unfolded sheet, the rows along x and the rulings along y |

Two factories build the usual inputs: `ReflectionFold::turns(origin, direction, axis, stations, angles)` makes the planes that turn the folded sheet about an axis by given angles at given stations along the ruling through `origin` (a reflection reverses the sense of every later turn, so on the unfolded sheet the angles alternate their sign); `ReflectionFold::arcs(start, across, up, count, width, rise)` makes a profile of circular arcs side by side, Engel's rows of barrels, and `ReflectionFold::waves(...)` an undulating profile of half sine waves, Buri's corrugation.

A sheet of two shallow barrels folded at three planes containing the x direction, up 35 degrees, down 70 and up 35 again (example `templates_curved_folding_2_reflection_fold`):

```cpp
const Point origin(0.0, 0.0, 400.0);
const Vector along(0.0, 1.0, 0.0);
ReflectionFold fold(
    ReflectionFold::arcs(Point(-3000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), 2, 3000.0, 400.0),
    along,
    20000.0,
    ReflectionFold::turns(origin, along, Vector(1.0, 0.0, 0.0), {4000.0, 10000.0, 16000.0}, {35.0 * Tolerance::TO_RADIANS, -70.0 * Tolerance::TO_RADIANS, 35.0 * Tolerance::TO_RADIANS}),
    std::nullopt,
    120.0
);
```

![reflection fold](templates/templates_curved_folding_2_reflection_fold_lamellas.webp)

The unfolded sheet with its creases on the left, the folded lamellas, and the flat cutting pattern on the right:

![reflection fold, sheet and pattern](templates/templates_curved_folding_2_reflection_fold_all.webp)

<details><summary>Example code: templates_curved_folding_2_reflection_fold.cpp</summary>

\include{lineno} templates_curved_folding_2_reflection_fold.cpp

</details>

### Curved folded plate {#curved_folded_plate}

Buri, Stotz and Weinand's curved folded plate timber structures (IBOIS EPFL, IABSE-IASS 2011; the Mendrisio pavilion of 2013 spans 13.5 m with 77 mm curved cross-laminated timber). Buri's doubly corrugated surface is set by a corrugation profile across the span and a cross-section profile along it; with an undulating corrugation profile the surface is a cylinder, and every kink of the cross-section profile, a reverse fold, is a reflection of the sheet in the plane bisecting the kink. Here three sine waves of 3 m wavelength and 400 mm amplitude across a 9 m sheet are swept along a polygonal arch of five segments turning 30 degrees at each of four reverse folds, from 60 degrees up at one foot to 60 degrees down at the other, spanning 13.5 m and rising 4.9 m; every half wave of every segment is one lamella (example `templates_curved_folding_3_curved_folded_plate`).

```cpp
const double climb = 60.0 * Tolerance::TO_RADIANS;
const Vector along(0.0, std::cos(climb), std::sin(climb));
const Vector up(0.0, -std::sin(climb), std::cos(climb));
ReflectionFold fold(
    ReflectionFold::waves(Point(-4500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), up, 6, 1500.0, 400.0),
    along,
    18100.0,
    ReflectionFold::turns(Point(0.0, 0.0, 0.0), along, Vector(1.0, 0.0, 0.0), {3620.0, 7240.0, 10860.0, 14480.0}, {-30.0 * Tolerance::TO_RADIANS, -30.0 * Tolerance::TO_RADIANS, -30.0 * Tolerance::TO_RADIANS, -30.0 * Tolerance::TO_RADIANS}),
    std::nullopt,
    120.0
);
```

![curved folded plate](templates/templates_curved_folding_3_curved_folded_plate_lamellas.webp)

![curved folded plate, section](templates/templates_curved_folding_3_curved_folded_plate_section.webp)

![curved folded plate, sheet and pattern](templates/templates_curved_folding_3_curved_folded_plate_all.webp)

<details><summary>Example code: templates_curved_folding_3_curved_folded_plate.cpp</summary>

\include{lineno} templates_curved_folding_3_curved_folded_plate.cpp

</details>

### Folded cylindrical surfaces {#folded_cylinders}

Engel's linear structure systems composed of folded cylindrical surfaces (*Structure Systems* pp. 242-244): a row of four barrels, each 6 m wide, folded at the kinks of a frame's or an arch's section, read from his sections with the people at 1.8 m. Every system is the same `ReflectionFold` with the profile `arcs(...)` across y and the planes `turns(...)` of its section along x, the first segment's climb setting the ruling direction; a kink where the sheet continues is a reflection, its crease the bisecting plane's oblique section of the barrels (the ovals of Engel's plans), and a ridge or crown where the section is symmetric is the `symmetry` plane, its crease the plane's section, straight in plan (his straight ridge lines). The shallow barrels (600 high) are the frames' and the cantilever's, the deep ones (1400) the arches' and the A-frame's, as Engel's end elevations show.

| page | system | climb | stations along the rulings | turns | symmetry at x | example |
| --- | --- | --- | --- | --- | --- | --- |
| 242 | two-hinged frame | 90 (column 6 m) | 6000 | -76 | 20000 | `templates_curved_folding_14_two_hinged_frame` |
| 242 | three-hinged frame | 90 (column 6 m) | 6000, 13000, 22000, 31000, 38000 of 44000 | -62, -20, -16, -20, -62 | none | `templates_curved_folding_15_three_hinged_frame` |
| 243 | two-hinged arch | 105 (leg leaning out) | 7500, 16500 | -50, -50 | 13500 | `templates_curved_folding_16_two_hinged_arch` |
| 243 | arch with top hinge | 90 (column 14 m) | 14000, 22000 | -40, -40 | 13000 | `templates_curved_folding_17_arch_with_top_hinge` |
| 244 | two-hinged A-frame | 110 (foot leaning out) | 5600 | -60 | 12500 | `templates_curved_folding_18_a_frame` |
| 244 | cantilevered beam on central supports | -3 | 14000, 19500, 25000 of 39000 | -72, 150, -72 | none | `templates_curved_folding_19_cantilever` |

```cpp
const double climb = 90.0 * Tolerance::TO_RADIANS;
const Vector along(std::cos(climb), 0.0, std::sin(climb));
const Vector up(-std::sin(climb), 0.0, std::cos(climb));
const Point foot(0.0, 12000.0, 0.0);
ReflectionFold fold(
    ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 600.0),
    along,
    30000.0,
    ReflectionFold::turns(Point(0.0, 0.0, 0.0) + up * 600.0, along, Vector(0.0, -1.0, 0.0), {6000.0}, {-76.0 * Tolerance::TO_RADIANS}),
    Plane::from_point_normal(Point(20000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
    120.0
);
```

#### Two-hinged frame {#two_hinged_frame}

![two-hinged frame](templates/templates_curved_folding_14_two_hinged_frame_lamellas.webp)

![two-hinged frame, plan](templates/templates_curved_folding_14_two_hinged_frame_plan.webp)

![two-hinged frame, section](templates/templates_curved_folding_14_two_hinged_frame_section.webp)

<details><summary>Example code: templates_curved_folding_14_two_hinged_frame.cpp</summary>

\include{lineno} templates_curved_folding_14_two_hinged_frame.cpp

</details>

#### Three-hinged frame {#three_hinged_frame}

![three-hinged frame](templates/templates_curved_folding_15_three_hinged_frame_lamellas.webp)

![three-hinged frame, plan](templates/templates_curved_folding_15_three_hinged_frame_plan.webp)

![three-hinged frame, section](templates/templates_curved_folding_15_three_hinged_frame_section.webp)

<details><summary>Example code: templates_curved_folding_15_three_hinged_frame.cpp</summary>

\include{lineno} templates_curved_folding_15_three_hinged_frame.cpp

</details>

#### Two-hinged arch {#two_hinged_arch}

![two-hinged arch](templates/templates_curved_folding_16_two_hinged_arch_lamellas.webp)

![two-hinged arch, section](templates/templates_curved_folding_16_two_hinged_arch_section.webp)

<details><summary>Example code: templates_curved_folding_16_two_hinged_arch.cpp</summary>

\include{lineno} templates_curved_folding_16_two_hinged_arch.cpp

</details>

#### Arch with top hinge {#arch_with_top_hinge}

![arch with top hinge](templates/templates_curved_folding_17_arch_with_top_hinge_lamellas.webp)

![arch with top hinge, section](templates/templates_curved_folding_17_arch_with_top_hinge_section.webp)

<details><summary>Example code: templates_curved_folding_17_arch_with_top_hinge.cpp</summary>

\include{lineno} templates_curved_folding_17_arch_with_top_hinge.cpp

</details>

#### Two-hinged A-frame {#a_frame}

![A-frame](templates/templates_curved_folding_18_a_frame_lamellas.webp)

![A-frame, section](templates/templates_curved_folding_18_a_frame_section.webp)

<details><summary>Example code: templates_curved_folding_18_a_frame.cpp</summary>

\include{lineno} templates_curved_folding_18_a_frame.cpp

</details>

#### Cantilevered beam on central supports {#cantilever}

![cantilever](templates/templates_curved_folding_19_cantilever_lamellas.webp)

![cantilever, section](templates/templates_curved_folding_19_cantilever_section.webp)

<details><summary>Example code: templates_curved_folding_19_cantilever.cpp</summary>

\include{lineno} templates_curved_folding_19_cantilever.cpp

</details>

## Cylinder vault {#cylinder_vault}

Engel's structure systems through interpenetration of cylindrical surfaces (pp. 238-241): equal circular cylinders over a regular polygon, meeting their neighbours in the vertical planes of the polygon's symmetry (`curved_folding/cylinder_vault.h`). `CylinderVault(sides, edge, radius, rise, kind, lamellas, thickness, name, placements)` has three kinds, and stands once at the origin or, with `placements`, once per given centre and scale:

| kind | segment | generatrices | profile | creases |
| --- | --- | --- | --- | --- |
| `Groin` | one per side, over the triangle from the side to the centre | from the side toward the centre, tilted up by `rise` (negative: sloping) | the arch over the side, a circle of `radius` across the generatrices with its springings at the corners (a half circle when the radius is half the side) | the vertical planes from the corners to the centre |
| `Diagonal` | one per corner, over the kite between the two neighbouring sides' middles | across the corner's diagonal, horizontal | the arc of `radius` from the corner on the ground up to the crown above the centre | the vertical planes from the sides' middles to the centre |
| `Cloister` | one per side, over the triangle | along the side, horizontal | the arc of `radius` from the side on the ground up to the crown above the centre | the vertical planes from the corners to the centre |

The constructor is the recipe: `compute_segments()` places every cylinder (its circle, its generatrices and the vertical planes around its footprint); `add_lamellas()` cuts every segment along its generatrices into `lamellas` strips, plus one cut wherever a generatrix changes the plane it starts or ends on (found by clipping the generatrix against the footprint, so no lamella spans two planes), and draws the strips at the corners in to where their last generatrix is at least the thickness long, so every cap is a face; every strip is a `PlateCurved`, `lamella_<segment>_<k>`, its four meridians the outer and the inner arc projected along the generatrices onto the two planes; `add_flat()` lays every lamella's development in a row per segment beside the model.

```cpp
CylinderVault vault(
    4,                            // sides of the polygon
    12000.0,                      // side
    6000.0,                       // radius, a half circle over the side
    0.0,                          // rise of the generatrices toward the centre, radians
    CylinderVault::Kind::Groin,   // generatrices from the side toward the centre
    12                            // lamellas per segment
);
```

#### Groin vault {#groin_vault}

Engel p. 238, generatrices in one plane: two half barrels of 12 m span crossing over a square, meeting on the diagonals (example `templates_curved_folding_4_groin_vault`). Plan, model and the four segments' developments:

![groin vault](templates/templates_curved_folding_4_groin_vault_lamellas.webp)

![groin vault, plan](templates/templates_curved_folding_4_groin_vault_plan.webp)

![groin vault with its flat pattern](templates/templates_curved_folding_4_groin_vault_all.webp)

<details><summary>Example code: templates_curved_folding_4_groin_vault.cpp</summary>

\include{lineno} templates_curved_folding_4_groin_vault.cpp

</details>

#### Generatrices sloping toward the centre {#groin_vault_sloping}

Engel p. 238: the groin vault's generatrices tilted down 15 degrees toward the centre, the arches over the sides standing higher than the centre (example `templates_curved_folding_5_groin_vault_sloping`, `rise = -15.0 * Tolerance::TO_RADIANS`).

![groin vault sloping](templates/templates_curved_folding_5_groin_vault_sloping_lamellas.webp)

<details><summary>Example code: templates_curved_folding_5_groin_vault_sloping.cpp</summary>

\include{lineno} templates_curved_folding_5_groin_vault_sloping.cpp

</details>

#### Generatrices rising toward the centre {#groin_vault_rising}

Engel p. 238: the generatrices tilted up 15 degrees, a dome of four cylinders (example `templates_curved_folding_6_groin_vault_rising`, `rise = 15.0 * Tolerance::TO_RADIANS`).

![groin vault rising](templates/templates_curved_folding_6_groin_vault_rising_lamellas.webp)

<details><summary>Example code: templates_curved_folding_6_groin_vault_rising.cpp</summary>

\include{lineno} templates_curved_folding_6_groin_vault_rising.cpp

</details>

#### Diagonal vault {#diagonal_vault}

Engel p. 238, right column: a cylinder segment per corner with its generatrices across the corner's diagonal, the four meeting on the lines from the sides' middles to the centre; the sides are cut as pointed arches from the corners to the sides' middles (example `templates_curved_folding_7_diagonal_vault`, `CylinderVault(4, 12000.0, 9000.0, 0.0, CylinderVault::Kind::Diagonal, 12)`).

![diagonal vault](templates/templates_curved_folding_7_diagonal_vault_lamellas.webp)

![diagonal vault, plan](templates/templates_curved_folding_7_diagonal_vault_plan.webp)

<details><summary>Example code: templates_curved_folding_7_diagonal_vault.cpp</summary>

\include{lineno} templates_curved_folding_7_diagonal_vault.cpp

</details>

#### Cloister vault {#cloister_vault}

The counterpart of the groin vault, the two barrels kept where they are lower: a segment per side with its generatrices along the side, a quarter circle from the side up to the crown (example `templates_curved_folding_8_cloister_vault`, `CylinderVault(4, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Cloister, 12)`).

![cloister vault](templates/templates_curved_folding_8_cloister_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_8_cloister_vault.cpp</summary>

\include{lineno} templates_curved_folding_8_cloister_vault.cpp

</details>

#### Three cylindrical segments {#triangle_vault}

Engel p. 239, a hexagonal plan with three cylindrical segments: three half barrels, each from a side toward the centre, meeting on the lines from the corners; Engel draws them over a hexagon whose three other sides are the barrels' flanks, here over the triangle alone (example `templates_curved_folding_9_triangle_vault`, `CylinderVault(3, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Groin, 12)`).

![triangle vault](templates/templates_curved_folding_9_triangle_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_9_triangle_vault.cpp</summary>

\include{lineno} templates_curved_folding_9_triangle_vault.cpp

</details>

#### Pentagonal plan with segments rising toward the centre {#pentagon_vault}

Engel p. 239 (example `templates_curved_folding_10_pentagon_vault`, `CylinderVault(5, 8000.0, 4000.0, 20.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin, 10)`).

![pentagon vault](templates/templates_curved_folding_10_pentagon_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_10_pentagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_10_pentagon_vault.cpp

</details>

#### Octagonal plan with segments sloping toward the centre {#octagon_vault}

Engel p. 239 (example `templates_curved_folding_11_octagon_vault`, `CylinderVault(8, 6000.0, 3000.0, -15.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin, 8)`).

![octagon vault](templates/templates_curved_folding_11_octagon_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_11_octagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_11_octagon_vault.cpp

</details>

#### Hexagonal plan with rising cylinder segments {#hexagon_vault}

Engel p. 240 (example `templates_curved_folding_12_hexagon_vault`, `CylinderVault(6, 8000.0, 4000.0, 25.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin, 10)`).

![hexagon vault](templates/templates_curved_folding_12_hexagon_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_12_hexagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_12_hexagon_vault.cpp

</details>

#### Cross-shaped plan with upright segments {#cross_vault}

Engel p. 240: four half cylinders standing almost upright on the sides of a square, tilted 70 degrees toward the centre, so the vertical sides cut them in tall half ellipses, the lobes of the cross-shaped plan, and the four lean in to meet at the top (example `templates_curved_folding_13_cross_vault`, `CylinderVault(4, 8000.0, 4000.0, 70.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin, 10)`).

![cross vault](templates/templates_curved_folding_13_cross_vault_lamellas.webp)

![cross vault, plan](templates/templates_curved_folding_13_cross_vault_plan.webp)

<details><summary>Example code: templates_curved_folding_13_cross_vault.cpp</summary>

\include{lineno} templates_curved_folding_13_cross_vault.cpp

</details>

#### Composition over a square grid {#vault_field}

Engel p. 241, composition of cylindrical surfaces crossing diagonally over a square grid plan: a groin vault on every bay of a 6 by 4 grid of 6 m bays, and over the four bays in the middle one groin vault at twice the scale, standing up above the field (example `templates_curved_folding_20_vault_field`, `CylinderVault(4, 6000.0, 3000.0, 0.0, CylinderVault::Kind::Groin, 8, 120.0, "vault_field", CylinderVault::field(6, 4, 6000.0))`).

![vault field](templates/templates_curved_folding_20_vault_field_lamellas.webp)

![vault field, elevation](templates/templates_curved_folding_20_vault_field_elevation.webp)

<details><summary>Example code: templates_curved_folding_20_vault_field.cpp</summary>

\include{lineno} templates_curved_folding_20_vault_field.cpp

</details>

## Not built {#curved_folding_not_built}

The right column of p. 238 with tilted generatrices (the diagonal vault's generatrices sloping or rising) breaks the mirror symmetry of the midline planes, so its creases are not planar and it is not made. Huffman's cone reflected in planes across its axis (Albers' concentric circles), a corrugated round roof of conical lamellas, is the one developable base besides the cylinder the reflection would take exactly, and is left for a cone variant of the fold.
