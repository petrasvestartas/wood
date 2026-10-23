# Folding {#templates_folding}

[TOC]

Folded plate structures after Heino Engel, *Structure Systems*, chapter 4 "Surface-active structure systems", Folded plate structures (Faltwerke, pp. 219-233), and two free-form folds. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is one group of the example's scene.

The topology is the same in every system: plates one after another, each meeting the next at a fold. A template builds the folded mid-surface as a mesh of planar faces, the plates' outer faces, and `mitered_plates` (`template_chamfer.h`) turns every face into one plate of the thickness: its inner face is its outer face moved along its normal by the thickness, and its side at a fold is the plane that halves the fold's angle, shared with the neighbour. A plate with thickness is therefore planar exactly when its face is planar, whatever the thickness, and two plates meet over the whole fold. So the geometry of each system is the rule that keeps its faces planar: a profile carried straight (quads), a profile that changes along the span (triangles), a pyramid (triangles and trapezoids).

| pages | Engel | template |
| --- | --- | --- |
| 219-221 | singly folded plate: threefold action, advantages over a rib slab, the number of folds, stiffening of the profile and of the free edges | [`PrismaticFold`](#prismatic_fold) |
| 222-224 | surfaces with counter-running folding: the counter profile at the end or in the centre, the centre elevated, the elevated centre deeper | [`CounterFold`](#counter_fold) |
| 224 | conical folding: a continuous fold profile with its upper edge cut by sloping planes | [`ConicalFold`](#conical_fold) |
| 225-227 | linear structure systems of folded surfaces: two- and three-hinged frames, arches | [`FoldedFrame`](#folded_frame) |
| 228, 231-233 | interpenetrating folded surfaces over a polygon plan, the pyramid and the truncated pyramid | [`PolygonFold`](#polygon_fold) |
| 229-230 | cross-folded surfaces over a square plan, subdivided from the corners, and their composition | [`CrossFold`](#cross_fold) |
| 232 | structure systems of folded triangular surfaces over square, triangular, pentagonal and hexagonal plans | [`TriangleFold`](#triangle_fold) |
| 229 | fans from the corners to the cross, ridges rising toward the centre | [`FanFold`](#fan_fold) |
| 220-233 | the remaining variants of every page | [every topology by page](#folding_variants) |
| - | curved folding after Knie's Zauberhut (Blumer Lehmann, 2020), curved lamellas | [`CurvedFold`](#curved_fold) |
| - | a free-form zigzag carried along an arch | [`ReflexFold`](#reflex_fold) |
| - | a free-form fold of rhombi on a surface | [`DiamondMesh`](#diamond_mesh) |

## Singly folded plate {#prismatic_fold}

A fold profile across y carried straight along x over the span, one plate per fold (`folding/prismatic_fold.h`, examples `templates_folding_3_prismatic_fold` and `templates_folding_4_folded_barrel`). Engel's threefold bearing action of the singly folded plate (p. 219): each plate carries as a slab between its folds (Plattenwirkung), as a deep beam along the span (Scheibenwirkung), and with its neighbours as a truss or frame across it (Fachwerkwirkung). Against a rib slab of the same plan it halves the slab span, since every fold acts as a rigid support, needs no ribs, since every plate is also a beam along the span, and gains carrying capacity with its construction height, the fold depth.

![prismatic fold](templates/templates_folding_3_prismatic_fold_all.webp)

```cpp
PrismaticFold fold(
    PrismaticFold::zigzag(4, 1500.0, 1200.0),   // four ridges, plates 1500 across in plan, 1200 deep
    12000.0,                                   // span
    120.0,                                     // thickness
    3,                                         // diaphragms: both ends and mid-span
    400.0                                      // edge flange
);
```

| | Engel | the default here |
| --- | --- | --- |
| profile | a zigzag of ridges and valleys, the supports under the valleys (p. 219) | four ridges, eight plates 1500 across in plan and 1200 deep, 12 m wide |
| span | the folds span between end supports; the depth is the construction height | 12 m, the depth a tenth of the span |
| plates | each fold plane a slab, a beam and a truss member | 120 mm, mitered at every fold |
| stiffening of the profile | the fold profile deforms: the lower edges spread, the plates buckle, the fold angles change; transverse diaphragms below or above the folds, or rigid frames below them (p. 221) | a diaphragm under the folds at both ends and at mid-span, 120 mm, one plate across the profile between the flanges, down to their bottom |
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

Every quad becomes a plate 120 thick, its sides at the folds on the bisector planes, so neighbours meet over the whole fold. Measured on the example: every outline lies in its plane within 0.00001 mm, and the two faces are parallel and 120.00 apart.

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
PrismaticFold barrel(PrismaticFold::polygon(7, 12000.0, 4000.0), 12000.0, 120.0, 2, 0.0, "folded_barrel");
```

<details><summary>Example code: templates_folding_3_prismatic_fold.cpp</summary>

\include{lineno} templates_folding_3_prismatic_fold.cpp

</details>

<details><summary>Example code: templates_folding_4_folded_barrel.cpp</summary>

\include{lineno} templates_folding_4_folded_barrel.cpp

</details>

## Counter-running folding {#counter_fold}

Fold profiles at stations along the span, joined station to station by straight fold lines (`folding/counter_fold.h`, examples `templates_folding_5_counter_fold`, `templates_folding_6_elevated_fold` and the variants of pp. 222-224 below). Engel (pp. 222-224) changes the profile along the span: counter-running, the ridges of one station standing where the next has its valleys; the counter profile in the centre; the centre elevated; the elevated centre deeper. A strip between two fold lines then has four corners that are not in one plane, a warped strip, so it cannot be one plate: it folds along one of its diagonals into two triangles, and a triangle is always planar. Which diagonal folds is the difference between Engel's ridge-to-ridge folding (First-zu-First), through the strip's two high corners, so a ridge runs diagonally from the ridge of one station to that of the next, and ridge-to-valley folding (First-zu-Kehle), through its two low corners, a valley; alternate folding (Wechselfaltung) takes the one and the other fold by fold. A strip still planar stays one quad, and two triangles that lie in one plane are joined into one plate (`merge_coplanar`, `fold_strips.h`), so a fold line the folding leaves straight, as the even lines of the elevated centre, is no fold. Every system was read off the book's plan and both elevations: the pattern of diagonals in plan, where they cross and which are ridges, the silhouette along and across the span, the depth at the ends and the centre.

![counter-running fold](templates/templates_folding_5_counter_fold_plates.webp)

```cpp
CounterFold fold(
    CounterFold::counter_running(4, 1500.0, 1500.0, 13500.0),   // four ridges, 1500 across and deep, over 13.5 m
    Folding::ridge_to_ridge,                                   // the diagonal every warped strip folds along
    120.0                                                      // thickness
);
```

| Engel | stations | `CounterFold::` | example |
| --- | --- | --- | --- |
| counter-running folding, same depth of the profiles and same elevation above ground, ridge to ridge (p. 222) | the zigzag at the start, its counter profile at the end | `counter_running(ridges, fold_width, depth, span)` | `templates_folding_5_counter_fold` |
| the same, ridge to valley (p. 222) | the same, `Folding::ridge_to_valley` | `counter_running(...)` | `templates_folding_28_ridge_to_valley` |
| counter profile in the centre, ridge to valley (p. 222) | the zigzag at both ends, its counter profile at mid-span | `counter_centre(ridges, fold_width, depth, span)` | `templates_folding_27_counter_centre` |
| centre fold elevated over the edge folds, identical depth, ridge to ridge, ridge to valley, alternate (p. 223) | the zigzag at both ends starting on a ridge, its counter profile at mid-span lifted by the depth | `elevated_centre(ridges, fold_width, depth, span, depth)` | `templates_folding_6_elevated_fold`, `29_elevated_ridge_to_valley`, `30_alternate_fold` |
| elevated centre profile deeper than the edge profiles, ridge to ridge, ridge to valley (p. 224) | the same, the centre deeper | `elevated_centre(ridges, fold_width, depth, span, middle_depth)` | `templates_folding_31_deep_centre`, `32_deep_centre_valley` |

### Stations

The fold profiles along the span, each a zigzag in a yz plane; the counter profile starts on a ridge, so its ridges stand over the other's valleys.

![stations](templates/templates_folding_5_counter_fold_stations.webp)

```cpp
for (size_t i = 0; i <= 2 * ridges; i++)
    points.push_back(Point(x, fold_width * static_cast<double>(i), elevation + ((i % 2 == 1) != counter ? depth : 0.0)));
```

### Mesh

Per strip between two stations and two fold lines: one quad when its fourth corner lies on the plane of the other three, else two triangles on the folding's diagonal, all facing in. Alternate folding turns fold by fold, a fold being the two strips about an even fold line, the edge fold first ridge to valley.

![mesh](templates/templates_folding_5_counter_fold_mesh.webp)

```cpp
const bool high_ac = height[a] + height[c] > height[b] + height[d];
const bool ridge = folding == Folding::ridge_to_ridge || (folding == Folding::alternate && ((i + 1) / 2) % 2 == 1);

if (high_ac == ridge) {
    faces.push_back({a, b, c});
    faces.push_back({a, c, d});
} else {
    faces.push_back({a, b, d});
    faces.push_back({b, c, d});
}
```

### Folds

Every edge of the mesh: the fold lines between the stations and the diagonals. From above, as Engel draws the plan, the ridges zigzag from one end to the other, meeting at the ends:

![plan](templates/templates_folding_5_counter_fold_plan.webp)

### Plates

One plate per face, mitered at every fold, as for the singly folded plate; from the end the strips cross in Engel's X's:

![end](templates/templates_folding_5_counter_fold_end.webp)

### Counter profile in the centre

The zigzag at both ends and its counter profile at mid-span, the same depth and elevation (p. 222), folded ridge to valley: the diagonals run from the ends' valleys to the centre's and cross there in the stars of Engel's plan, one at every second fold line.

![counter centre](templates/templates_folding_27_counter_centre_plates.webp)

![counter centre plan](templates/templates_folding_27_counter_centre_plan.webp)

### Elevated centre

The centre fold elevated over the edge folds (p. 223): the zigzag at both ends starts on a ridge, and the counter profile at mid-span is lifted by the depth, so its valleys stand level with the ends' ridges and the even fold lines run straight through the span, the level line of Engel's front elevation, while the odd ones rise from the ends' valleys to the centre's ridges, twice the depth high. Ridge to ridge, the diagonals run from the ends' even lines to the centre's odd ones; either side of a straight line the two triangles are one plane and one plate, so the centre station creases only at the odd lines, as the plan draws it.

![elevated fold](templates/templates_folding_6_elevated_fold_plates.webp)

![elevated fold plan](templates/templates_folding_6_elevated_fold_plan.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::ridge_to_ridge, 120.0, "elevated_fold");
```

Ridge to valley the diagonals go the other way, from the ends' valleys to the centre's even lines, and the centre creases at every fold; alternate folding takes the two by turns, so half the straight lines and half the centre's creases vanish into quads:

![elevated ridge to valley](templates/templates_folding_29_elevated_ridge_to_valley_plates.webp)

![alternate fold](templates/templates_folding_30_alternate_fold_plates.webp)

### Deeper centre

The elevated centre profile deeper than the edge profiles (p. 224): the same construction with the ends 1000 deep and the centre 1800, so the odd fold lines rise 2800 and the end elevation shows Engel's tall diamonds, each with the odd fold line through it.

![deep centre](templates/templates_folding_31_deep_centre_plates.webp)

![deep centre end](templates/templates_folding_31_deep_centre_end.webp)

<details><summary>Example code: templates_folding_5_counter_fold.cpp</summary>

\include{lineno} templates_folding_5_counter_fold.cpp

</details>

<details><summary>Example code: templates_folding_6_elevated_fold.cpp</summary>

\include{lineno} templates_folding_6_elevated_fold.cpp

</details>

## Conical folding {#conical_fold}

A continuous fold profile with its upper edge cut by sloping planes (p. 224, Abgeschrägte Faltung; `folding/conical_fold.h`, example `templates_folding_7_conical_fold`). The folds run the whole span with level valleys and ridges `depth` high, and two planes sloping up from `end_depth` above the valleys at either end to the ridges at mid-span cut the top off. Below the cut every flank keeps its plane: a pentagon from the valley line up to its cut edge, which rises from the end to the ridge point at mid-span and falls again. Above it the cut is a thin triangle per fold and half span in the sloping plane, from the flat top of the fold at the end to the ridge point. So the end profiles are trapezoids and the middle a sharp zigzag, and in plan every fold is the elongated X Engel draws, the cut edges of its two flanks crossing at the ridge point.

![conical fold](templates/templates_folding_7_conical_fold_plates.webp)

![conical fold plan](templates/templates_folding_7_conical_fold_plan.webp)

![conical fold end](templates/templates_folding_7_conical_fold_end.webp)

```cpp
ConicalFold fold(
    4,         // folds
    1500.0,    // every flank across in plan
    1500.0,    // the ridges over the valleys at mid-span
    600.0,     // the cut over the valleys at either end
    14400.0,   // span
    120.0      // thickness
);
```

```cpp
// the flank pentagons, from the valley line up to the cut edge through the ridge point
face({Point(0.0, valley, 0.0), Point(0.0, valley + run, end_depth), peak, Point(span, valley + run, end_depth), Point(span, valley, 0.0)});
face({Point(0.0, next, 0.0), Point(0.0, next - run, end_depth), peak, Point(span, next - run, end_depth), Point(span, next, 0.0)});

// the cut triangles in the sloping planes, from the flat end of the fold to the ridge point
face({Point(0.0, valley + run, end_depth), Point(0.0, next - run, end_depth), peak});
face({Point(span, valley + run, end_depth), Point(span, next - run, end_depth), peak});
```

<details><summary>Example code: templates_folding_7_conical_fold.cpp</summary>

\include{lineno} templates_folding_7_conical_fold.cpp

</details>

## Folded frames and arches {#folded_frame}

A frame or an arch of triangular plates, bay by bay along y (`folding/folded_frame.h`, examples `templates_folding_8_two_hinged_frame`, `templates_folding_9_three_hinged_frame`, `templates_folding_10_three_hinged_arch` and the variants below). Engel (pp. 225-227): the folded surface becomes a linear structure system, its fold depth following the moments, small at a hinge, deep at a knee. Every system was read off the book's front elevation (the frame's outline: feet, knees, eaves, ridge, hinges), its side elevation (the diamonds and kites of the folds, where the depth is large and where it vanishes) and its plan (which diagonals run across, and where they cross). Half a bay is one fold line, a `zigzag` from the foot to the crown at mid-span between two rails: the bay's edge y 0, where the valleys lie, the frame's inner line, and the bay's centre y bay / 2, where the ridges lie, bulging out at the knees. Every point of the zigzag makes a planar triangle with the last point on each rail, so the plates are exactly the triangles of Engel's plans. The crown is the zigzag's last two points at mid-span; the other half of the span mirrors the zigzag, the other half of the bay mirrors the rails, and the bays repeat along y. All six systems together: 460 plates, every one a planar triangle 120 mm thick, cut back at the nodes by `node_cut_plates` and flush along both sides.

![two-hinged frame](templates/templates_folding_8_two_hinged_frame_plates.webp)

```cpp
FoldedFrame frame(
    FoldedFrame::two_hinged_frame(),   // the fold line of half a bay: foot, knee, eaves corner, ridge top, ceiling peak
    4500.0,                            // bay
    4,                                 // bays
    120.0                              // thickness
);
```

| Engel | `FoldedFrame::` | the fold line | example |
| --- | --- | --- | --- |
| two-hinged frame, ridge-to-ridge folding (p. 225) | `two_hinged_frame()` | foot (a line along the bay edge), knee bulging out, eaves corner, ridge top at mid-span, ceiling peak under it: a vertical plate at every bay edge, the roof folded from the eaves corners to the ridge | `templates_folding_8_two_hinged_frame` |
| two-hinged frame, ridge-to-valley folding (p. 225) | `two_hinged_frame(..., Folding::ridge_to_valley)` | point foot, eaves corner, knee, ceiling peak, ridge top: two plates per column on a point, the roof a fan from the knee to the mid-span corners | `templates_folding_33_frame_ridge_to_valley` |
| three-hinged frame, ridge folding (p. 226) | `three_hinged_frame()` | point foot, eaves corner, knee, ridge over the quarter point, mid-span ceiling corner, hinge: the diagonals from the eaves cross at the quarter-point ridge | `templates_folding_9_three_hinged_frame` |
| two-hinged A-frame, ridge-to-valley folding (p. 226) | `a_frame()` | foot at the bay edge, knee, waist at the bay centre, apex, ridge: the diagonals from the feet cross at the waist, a star of six plates | `templates_folding_34_a_frame` |
| arch with a top hinge (p. 227) | `top_hinged_arch()` | foot, trunk valley, knee, haunch valley, ridge peak over 0.36 of the span, its valley, the hinge at mid-span | `templates_folding_35_pointed_arch` |
| three-hinged arch (p. 227) | `three_hinged_arch()` | foot, ankle, knee, haunch valley, haunch ridge, crown corners over 0.27 of the span, the hinge at mid-span | `templates_folding_10_three_hinged_arch` |

The section shows Engel's outline: the columns bulging out at the knee, the roof rising to its ridge over the flat ceiling peak:

![two-hinged frame section](templates/templates_folding_8_two_hinged_frame_section.webp)

From the side the folds are his diamonds, deepest at the knees, and from above his plan, the ridge running straight from the knee through the eaves corner to the ridge top:

![two-hinged frame side](templates/templates_folding_8_two_hinged_frame_side.webp)

![two-hinged frame plan](templates/templates_folding_8_two_hinged_frame_plan.webp)

```cpp
for (size_t i = 0; i < count; i++) {
    const Point& point = zigzag[i];

    if (valley && ridge)
        triangles.push_back({*valley, *ridge, point});

    if (std::abs(point[1] - 0.5 * bay) < std::abs(point[1]))
        ridge = point;
    else
        valley = point;
}
```

### Ridge to valley

The same portal with the other fold line (p. 225, bottom): the columns stand on points at the bay centres and the roof folds from the knee straight to the ceiling peak, the valley, so the lines of the plan run from the knee to the mid-span corners.

![frame ridge to valley](templates/templates_folding_33_frame_ridge_to_valley_plates.webp)

![frame ridge to valley plan](templates/templates_folding_33_frame_ridge_to_valley_plan.webp)

### Three-hinged frame

The roof peaks over the quarter points of the span and dips to the hinge at mid-span over a flat ceiling (p. 226, top); in plan the diagonals from the eaves corners cross at the quarter-point ridge, the star of Engel's drawing.

![three-hinged frame](templates/templates_folding_9_three_hinged_frame_plates.webp)

![three-hinged frame section](templates/templates_folding_9_three_hinged_frame_section.webp)

### A-frame

Two legs from feet at the bay edges to the apex, their diagonals crossing at the waist, the star of six plates of Engel's plan (p. 226, bottom); the kites of his side elevation hang from the ridge.

![a frame](templates/templates_folding_34_a_frame_plates.webp)

![a frame section](templates/templates_folding_34_a_frame_section.webp)

### Arches

The arch with a top hinge (p. 227, top): nearly vertical legs, a narrow trunk, the ridge peaking either side of the hinge at mid-span. The three-hinged arch (p. 227, bottom): an octagonal arch on point feet, its ankles' level line, the crown's ridge and valley meeting in one line along the bays at the hinge, the big X of Engel's plan at mid-span.

![pointed arch](templates/templates_folding_35_pointed_arch_plates.webp)

![pointed arch section](templates/templates_folding_35_pointed_arch_section.webp)

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
    120.0                                           // thickness
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

Where three or more plates meet at a node, the bisector planes of the folds around it need not meet in one line, so their corners would pass through each other. `node_cut_plates` (`template_chamfer.h`) cuts each corner individually and only as far as needed: for every other plate at the node it takes the plane through the node halving the two plates' planes, which separates them as the miter plane separates two neighbours, and cuts the corner by the deepest such plane it crosses, on both outlines; a corner crossing none, every corner of a two-plate node and every corner where the plates meet in one point, as at a pyramid's apex, stays sharp. Plates on both sides of every bisector cannot meet. Before that, neighbouring coplanar faces are merged into one plate (`merged_coplanar`): two coplanar triangles become one bigger triangle, and a vertex left straight on the merged outline is dropped unless another plate still needs it.

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
    4500.0,    // centre
    120.0,     // thickness
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
    120.0      // thickness
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
    120.0       // thickness
);
```

![square](templates/templates_folding_20_triangle_square_plates.webp)

![triangle](templates/templates_folding_21_triangle_triangle_plates.webp)

![pentagon](templates/templates_folding_22_triangle_pentagon_plates.webp)

![hexagon](templates/templates_folding_23_triangle_hexagon_plates.webp)

<details><summary>Example code: templates_folding_20_triangle_square.cpp</summary>

\include{lineno} templates_folding_20_triangle_square.cpp

</details>

## Curved folding {#curved_fold}

The folded tent roof of Knie's Zauberhut in Rapperswil (Blumer Lehmann with Carlos Martinez Architekten, 2020; [project page](https://www.blumer-lehmann.com/bauprojekte-realisieren/bauen/free-form/knies-zauberhut.html)): a freestanding folded shell of 32 m diameter rising into a tower 26 m high with a round top of 7 m, its valleys running down to concrete columns, its ridges ending in a gable over the eaves between them (`folding/curved_fold.h`, example `templates_folding_37_curved_fold`). Every ridge and valley is a smooth NURBS curve, a meridian interpolated through rings, each ring the same star of ridge and valley points scaled by the trumpet curve's radius, flat at the eaves and vertical at the tower, and lifted to its height. Every fold strip between two neighbouring meridians is one lamella, a [`PlateCurved`](@ref elements_plate_curved) in the group `lamellas`, named `lamella_<j>`: a closed curved solid of six exact NURBS patches, with no facet and no triangle, as the 5 cm sheathing on the uniaxially curved glulam of the built roof. Its outer face is the ruled surface between the two meridians, its inner face the one between the inner meridians, where the two neighbouring strips offset by the thickness meet; its side faces run along the folds, so two neighbours share their side face as mitered planar plates do, and a cap closes it at the eaves and at the top.

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

![curved fold](templates/templates_folding_37_curved_fold_lamellas.webp)

![curved fold elevation](templates/templates_folding_37_curved_fold_elevation.webp)

<details><summary>Example code: templates_folding_37_curved_fold.cpp</summary>

\include{lineno} templates_folding_37_curved_fold.cpp

</details>

## Every topology by page {#folding_variants}

The remaining systems of Engel's pages, each an example of the templates above with its own stations, frame line, heights or folding:

| page | system | example |
| --- | --- | --- |
| 220 | one fold: the edge stresses at the common fold keep their tendency | `templates_folding_25_one_fold` |
| 220 | three folds: the side folds compensate each other through shear | `templates_folding_26_three_folds` |
| 222 | counter profile in the centre, ridge to valley | `templates_folding_27_counter_centre` |
| 222 | counter-running folding, ridge to valley | `templates_folding_28_ridge_to_valley` |
| 223 | centre fold elevated, ridge to valley | `templates_folding_29_elevated_ridge_to_valley` |
| 223 | centre fold elevated, alternate folding | `templates_folding_30_alternate_fold` |
| 224 | elevated centre profile deeper than the edge profiles, ridge to ridge | `templates_folding_31_deep_centre` |
| 224 | elevated centre profile deeper, ridge to valley | `templates_folding_32_deep_centre_valley` |
| 225 | two-hinged frame, ridge to valley | `templates_folding_33_frame_ridge_to_valley` |
| 226 | two-hinged A-frame, ridge to valley | `templates_folding_34_a_frame` |
| 227 | arch with a top hinge, the ridge peaking either side of it | `templates_folding_35_pointed_arch` |
| 233 | variation of the truncated pyramid: the side trapezoids folded from their raised outer midpoints (`PolygonFold` `edge_lift`) | `templates_folding_36_folded_truncated_pyramid` |

![25_one_fold](templates/templates_folding_25_one_fold_plates.webp)

![26_three_folds](templates/templates_folding_26_three_folds_plates.webp)

![27_counter_centre](templates/templates_folding_27_counter_centre_plates.webp)

![28_ridge_to_valley](templates/templates_folding_28_ridge_to_valley_plates.webp)

![29_elevated_ridge_to_valley](templates/templates_folding_29_elevated_ridge_to_valley_plates.webp)

![30_alternate_fold](templates/templates_folding_30_alternate_fold_plates.webp)

![31_deep_centre](templates/templates_folding_31_deep_centre_plates.webp)

![32_deep_centre_valley](templates/templates_folding_32_deep_centre_valley_plates.webp)

![33_frame_ridge_to_valley](templates/templates_folding_33_frame_ridge_to_valley_plates.webp)

![34_a_frame](templates/templates_folding_34_a_frame_plates.webp)

![35_pointed_arch](templates/templates_folding_35_pointed_arch_plates.webp)

![36_folded_truncated_pyramid](templates/templates_folding_36_folded_truncated_pyramid_plates.webp)

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
