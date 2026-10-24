# Curved folding {#templates_curved_folding}

[TOC]

Folded structures whose creases are curves: every lamella is a curved plate, a [`PlateCurved`](@ref elements_plate_curved), the strip of a developable surface between two curves, bent from a flat panel about one axis and never twisted, as glulam and curved cross-laminated timber are made. Three constructions, each a `WoodSession` whose constructor is its recipe, one group per step, and every picture below one group of the example's scene:

| template | header | source | examples |
| --- | --- | --- | --- |
| [`CurvedFold`](#curved_fold_zauberhut) | `curved_folding/curved_fold.h` | Knie's Zauberhut, Blumer Lehmann 2020: a tent roof of curved folds, the meridians interpolated | `templates_curved_folding_1_curved_fold` |
| [`ReflectionFold`](#reflection_fold) | `curved_folding/reflection_fold.h` | Mitani and Igarashi, *Interactive Design of Planar Curved Folding by Reflection*, Pacific Graphics 2011; Huffman 1976; Buri, Stotz and Weinand, *Curved Folded Plate Timber Structures*, IABSE-IASS 2011; Engel, *Structure Systems* pp. 242-244, reflections and mirror folds | `templates_curved_folding_{2_reflection_fold,3_curved_folded_plate,13_two_hinged_frame,...,18_cantilever}` |
| [`CylinderVault`](#cylinder_vault) | `curved_folding/cylinder_vault.h` | Engel pp. 238-241, structure systems through interpenetration of cylindrical surfaces and their composition over a square grid | `templates_curved_folding_{4_groin_vault,...,12_cross_vault,19_vault_field}` |
| [`CurvedCreaseUnfolding`](#curved_crease_unfolding) | `curved_folding/curved_crease_unfolding.h` | Scheder-Bieschin, Van Mele and Block, *Curved-Crease Flat-Foldable Bending-Active Plate Structures*, DMSB 2023; Tachi, *Composite Rigid-Foldable Curved Origami Structure*, Transformables 2013; Shimoda, Tachi and Sato 2020, the axis shift | `templates_curved_folding_{20_curved_crease_unfolding,21_catenary_unfolding}` |
| [`CreaseQuadrants`](#crease_quadrants) | `curved_folding/crease_quadrants.h` | Tachi 2013; Scheder-Bieschin, Van Mele and Block 2023, Figs. 2-3: the four ways two strips join on one curved crease | `templates_curved_folding_22_crease_quadrants` |
| [`InclinedCreaseUnfolding`](#inclined_crease_unfolding) | `curved_folding/inclined_crease_unfolding.h` | Scheder-Bieschin, Van Mele and Block 2023, Figs. 10-12: synclastic and anticlastic vaults, articulated vaults, corrugated dome and funnel | `templates_curved_folding_{23_synclastic_unfolding,...,30_corrugated_funnel}` |
| [`Development`](#development) | `curved_folding/development.h` | the unfolding of a ruled strip, the check that every lamella is cut flat | every example prints it |

Every example prints the number of lamellas or panels, how many are closed solids, and the development distortion and ruling twist of the worst one; on every model below both are at the rounding floor (1e-14 and 1e-8), the lamellas being exact cylinders.

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

A cylindrical sheet folded at planar creases (`curved_folding/reflection_fold.h`). The sheet is a profile, planar curves one after another sharing their ends (`profile`), swept `length` along the unit `direction`. Every `Fold` turns the rulings about the folds' `axis` by its `angle` at its `station` along the rulings, and the crease is the fold plane's section of the sheet, a planar curve, an ellipse arc when the profile is an arc. A fold is made in one of two ways:

| `Fold::reflected` | construction | the crease | the bulge | the development |
| --- | --- | --- | --- | --- |
| `true` | Mitani and Igarashi's reflection: the part of the sheet beyond the plane is mirrored in it, the plane bisecting the angle between the old rulings and the new | the plane's oblique section, the long oval of a sharp kink | flips: the sheet is turned over, a vault folds into a trough, as Engel's end elevations show at the knees | the whole sheet stays one development (Mitani's closure theorem) |
| `false` | Engel's mirror fold: the next segment is the mirror image of the one before in the plane bisecting the fold's angle | the bisecting plane's section | kept: a vault folds onto a vault, the capsules of Engel's two-hinged arch | every segment is cut flat on its own; the two sides of the crease develop as mirror images, not as one sheet |

An optional `symmetry` plane is the other planar crease, a frame's ridge or an arch's crown: the sheet ends on the plane and the folded half is mirrored in it, the two halves meeting along the plane's section, straight in plan (Engel's straight ridge lines). A column is the sheet too: a reflection whose plane crosses the first segment higher in the middle of a barrel than at its cusps (a reflection's crease runs `rise / tan(angle / 2)` further along the rulings at the bulge than at the cusps) leaves a leg under every barrel rising as an arch to the crease. The constructor is the recipe:

```cpp
    // segments: the sheet's straight runs between the folds, each with its rulings, its two planes and its underside
    const Plane first = compute_profile_plane();
    const std::vector<Segment> segments = compute_segments(first);

    // creases: every fold plane's section of the sheet, the profile carried along the rulings from plane to plane
    const std::vector<std::vector<NurbsCurve>> creases = compute_creases(segments);

    // offsets: every segment's first crease moved the thickness to the sheet's underside, the plates' inner faces
    const std::vector<std::vector<NurbsCurve>> offsets = compute_offsets(segments, creases);

    // creases drawn, lamellas, flat, and the mirrored half when there is a symmetry plane
```

| step | what it checks or makes |
| --- | --- |
| segments | the fold plane through the station on the ruling from the profile's first point, its normal `rulings - next` for a reflection and `rulings + next` for a mirror fold; the underside's sign flipped by every reflection; the end the symmetry plane or the profile's plane moved by `length` |
| creases | each crease the one before projected along its segment's rulings onto the next plane (exact: an affine map keeps the arcs' weights); every ruling must run forward from crease to crease, and a reflection's plane must not stand on the sheet, where its fold would be flat and the sheet would cross itself (a barrel cannot be turned in plan by a vertical mirror: at the crown the sheet is perpendicular to it) |
| offsets | the crease moved the thickness along the sheet's normal to the underside, its ends mitred where two curves of the profile meet at a kink |
| lamellas | between two creases and two profile curves one `PlateCurved`, `lamella_<segment>_<row>`: its outer meridians the two creases, its inner meridians the offset cut by the mitre, the crease plane itself at a mirror fold, a sheet end or the symmetry plane, and at a reflection the surface through the crease perpendicular to the plane, which the two plates lie on either side of with their exact thickness |
| flat | the cutting pattern beside the model: every lamella's `Development` where it lies on the unrolled sheet, the rows along x and the rulings along y |

Two factories build the usual profiles: `ReflectionFold::arcs(start, across, up, count, width, rise)`, circular arcs side by side, Engel's rows of barrels, and `ReflectionFold::waves(...)`, half sine waves, Buri's corrugation.

A sheet of two shallow barrels reflected at three planes containing the x direction, up 35 degrees, down 70 and up 35 again (example `templates_curved_folding_2_reflection_fold`):

```cpp
const Vector along(0.0, 1.0, 0.0);
ReflectionFold fold(
    ReflectionFold::arcs(Point(-3000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), 2, 3000.0, 400.0),
    along,
    Vector(1.0, 0.0, 0.0),
    20000.0,
    {{4000.0, 35.0 * Tolerance::TO_RADIANS}, {10000.0, -70.0 * Tolerance::TO_RADIANS}, {16000.0, 35.0 * Tolerance::TO_RADIANS}}
);
```

![reflection fold](templates/templates_curved_folding_2_reflection_fold_lamellas.webp)

The end elevation shows the bulge flipping at every reflection, and the flat cutting pattern lies beside the model:

![reflection fold, end](templates/templates_curved_folding_2_reflection_fold_end.webp)

![reflection fold and pattern](templates/templates_curved_folding_2_reflection_fold_all.webp)

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
    Vector(1.0, 0.0, 0.0),
    18100.0,
    {{3620.0, -30.0 * Tolerance::TO_RADIANS}, {7240.0, -30.0 * Tolerance::TO_RADIANS}, {10860.0, -30.0 * Tolerance::TO_RADIANS}, {14480.0, -30.0 * Tolerance::TO_RADIANS}}
);
```

![curved folded plate](templates/templates_curved_folding_3_curved_folded_plate_lamellas.webp)

![curved folded plate, section](templates/templates_curved_folding_3_curved_folded_plate_section.webp)

![curved folded plate and pattern](templates/templates_curved_folding_3_curved_folded_plate_all.webp)

<details><summary>Example code: templates_curved_folding_3_curved_folded_plate.cpp</summary>

\include{lineno} templates_curved_folding_3_curved_folded_plate.cpp

</details>

### Folded cylindrical surfaces {#folded_cylinders}

Engel's linear structure systems composed of folded cylindrical surfaces (*Structure Systems* pp. 242-244): a row of four barrels, each 6 m wide, folded at the kinks of a frame's or an arch's section, read from his sections, plans and end elevations with the people at 1.8 m. Every system is the same `ReflectionFold` with the profile `arcs(...)` across y and the folds along x, the first segment's climb setting the ruling direction. His end elevations say which kind every fold is: where the barrels flip from vaults (the columns, rounded arches at the feet) to troughs (the roof, hanging scallops at the top) the fold is a reflection; where the barrels stay rounded all the way, the two-hinged arch's capsules and the arch with top hinge's trough roof over its arched legs, the folds are mirror folds. A ridge or crown with a straight line in his plan is the `symmetry` plane, the ovals of his plans the sections of the barrels by the fold planes.

| page | system | climb | folds: station, turn, kind | symmetry at x | barrels | example |
| --- | --- | --- | --- | --- | --- | --- |
| 242 | two-hinged frame | 90, column 6 m | 6000, -76, reflection | 20000 | 600 high, inward | `templates_curved_folding_13_two_hinged_frame` |
| 242 | three-hinged frame | 90, column 6 m | 6000, -62; 14400, -23; 23400, -50, reflections | 17795, the hinge in the notch's bottom | 400 high, outward | `templates_curved_folding_14_three_hinged_frame` |
| 243 | two-hinged arch | 105, leg leaning out | 11000, -50; 22000, -50, mirror folds | 11000 | 2400 high | `templates_curved_folding_15_two_hinged_arch` |
| 243 | arch with top hinge | 90, legs | 300, -15; 18300, -40, reflections; 25300, -35, mirror fold | 13000 | 1600 deep | `templates_curved_folding_16_arch_with_top_hinge` |
| 244 | two-hinged A-frame | 110, foot leaning out | 5600, -60, reflection | 12500 | 1400 high | `templates_curved_folding_17_a_frame` |
| 244 | cantilevered beam on central supports | -3, roof 6.1 m up | 14000, -72; 19500, 150; 25000, -72, reflections | none | 600 deep troughs | `templates_curved_folding_18_cantilever` |

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
    Plane::from_point_normal(Point(20000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0))
);
```

#### Two-hinged frame {#two_hinged_frame}

A low wide hall: 6 m columns bulging inward, reflected 76 degrees at the knee onto a roof of troughs rising 14 degrees to the ridge, the plane of symmetry 20 m from the columns; span 40 m, ridge 11 m.

![two-hinged frame](templates/templates_curved_folding_13_two_hinged_frame_lamellas.webp)

![two-hinged frame, section](templates/templates_curved_folding_13_two_hinged_frame_section.webp)

![two-hinged frame, end](templates/templates_curved_folding_13_two_hinged_frame_end.webp)

![two-hinged frame, plan](templates/templates_curved_folding_13_two_hinged_frame_plan.webp)

<details><summary>Example code: templates_curved_folding_13_two_hinged_frame.cpp</summary>

\include{lineno} templates_curved_folding_13_two_hinged_frame.cpp

</details>

#### Three-hinged frame {#three_hinged_frame}

6 m columns, a roof of a steep slope and a shoulder per side, three reflections, and a notch at mid-span whose bottom, the hinge, is the plane of symmetry; the notch's slopes between the shoulder creases are the hatched ovals of Engel's plan, the shoulders' rounded tops the top of his end elevation; span 36 m, shoulders 10.7 m, hinge 9.3 m.

![three-hinged frame](templates/templates_curved_folding_14_three_hinged_frame_lamellas.webp)

![three-hinged frame, section](templates/templates_curved_folding_14_three_hinged_frame_section.webp)

![three-hinged frame, end](templates/templates_curved_folding_14_three_hinged_frame_end.webp)

![three-hinged frame, plan](templates/templates_curved_folding_14_three_hinged_frame_plan.webp)

<details><summary>Example code: templates_curved_folding_14_three_hinged_frame.cpp</summary>

\include{lineno} templates_curved_folding_14_three_hinged_frame.cpp

</details>

#### Two-hinged arch {#two_hinged_arch}

Deep barrels of 2400 rise from hinged feet as legs leaning 15 degrees outward, mirror-folded twice by 50 degrees into a tall polygonal arch with a flat crown, every segment a vault, the capsules of Engel's end elevation; span 22 m, crown 20 m.

![two-hinged arch](templates/templates_curved_folding_15_two_hinged_arch_lamellas.webp)

![two-hinged arch, section](templates/templates_curved_folding_15_two_hinged_arch_section.webp)

![two-hinged arch, end](templates/templates_curved_folding_15_two_hinged_arch_end.webp)

<details><summary>Example code: templates_curved_folding_15_two_hinged_arch.cpp</summary>

\include{lineno} templates_curved_folding_15_two_hinged_arch.cpp

</details>

#### Arch with top hinge {#arch_with_top_hinge}

The columns are the sheet itself: four barrels of 1600 depth stand on the ground as a vertical leg and are reflected 15 degrees at the eaves onto a roof rising 75 degrees. The reflection plane runs through a station 300 mm up the ruling at the barrels' cusps, and on the bulging legs it crosses the middle of every barrel 12 m higher than its cusps, so every crease is a tall arch and every leg a column rising under its trough to the crease, the roof's cusps coming down to the ground between the legs (the hatched notches of Engel's plan) and the roof hanging as troughs between the cusps (the wine glasses of his end elevation). The steep roof, turned over by the reflection into a vault, is reflected again 40 degrees onto a shoulder at 35 degrees, a trough, and mirror-folded 35 degrees onto the level crown, which keeps the trough: the cups of his end elevation; the hinge in the crown is the plane of symmetry. Span 26 m, crown 19.8 m.

![arch with top hinge](templates/templates_curved_folding_16_arch_with_top_hinge_lamellas.webp)

![arch with top hinge, section](templates/templates_curved_folding_16_arch_with_top_hinge_section.webp)

![arch with top hinge, end](templates/templates_curved_folding_16_arch_with_top_hinge_end.webp)

![arch with top hinge, plan](templates/templates_curved_folding_16_arch_with_top_hinge_plan.webp)

<details><summary>Example code: templates_curved_folding_16_arch_with_top_hinge.cpp</summary>

\include{lineno} templates_curved_folding_16_arch_with_top_hinge.cpp

</details>

#### Two-hinged A-frame {#a_frame}

Deep barrels from splayed feet, reflected 60 degrees at the knee into two steep legs of troughs hanging between the cusps that run up to the apex, the plane of symmetry; span 25 m, apex 22.4 m.

![A-frame](templates/templates_curved_folding_17_a_frame_lamellas.webp)

![A-frame, section](templates/templates_curved_folding_17_a_frame_section.webp)

![A-frame, end](templates/templates_curved_folding_17_a_frame_end.webp)

<details><summary>Example code: templates_curved_folding_17_a_frame.cpp</summary>

\include{lineno} templates_curved_folding_17_a_frame.cpp

</details>

#### Cantilevered beam on central supports {#cantilever}

A roof of four shallow barrels 39 m long, reflected down in the middle into a V of two legs, cantilevering 14 m both ways; the roof's two arms are a trough and a vault, every reflection turning the sheet over.

![cantilever](templates/templates_curved_folding_18_cantilever_lamellas.webp)

![cantilever, section](templates/templates_curved_folding_18_cantilever_section.webp)

![cantilever, end](templates/templates_curved_folding_18_cantilever_end.webp)

<details><summary>Example code: templates_curved_folding_18_cantilever.cpp</summary>

\include{lineno} templates_curved_folding_18_cantilever.cpp

</details>

## Cylinder vault {#cylinder_vault}

Engel's structure systems through interpenetration of cylindrical surfaces (pp. 238-241): equal circular cylinders over a regular polygon, one segment per side, meeting their neighbours in the vertical planes through the corners and the centre (`curved_folding/cylinder_vault.h`). `CylinderVault(sides, edge, radius, rise, kind, wall, thickness, name, placements)` has two kinds, and stands once at the origin or, with `placements`, once per given centre and scale:

| kind | segment | generatrices | profile | creases |
| --- | --- | --- | --- | --- |
| `Groin` | one per side, over the triangle from the side to the centre | from the side toward the centre, tilted up by `rise` (negative: sloping) | the arch over the side, a circle of `radius` across the generatrices with its springings at the corners (a half circle when the radius is half the side) | the vertical planes from the corners to the centre, two half ellipses meeting at the apex |
| `Cloister` | one per side, over the triangle, on a `wall` | along the side, horizontal | the arc of `radius` from the wall's top up to the peak over the centre | the vertical planes from the corners to the centre, quarter ellipses rising to the peak |

Every segment is **one panel**, a `PlateCurved` named `lamella_<segment>`: its outer face the exact cylinder between the two vertical planes its generatrices run between; its meridians one rational quadratic NURBS each, a span per plane with a C0 knot where the generatrices change from one plane to the next (`compute_meridian`: the arc projected along the generatrices span by span, the groin's crease two half ellipses meeting at the apex); its inner face the cylinder of the smaller radius, whose sections by the same planes are its inner meridians, so two panels across a crease share their side face; its caps the first and the last generatrix, drawn in from the corner or the apex to where they are a millimetre long (`compute_spans`), so the solid has no zero-length edge. The cloister vault's walls are flat panels `wall_<segment>` in the group `walls`. Every panel is bent about one axis and cut from a flat panel; `add_flat()` lays every panel's development in a row beside the model.

```cpp
CylinderVault vault(
    4,                            // sides of the polygon
    12000.0,                      // side
    6000.0,                       // radius, a half circle over the side
    0.0,                          // rise of the generatrices toward the centre, radians
    CylinderVault::Kind::Groin    // generatrices from the side toward the centre
);
```

#### Groin vault {#groin_vault}

Engel p. 238, generatrices in one plane: two half barrels of 12 m span crossing over a square, meeting on the diagonals, every quarter one panel (example `templates_curved_folding_4_groin_vault`). Model, plan and the four panels' developments:

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

#### Pavilion vault on walls {#pavilion_vault}

Engel p. 238, right column: the pavilion (cloister) vault, the two barrels kept where they are lower. On a 12 m square stand four walls 4 m high, and on every wall a cylinder segment whose generatrices run along the wall, a quarter circle of 6 m radius from the wall's top up to the peak 6 m higher over the centre; the four segments are mirror images in the diagonal planes and meet in them along quarter ellipses rising from the corners to the peak (example `templates_curved_folding_7_pavilion_vault`, `CylinderVault(4, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Cloister, 4000.0)`).

![pavilion vault](templates/templates_curved_folding_7_pavilion_vault_lamellas.webp)

![pavilion vault, plan](templates/templates_curved_folding_7_pavilion_vault_plan.webp)

<details><summary>Example code: templates_curved_folding_7_pavilion_vault.cpp</summary>

\include{lineno} templates_curved_folding_7_pavilion_vault.cpp

</details>

#### Three cylindrical segments {#triangle_vault}

Engel p. 239, a hexagonal plan with three cylindrical segments: three half barrels, each from a side toward the centre, meeting on the lines from the corners; Engel draws them over a hexagon whose three other sides are the barrels' flanks, here over the triangle alone (example `templates_curved_folding_8_triangle_vault`, `CylinderVault(3, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Groin)`).

![triangle vault](templates/templates_curved_folding_8_triangle_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_8_triangle_vault.cpp</summary>

\include{lineno} templates_curved_folding_8_triangle_vault.cpp

</details>

#### Pentagonal plan with segments rising toward the centre {#pentagon_vault}

Engel p. 239 (example `templates_curved_folding_9_pentagon_vault`, `CylinderVault(5, 8000.0, 4000.0, 20.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin)`).

![pentagon vault](templates/templates_curved_folding_9_pentagon_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_9_pentagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_9_pentagon_vault.cpp

</details>

#### Octagonal plan with segments sloping toward the centre {#octagon_vault}

Engel p. 239 (example `templates_curved_folding_10_octagon_vault`, `CylinderVault(8, 6000.0, 3000.0, -15.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin)`).

![octagon vault](templates/templates_curved_folding_10_octagon_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_10_octagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_10_octagon_vault.cpp

</details>

#### Hexagonal plan with rising cylinder segments {#hexagon_vault}

Engel p. 240 (example `templates_curved_folding_11_hexagon_vault`, `CylinderVault(6, 8000.0, 4000.0, 25.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin)`).

![hexagon vault](templates/templates_curved_folding_11_hexagon_vault_lamellas.webp)

<details><summary>Example code: templates_curved_folding_11_hexagon_vault.cpp</summary>

\include{lineno} templates_curved_folding_11_hexagon_vault.cpp

</details>

#### Cross-shaped plan with upright segments {#cross_vault}

Engel p. 240: four half cylinders standing almost upright on the sides of a square, tilted 70 degrees toward the centre, so the vertical sides cut them in tall half ellipses, the lobes of the cross-shaped plan, and the four lean in to meet at the top (example `templates_curved_folding_12_cross_vault`, `CylinderVault(4, 8000.0, 4000.0, 70.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin)`).

![cross vault](templates/templates_curved_folding_12_cross_vault_lamellas.webp)

![cross vault, plan](templates/templates_curved_folding_12_cross_vault_plan.webp)

<details><summary>Example code: templates_curved_folding_12_cross_vault.cpp</summary>

\include{lineno} templates_curved_folding_12_cross_vault.cpp

</details>

#### Composition over a square grid {#vault_field}

Engel p. 241, composition of cylindrical surfaces crossing diagonally over a square grid plan: a groin vault on every bay of a 6 by 4 grid of 6 m bays, and over the four bays in the middle one groin vault at twice the scale, standing up above the field (example `templates_curved_folding_19_vault_field`, `CylinderVault(4, 6000.0, 3000.0, 0.0, CylinderVault::Kind::Groin, 0.0, 120.0, "vault_field", CylinderVault::field(6, 4, 6000.0))`).

![vault field](templates/templates_curved_folding_19_vault_field_lamellas.webp)

![vault field, elevation](templates/templates_curved_folding_19_vault_field_elevation.webp)

<details><summary>Example code: templates_curved_folding_19_vault_field.cpp</summary>

\include{lineno} templates_curved_folding_19_vault_field.cpp

</details>

## Curved-crease unfolding {#curved_crease_unfolding}

A mechanism, not a fixed shape: flat plates cut as bands between two curved edges are stacked and hinged to their neighbours along those edges, one plate's edge on the top of the stack and the next's at the bottom. Pulled apart like an accordion the stack can only open if every plate bends about its rulings, because a curved hinge cannot stay flat while the plates on either side of it turn: the curvature of the plan becomes the curvature of the profile. Each crease stays in a plane parallel to the stack and keeps its length, each plate is the mirror image of the one before it in the plane of their shared crease, and the stack opens into a corrugated vault with one degree of freedom (Scheder-Bieschin, Van Mele and Block, DMSB 2023, their curved-crease unfolding; Tachi's closed folding, 2013). A straight edge would only hinge.

| quantity | at opening a (0 flat-folded) |
| --- | --- |
| crease curvature in space | the plate edge's curvature as cut over cos a (Fuchs and Tabachnikov, cos a = k2D / k3D): the arch rises and narrows as it opens |
| rulings | perpendicular to the crease, at a to the crease plane (the paper's ultra-special case: a planar crease with a constant reflection angle); a circular crease gives exact cones |
| corrugation depth | width cos a, the second crease the first offset in its plane |
| crease plane spacing | width sin a + thickness cos a: the axis shift, a plate's two hinges on its two faces, so the plates fold completely closed into a stack of count x thickness |

`CurvedCreaseUnfolding(target, design, width, count, openings, thickness)` takes the profile wanted at the design opening; the plates are cut with every curvature of it times cos(design), and the stack is drawn at every opening in `openings` side by side, 0 the flat-folded stack. Every plate is a `PlateCurved` between four offsets of the bent crease; its outer face is checked developable (`distortion()`) and its mid-surface at every opening is developed onto the plate as cut (`isometry()`, mm): the opening bends the plates and never stretches them. The plate as cut is drawn flat on the floor in the group `flat`. Real plates bend, so they are thin (plywood or LVL, 40 mm on a 9 m span here, the paper's 8 mm on 2.5 m) and hinged with textile or steel hinges along the curved edges.

### Arch {#curved_crease_arch}

Sixteen plates 600 mm wide and 40 mm thick cut to a semicircle 9 m wide and 4.5 m high at an opening of 50 degrees, drawn flat-folded (an arc 11.9 m wide and 3.3 m high, the stack 640 mm thick), at 25 and at 50 degrees; every plate an exact cone, development distortion 3e-15, isometry 0.007 mm (example `templates_curved_folding_20_curved_crease_unfolding`).

![curved-crease unfolding](templates/templates_curved_folding_20_curved_crease_unfolding_all.webp)

![curved-crease unfolding, elevation](templates/templates_curved_folding_20_curved_crease_unfolding_elevation.webp)

```cpp
CurvedCreaseUnfolding unfolding(CurvedCreaseUnfolding::arc(9000.0, 4500.0), 50.0 * Tolerance::TO_RADIANS, 600.0, 16, {0.0, 25.0 * Tolerance::TO_RADIANS, 50.0 * Tolerance::TO_RADIANS}, 40.0);
```

<details><summary>Example code: templates_curved_folding_20_curved_crease_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_20_curved_crease_unfolding.cpp

</details>

### Catenary {#curved_crease_catenary}

The paper's Fig. 9c: twenty plates 500 mm wide cut to a catenary 10 m wide and 6 m high at 45 degrees, drawn flat-folded (13.1 m wide, 4.7 m high) and open; the creases interpolated through the bent curve, the plates developable strips between them (example `templates_curved_folding_21_catenary_unfolding`).

![catenary unfolding](templates/templates_curved_folding_21_catenary_unfolding_all.webp)

```cpp
CurvedCreaseUnfolding unfolding(CurvedCreaseUnfolding::catenary(10000.0, 6000.0), 45.0 * Tolerance::TO_RADIANS, 500.0, 20, {0.0, 45.0 * Tolerance::TO_RADIANS}, 40.0);
```

<details><summary>Example code: templates_curved_folding_21_catenary_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_21_catenary_unfolding.cpp

</details>

## Four quadrants of a crease {#crease_quadrants}

`CreaseQuadrants` (`curved_folding/crease_quadrants.h`, Tachi 2013; Scheder-Bieschin, Van Mele and Block 2023, Figs. 2-3): a developable through a planar curved crease has its rulings r on one side of the crease plane and -r on the other, and their mirror images Mr and -Mr. Two strips joined on the crease take one ruling each, which gives the four ways a curved crease can join two plates: folded from one sheet or unfolded from two mirror sheets.

| quadrant | rulings | kind | at the crease |
| --- | --- | --- | --- |
| Q1 | r, -Mr | folding: one sheet, both strips in front of the plane | convex for one strip, concave for the other; sector angles summing to 360, developable |
| Q2 | r, Mr | unfolding: two mirror sheets | convex for both: synclastic, sector angles summing to less than 360 |
| Q3 | Mr, -r | folding the other way, both strips behind the plane | as Q1 |
| Q4 | -r, -Mr | unfolding | concave for both: anticlastic, sector angles summing to more than 360 |

Every unfolding of this page alternates Q2 and Q4 creases. A folding is mitred on the plane through the crease square to it, an unfolding on the crease plane.

### Q1 to Q4 on one arc {#crease_quadrants_example}

An arc 4 m wide and 1.5 m high, strips 1200 mm wide and 40 mm thick, the rulings 40 degrees off the crease plane; from the left Q1, Q2, Q3, Q4 (example `templates_curved_folding_22_crease_quadrants`).

![q1 to q4 on one arc](templates/templates_curved_folding_22_crease_quadrants_all.webp)

```cpp
CreaseQuadrants quadrants(Primitives::arc(Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)), 40.0 * Tolerance::TO_RADIANS, 1200.0, 40.0);
```

<details><summary>Example code: templates_curved_folding_22_crease_quadrants.cpp</summary>

\include{lineno} templates_curved_folding_22_crease_quadrants.cpp

</details>

## Curved-crease unfolding on inclined planes {#inclined_crease_unfolding}

`InclinedCreaseUnfolding` (`curved_folding/inclined_crease_unfolding.h`, Scheder-Bieschin, Van Mele and Block 2023, Figs. 10-12): the open state of the unfolding with crease planes that are no longer parallel. The given crease lies on one of the planes; the plate after it is the strip of rulings leaving it square to it at the opening, toward its concave side, cut by the next plane, the plate before it its mirror image, and every further plate the mirror image of its neighbour in their shared crease plane. Every crease stays planar and every plate a developable cut from a flat band; the plates are mitred on the crease planes. Only the open state is drawn: with inclined planes the stack closes only by bending its rulings.

| factory | planes | figure |
| --- | --- | --- |
| `leaning(origin, along, axis, count, spacing, lean)` | standing on the floor `spacing` apart, the i-th from the middle turned by i `lean` about the span: drawn in at the apex (synclastic) or spread (anticlastic) | 10b, 10c |
| `zigzag(origin, along, axis, count, spacing, tilt)` | `spacing` apart, every second one turned by `tilt` and `-tilt` in turn about `axis` through its point, so the mirror images' turns cancel and the vault runs straight: about the span at the apex, the floor or half height (Fig. 11a, b, c), about a vertical axis to the sides (Fig. 12a) | 11, 12a |
| `fan(origin, axis, first, count, angle)` | through one axis, `angle` apart: around a vertical axis a dome or a funnel | 12b, 12c |

Two mirrors compose to a turn about the line where their planes meet. For planes that lean about the span that line runs high above or deep below the floor, so a synclastic or anticlastic vault built only by reflection lifts its supports toward its ends (`drift()`: 276 mm at the ends of 23 and 24, the paper's plans show the support lines bend too). Planes turned in pairs that cancel keep the supports within 30 to 50 mm, and vertical planes keep them on the floor exactly.

### Synclastic vault {#synclastic_unfolding}

The semicircle 9 m wide and 4.5 m high, seventeen planes 500 mm apart on the floor, each leaning 1 degree more toward the middle: the apex drawn in, the end arches leaning inward (Fig. 10b) (example `templates_curved_folding_23_synclastic_unfolding`).

![synclastic vault](templates/templates_curved_folding_23_synclastic_unfolding_plates.webp)

```cpp
InclinedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, InclinedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 16, 500.0, 1.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_23_synclastic_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_23_synclastic_unfolding.cpp

</details>

### Anticlastic vault {#anticlastic_unfolding}

The same planes leaning 1 degree more away from the middle: the apex spread, the end arches leaning out, a saddle (Fig. 10c) (example `templates_curved_folding_24_anticlastic_unfolding`).

![anticlastic vault](templates/templates_curved_folding_24_anticlastic_unfolding_plates.webp)

```cpp
InclinedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, InclinedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 16, 500.0, -1.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_24_anticlastic_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_24_anticlastic_unfolding.cpp

</details>

### Inclined about the span at the apex {#apex_level_unfolding}

Every second plane turned 3 degrees, alternately each way, about a line along the span at the apex: the apex spacing constant, the supports alternately close and far, the ridges and valleys articulated at the feet (Fig. 11a) (example `templates_curved_folding_25_apex_level_unfolding`).

![inclined about the span at the apex](templates/templates_curved_folding_25_apex_level_unfolding_plates.webp)

```cpp
InclinedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, InclinedCreaseUnfolding::zigzag(Point(0.0, 0.0, 4500.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 16, 500.0, 3.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_25_apex_level_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_25_apex_level_unfolding.cpp

</details>

### Inclined about the span at the floor {#support_level_unfolding}

The same turned about a line on the floor: the supports keep their spacing and the apex alternates (Fig. 11b) (example `templates_curved_folding_26_support_level_unfolding`).

![inclined about the span at the floor](templates/templates_curved_folding_26_support_level_unfolding_plates.webp)

```cpp
InclinedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, InclinedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 16, 500.0, 3.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_26_support_level_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_26_support_level_unfolding.cpp

</details>

### Inclined about the span at half height {#alternating_unfolding}

Every second plane turned 4 degrees about a line at half height: plates wide at the apex and narrow at the supports and the other way round, articulated alternately at both (Fig. 11c) (example `templates_curved_folding_27_alternating_unfolding`).

![inclined about the span at half height](templates/templates_curved_folding_27_alternating_unfolding_plates.webp)

```cpp
InclinedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, InclinedCreaseUnfolding::zigzag(Point(0.0, 0.0, 2250.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 16, 500.0, 4.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_27_alternating_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_27_alternating_unfolding.cpp

</details>

### Inclined about a vertical axis {#sideways_unfolding}

Every second plane turned 4 degrees about a vertical axis through the crown: the plane distance shrinks toward one foot and grows toward the other, the vault articulated to its sides (Fig. 12a) (example `templates_curved_folding_28_sideways_unfolding`).

![inclined about a vertical axis](templates/templates_curved_folding_28_sideways_unfolding_plates.webp)

```cpp
InclinedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, InclinedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 16, 500.0, 4.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_28_sideways_unfolding.cpp</summary>

\include{lineno} templates_curved_folding_28_sideways_unfolding.cpp

</details>

### Corrugated dome {#corrugated_dome}

A meridian arc of a 6 m sphere up to 80 degrees, 25 meridian planes 15 degrees apart, the rulings at 60 degrees toward the centre: ridges the arc turned by 30 degrees, valleys its mirror images, a star at the base and an oculus of 2.1 m (Fig. 12b) (example `templates_curved_folding_29_corrugated_dome`).

![corrugated dome](templates/templates_curved_folding_29_corrugated_dome_plates.webp)

```cpp
InclinedCreaseUnfolding unfolding(Primitives::arc(Point(6000.0, 0.0, 0.0), Point(6000.0 * std::cos(0.7), 0.0, 6000.0 * std::sin(0.7)), Point(6000.0 * std::cos(1.4), 0.0, 6000.0 * std::sin(1.4))), 60.0 * Tolerance::TO_RADIANS, InclinedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 24, 15.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_29_corrugated_dome.cpp</summary>

\include{lineno} templates_curved_folding_29_corrugated_dome.cpp

</details>

### Corrugated funnel {#corrugated_funnel}

A quarter circle of 5 m radius flaring from a stem 0.8 m off the axis to a rim 5.8 m out and 5 m up, 33 meridian planes 11.25 degrees apart, the rulings at 75 degrees: a funnel column with a star at the rim (Fig. 12c) (example `templates_curved_folding_30_corrugated_funnel`).

![corrugated funnel](templates/templates_curved_folding_30_corrugated_funnel_plates.webp)

```cpp
InclinedCreaseUnfolding unfolding(Primitives::arc(Point(800.0, 0.0, 0.0), Point(5800.0 - 5000.0 * std::cos(0.25 * Tolerance::PI), 0.0, 5000.0 * std::sin(0.25 * Tolerance::PI)), Point(5800.0, 0.0, 5000.0)), 75.0 * Tolerance::TO_RADIANS, InclinedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 32, 11.25 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_curved_folding_30_corrugated_funnel.cpp</summary>

\include{lineno} templates_curved_folding_30_corrugated_funnel.cpp

</details>

## Not built {#curved_folding_not_built}

Engel's segments with tilted generatrices over a corner (the diagonal vault's generatrices sloping or rising) break the mirror symmetry of the midline planes, so their creases are not planar and they are not made. Huffman's cone reflected in planes across its axis (Albers' concentric circles), a corrugated round roof of conical lamellas, is the one developable base besides the cylinder the reflection would take exactly, and is left for a cone variant of the fold.
