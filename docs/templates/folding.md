# Folding flat {#templates_folding}

[TOC]

Planar folded-plate timber structures (`src/templates/folding/*.h`). Each template subclasses `WoodSession` and offsets a planar-faced mid-surface mesh into mitered `Plate` solids (`offset_plates` / `mitered_plates`).

![Folding Geometry](diagrams/wood_folding_geometry.svg)

| Geometric Invariant | Construction (`template_chamfer.h`) |
| --- | --- |
| **Planarity with thickness** | Outer face $P_f$ offset by `thickness` along normal $n_f$; fold side cut on the angle-bisector plane shared with the neighbour |
| **Multi-plate node offset (`offset_plates`)** | Valence-$k > 3$ vertices split into valence-3 inner intersections by optimal cycle triangulation so all plates meet on exact planar faces without gaps or chamfers |
| **Coplanar merge (`merged_coplanar`)** | Adjacent triangles sharing a coplanar crease are merged into a single polygon plate before offsetting |

| Geometry | Section |
| --- | --- |
| singly folded plate: threefold action, the number of folds, stiffening of the profile and of the free edges | [Singly folded plate](#prismatic_fold) |
| counter-running folding: the counter profile at the end or in the centre, the centre elevated, the elevated centre deeper; ridge to ridge, ridge to valley, alternate | [Counter-running folding](#counter_fold) |
| conical folding: a continuous fold profile with its upper edge cut by sloping planes | [Conical folding](#conical_fold) |
| linear structure systems of folded surfaces: two- and three-hinged frames, the A-frame, arches | [Folded frames and arches](#folded_frame) |
| singly folded surfaces over triangular, square and hexagonal plans | [Singly folded surfaces](#single_fold) |
| cross-folded surfaces spanned diagonally: ridges folded upwardly, rising or sloping toward the centre | [Cross-folded surfaces](#diagonal_fold) |
| composition of cross-folded squares with a central star | [Composition](#fold_composition) |
| the pyramid, the doubly folded truncated pyramid and its three variations | [Pyramids](#truncated_pyramid) |
| folded triangular surfaces over square, triangular, pentagonal and hexagonal plans | [Folded triangular surfaces](#triangle_fold) |
| a free-form zigzag carried along an arch | [Reflex fold](#reflex_fold) |
| a free-form fold of rhombi on a surface | [Diamond mesh](#diamond_mesh) |

## Singly folded plate {#prismatic_fold}

`PrismaticFold(profile, span, thickness, diaphragms, edge_flange)` (`folding/prismatic_fold.h`): sweeps `zigzag(ridges, fold_width, depth)` or `polygon(folds, width, rise)` along $x$ over `span` with transverse `diaphragms` and free-edge buckling flanges (`edge_flange`).

### Zigzag with diaphragms and edge flanges {#folding_prismatic_fold}

The singly folded plate with the defaults: four ridges, eight plates 1500 across and 1200 deep over a 12 m span, diaphragms at both ends and mid-span, a 400 flange folded down at each free edge.

![prismatic fold](templates/templates_folding_3_prismatic_fold_all.webp)

```cpp
PrismaticFold fold;
```

<details><summary>Example code: templates_folding_3_prismatic_fold.cpp</summary>

\include{lineno} templates_folding_3_prismatic_fold.cpp

</details>

### One fold {#folding_one_fold}

The system with one fold: two plates on a 12 m arch rising 3000 meet at one ridge, the edge stresses at the common fold keeping their tendency; a diaphragm at each end.

![one fold](templates/templates_folding_25_one_fold_plates.webp)

```cpp
PrismaticFold fold(PrismaticFold::polygon(1, 12000.0, 3000.0), 12000.0, 120.0, 2, 0.0, "one_fold");
```

<details><summary>Example code: templates_folding_25_one_fold.cpp</summary>

\include{lineno} templates_folding_25_one_fold.cpp

</details>

### Three folds {#folding_three_folds}

The system with three folds: four plates on an arch rising 3500, the edge stresses at the side folds opposing each other and compensating through shear; a diaphragm at each end.

![three folds](templates/templates_folding_26_three_folds_plates.webp)

```cpp
PrismaticFold fold(PrismaticFold::polygon(3, 12000.0, 3500.0), 12000.0, 120.0, 2, 0.0, "three_folds");
```

<details><summary>Example code: templates_folding_26_three_folds.cpp</summary>

\include{lineno} templates_folding_26_three_folds.cpp

</details>

### Many folds: the folded barrel {#folding_folded_barrel}

The system with many folds: seven folds on a circular arc 12 m wide and 4 m high, its form and action approaching a cylindrical shell, a diaphragm at each end and no edge flanges.

![folded barrel](templates/templates_folding_4_folded_barrel_all.webp)

```cpp
PrismaticFold barrel(PrismaticFold::polygon(7, 12000.0, 4000.0), 12000.0, 120.0, 2, 0.0, "folded_barrel");
```

<details><summary>Example code: templates_folding_4_folded_barrel.cpp</summary>

\include{lineno} templates_folding_4_folded_barrel.cpp

</details>

## Counter-running folding {#counter_fold}

`CounterFold(stations, folding, thickness)` (`folding/counter_fold.h`): triangulates non-planar strips between station profiles via `Folding::ridge_to_ridge`, `Folding::ridge_to_valley`, or `Folding::alternate`, merging coplanar triangles (`merge_coplanar`). Stations: `counter_running`, `counter_centre`, `elevated_centre`.

### Counter-running, ridge to ridge {#folding_counter_fold}

Counter-running folding of the same depth and elevation: a zigzag of four ridges at the start of a 13.5 m span, its counter profile at the end, every strip folded through its high corners, the strips crossing in the X's's end elevation; 16 plates.

![counter fold](templates/templates_folding_5_counter_fold_plates.webp)

```cpp
CounterFold fold;
```

<details><summary>Example code: templates_folding_5_counter_fold.cpp</summary>

\include{lineno} templates_folding_5_counter_fold.cpp

</details>

### Counter-running, ridge to valley {#folding_ridge_to_valley}

The same stations folded ridge to valley: every strip folded through its low corners, a valley running from the valley of one end to the valley of the other, the pyramids's end elevation; 16 plates.

![ridge to valley](templates/templates_folding_28_ridge_to_valley_plates.webp)

```cpp
CounterFold fold(CounterFold::counter_running(4, 1500.0, 1500.0, 13500.0), Folding::ridge_to_valley, 120.0, "ridge_to_valley");
```

<details><summary>Example code: templates_folding_28_ridge_to_valley.cpp</summary>

\include{lineno} templates_folding_28_ridge_to_valley.cpp

</details>

### Counter profile in the centre {#folding_counter_centre}

The counter profile in the centre, ridge to valley: the zigzag at both ends, its counter profile at mid-span, the diagonals running from the ends' valleys to the centre's and crossing there in the stars's plan, one at every second fold line; 32 plates.

![counter centre](templates/templates_folding_27_counter_centre_plates.webp)

```cpp
CounterFold fold(CounterFold::counter_centre(4, 1500.0, 1500.0, 13500.0), Folding::ridge_to_valley, 120.0, "counter_centre");
```

<details><summary>Example code: templates_folding_27_counter_centre.cpp</summary>

\include{lineno} templates_folding_27_counter_centre.cpp

</details>

### Elevated centre, ridge to ridge {#folding_elevated_fold}

The centre fold elevated over the edge folds with identical depth: the counter profile at mid-span lifted by the depth, so the even fold lines run straight through the 15 m span and the odd ones rise twice the depth; the triangles either side of a straight line are one plate, so the centre creases only at the odd lines; 24 plates.

![elevated fold](templates/templates_folding_6_elevated_fold_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::ridge_to_ridge, 120.0, "elevated_fold");
```

<details><summary>Example code: templates_folding_6_elevated_fold.cpp</summary>

\include{lineno} templates_folding_6_elevated_fold.cpp

</details>

### Elevated centre, ridge to valley {#folding_elevated_ridge_to_valley}

The same stations folded ridge to valley: the diagonals run from the ends' valleys to the centre's even lines and cross there, and the centre creases at every fold, the vertical line's plan; 32 plates.

![elevated ridge to valley](templates/templates_folding_29_elevated_ridge_to_valley_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::ridge_to_valley, 120.0, "elevated_ridge_to_valley");
```

<details><summary>Example code: templates_folding_29_elevated_ridge_to_valley.cpp</summary>

\include{lineno} templates_folding_29_elevated_ridge_to_valley.cpp

</details>

### Elevated centre, alternate folding {#folding_alternate_fold}

The same stations with alternate folding: ridge to valley and ridge to ridge fold by fold, the edge fold first, so half the straight lines and half the centre's creases vanish into planar quads, the diamonds's end elevation without a line through them; 20 plates.

![alternate fold](templates/templates_folding_30_alternate_fold_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::alternate, 120.0, "alternate_fold");
```

<details><summary>Example code: templates_folding_30_alternate_fold.cpp</summary>

\include{lineno} templates_folding_30_alternate_fold.cpp

</details>

### Deeper centre, ridge to ridge {#folding_deep_centre}

The elevated centre profile deeper than the edge profiles: the ends 1000 deep and the centre 1800, so the odd fold lines rise 2800 and the end elevation shows the tall diamonds; 24 plates.

![deep centre](templates/templates_folding_31_deep_centre_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1000.0, 15000.0, 1800.0), Folding::ridge_to_ridge, 120.0, "deep_centre");
```

<details><summary>Example code: templates_folding_31_deep_centre.cpp</summary>

\include{lineno} templates_folding_31_deep_centre.cpp

</details>

### Deeper centre, ridge to valley {#folding_deep_centre_valley}

The same stations folded ridge to valley: the diagonals from the ends' valleys to the centre's, crossing at the centre on the even lines, where the centre creases at every fold; 32 plates.

![deep centre valley](templates/templates_folding_32_deep_centre_valley_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1000.0, 15000.0, 1800.0), Folding::ridge_to_valley, 120.0, "deep_centre_valley");
```

<details><summary>Example code: templates_folding_32_deep_centre_valley.cpp</summary>

\include{lineno} templates_folding_32_deep_centre_valley.cpp

</details>

## Conical folding {#conical_fold}

`ConicalFold(folds, fold_width, depth, end_depth, span, thickness)` (`folding/conical_fold.h`): truncates a continuous zigzag of height `depth` by two planes sloping from `end_depth` at the supports to full `depth` at mid-span.

### Conical fold {#folding_conical_fold}

Conical folding with the defaults: four folds over 14.4 m, their ridges 1500 high at mid-span cut down to 600 at the ends; 16 plates.

![conical fold](templates/templates_folding_7_conical_fold_plates.webp)

```cpp
ConicalFold fold;
```

<details><summary>Example code: templates_folding_7_conical_fold.cpp</summary>

\include{lineno} templates_folding_7_conical_fold.cpp

</details>

## Folded frames and arches {#folded_frame}

`FoldedFrame(zigzag, bay, bays, thickness)` (`folding/folded_frame.h`): triangulated portal frames and arches with moment-scaled knee depth (`two_hinged_frame`, `three_hinged_frame`, `a_frame`, `top_hinged_arch`, `three_hinged_arch`).

### Two-hinged frame, ridge to ridge {#folding_two_hinged_frame}

The two-hinged frame with ridge-to-ridge folding: a 30 m portal of four 4.5 m bays on line feet, its columns bulging out at the knee, a vertical plate at every bay edge, the roof folded from the eaves corners to the ridge top; 64 plates.

![two-hinged frame](templates/templates_folding_8_two_hinged_frame_plates.webp)

```cpp
FoldedFrame frame;
```

<details><summary>Example code: templates_folding_8_two_hinged_frame.cpp</summary>

\include{lineno} templates_folding_8_two_hinged_frame.cpp

</details>

### Three-hinged frame {#folding_three_hinged_frame}

The three-hinged frame with ridge folding: four 4.25 m bays on point feet, the roof peaking over the quarter points of the span and dipping to the hinge at mid-span over a flat ceiling, the diagonals from the eaves crossing at the quarter-point ridge; 64 plates.

![three-hinged frame](templates/templates_folding_9_three_hinged_frame_plates.webp)

```cpp
FoldedFrame frame(FoldedFrame::three_hinged_frame(), 4250.0, 4, 120.0, "three_hinged_frame");
```

<details><summary>Example code: templates_folding_9_three_hinged_frame.cpp</summary>

\include{lineno} templates_folding_9_three_hinged_frame.cpp

</details>

### A-frame {#folding_a_frame}

The two-hinged A-frame with ridge-to-valley folding: legs 21 m apart meeting at an apex 12.7 m high under a ridge 18.1 m high, their diagonals crossing at the waist in the star of six plates's plan; 48 plates.

![a frame](templates/templates_folding_34_a_frame_plates.webp)

```cpp
FoldedFrame frame(FoldedFrame::a_frame(), 3850.0, 4, 120.0, "a_frame");
```

<details><summary>Example code: templates_folding_34_a_frame.cpp</summary>

\include{lineno} templates_folding_34_a_frame.cpp

</details>

### Arch with a top hinge {#folding_pointed_arch}

The arch with a hinge at the crown: four 4.95 m bays over a 17.9 m span, 26.1 m high, nearly vertical legs on a narrow trunk, the ridge peaking either side of the hinge at mid-span; 96 plates.

![pointed arch](templates/templates_folding_35_pointed_arch_plates.webp)

```cpp
FoldedFrame frame(FoldedFrame::top_hinged_arch(), 4950.0, 4, 120.0, "pointed_arch");
```

<details><summary>Example code: templates_folding_35_pointed_arch.cpp</summary>

\include{lineno} templates_folding_35_pointed_arch.cpp

</details>

### Three-hinged arch {#folding_three_hinged_arch}

The three-hinged arch: an octagonal arch 21.5 m wide and 20.5 m high on point feet over five 5 m bays, hinged at the crown, where ridge and valley meet in one line, the big X's plan; 140 plates.

![three-hinged arch](templates/templates_folding_10_three_hinged_arch_plates.webp)

```cpp
FoldedFrame frame(FoldedFrame::three_hinged_arch(), 5000.0, 5, 120.0, "three_hinged_arch");
```

<details><summary>Example code: templates_folding_10_three_hinged_arch.cpp</summary>

\include{lineno} templates_folding_10_three_hinged_arch.cpp

</details>

## Singly folded surfaces over a polygon plan {#single_fold}

`SingleFold` (`folding/single_fold.h`): over a regular polygon plan standing on its corners, every sector between two corner-to-centre lines is folded once, on the ridge from its edge middle to the centre, two triangles; the edge middle is raised, so an eave triangle rises from each ground edge to it: leaning out (`overhang` > 0, a star-shaped outline), vertical (a gable) or leaning in. The ridges are level, rising or falling to the centre.

### Triangular plan, horizontal ridges {#folding_triangle_fold}

Three sectors, the ridges level from edge middle to centre at 7 m, the eave triangles leaning 2.9 m out over the corners on the ground, a star-shaped outline; 9 plates.

![triangular plan, horizontal ridges](templates/templates_folding_11_triangle_fold_plates.webp)

```cpp
SingleFold fold(3, 12000.0, 2900.0, 7000.0, 7000.0, 120.0, "triangle_fold");
```

<details><summary>Example code: templates_folding_11_triangle_fold.cpp</summary>

\include{lineno} templates_folding_11_triangle_fold.cpp

</details>

### Square plan, ridges rising toward the centre {#folding_square_fold}

A vertical gable on every edge to 5.4 m, the ridges rising to the apex at 6.9 m; 12 plates.

![square plan, ridges rising toward the centre](templates/templates_folding_12_square_fold_plates.webp)

```cpp
SingleFold fold;
```

<details><summary>Example code: templates_folding_12_square_fold.cpp</summary>

\include{lineno} templates_folding_12_square_fold.cpp

</details>

### Hexagonal plan, ridges sloping to the centre {#folding_hexagon_fold}

The eave triangles leaning 2.1 m inward to peaks at 8.3 m, the ridges falling to the centre at 5.2 m; 18 plates.

![hexagonal plan, ridges sloping to the centre](templates/templates_folding_13_hexagon_fold_plates.webp)

```cpp
SingleFold fold(6, 12000.0, -2100.0, 8300.0, 5200.0, 120.0, "hexagon_fold");
```

<details><summary>Example code: templates_folding_13_hexagon_fold.cpp</summary>

\include{lineno} templates_folding_13_hexagon_fold.cpp

</details>

## Cross-folded surfaces spanned diagonally {#diagonal_fold}

`DiagonalFold` (`folding/diagonal_fold.h`): a square quartered by the cross from its edge middles, the edge middles on the ground; in each quarter the diagonal ridge from the corner to the centre is folded at one point, so a quarter is four triangles and the plan shows chevrons. `DiagonalHeights` sets the corner, the edge middle, the fold point, its position along the diagonal and the centre.

### Ridges folded upwardly {#folding_cross_fold}

The diagonal ridges folded up at their middles to 6.2 m, the corners and the centre at 3.3 m; 16 plates.

![ridges folded upwardly](templates/templates_folding_16_cross_fold_plates.webp)

```cpp
DiagonalFold fold;
```

<details><summary>Example code: templates_folding_16_cross_fold.cpp</summary>

\include{lineno} templates_folding_16_cross_fold.cpp

</details>

### Ridges rising toward the centre {#folding_cross_fold_rising}

The corners at 3.1 m, the fold point two thirds along at 4.9 m, the apex at 9 m; 16 plates.

![ridges rising toward the centre](templates/templates_folding_17_cross_fold_rising_plates.webp)

```cpp
DiagonalFold fold(18000.0, {3100.0, 0.0, 4900.0, 0.67, 9000.0}, 120.0, "cross_fold_rising");
```

<details><summary>Example code: templates_folding_17_cross_fold_rising.cpp</summary>

\include{lineno} templates_folding_17_cross_fold_rising.cpp

</details>

### Ridges sloping toward the centre {#folding_cross_fold_sloping}

The corners highest at 7 m, the fold point at 4.3 m, the centre at 3.5 m; 16 plates.

![ridges sloping toward the centre](templates/templates_folding_18_cross_fold_sloping_plates.webp)

```cpp
DiagonalFold fold(18000.0, {7000.0, 0.0, 4300.0, 0.43, 3500.0}, 120.0, "cross_fold_sloping");
```

<details><summary>Example code: templates_folding_18_cross_fold_sloping.cpp</summary>

\include{lineno} templates_folding_18_cross_fold_sloping.cpp

</details>

## Composition of cross-folded squares {#fold_composition}

`FoldComposition` (`folding/fold_composition.h`): a field of square cells, each folded on its diagonals to a peak, the nodes alternating between the ground and a height as a chessboard, and one block of cells carrying a taller upward-folded star.

### Composition with a central star {#folding_fold_composition}

Six by four cells of 12 m folded to peaks on a chessboard of ground and 4.6 m nodes, the middle two by two cells carrying one star 15 m high; 96 plates.

![composition with a central star](templates/templates_folding_19_fold_composition_plates.webp)

```cpp
FoldComposition fold;
```

<details><summary>Example code: templates_folding_19_fold_composition.cpp</summary>

\include{lineno} templates_folding_19_fold_composition.cpp

</details>

## Pyramids and truncated pyramids {#truncated_pyramid}

`TruncatedPyramid` (`folding/truncated_pyramid.h`): a pyramid or truncated pyramid over a square, optionally folded further: `edge_fold` pulls each base edge's middle in and raises it (`FoldPoint {along, rise}`), `corner_fold` bends each corner line at a raised point; the three variations of the doubly folded truncated pyramid.

### Pyramid {#folding_pyramid}

Four triangles over an 18 m square rising to an apex at 6 m; 4 plates.

![pyramid](templates/templates_folding_14_pyramid_plates.webp)

```cpp
TruncatedPyramid fold(9000.0, 0.0, 6000.0, std::nullopt, std::nullopt, 120.0, "pyramid");
```

<details><summary>Example code: templates_folding_14_pyramid.cpp</summary>

\include{lineno} templates_folding_14_pyramid.cpp

</details>

### Truncated pyramid, the basic form {#folding_truncated_pyramid}

Four trapezoids and a flat 6 m top 6.8 m high over an 18 m square; 5 plates.

![truncated pyramid, the basic form](templates/templates_folding_15_truncated_pyramid_plates.webp)

```cpp
TruncatedPyramid fold;
```

<details><summary>Example code: templates_folding_15_truncated_pyramid.cpp</summary>

\include{lineno} templates_folding_15_truncated_pyramid.cpp

</details>

### Variation 1: folded edges {#folding_folded_truncated_pyramid}

Each base edge's middle pulled in a fifth and raised to 54 %, a fan of three triangles above it and a steep triangle below; 17 plates.

![variation 1: folded edges](templates/templates_folding_36_folded_truncated_pyramid_plates.webp)

```cpp
TruncatedPyramid fold(9000.0, 3000.0, 6800.0, FoldPoint{0.21, 0.54}, std::nullopt, 120.0, "folded_truncated_pyramid");
```

<details><summary>Example code: templates_folding_36_folded_truncated_pyramid.cpp</summary>

\include{lineno} templates_folding_36_folded_truncated_pyramid.cpp

</details>

### Variation 2: folded corners {#folding_truncated_pyramid_corner_folds}

The corner lines bent at a fifth of their length, raised to half height, the sides fanning from the base middle on the ground; 21 plates.

![variation 2: folded corners](templates/templates_folding_38_truncated_pyramid_corner_folds_plates.webp)

```cpp
TruncatedPyramid fold(9000.0, 3000.0, 6800.0, FoldPoint{0.07, 0.0}, FoldPoint{0.22, 0.5}, 120.0, "truncated_pyramid_corner_folds");
```

<details><summary>Example code: templates_folding_38_truncated_pyramid_corner_folds.cpp</summary>

\include{lineno} templates_folding_38_truncated_pyramid_corner_folds.cpp

</details>

### Variation 3: folded corners and edges {#folding_truncated_pyramid_corner_edge_folds}

The corner bends and the raised edge middles at the same half height, a level fold ring; 25 plates.

![variation 3: folded corners and edges](templates/templates_folding_39_truncated_pyramid_corner_edge_folds_plates.webp)

```cpp
TruncatedPyramid fold(9000.0, 3000.0, 6800.0, FoldPoint{0.28, 0.5}, FoldPoint{0.22, 0.5}, 120.0, "truncated_pyramid_corner_edge_folds");
```

<details><summary>Example code: templates_folding_39_truncated_pyramid_corner_edge_folds.cpp</summary>

\include{lineno} templates_folding_39_truncated_pyramid_corner_edge_folds.cpp

</details>

## Folded triangular surfaces {#triangle_fold}

`TriangleFold` (`folding/triangle_fold.h`): a regular polygon plan cut by the lines joining its edge middles; a vertical gable stands on every edge up to its raised middle, the corner triangles fold down to the corners on the ground, and the polygon of the middles is split into triangles to a raised centre, or stays one flat face.

### Square plan {#folding_triangle_square}

Gables to 5.0 m, the centre at 6.5 m; 12 plates.

![square plan](templates/templates_folding_20_triangle_square_plates.webp)

```cpp
TriangleFold fold;
```

<details><summary>Example code: templates_folding_20_triangle_square.cpp</summary>

\include{lineno} templates_folding_20_triangle_square.cpp

</details>

### Triangular plan {#folding_triangle_triangle}

Gables to 7.5 m, the middle triangle flat; 7 plates.

![triangular plan](templates/templates_folding_21_triangle_triangle_plates.webp)

```cpp
TriangleFold fold(3, 12000.0, 7500.0, 7500.0, 120.0, "triangle_triangle");
```

<details><summary>Example code: templates_folding_21_triangle_triangle.cpp</summary>

\include{lineno} templates_folding_21_triangle_triangle.cpp

</details>

### Pentagonal plan {#folding_triangle_pentagon}

Gables to 4.8 m, the centre at 6.0 m; 15 plates.

![pentagonal plan](templates/templates_folding_22_triangle_pentagon_plates.webp)

```cpp
TriangleFold fold(5, 12000.0, 4800.0, 6000.0, 120.0, "triangle_pentagon");
```

<details><summary>Example code: templates_folding_22_triangle_pentagon.cpp</summary>

\include{lineno} templates_folding_22_triangle_pentagon.cpp

</details>

### Hexagonal plan {#folding_triangle_hexagon}

Gables to 4.2 m, the centre at 5.3 m; 18 plates.

![hexagonal plan](templates/templates_folding_23_triangle_hexagon_plates.webp)

```cpp
TriangleFold fold(6, 12000.0, 4200.0, 5300.0, 120.0, "triangle_hexagon");
```

<details><summary>Example code: templates_folding_23_triangle_hexagon.cpp</summary>

\include{lineno} templates_folding_23_triangle_hexagon.cpp

</details>

## Reflex fold {#reflex_fold}

`ReflexFold(cross_section, profile, thickness, chamfer_bottom, chamfer_top, chamfer_angle)` (`folding/reflex_fold.h`): carries a zigzag `profile` along an arch `cross_section`, folding at each arch vertex bisector plane (`mitered_plates`).

### Default arch and zigzag {#folding_reflex_fold}

The reflex fold with the defaults: the zigzag folded at every point of the arch, one 10 mm plate per quad with 20 mm chamfers.

![reflex fold](templates/templates_folding_1_reflex_fold_plates.webp)

```cpp
ReflexFold fold;
```

<details><summary>Example code: templates_folding_1_reflex_fold.cpp</summary>

\include{lineno} templates_folding_1_reflex_fold.cpp

</details>

## Diamond mesh {#diamond_mesh}

`DiamondMesh(surface, u_divisions, v_divisions, thickness, chamfer, chamfer_angle)` (`folding/diamond_mesh.h`) divides a NURBS surface into `u_divisions` by `v_divisions` cells and joins each cell's corners to its centre and the next cell's centre, so two triangles make a rhombus across every cell edge; every triangle becomes a plate mitered at its folds, its corners sharper than `chamfer_angle` chamfered by `chamfer` (`mitered_plates`). The default surface, `default_surface()`, is a bicubic arch 3000 across, 5000 long and 1500 high.

### Rhombi on the default arch {#folding_diamond_mesh}

The diamond mesh on the default arch: 8 by 4 cells split into rhombi between the cell centres, one 40 mm plate per triangle with 10 mm chamfers.

![diamond mesh](templates/templates_folding_2_diamond_mesh_plates.webp)

```cpp
DiamondMesh diamond(
    DiamondMesh::default_surface(),
    8,
    4,
    40.0,
    10.0,
    180.0
);
```

<details><summary>Example code: templates_folding_2_diamond_mesh.cpp</summary>

\include{lineno} templates_folding_2_diamond_mesh.cpp

</details>
