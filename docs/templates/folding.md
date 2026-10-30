# Folding Flat {#templates_folding}

[TOC]

Planar folded-plate timber structures in `src/templates/folding/*.h`. Each template builds a planar-faced mid-surface mesh and offsets it into mitered `Plate` solids.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#reflex_fold">
    <img src="templates_folding_1_reflex_fold_plates.webp" alt="Reflex Fold">
    <span class="wood-card-title">Reflex Fold</span>
  </a>
  <a class="wood-card" href="#diamond_mesh">
    <img src="templates_folding_2_diamond_mesh_plates.webp" alt="Diamond Mesh">
    <span class="wood-card-title">Diamond Mesh</span>
  </a>
  <a class="wood-card" href="#prismatic_fold">
    <img src="templates_folding_3_prismatic_fold_all.webp" alt="Prismatic Fold">
    <span class="wood-card-title">Prismatic Fold</span>
  </a>
  <a class="wood-card" href="#counter_fold">
    <img src="templates_folding_5_counter_fold_plates.webp" alt="Counter Fold">
    <span class="wood-card-title">Counter Fold</span>
  </a>
  <a class="wood-card" href="#conical_fold">
    <img src="templates_folding_7_conical_fold_plates.webp" alt="Conical Fold">
    <span class="wood-card-title">Conical Fold</span>
  </a>
  <a class="wood-card" href="#folded_frame">
    <img src="templates_folding_8_two_hinged_frame_plates.webp" alt="Folded Frame">
    <span class="wood-card-title">Folded Frame</span>
  </a>
  <a class="wood-card" href="#single_fold">
    <img src="templates_folding_12_square_fold_plates.webp" alt="Single Fold">
    <span class="wood-card-title">Single Fold</span>
  </a>
  <a class="wood-card" href="#diagonal_fold">
    <img src="templates_folding_16_cross_fold_plates.webp" alt="Diagonal Fold">
    <span class="wood-card-title">Diagonal Fold</span>
  </a>
  <a class="wood-card" href="#fold_composition">
    <img src="templates_folding_19_fold_composition_plates.webp" alt="Fold Composition">
    <span class="wood-card-title">Fold Composition</span>
  </a>
  <a class="wood-card" href="#truncated_pyramid">
    <img src="templates_folding_39_truncated_pyramid_corner_edge_folds_plates.webp" alt="Truncated Pyramid">
    <span class="wood-card-title">Truncated Pyramid</span>
  </a>
  <a class="wood-card" href="#triangle_fold">
    <img src="templates_folding_20_triangle_square_plates.webp" alt="Triangle Fold">
    <span class="wood-card-title">Triangle Fold</span>
  </a>
</div>

## Reflex fold {#reflex_fold}

`ReflexFold(cross_section, profile, thickness, chamfer_bottom, chamfer_top, chamfer_angle)` (`folding/reflex_fold.h`): sweeps a zigzag `profile` along an arch `cross_section` (two profile curves), folding at each arch vertex bisector plane (`mitered_plates`).

### Default arch and zigzag {#folding_reflex_fold}

Default reflex fold arch from two profile curves (`cross_section` and `profile`) with `10 mm` mitered plates and `20 mm` corner chamfers.

![](templates/templates_folding_1_reflex_fold_plates.webp)

```cpp
ReflexFold fold;
```

## Diamond mesh {#diamond_mesh}

`DiamondMesh(surface, u_divisions, v_divisions, thickness, chamfer, chamfer_angle)` (`folding/diamond_mesh.h`): divides a NURBS surface into `u_divisions × v_divisions` cells and connects each cell's corners to adjacent cell centres to form triangulated diamond rhombi (`mitered_plates`).

### Rhombi on the default arch {#folding_diamond_mesh}

An `8 × 4` diamond mesh on the default bicubic arch (`3000 × 5000 × 1500 mm`), with `40 mm` thick mitered plates and `10 mm` corner chamfers.

![](templates/templates_folding_2_diamond_mesh_plates.webp)

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

## Singly folded plate {#prismatic_fold}

`PrismaticFold(profile, span, thickness, diaphragms, edge_flange)` (`folding/prismatic_fold.h`): sweeps `zigzag(ridges, fold_width, depth)` or `polygon(folds, width, rise)` along `x` over `span` with transverse `diaphragms` and free-edge buckling flanges (`edge_flange`).

### Zigzag with diaphragms and edge flanges {#folding_prismatic_fold}

Four ridges and eight plates (`1500 mm` wide, `1200 mm` deep) over a `12 m` span, with transverse diaphragms at both ends and mid-span and a `400 mm` downward flange at each free edge.

![](templates/templates_folding_3_prismatic_fold_all.webp)

```cpp
PrismaticFold fold;
```

### One fold {#folding_one_fold}

Two plates over a `12 m` span rising `3000 mm` to a single central ridge, stiffened by a diaphragm at each end.

![](templates/templates_folding_25_one_fold_plates.webp)

```cpp
PrismaticFold fold(PrismaticFold::polygon(1, 12000.0, 3000.0), 12000.0, 120.0, 2, 0.0, "one_fold");
```

### Three folds {#folding_three_folds}

Four plates over a `12 m` arch rising `3500 mm`, with a diaphragm at each end.

![](templates/templates_folding_26_three_folds_plates.webp)

```cpp
PrismaticFold fold(PrismaticFold::polygon(3, 12000.0, 3500.0), 12000.0, 120.0, 2, 0.0, "three_folds");
```

### Many folds: the folded barrel {#folding_folded_barrel}

Seven folds inscribed on a circular arc `12 m` wide and `4 m` high, approaching a cylindrical barrel vault with a diaphragm at each end.

![](templates/templates_folding_4_folded_barrel_all.webp)

```cpp
PrismaticFold barrel(PrismaticFold::polygon(7, 12000.0, 4000.0), 12000.0, 120.0, 2, 0.0, "folded_barrel");
```

## Counter-running folding {#counter_fold}

`CounterFold(stations, folding, thickness)` (`folding/counter_fold.h`): triangulates non-planar strips between station profiles via `Folding::ridge_to_ridge`, `Folding::ridge_to_valley`, or `Folding::alternate`, merging coplanar triangles (`merged_coplanar`). Station generators: `counter_running`, `counter_centre`, `elevated_centre`.

### Counter-running, ridge to ridge {#folding_counter_fold}

A four-ridge zigzag at the start of a `13.5 m` span paired with its inverted counter-profile at the far end, triangulated through the high corners into an X-shaped end elevation (`16` plates).

![](templates/templates_folding_5_counter_fold_plates.webp)

```cpp
CounterFold fold;
```

### Counter-running, ridge to valley {#folding_ridge_to_valley}

The same counter-running stations triangulated through their low corners, running valleys diagonally from end to end with a pyramid-shaped end elevation (`16` plates).

![](templates/templates_folding_28_ridge_to_valley_plates.webp)

```cpp
CounterFold fold(CounterFold::counter_running(4, 1500.0, 1500.0, 13500.0), Folding::ridge_to_valley, 120.0, "ridge_to_valley");
```

### Counter profile in the centre {#folding_counter_centre}

Matching zigzag profiles at both ends with a counter-profile at mid-span, folded ridge-to-valley so the diagonals cross in a star pattern at every second fold line (`32` plates).

![](templates/templates_folding_27_counter_centre_plates.webp)

```cpp
CounterFold fold(CounterFold::counter_centre(4, 1500.0, 1500.0, 13500.0), Folding::ridge_to_valley, 120.0, "counter_centre");
```

### Elevated centre, ridge to ridge {#folding_elevated_fold}

Counter-profile at mid-span lifted by the fold depth (`1500 mm`) over a `15 m` span so even fold lines run level while odd fold lines rise to double depth; coplanar triangles merge across the straight lines (`24` plates).

![](templates/templates_folding_6_elevated_fold_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::ridge_to_ridge, 120.0, "elevated_fold");
```

### Elevated centre, ridge to valley {#folding_elevated_ridge_to_valley}

The same elevated-centre stations folded ridge-to-valley so diagonals run from the end valleys to the even lines at mid-span, creasing every fold along the centre (`32` plates).

![](templates/templates_folding_29_elevated_ridge_to_valley_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::ridge_to_valley, 120.0, "elevated_ridge_to_valley");
```

### Elevated centre, alternate folding {#folding_alternate_fold}

The same elevated-centre stations with `Folding::alternate` (alternating ridge-to-valley and ridge-to-ridge fold by fold), merging half of the centre creases into planar diamond quads (`20` plates).

![](templates/templates_folding_30_alternate_fold_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::alternate, 120.0, "alternate_fold");
```

### Deeper centre, ridge to ridge {#folding_deep_centre}

End profiles `1000 mm` deep paired with an elevated `1800 mm` centre profile so the odd fold lines rise `2800 mm` at mid-span (`24` plates).

![](templates/templates_folding_31_deep_centre_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1000.0, 15000.0, 1800.0), Folding::ridge_to_ridge, 120.0, "deep_centre");
```

### Deeper centre, ridge to valley {#folding_deep_centre_valley}

The same deeper-centre stations folded ridge-to-valley (`32` plates).

![](templates/templates_folding_32_deep_centre_valley_plates.webp)

```cpp
CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1000.0, 15000.0, 1800.0), Folding::ridge_to_valley, 120.0, "deep_centre_valley");
```

## Conical folding {#conical_fold}

`ConicalFold(folds, fold_width, depth, end_depth, span, thickness)` (`folding/conical_fold.h`): clips a continuous zigzag of height `depth` by two planes sloping from `end_depth` at the supports to full `depth` at mid-span.

### Conical fold {#folding_conical_fold}

Four folds over a `14.4 m` span with ridges rising from `600 mm` at the supports to `1500 mm` at mid-span (`16` plates).

![](templates/templates_folding_7_conical_fold_plates.webp)

```cpp
ConicalFold fold;
```

## Folded frames and arches {#folded_frame}

`FoldedFrame(zigzag, bay, bays, thickness)` (`folding/folded_frame.h`): triangulated portal frames and arches with moment-scaled knee depth (`two_hinged_frame`, `three_hinged_frame`, `a_frame`, `top_hinged_arch`, `three_hinged_arch`).

### Two-hinged frame, ridge to ridge {#folding_two_hinged_frame}

A `30 m` portal of four `4.5 m` bays on line feet with deepened knees and ridge-to-ridge roof folding (`64` plates).

![](templates/templates_folding_8_two_hinged_frame_plates.webp)

```cpp
FoldedFrame frame;
```

### Three-hinged frame {#folding_three_hinged_frame}

Four `4.25 m` bays on point feet, peaking over the quarter-span points and dipping to the crown hinge at mid-span (`64` plates).

![](templates/templates_folding_9_three_hinged_frame_plates.webp)

```cpp
FoldedFrame frame(FoldedFrame::three_hinged_frame(), 4250.0, 4, 120.0, "three_hinged_frame");
```

### A-frame {#folding_a_frame}

Two-hinged A-frame with ridge-to-valley folding: legs `21 m` apart meeting at an apex `12.7 m` high under a `18.1 m` ridge (`48` plates).

![](templates/templates_folding_34_a_frame_plates.webp)

```cpp
FoldedFrame frame(FoldedFrame::a_frame(), 3850.0, 4, 120.0, "a_frame");
```

### Arch with a top hinge {#folding_pointed_arch}

Four `4.95 m` bays over a `17.9 m` span and `26.1 m` height, with steep lower legs and a central crown hinge (`96` plates).

![](templates/templates_folding_35_pointed_arch_plates.webp)

```cpp
FoldedFrame frame(FoldedFrame::top_hinged_arch(), 4950.0, 4, 120.0, "pointed_arch");
```

### Three-hinged arch {#folding_three_hinged_arch}

Octagonal three-hinged arch (`21.5 m` span, `20.5 m` rise) on point feet across five `5 m` bays (`140` plates).

![](templates/templates_folding_10_three_hinged_arch_plates.webp)

```cpp
FoldedFrame frame(FoldedFrame::three_hinged_arch(), 5000.0, 5, 120.0, "three_hinged_arch");
```

## Singly folded surfaces over a polygon plan {#single_fold}

`SingleFold(sides, radius, overhang, eaves, centre, thickness)` (`folding/single_fold.h`): over a regular polygon plan supported at its corners, each sector is folded along the radial ridge from the mid-edge gable peak (`eaves`) to the central apex (`centre`), with the perimeter gable leaning outward (`overhang > 0`), standing vertical (`overhang = 0`), or leaning inward (`overhang < 0`).

### Triangular plan, horizontal ridges {#folding_triangle_fold}

Three sectors with horizontal ridges at `7 m` and eave triangles leaning `2.9 m` outward (`9` plates).

![](templates/templates_folding_11_triangle_fold_plates.webp)

```cpp
SingleFold fold(3, 12000.0, 2900.0, 7000.0, 7000.0, 120.0, "triangle_fold");
```

### Square plan, ridges rising toward the centre {#folding_square_fold}

Vertical perimeter gables rising to `5.4 m` with radial ridges climbing to `6.9 m` at the central apex (`12` plates).

![](templates/templates_folding_12_square_fold_plates.webp)

```cpp
SingleFold fold;
```

### Hexagonal plan, ridges sloping to the centre {#folding_hexagon_fold}

Eave triangles leaning `2.1 m` inward to `8.3 m` peaks with ridges sloping down to `5.2 m` at the centre (`18` plates).

![](templates/templates_folding_13_hexagon_fold_plates.webp)

```cpp
SingleFold fold(6, 12000.0, -2100.0, 8300.0, 5200.0, 120.0, "hexagon_fold");
```

## Cross-folded surfaces spanned diagonally {#diagonal_fold}

`DiagonalFold(span, heights, thickness)` (`folding/diagonal_fold.h`): a square plan supported at the four edge midpoints and quartered by chevron folds along both diagonals. `DiagonalHeights` sets the corner height, edge-middle height, intermediate diagonal fold height and position, and central apex height.

### Ridges folded upwardly {#folding_cross_fold}

Diagonal ridges folded upward to `6.2 m` at their midpoints, with corners and centre at `3.3 m` (`16` plates).

![](templates/templates_folding_16_cross_fold_plates.webp)

```cpp
DiagonalFold fold;
```

### Ridges rising toward the centre {#folding_cross_fold_rising}

Corners at `3.1 m`, intermediate fold point two-thirds along the diagonal at `4.9 m`, and central apex at `9 m` (`16` plates).

![](templates/templates_folding_17_cross_fold_rising_plates.webp)

```cpp
DiagonalFold fold(18000.0, {3100.0, 0.0, 4900.0, 0.67, 9000.0}, 120.0, "cross_fold_rising");
```

### Ridges sloping toward the centre {#folding_cross_fold_sloping}

Corners raised to `7 m`, intermediate fold point at `4.3 m`, and centre dipping to `3.5 m` (`16` plates).

![](templates/templates_folding_18_cross_fold_sloping_plates.webp)

```cpp
DiagonalFold fold(18000.0, {7000.0, 0.0, 4300.0, 0.43, 3500.0}, 120.0, "cross_fold_sloping");
```

## Composition of cross-folded squares {#fold_composition}

`FoldComposition` (`folding/fold_composition.h`): a `6 × 4` grid of `12 m` square cells folded on their diagonals over alternating ground and `4.6 m` nodes, with the central `2 × 2` bay carrying an elevated `15 m` 8-pointed star vault (`96` plates).

### Composition with a central star {#folding_fold_composition}

![](templates/templates_folding_19_fold_composition_plates.webp)

```cpp
FoldComposition fold;
```

## Pyramids and truncated pyramids {#truncated_pyramid}

`TruncatedPyramid(half_base, half_top, height, edge_fold, corner_fold, thickness)` (`folding/truncated_pyramid.h`): a pyramid or truncated pyramid over a square plan, optionally creased along its side faces (`edge_fold`) and corner ridges (`corner_fold`) via `FoldPoint{along, rise}`.

### Pyramid {#folding_pyramid}

Four triangular plates over an `18 m` square base rising to a `6 m` apex (`4` plates).

![](templates/templates_folding_14_pyramid_plates.webp)

```cpp
TruncatedPyramid fold(9000.0, 0.0, 6000.0, std::nullopt, std::nullopt, 120.0, "pyramid");
```

### Truncated pyramid, the basic form {#folding_truncated_pyramid}

Four trapezoidal side plates and a flat `6 m` top cap at `6.8 m` height over an `18 m` square base (`5` plates).

![](templates/templates_folding_15_truncated_pyramid_plates.webp)

```cpp
TruncatedPyramid fold;
```

### Variation 1: folded edges {#folding_folded_truncated_pyramid}

Each base edge midpoint pulled inward by `21%` and raised to `54%` of the height, splitting each side into four triangles (`17` plates).

![](templates/templates_folding_36_folded_truncated_pyramid_plates.webp)

```cpp
TruncatedPyramid fold(9000.0, 3000.0, 6800.0, FoldPoint{0.21, 0.54}, std::nullopt, 120.0, "folded_truncated_pyramid");
```

### Variation 2: folded corners {#folding_truncated_pyramid_corner_folds}

Corner ridges bent at `22%` of their span and raised to half height, with side facets fanning from the base edge midpoints (`21` plates).

![](templates/templates_folding_38_truncated_pyramid_corner_folds_plates.webp)

```cpp
TruncatedPyramid fold(9000.0, 3000.0, 6800.0, FoldPoint{0.07, 0.0}, FoldPoint{0.22, 0.5}, 120.0, "truncated_pyramid_corner_folds");
```

### Variation 3: folded corners and edges {#folding_truncated_pyramid_corner_edge_folds}

Both corner bends and raised edge midpoints aligned at half height to form a horizontal fold ring (`25` plates).

![](templates/templates_folding_39_truncated_pyramid_corner_edge_folds_plates.webp)

```cpp
TruncatedPyramid fold(9000.0, 3000.0, 6800.0, FoldPoint{0.28, 0.5}, FoldPoint{0.22, 0.5}, 120.0, "truncated_pyramid_corner_edge_folds");
```

## Folded triangular surfaces {#triangle_fold}

`TriangleFold(sides, radius, middle, centre, thickness)` (`folding/triangle_fold.h`): a regular polygon plan with vertical perimeter gables rising to `middle`, corner triangles folding down to the ground supports, and an inner ring of roof triangles meeting at `centre` (or a single flat roof plate when `centre == middle`).

### Square plan {#folding_triangle_square}

Perimeter gables rising to `5.0 m` and central apex at `6.5 m` (`12` plates).

![](templates/templates_folding_20_triangle_square_plates.webp)

```cpp
TriangleFold fold;
```

### Triangular plan {#folding_triangle_triangle}

Perimeter gables at `7.5 m` with a flat central triangle (`7` plates).

![](templates/templates_folding_21_triangle_triangle_plates.webp)

```cpp
TriangleFold fold(3, 12000.0, 7500.0, 7500.0, 120.0, "triangle_triangle");
```

### Pentagonal plan {#folding_triangle_pentagon}

Perimeter gables at `4.8 m` and central apex at `6.0 m` (`15` plates).

![](templates/templates_folding_22_triangle_pentagon_plates.webp)

```cpp
TriangleFold fold(5, 12000.0, 4800.0, 6000.0, 120.0, "triangle_pentagon");
```

### Hexagonal plan {#folding_triangle_hexagon}

Perimeter gables at `4.2 m` and central apex at `5.3 m` (`18` plates).

![](templates/templates_folding_23_triangle_hexagon_plates.webp)

```cpp
TriangleFold fold(6, 12000.0, 4200.0, 5300.0, 120.0, "triangle_hexagon");
```
