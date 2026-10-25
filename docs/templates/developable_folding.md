# Folding developable {#templates_developable_folding}

[TOC]

Folded structures of developable plates other than cylinders: cones and the strips of rulings leaving a curved crease, every plate a [`PlateCurved`](@ref elements_plate_curved) bent from a flat panel; every plate is checked flat by its [development](#development).

| template | header | source | examples |
| --- | --- | --- | --- |
| [`CurvedFold`](#curved_fold_roof) | `developable_folding/curved_fold.h` | a tent roof of curved folds with interpolated meridians | `templates_developable_folding_1_curved_fold` |
| [`CurvedCreaseUnfolding`](#curved_crease_unfolding) | `developable_folding/curved_crease_unfolding.h` | flat plates with curved edges hinged into a corrugated vault, a dome or a funnel | `templates_developable_folding_{2,3,5..12}_*` |
| [`CreaseQuadrants`](#crease_quadrants) | `developable_folding/crease_quadrants.h` | the four ways two strips join on one planar curved crease | `templates_developable_folding_4_crease_quadrants` |
| [`Development`](#development) | `development.h` | the unfolding of a ruled strip, the check that every plate, cylinder or other developable, is cut flat | every example prints it |

Every example prints the number of lamellas or panels, how many are closed solids, how many contacts the session's contact detection finds between them, and the development distortion and ruling twist of the worst one; on every model below the distortion is at the rounding floor (1e-14), the lamellas being exact cylinders. The contacts are the seams: every two lamellas that share a face are found, so the count below each example is the proof that its mitres meet, creases times lamellas per crease plus the joints between neighbouring barrels of a row.

## Development {#development}

`Development(a, b, samples)` unfolds the ruled strip between two curves into the plane, ruling by ruling: every quad between two rulings is split along one diagonal into two triangles laid flat with their edge lengths kept. On a developable strip, a cylinder or a cone, the quads are planar, so the other diagonal keeps its length too: its largest relative change is the `distortion`, the error of cutting the lamella from a flat panel, 0 for an exact cylinder. The `twist` measures the same on the smooth strip: the sine of the angle by which a ruling leaves the plane of the two curves' tangents. The `outline` is the flat cutting outline, drawn in every model's group `flat` beside it, every lamella where it lies on the unfolded sheet or the unrolled segment.

```cpp
const Development development(lamella->outer_a, lamella->outer_b, 32);
development.distortion; // 0 on a cylinder, the relative length error of a twisted strip
development.outline; // the flat outline in the xy plane, the first ruling on the y axis
```

## Curved fold {#curved_fold_roof}

A folded tent roof: a freestanding folded shell of 32 m diameter rising into a tower 26 m high with a round top of 7 m, its valleys running down to concrete columns, its ridges ending in a gable over the eaves between them (`developable_folding/curved_fold.h`, example `templates_developable_folding_1_curved_fold`). Every ridge and valley is a smooth NURBS curve, a meridian interpolated through rings, each ring the same star of ridge and valley points scaled by the trumpet curve's radius, flat at the eaves and vertical at the tower, and lifted to its height. Every fold strip between two neighbouring meridians is one lamella, a [`PlateCurved`](@ref elements_plate_curved) in the group `lamellas`, named `lamella_<j>`: a closed curved solid of six exact NURBS patches, with no facet and no triangle, as the 5 cm sheathing on the uniaxially curved glulam of the built roof. Its outer face is the ruled surface between the two meridians, its inner face the one between the inner meridians, where the two neighbouring strips offset by the thickness meet; its side faces run along the folds, so two neighbours share their side face as mitered planar plates do, and a cap closes it at the eaves and at the top. Unlike the templates below, its strips are ruled between interpolated meridians, not cylinders: the free-form reference the exact constructions are measured against.

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

![curved fold](templates/templates_developable_folding_1_curved_fold_lamellas.webp)

<details><summary>Example code: templates_developable_folding_1_curved_fold.cpp</summary>

\include{lineno} templates_developable_folding_1_curved_fold.cpp

</details>

## Curved-crease unfolding {#curved_crease_unfolding}

A mechanism, not a fixed shape: flat plates cut as bands between two curved edges are stacked and hinged to their neighbours along those edges, one plate's edge on the top of the stack and the next's at the bottom. Pulled apart like an accordion the stack can only open if every plate bends about its rulings, because a curved hinge cannot stay flat while the plates on either side of it turn: the curvature of the plan becomes the curvature of the profile. Each crease stays in its plane and keeps its length, its curvature in space is the cut edge's curvature over the cosine of the opening (cos a = k2D / k3D), the rulings leave it perpendicular to it at the opening to the crease plane (a circular crease gives exact cones), and each plate is the mirror image of the one before it in the plane of their shared crease. The hinges alternate between the two faces of the stack, the axis shift of width sin a + thickness cos a between the crease planes, so the plates fold completely closed into a stack of count times thickness. A straight edge would only hinge.

The template draws the open state: the given crease lies on one of the `planes`; the plate after it is the strip of rulings leaving it square to it at the `opening`, toward its concave side, cut by the next plane, the plate before it its mirror image, and every further plate the mirror image of its neighbour in their shared crease plane. Every crease stays planar and every plate is a developable cut from a flat band, a `PlateCurved` mitred on the two planes of its creases, so it shares one face with each neighbour; the session's contact detection finds every one. On parallel planes the stack folds flat; with inclined planes it closes only by bending its rulings.

| factory | planes | figure |
| --- | --- | --- |
| `zigzag(origin, along, axis, count, spacing, tilt)` | `spacing` apart, every second one turned by `tilt` and `-tilt` in turn about `axis` through its point, so the mirror images' turns cancel and the vault runs straight; with `tilt` 0 parallel, the unfolding on parallel planes | 7-9; 11, 12a |
| `leaning(origin, along, axis, count, spacing, lean)` | standing on the floor `spacing` apart, the i-th from the middle turned by i `lean` about the span: drawn in at the apex (synclastic) or spread (anticlastic) | 10b, 10c |
| `fan(origin, axis, first, count, angle)` | through one axis, `angle` apart: around a vertical axis a dome or a funnel | 12b, 12c |

Two mirrors compose to a turn about the line where their planes meet, so planes that lean about the span lift the supports toward the ends (`drift()`); vertical planes keep them on the floor. Each example shows the principle on four plates, three creases and three contacts.

### Arch {#curved_crease_arch}

Four 40 mm plates on five parallel planes 1000 mm apart, the crease a semicircle 9 m wide and 4.5 m high, opened 50 degrees; every plate an exact cone (example `templates_developable_folding_2_curved_crease_unfolding`).

![arch](templates/templates_developable_folding_2_curved_crease_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0), 40.0);
```

<details><summary>Example code: templates_developable_folding_2_curved_crease_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_2_curved_crease_unfolding.cpp

</details>

### Catenary {#curved_crease_catenary}

Five parallel planes 1000 mm apart, the crease a catenary 10 m wide and 6 m high (`CurvedCreaseUnfolding::catenary`), opened 45 degrees (example `templates_developable_folding_3_catenary_unfolding`).

![catenary](templates/templates_developable_folding_3_catenary_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(CurvedCreaseUnfolding::catenary(10000.0, 6000.0), 45.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0), 40.0);
```

<details><summary>Example code: templates_developable_folding_3_catenary_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_3_catenary_unfolding.cpp

</details>

### Synclastic vault {#synclastic_unfolding}

Five planes 1000 mm apart on the floor, each leaning 5 degrees more toward the middle: the apex drawn in (example `templates_developable_folding_5_synclastic_unfolding`).

![synclastic vault](templates/templates_developable_folding_5_synclastic_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 5.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_5_synclastic_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_5_synclastic_unfolding.cpp

</details>

### Anticlastic vault {#anticlastic_unfolding}

The planes leaning 5 degrees more away from the middle: the apex spread, a saddle (example `templates_developable_folding_6_anticlastic_unfolding`).

![anticlastic vault](templates/templates_developable_folding_6_anticlastic_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, -5.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_6_anticlastic_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_6_anticlastic_unfolding.cpp

</details>

### Inclined about the span at the apex {#apex_level_unfolding}

Every second plane turned 8 degrees, alternately each way, about a line along the span at the apex: the apex spacing kept, the fold articulated at the feet (example `templates_developable_folding_7_apex_level_unfolding`).

![inclined about the span at the apex](templates/templates_developable_folding_7_apex_level_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 4500.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_7_apex_level_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_7_apex_level_unfolding.cpp

</details>

### Inclined about the span at the floor {#support_level_unfolding}

The same turned about a line on the floor: the supports kept, the fold articulated at the crown (example `templates_developable_folding_8_support_level_unfolding`).

![inclined about the span at the floor](templates/templates_developable_folding_8_support_level_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_8_support_level_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_8_support_level_unfolding.cpp

</details>

### Inclined about the span at half height {#alternating_unfolding}

Every second plane turned 8 degrees, alternately each way, about a line at half height: each plate wide at one end and narrow at the other (example `templates_developable_folding_9_alternating_unfolding`).

![inclined about the span at half height](templates/templates_developable_folding_9_alternating_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 2250.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_9_alternating_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_9_alternating_unfolding.cpp

</details>

### Inclined about a vertical axis {#sideways_unfolding}

Every second plane turned 8 degrees, alternately each way, about a vertical axis through the crown: the plane distance shrinks toward one foot and grows toward the other (example `templates_developable_folding_10_sideways_unfolding`).

![inclined about a vertical axis](templates/templates_developable_folding_10_sideways_unfolding_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)), 50.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 8.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_10_sideways_unfolding.cpp</summary>

\include{lineno} templates_developable_folding_10_sideways_unfolding.cpp

</details>

### Corrugated dome {#corrugated_dome}

A meridian arc of a 6 m sphere up to 80 degrees, 25 meridian planes 15 degrees apart, the rulings at 60 degrees toward the centre: ridges the arc turned by 30 degrees, valleys its mirror images, a star at the base and an oculus of 2.1 m; 24 contacts (example `templates_developable_folding_11_corrugated_dome`).

![corrugated dome](templates/templates_developable_folding_11_corrugated_dome_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(6000.0, 0.0, 0.0), Point(6000.0 * std::cos(0.7), 0.0, 6000.0 * std::sin(0.7)), Point(6000.0 * std::cos(1.4), 0.0, 6000.0 * std::sin(1.4))), 60.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 24, 15.0 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_11_corrugated_dome.cpp</summary>

\include{lineno} templates_developable_folding_11_corrugated_dome.cpp

</details>

### Corrugated funnel {#corrugated_funnel}

A quarter circle of 5 m radius flaring from a stem 0.8 m off the axis to a rim 5.8 m out and 5 m up, 33 meridian planes 11.25 degrees apart, the rulings at 75 degrees: a funnel column with a star at the rim; 32 contacts (example `templates_developable_folding_12_corrugated_funnel`).

![corrugated funnel](templates/templates_developable_folding_12_corrugated_funnel_plates.webp)

```cpp
CurvedCreaseUnfolding unfolding(Primitives::arc(Point(800.0, 0.0, 0.0), Point(5800.0 - 5000.0 * std::cos(0.25 * Tolerance::PI), 0.0, 5000.0 * std::sin(0.25 * Tolerance::PI)), Point(5800.0, 0.0, 5000.0)), 75.0 * Tolerance::TO_RADIANS, CurvedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 32, 11.25 * Tolerance::TO_RADIANS), 40.0);
```

<details><summary>Example code: templates_developable_folding_12_corrugated_funnel.cpp</summary>

\include{lineno} templates_developable_folding_12_corrugated_funnel.cpp

</details>

## Four quadrants of a crease {#crease_quadrants}

`CreaseQuadrants` (`developable_folding/crease_quadrants.h`, the four ways two strips join on one planar curved crease): a developable through a planar curved crease has its rulings r on one side of the crease plane and -r on the other, and their mirror images Mr and -Mr. Two strips joined on the crease take one ruling each, which gives the four ways a curved crease can join two plates: folded from one sheet or unfolded from two mirror sheets.

| quadrant | rulings | kind | at the crease |
| --- | --- | --- | --- |
| Q1 | r, -Mr | folding: one sheet, both strips in front of the plane | convex for one strip, concave for the other; sector angles summing to 360, developable |
| Q2 | r, Mr | unfolding: two mirror sheets | convex for both: synclastic, sector angles summing to less than 360 |
| Q3 | Mr, -r | folding the other way, both strips behind the plane | as Q1 |
| Q4 | -r, -Mr | unfolding | concave for both: anticlastic, sector angles summing to more than 360 |

Every unfolding of this page alternates Q2 and Q4 creases. A folding is mitred on the plane through the crease square to it, an unfolding on the crease plane.

### Q1 to Q4 on one arc {#crease_quadrants_example}

An arc 4 m wide and 1.5 m high, strips 1200 mm wide and 40 mm thick, the rulings 40 degrees off the crease plane; from the left Q1, Q2, Q3, Q4; 8 strips, 4 contacts, one on each crease (example `templates_developable_folding_4_crease_quadrants`).

![q1 to q4 on one arc](templates/templates_developable_folding_4_crease_quadrants_all.webp)

```cpp
CreaseQuadrants quadrants(Primitives::arc(Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)), 40.0 * Tolerance::TO_RADIANS, 1200.0, 40.0);
```

<details><summary>Example code: templates_developable_folding_4_crease_quadrants.cpp</summary>

\include{lineno} templates_developable_folding_4_crease_quadrants.cpp

</details>

