# Folding {#templates_folding}

[TOC]

Folded plate structures after Heino Engel, *Structure Systems*, chapter 4 "Surface-active structure systems", Folded plate structures (Faltwerke, pp. 219-233), and two free-form folds. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is one group of the example's scene.

The topology is the same in every system: plates one after another, each meeting the next at a fold. A template builds the folded mid-surface as a mesh of planar faces, the plates' outer faces, and `mitered_plates` (`template_chamfer.h`) turns every face into one plate of the thickness: its inner face is its outer face moved along its normal by the thickness, and its side at a fold is the plane that halves the fold's angle, shared with the neighbour. A plate with thickness is therefore planar exactly when its face is planar, whatever the thickness, and two plates meet over the whole fold. So the geometry of each system is the rule that keeps its faces planar: a profile carried straight (quads), a profile that changes along the span (triangles), a pyramid (triangles and trapezoids).

| pages | Engel | template |
| --- | --- | --- |
| 219-221 | singly folded plate: threefold action, advantages over a rib slab, the number of folds, stiffening of the profile and of the free edges | [`PrismaticFold`](#prismatic_fold) |
| 222-224 | surfaces with counter-running folding and with conical folding | [`CounterFold`](#counter_fold) |
| 225-227 | linear structure systems of folded surfaces: two- and three-hinged frames, arches | [`FoldedFrame`](#folded_frame) |
| 228, 231-233 | interpenetrating folded surfaces over a polygon plan, the pyramid and the truncated pyramid | [`PolygonFold`](#polygon_fold) |
| 229-230 | cross-folded surfaces over a square plan, subdivided from the corners, and their composition | [`CrossFold`](#cross_fold) |
| 232 | structure systems of folded triangular surfaces over square, triangular, pentagonal and hexagonal plans | [`TriangleFold`](#triangle_fold) |
| 229 | fans from the corners to the cross, ridges rising toward the centre | [`FanFold`](#fan_fold) |
| - | a free-form zigzag carried along an arch | [`ReflexFold`](#reflex_fold) |
| - | a free-form fold of rhombi on a surface | [`DiamondMesh`](#diamond_mesh) |

## Singly folded plate {#prismatic_fold}

A fold profile across y carried straight along x over the span, one plate per fold (`folding/prismatic_fold.h`, examples `templates_folding_3_prismatic_fold` and `templates_folding_4_folded_barrel`). Engel's threefold bearing action of the singly folded plate (p. 219): each plate carries as a slab between its folds (Plattenwirkung), as a deep beam along the span (Scheibenwirkung), and with its neighbours as a truss or frame across it (Fachwerkwirkung). Against a rib slab of the same plan it halves the slab span, since every fold acts as a rigid support, needs no ribs, since every plate is also a beam along the span, and gains carrying capacity with its construction height, the fold depth.

![prismatic fold](templates/templates_folding_3_prismatic_fold_all.webp)

```cpp
PrismaticFold fold(
    PrismaticFold::zigzag(4, 1500.0, 1200.0),   // four ridges, plates 1500 across in plan, 1200 deep
    12000.0,                                   // span
    240.0,                                     // thickness
    3,                                         // diaphragms: both ends and mid-span
    400.0                                      // edge flange
);
```

| | Engel | the default here |
| --- | --- | --- |
| profile | a zigzag of ridges and valleys, the supports under the valleys (p. 219) | four ridges, eight plates 1500 across in plan and 1200 deep, 12 m wide |
| span | the folds span between end supports; the depth is the construction height | 12 m, the depth a tenth of the span |
| plates | each fold plane a slab, a beam and a truss member | 240 mm, mitered at every fold |
| stiffening of the profile | the fold profile deforms: the lower edges spread, the plates buckle, the fold angles change; transverse diaphragms below or above the folds, or rigid frames below them (p. 221) | a diaphragm under the folds at both ends and at mid-span, 240 mm, one plate across the profile between the flanges, down to their bottom |
| stiffening of the free edges | the outer plates buckle under components normal to their plane; a vertical edge beam for shallow folding, a horizontal one for steep folding (p. 221) | a 400 flange folded down at each free edge, the folds being shallow (38.7 degrees), mitered like every other fold |

### Profile

The profile is the cross section of the folds in the yz plane, running along +y: `zigzag(ridges, fold_width, depth)` for the folded plate of p. 219, `polygon(folds, width, rise)` for the systems of p. 220.

![profile](templates/templates_folding_3_prismatic_fold_profile.webp)

```cpp
for (size_t i = 0; i <= 2 * ridges; i++)
    points.push_back(Point(0.0, fold_width * static_cast<double>(i), i % 2 == 1 ? depth : 0.0));
```

### Folds

A fold line along the span at every profile point: the ridges, the valleys, the flanges' folds and the two free edges.

![folds](templates/templates_folding_3_prismatic_fold_folds.webp)

```cpp
for (size_t i = 0; i < _folded.point_count(); i++)
    add_polyline(Polyline({_folded[i], _folded[i] + x * span}), folds);
```

### Mesh

One quad per profile segment between its two fold lines. A straight segment carried straight is a planar quad, so every plate of a singly folded plate is planar. The quads face inwards, so the plates grow from the outer face into the fold.

![mesh](templates/templates_folding_3_prismatic_fold_mesh.webp)

```cpp
for (size_t i = 0; i + 1 < count; i++)
    faces.push_back({2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1});
```

### Plates

Every quad becomes a plate 240 thick, its sides at the folds on the bisector planes, so neighbours meet over the whole fold. Measured on the example: every outline lies in its plane within 0.00001 mm, and the two faces are parallel and 240.00 apart.

![plates](templates/templates_folding_3_prismatic_fold_plates.webp)

```cpp
for (const std::shared_ptr<Plate>& plate : mitered_plates(_mesh, thickness, 0.0, 0.0, 180.0))
    add(plate, plates);
```

### Diaphragms

The transverse diaphragms below the folds (p. 221, Untergesetzte Querscheiben): at both ends and evenly between, one plate each across the whole profile, filling it under the plates' inner faces and between the edge flanges' inner faces down to the flanges' bottom, so it meets every plate on its inner face and the valleys and corners need no joint of their own. The inner profile moves every fold along its bisector by the thickness over the cosine of its half angle, so the diaphragm touches the plates' inner faces exactly.

![diaphragms](templates/templates_folding_3_prismatic_fold_diaphragms.webp)

```cpp
Vector bisector = normals[i - 1] + normals[i];
bisector.normalize_self();
inner.push_back(_folded[i] + bisector * (thickness / bisector.dot(normals[i])));
```

### Edge flanges

The free edges stiffened by a flange folded onto the outer plate (p. 221, Randversteifer): down beside a shallow fold, outwards beside a steep one. The flange is one more segment of the profile, so it is mitered and cut like every other fold.

```cpp
points.insert(points.begin(), profile[0] + (start_steep ? Vector(0.0, -edge_flange, 0.0) : Vector(0.0, 0.0, -edge_flange)));
points.push_back(profile[last] + (end_steep ? Vector(0.0, edge_flange, 0.0) : Vector(0.0, 0.0, -edge_flange)));
```

From the end, the diaphragm between the two flanges:

![end](templates/templates_folding_3_prismatic_fold_end.webp)

### Number of folds

Engel (p. 220): with one fold the edge stresses at the common edge have the same tendency and the stress distribution stays unchanged; with two the unstressed horizontal plate is loaded through shear and reduces the edge stresses; with three the edge stresses at the side folds oppose each other and largely compensate through shear; with many the stresses are further distributed, and form and behaviour approach those of a cylindrical shell. `polygon(folds, width, rise)` puts the corners of folds + 1 plates on the circular arc over the width rising rise; the example takes seven folds, 12 m wide and 4 m high, with a diaphragm at each end and no edge beams.

![folded barrel](templates/templates_folding_4_folded_barrel_all.webp)

![folded barrel end](templates/templates_folding_4_folded_barrel_end.webp)

```cpp
PrismaticFold barrel(PrismaticFold::polygon(7, 12000.0, 4000.0), 12000.0, 240.0, 2, 0.0, "folded_barrel");
```

<details><summary>Example code: templates_folding_3_prismatic_fold.cpp</summary>

\include{lineno} templates_folding_3_prismatic_fold.cpp

</details>

<details><summary>Example code: templates_folding_4_folded_barrel.cpp</summary>

\include{lineno} templates_folding_4_folded_barrel.cpp

</details>

## Counter-running and conical folding {#counter_fold}

Fold profiles at stations along the span, joined station to station by straight fold lines (`folding/counter_fold.h`, examples `templates_folding_5_counter_fold`, `templates_folding_6_elevated_fold` and `templates_folding_7_conical_fold`). Engel (pp. 222-224) changes the profile along the span: counter-running, the ridges of one station standing where the next has its valleys; raised towards the middle; deeper towards the middle. A strip between two fold lines then has four corners that are not in one plane, a warped strip, so it cannot be one plate: it folds along one of its diagonals into two triangles, and a triangle is always planar. Which diagonal folds is the difference between Engel's ridge-to-ridge folding (First-zu-First), through the strip's two high corners, so the ridges run diagonally from the ridges of one station to those of the next, and ridge-to-valley folding (First-zu-Kehle), through its two low corners, a valley; alternate folding (Wechselfaltung) takes the one and the other strip by strip. A strip still planar, two stations of the same profile, stays one quad. Measured on all three systems with all three foldings: 300 plates, every one 240.00 thick, its outlines in their planes within 0.00001 mm and its faces parallel.

![counter-running fold](templates/templates_folding_5_counter_fold_plates.webp)

```cpp
CounterFold fold(
    CounterFold::counter_running(5, 1500.0, 1500.0, 18000.0),   // five ridges, 1500 across and deep, over 18 m
    Folding::ridge_to_ridge,                      // the diagonal every warped strip folds along
    240.0                                                      // thickness
);
```

| Engel | stations | `CounterFold::` |
| --- | --- | --- |
| counter-running folding, same depth of the profiles and same elevation above ground (p. 222) | the zigzag at the start, its counter profile at the end | `counter_running(ridges, fold_width, depth, span)` |
| counter profile in the centre (p. 222) | the zigzag at both ends, its counter profile at mid-span | `counter_centre(ridges, fold_width, depth, span, 0.0, depth)` |
| centre fold elevated over the edge folds, identical depth (p. 223) | the counter profile at mid-span raised | `counter_centre(ridges, fold_width, depth, span, rise, depth)` |
| elevated centre profile deeper than the edge profiles (p. 224) | the counter profile at mid-span raised and deeper | `counter_centre(ridges, fold_width, depth, span, rise, middle_depth)` |
| conical folding: a continuous profile, its upper edge cut by sloping planes (p. 224) | one zigzag throughout, valleys level, the ridges higher at mid-span | `conical(ridges, fold_width, depth, span, middle_depth)` |
| ridge-to-ridge, ridge-to-valley, alternate folding (pp. 222, 223) | the diagonal of every warped strip | `Folding::ridge_to_ridge`, `ridge_to_valley`, `alternate` |

### Stations

The fold profiles along the span, each a zigzag in a yz plane; the counter profile starts on a ridge, so its ridges stand over the other's valleys.

![stations](templates/templates_folding_5_counter_fold_stations.webp)

```cpp
for (size_t i = 0; i <= 2 * ridges; i++)
    points.push_back(Point(x, fold_width * static_cast<double>(i), elevation + ((i % 2 == 1) != counter ? depth : 0.0)));
```

### Mesh

Per strip between two stations and two fold lines: one quad when its fourth corner lies on the plane of the other three, else two triangles on the folding's diagonal, all facing in.

![mesh](templates/templates_folding_5_counter_fold_mesh.webp)

```cpp
const bool high_ac = points[a][2] + points[c][2] > points[b][2] + points[d][2];
const bool ridge = folding == Folding::ridge_to_ridge || (folding == Folding::alternate && i % 2 == 0);

if (high_ac == ridge) {
    faces.push_back({a, b, c});
    faces.push_back({a, c, d});
} else {
    faces.push_back({a, b, d});
    faces.push_back({b, c, d});
}
```

### Folds

Every edge of the mesh: the fold lines between the stations and the diagonals. From above, as Engel draws the plan, the ridges zigzag from one end to the other:

![plan](templates/templates_folding_5_counter_fold_plan.webp)

### Plates

One plate per triangle, mitered at every fold, as for the singly folded plate.

![plates](templates/templates_folding_5_counter_fold_plates.webp)

### Elevated centre

The centre fold elevated over the edge folds (p. 223): the zigzag at both ends, its counter profile at mid-span 3000 higher, ridge to ridge.

![elevated fold](templates/templates_folding_6_elevated_fold_plates.webp)

![elevated fold plan](templates/templates_folding_6_elevated_fold_plan.webp)

```cpp
CounterFold fold(CounterFold::counter_centre(5, 1500.0, 1500.0, 18000.0, 3000.0, 1500.0), Folding::ridge_to_ridge, 240.0, "elevated_fold");
```

### Conical folding

Sloped folding (p. 224, Abgeschrägte Faltung): one zigzag throughout, its valleys level and its ridges 600 high at both ends and 1800 at mid-span. A strip between a level valley and a sloping ridge is warped, so the plates fan out from the high ridges at mid-span, crossing in plan as Engel draws them.

![conical fold](templates/templates_folding_7_conical_fold_plates.webp)

![conical fold plan](templates/templates_folding_7_conical_fold_plan.webp)

```cpp
CounterFold fold(CounterFold::conical(5, 1500.0, 600.0, 18000.0, 1800.0), Folding::ridge_to_ridge, 240.0, "conical_fold");
```

<details><summary>Example code: templates_folding_5_counter_fold.cpp</summary>

\include{lineno} templates_folding_5_counter_fold.cpp

</details>

<details><summary>Example code: templates_folding_6_elevated_fold.cpp</summary>

\include{lineno} templates_folding_6_elevated_fold.cpp

</details>

<details><summary>Example code: templates_folding_7_conical_fold.cpp</summary>

\include{lineno} templates_folding_7_conical_fold.cpp

</details>

## Folded frames and arches {#folded_frame}

A frame or an arch whose cross section is a fold profile (`folding/folded_frame.h`, examples `templates_folding_8_two_hinged_frame`, `templates_folding_9_three_hinged_frame`, `templates_folding_10_three_hinged_arch`). Engel (pp. 225-227): the folded surface becomes a linear structure system, its fold depth following the moments, small at a hinge, deep at a knee. The frame line is the inner line, the valleys. At every corner the zigzag stands in the plane halving the corner, its ridges offset outwards along the bisector by the depth over the cosine of the half corner, so a ridge between two corners of one phase is parallel to the frame line and its strips planar. The phase turns at every corner, so the strips warp and fold into triangles on `fold_strips`, shared with `CounterFold`: the diamonds of Engel's elevations. Measured on both frames with all three foldings: 598 plates, every one 240.00 thick, planar within 0.00001 mm.

![two-hinged frame](templates/templates_folding_8_two_hinged_frame_plates.webp)

```cpp
FoldedFrame frame(
    FoldedFrame::portal(30000.0, 5000.0, 2),                      // inner line: columns with a knee, the ceiling with a corner at mid-span
    4,                                                            // ridges
    2000.0,                                                       // fold width
    {300.0, 2000.0, 900.0, 3200.0, 900.0, 2000.0, 300.0},         // depth per corner: hinges 300, knees 2000, ridge 3200
    true,                                                         // phase turning at every corner
    Folding::ridge_to_ridge,
    240.0                                                         // thickness
);
```

| Engel | frame line | depths |
| --- | --- | --- |
| two-hinged frame, ridge-to-ridge folding (p. 225) | `portal(30000, 5000, 2)` | 300 at the feet, deepest at the knees and the ridge |
| two-hinged frame, ridge-to-valley folding (p. 225) | the same | the same, `Folding::ridge_to_valley` |
| three-hinged frame, ridge folding (p. 226) | `portal(30000, 5000, 4)` | 300 at the feet and at mid-span, the three hinges |
| three-hinged arch (p. 227) | `arch(24000, 9000, 6)` | 300 at the feet and the crown |

The section shows the inner line straight and the folds bulging out at the knees and rising to the ridge, as Engel draws it:

![two-hinged frame section](templates/templates_folding_8_two_hinged_frame_section.webp)

```cpp
const double miter = 1.0 / bisector.dot(after);
const bool turned = counter && k % 2 == 1;

for (size_t i = 0; i <= 2 * ridges; i++) {
    const double height = (i % 2 == 1) != turned ? depths[k] : 0.0;
    points.push_back(frame[k] + y * (fold_width * static_cast<double>(i)) + bisector * (height * miter));
}
```

### Three-hinged frame

![three-hinged frame](templates/templates_folding_9_three_hinged_frame_plates.webp)

![three-hinged frame section](templates/templates_folding_9_three_hinged_frame_section.webp)

### Three-hinged arch

![three-hinged arch](templates/templates_folding_10_three_hinged_arch_plates.webp)

![three-hinged arch section](templates/templates_folding_10_three_hinged_arch_section.webp)

<details><summary>Example code: templates_folding_8_two_hinged_frame.cpp</summary>

\include{lineno} templates_folding_8_two_hinged_frame.cpp

</details>

<details><summary>Example code: templates_folding_9_three_hinged_frame.cpp</summary>

\include{lineno} templates_folding_9_three_hinged_frame.cpp

</details>

<details><summary>Example code: templates_folding_10_three_hinged_arch.cpp</summary>

\include{lineno} templates_folding_10_three_hinged_arch.cpp

</details>

## Folds over a polygon plan {#polygon_fold}

Singly folded surfaces, one per side of a regular polygon plan, their folds parallel to that side, interpenetrating along the lines from the corners to the centre (`folding/polygon_fold.h`, examples `templates_folding_11_triangle_fold` to `templates_folding_15_truncated_pyramid`). Every fold is a concentric copy of the plan at one height, so a band between two rings in one sector lies between two parallel edges and is a planar trapezoid, whatever the heights. The centre is an apex, the pyramid, or the innermost ring closed by a flat top, the truncated pyramid.

```cpp
PolygonFold fold(
    4,                                              // sides
    12000.0,                                        // circumradius
    {0.0, 1800.0, 900.0, 2700.0, 1800.0, 3600.0},  // ring heights from the edge inwards
    4500.0,                                         // apex
    false,                                          // truncated: a flat top instead of the apex
    240.0                                           // thickness
);
```

| Engel | example | rings |
| --- | --- | --- |
| triangular plan, horizontal ridges (p. 228) | `templates_folding_11_triangle_fold` | 3 sides, ridges all 1800 |
| square plan, ridges rising toward the centre (p. 228) | `templates_folding_12_square_fold` | 4 sides, rising to a 4500 apex |
| hexagonal plan, ridges sloping to the centre (p. 228) | `templates_folding_13_hexagon_fold` | 6 sides, falling to a 600 centre |
| pyramidal folded plate (p. 231) | `templates_folding_14_pyramid` | 4 sides, one ring, a 6000 apex |
| doubly folded truncated pyramid (p. 233) | `templates_folding_15_truncated_pyramid` | 4 sides, a flat top |

![square fold](templates/templates_folding_12_square_fold_plates.webp)

![triangle fold](templates/templates_folding_11_triangle_fold_plates.webp)

![hexagon fold](templates/templates_folding_13_hexagon_fold_plates.webp)

![pyramid](templates/templates_folding_14_pyramid_plates.webp)

![truncated pyramid](templates/templates_folding_15_truncated_pyramid_plates.webp)

### Corners

Where three or more plates meet at a vertex, their mitered corners would pass through each other. As the reflex fold does, every corner of both outlines is cut back along its two edges by the chamfer (`mitered_plates(mesh, thickness, chamfer, chamfer, 180.0)`, every corner below 180 degrees), here the thickness; neighbours cut the same length off their shared edge, so their miter faces still meet, and a small opening is left at the vertex. `CounterFold`, `FoldedFrame` and `PolygonFold` all cut their corners this way; the singly folded plate has only two plates at a vertex and keeps them sharp.

![corners](templates/templates_folding_8_two_hinged_frame_corner.webp)

<details><summary>Example code: templates_folding_12_square_fold.cpp</summary>

\include{lineno} templates_folding_12_square_fold.cpp

</details>

## Cross folds {#cross_fold}

The waterbomb fold of a square sheet (`folding/cross_fold.h`, examples `templates_folding_16_cross_fold` to `templates_folding_19_fold_composition`): straight fold lines from the centre to the four corners, the diagonals, and to the edges' midpoints, the cross; every half edge between a corner and a midpoint can be divided again, a fold line to every division point. Every face between two straight fold lines through the centre is a planar triangle, whatever the heights; the edge points alternate low and high from the corners, so the fold lines alternate valley and ridge. The centre's height sets Engel's ridges rising toward the centre or sloping to it (p. 229). Units in columns and rows compose a larger roof (p. 230), their shared edge points welded so neighbouring plates meet mitered.

```cpp
CrossFold fold(
    18000.0,   // square side
    3,         // divisions of every half edge
    0.0,       // low: corners and every second division point
    3000.0,    // high: the other division points
    6000.0,    // centre
    240.0,     // thickness
    1, 1       // columns, rows
);
```

| Engel | example |
| --- | --- |
| the waterbomb: diagonals valleys, cross ridges rising toward the centre (p. 229) | `templates_folding_16_cross_fold`, 8 triangles |
| subdivided from the four corners (p. 229) | `templates_folding_17_cross_fold_divided`, 24 triangles |
| ridges sloping toward the centre (p. 229) | `templates_folding_18_cross_fold_sloping` |
| composition over square plans (p. 230) | `templates_folding_19_fold_composition`, 4 x 3 units, 96 triangles |

![cross fold](templates/templates_folding_16_cross_fold_plates.webp)

![cross fold divided](templates/templates_folding_17_cross_fold_divided_plates.webp)

![cross fold sloping](templates/templates_folding_18_cross_fold_sloping_plates.webp)

![fold composition](templates/templates_folding_19_fold_composition_plates.webp)

<details><summary>Example code: templates_folding_17_cross_fold_divided.cpp</summary>

\include{lineno} templates_folding_17_cross_fold_divided.cpp

</details>

## Fans from the corners {#fan_fold}

The cross from the edges' midpoints to the centre divides a square into four quarters, and in every quarter straight fold lines fan out from its corner to points along the two half cross lines and to the centre (`folding/fan_fold.h`, example `templates_folding_24_fan_fold`, Engel p. 229, ridges rising toward the centre). Every face between two lines from one corner is a planar triangle; the cross rises from the midpoints to the centre, every second point raised by the fold depth, so the fan's lines alternate ridge and valley and the cross folds too, shared by the two quarters beside it.

```cpp
FanFold fold(
    18000.0,   // square side
    3,         // points along every half cross line
    0.0,       // corners
    600.0,     // edges' midpoints
    3000.0,    // centre
    1800.0,    // fold depth of every second point
    240.0      // thickness
);
```

![fan fold](templates/templates_folding_24_fan_fold_plates.webp)

![fan fold plan](templates/templates_folding_24_fan_fold_plan.webp)

<details><summary>Example code: templates_folding_24_fan_fold.cpp</summary>

\include{lineno} templates_folding_24_fan_fold.cpp

</details>

## Folded triangular surfaces {#triangle_fold}

A regular polygon plan cut by the lines joining its edges' midpoints into a triangle at every corner, the polygon of the midpoints one flat face or split into triangles to a raised centre (`folding/triangle_fold.h`, examples `templates_folding_20_triangle_square` to `templates_folding_23_triangle_hexagon`, Engel p. 232). Every face is a triangle or a flat polygon at one height, so every plate is planar: the corners on the supports, the midpoints high, the corner triangles folding down.

```cpp
TriangleFold fold(
    4,          // sides
    12000.0,    // circumradius
    0.0,        // corners
    4000.0,     // edges' midpoints
    7000.0,     // centre; equal to the midpoints keeps their polygon one flat face
    240.0       // thickness
);
```

![square](templates/templates_folding_20_triangle_square_plates.webp)

![triangle](templates/templates_folding_21_triangle_triangle_plates.webp)

![pentagon](templates/templates_folding_22_triangle_pentagon_plates.webp)

![hexagon](templates/templates_folding_23_triangle_hexagon_plates.webp)

<details><summary>Example code: templates_folding_20_triangle_square.cpp</summary>

\include{lineno} templates_folding_20_triangle_square.cpp

</details>

## Reflex fold {#reflex_fold}

A zigzag profile carried along an arch and folded at every arch point, one plate per fold (`folding/reflex_fold.h`, example `templates_folding_1_reflex_fold`).

![reflex fold](templates/templates_folding_1_reflex_fold_plates.webp)

```cpp
ReflexFold fold(
    ReflexFold::default_cross_section(),
    ReflexFold::default_profile(),
    10.0,   // thickness
    20.0,   // chamfer_bottom
    20.0,   // chamfer_top
    180.0   // chamfer_angle: corners sharper than this are chamfered
);
```

### Curves

The cross section is the arch the profile travels along; the profile is the zigzag that folds.

![curves](templates/templates_folding_1_reflex_fold_curves.webp)

```cpp
const std::shared_ptr<TreeNode> curves = add_group("curves");
add_polyline(cross_section, curves);
add_polyline(profile, curves);
```

### Fold planes

Each inner arch point gets the plane that halves the arch's angle there, the two ends a plane normal to z.

![fold planes](templates/templates_folding_1_reflex_fold_fold_planes.webp)

```cpp
const Vector previous = (cross_section[i - 1] - cross_section[i]).normalized();
const Vector next = (cross_section[i + 1] - cross_section[i]).normalized();
normal = previous + next;
planes.push_back(Plane::from_point_normal(cross_section[i], normal));
```

### Mesh

Every profile point slides along the arch segment until it meets the next fold plane, and two rows make a strip of quads.

![mesh](templates/templates_folding_1_reflex_fold_mesh.webp)

```cpp
const Line along = Line::from_points(previous, previous + step);
const std::optional<Point> moved = Intersection::line_plane(along, fold_planes[i], false);
points.push_back(moved.value_or(previous));
```

### Plates

Every quad becomes a plate mitered at its folds, its corners chamfered (`mitered_plates`, shared by the shells and folding templates).

![plates](templates/templates_folding_1_reflex_fold_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> folds = mitered_plates(
    _mesh,
    thickness,
    chamfer_bottom,
    chamfer_top,
    chamfer_angle
);

for (const std::shared_ptr<Plate>& plate : folds)
    add(plate, plates);
```

<details><summary>Example code: templates_folding_1_reflex_fold.cpp</summary>

\include{lineno} templates_folding_1_reflex_fold.cpp

</details>

## Diamond mesh {#diamond_mesh}

A NURBS surface split into rhombi of two triangles each, one plate per triangle (`folding/diamond_mesh.h`, example `templates_folding_2_diamond_mesh`).

![diamond mesh](templates/templates_folding_2_diamond_mesh_plates.webp)

```cpp
DiamondMesh diamond(
    DiamondMesh::default_surface(),
    8,      // u_divisions
    4,      // v_divisions
    40.0,   // thickness
    10.0,   // chamfer
    180.0   // chamfer_angle
);
```

### Surface

The default surface is a bicubic arch, 3000 across, 5000 long and 1500 high.

![surface](templates/templates_folding_2_diamond_mesh_surface.webp)

```cpp
NurbsSurface arch = NurbsSurface::create(
    false,
    false,
    3,
    3,
    4,
    4,
    points
);
```

### Mesh

Each cell's corners join its centre and the next cell's centre, so two triangles make a rhombus across every cell edge.

![mesh](templates/templates_folding_2_diamond_mesh_mesh.webp)

```cpp
triangles.push_back({b, d, centre});
triangles.push_back({a, centre, c});
triangles.push_back({d, next, centre});
triangles.push_back({next, c, centre});
```

### Plates

Every triangle becomes a plate mitered at its folds, its corners chamfered.

![plates](templates/templates_folding_2_diamond_mesh_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> triangles = mitered_plates(
    _mesh,
    thickness,
    chamfer,
    chamfer,
    chamfer_angle
);
```

<details><summary>Example code: templates_folding_2_diamond_mesh.cpp</summary>

\include{lineno} templates_folding_2_diamond_mesh.cpp

</details>
