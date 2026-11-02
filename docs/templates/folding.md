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

## Reflex Fold {#reflex_fold}

`ReflexFold(cross_section, profile, thickness, chamfer_bottom, chamfer_top, chamfer_angle)` (`folding/reflex_fold.h`): sweeps a zigzag `profile` along an arch `cross_section` (two profile curves), folding at each arch vertex bisector plane (`mitered_plates`).

### Default arch and zigzag {#folding_reflex_fold}

Default reflex fold arch from two profile curves (`cross_section` and `profile`) with `10 mm` mitered plates and `20 mm` corner chamfers.

![](templates/templates_folding_1_reflex_fold_plates.webp)

```cpp
ReflexFold fold;
```

## Diamond Mesh {#diamond_mesh}

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

## Prismatic Fold {#prismatic_fold}

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

## Counter Fold {#counter_fold}

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

## Conical Fold {#conical_fold}

`ConicalFold(folds, fold_width, depth, end_depth, span, thickness)` (`folding/conical_fold.h`): clips a continuous zigzag of height `depth` by two planes sloping from `end_depth` at the supports to full `depth` at mid-span.

### Conical fold {#folding_conical_fold}

Four folds over a `14.4 m` span with ridges rising from `600 mm` at the supports to `1500 mm` at mid-span (`16` plates).

![](templates/templates_folding_7_conical_fold_plates.webp)

```cpp
ConicalFold fold;
```

## Folded Frame {#folded_frame}

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

## Single Fold {#single_fold}

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

## Diagonal Fold {#diagonal_fold}

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

## Fold Composition {#fold_composition}

`FoldComposition` (`folding/fold_composition.h`): a `6 × 4` grid of `12 m` square cells folded on their diagonals over alternating ground and `4.6 m` nodes, with the central `2 × 2` bay carrying an elevated `15 m` 8-pointed star vault (`96` plates).

### Composition with a central star {#folding_fold_composition}

![](templates/templates_folding_19_fold_composition_plates.webp)

```cpp
FoldComposition fold;
```

## Truncated Pyramid {#truncated_pyramid}

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

## Triangle Fold {#triangle_fold}

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

## Examples {#folding_examples}

### 1 Reflex Fold {#folding_example_1}

The reflex fold template: a zigzag profile carried along an arch, folded onto the plane that halves the arch's angle at every point, one 10 mm plate per quad with 20 mm chamfers, each step in its own group (curves, fold_planes, mesh, plates).

\include{lineno} templates_folding_1_reflex_fold.cpp

### 2 Diamond Mesh {#folding_example_2}

The diamond mesh template on its default arch, 3000 by 5000 with a rise of 1500: 8 by 4 cells split into rhombi between the cell centres, two triangles each, one 40 mm plate per triangle with 10 mm chamfers, each step in its own group (surface, mesh, plates).

\include{lineno} templates_folding_2_diamond_mesh.cpp

### 3 Prismatic Fold {#folding_example_3}

Step 3 of the folding templates: the singly folded plate. A zigzag profile of four ridges, eight plates each 1500 across in plan and 1200 high, so 12 m wide, carried straight over a 12 m span, the depth a tenth of the span; one 120 mm plate per fold, mitered at the folds, acting as a slab between its folds, as a deep beam along the span and with its neighbours as a truss across it. Three transverse diaphragms under the folds, one at each end and one at mid-span, each one plate across the profile between the flanges down to their bottom, stiffen the profile against a change of its fold angles; a 400 flange folded down at each free edge, the folds being shallow, mitered like every other fold, stiffens it against buckling. Each step is a group: profile, folds, mesh, plates, diaphragms.

\include{lineno} templates_folding_3_prismatic_fold.cpp

### 4 Folded Barrel {#folding_example_4}

Step 4 of the folding templates: the system with many folds. The more folds a profile has, the more the edge stresses of its plates balance each other through shear, until its form and action approach those of a cylindrical shell: seven folds, eight 120 mm plates with their corners on a circular arc 12 m wide and 4 m high, carried straight over a 12 m span, a diaphragm closing each end under the folds. Each step is a group: profile, folds, mesh, plates, diaphragms.

\include{lineno} templates_folding_4_folded_barrel.cpp

### 5 Counter Fold {#folding_example_5}

Step 5 of the folding templates: a surface with counter-running folding, the same depth of the fold profiles and the same elevation above ground, ridge-to-ridge folding, read off the construction's plan and both elevations. A zigzag of four ridges, plates 1500 across and 1500 deep, stands at the start of a 13.5 m span, nine plates long, and its counter profile at the end, so every fold line runs from a valley to a ridge; every strip is warped and folds along the diagonal through its two high corners, a ridge running from the ridge of one end to the ridge of the other, the diagonals of neighbouring strips meeting at the ends in the zigzag's plan, the strips crossing in the X's of his end elevation: 16 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_5_counter_fold.cpp

### 6 Elevated Fold {#folding_example_6}

Step 6 of the folding templates: a surface with counter-running folding, the centre fold elevated over the edge fold with identical depth of profiles, ridge-to-ridge folding, read off the construction's plan and both elevations. A zigzag of four ridges, plates 1500 across and 1500 deep, starting on a ridge at the edge, stands at both ends of a 15 m span, and at mid-span its counter profile lifted by the depth: its valleys level with the ends' ridges, so the even fold lines run straight through the whole span, as the level line's front elevation, while the odd ones rise from the ends' valleys to the centre's ridges, twice the depth high. Folded ridge to ridge, the diagonals run from the ends' even fold lines to the centre's odd ones, crossing there in the stars of the plan; either side of a straight fold line the two triangles lie in one plane, so they are one plate and the centre station is no crease on the even lines, as the plan draws it: 24 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_6_elevated_fold.cpp

### 7 Conical Fold {#folding_example_7}

Step 7 of the folding templates: a surface with conical folding, a continuous fold profile with its upper edge cut by a sloping plane, read off the construction's plan and both elevations. Four folds of plates 1500 across, their valleys level over a 14.4 m span and their ridges 1500 high, are cut by two planes sloping from 600 above the valleys at either end up to the ridges at mid-span: below the cut every flank keeps its plane, a pentagon from the valley line to its cut edge, which rises from the end to the ridge point at mid-span and falls again, and above it the cut is a thin triangle per fold and half span, from the flat top of the fold at the end to the ridge point, so the end profiles are trapezoids and the middle a sharp zigzag, the elongated X of every fold in the plan: 16 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_7_conical_fold.cpp

### 8 Two Hinged Frame {#folding_example_8}

Step 8 of the folding templates: a linear structure system of folded surfaces, the two-hinged frame with ridge-to-ridge folding, read off the construction's plan and both elevations. Half a bay is one fold line from the foot to the crown, zigzagging between the bay's edge, the frame's inner line of valleys, and the bay's centre, the ridges: the foot, hinged along the whole bay, the knee bulging out 2200 at half the 5800 column, the eaves corner, the ridge top 8750 high at mid-span and the ceiling peak 6600 under it; every point makes a planar triangle with the last valley and the last ridge, so a column is a pyramid of plates around its knee, a vertical plate at every bay edge, and the roof folds from the eaves corners up to the ridge top, the ridge running straight from the knee through the frame's inner corner to the top, the diamonds's side elevation and the hexagons of his plan. Mirrored over mid-span and over the bay's centre and repeated over four 4.5 m bays: 64 planar plates 120 mm thick, flush along both sides. Each step is a group: zigzag, mesh, folds, plates.

\include{lineno} templates_folding_8_two_hinged_frame.cpp

### 9 Three Hinged Frame {#folding_example_9}

Step 9 of the folding templates: the three-hinged frame with ridge folding. A 30 m portal of four 4.25 m bays on point feet at the bay centres, 6250 to the eaves, the knee bulging out 1900 at half the column; the roof rises from the eaves to its ridge 8250 high over the quarter points of the span and dips to the hinge 6800 high at mid-span over a flat ceiling. The fold line runs foot, eaves corner, knee, quarter-point ridge, mid-span ceiling corner, hinge, so the diagonals from the eaves corners cross at the quarter-point ridge, the star's plan, and the diamonds of his side elevation stand on the knees: 64 planar plates 120 mm thick. Each step is a group: zigzag, mesh, folds, plates.

\include{lineno} templates_folding_9_three_hinged_frame.cpp

### 10 Three Hinged Arch {#folding_example_10}

Step 10 of the folding templates: the three-hinged arch. Five 5 m bays over a 21.5 m span, 20.5 m high at the proportions: from a point foot at the bay's centre the fold line rises to the ankle's level line at 0.29 of the height, the knee bulging out 0.23 of the span, the haunch's valley at 0.59 and its ridge at 0.67, the crown's corners over 0.27 of the span and the hinge at mid-span, where ridge and valley meet in one line along the bays; the diagonals from the crown corners cross at the mid-span bay edges, the big X's plan: 140 planar plates 120 mm thick. Each step is a group: zigzag, mesh, folds, plates.

\include{lineno} templates_folding_10_three_hinged_arch.cpp

### 11 Triangle Fold {#folding_example_11}

Step 11 of the folding templates: structure systems through interpenetration of folded surfaces, singly folded surfaces over a triangular plan with horizontal ridges. The plan an equilateral triangle on a 12 m circumradius standing on its three corners; every edge's middle stands 2900 out from the edge and 7000 high, as high as the centre, so the three ridges from the middles to the centre are horizontal and the plan's outline is a six-pointed star; every sector is one singly folded surface, two planar triangles meeting on its ridge and its neighbours on the valleys from the corners to the centre, and a steep eave triangle from every edge on the ground leans outward up to its middle: nine plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_11_triangle_fold.cpp

### 12 Square Fold {#folding_example_12}

Step 12 of the folding templates: singly folded surfaces over a square plan with ridges rising toward the centre. An 18 m square standing on its corners; every edge's middle is in line with the edge and 5400 high, the centre 6900, so every edge carries a vertical gable triangle from the ground up to its peak and the four ridges rise from the peaks to the apex; every sector is two planar triangles folded on its ridge, meeting its neighbours on the valleys from the corners to the apex, as the front elevation shows them, the gables' verticals at the sides and the shallow gable top: twelve plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_12_square_fold.cpp

### 13 Hexagon Fold {#folding_example_13}

Step 13 of the folding templates: singly folded surfaces over a hexagonal plan with ridges sloping to the centre. A regular hexagon on a 12 m circumradius standing on its six corners; every edge's middle is pulled 2100 in from the edge and raised to 8300, the centre only 5200, so the six ridges slope down from the peaks to the centre and the eave triangles from the edges on the ground lean inward, seen in the plan; every sector is two planar triangles folded on its ridge: eighteen plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_13_hexagon_fold.cpp

### 14 Pyramid {#folding_example_14}

Step 14 of the folding templates: the pyramidal folded plate: over an 18 m square on the ground four triangular plates rise to an apex at 6000, each acting as a slab, as a plate and with its neighbours as a truss, each pair of opposite faces stiffening the other pair: four plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_14_pyramid.cpp

### 15 Truncated Pyramid {#folding_example_15}

Step 15 of the folding templates: the doubly folded truncated pyramid, the basic structure form his variations fold further. An 18 m square on the ground, four trapezoids rising to a flat 6 m square top 6800 high, as his plan and perspective show: four trapezoids and the top, five plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_15_truncated_pyramid.cpp

### 16 Cross Fold {#folding_example_16}

Step 16 of the folding templates: cross-folded surfaces spanned diagonally over a square plan with the ridges folded upwardly. An 18 m square; the cross from the edges' middles to the centre divides it into four sectors, each a surface folded diagonally: the ridge from the corner to the centre is folded upward at its middle, 6200 high, while the corner and the centre are both 3300 high and the edges' middles stand on the ground, so every edge is a valley from corner to corner, the lines from middle to middle bend up over the ridges and the structure stands on the four middles; every sector is four planar triangles around its fold point: sixteen plates 120 mm thick, their edges on the plan's sides cut flush. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_16_cross_fold.cpp

### 17 Cross Fold Rising {#folding_example_17}

Step 17 of the folding templates: cross-folded surfaces spanned diagonally over a square plan with the ridges rising toward the centre. An 18 m square standing on its four edges' middles; every diagonal ridge rises from its corner, 3100 high, to a fold point two thirds of the way along it, 4900 high, and on more steeply to the apex at 9000, so every edge is a valley from corner to corner, the lines from middle to middle fold over the ridges, the chevrons's plan pointing to the centre, and his front elevation's straight slope from the foot to the apex is the line from the middle to the centre; every sector is four planar triangles: sixteen plates 120 mm thick, their edges on the plan's sides cut flush. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_17_cross_fold_rising.cpp

### 18 Cross Fold Sloping {#folding_example_18}

Step 18 of the folding templates: cross-folded surfaces spanned diagonally over a square plan with the ridges sloping toward the centre. An 18 m square whose corners are the highest points, 7000; every diagonal ridge drops steeply to a fold point less than half way along it, 4300 high, and on gently to the centre at 3500, the edges' middles on the ground carrying the structure, so every edge is a deep valley from corner to corner and the chevrons's plan point to the corners; every sector is four planar triangles: sixteen plates 120 mm thick, their edges on the plan's sides cut flush. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_18_cross_fold_sloping.cpp

### 19 Fold Composition {#folding_example_19}

Step 19 of the folding templates: the composition of surfaces folded crosswise over square plans. Six by four cells of 12 m, 72 by 48 m; the field's nodes alternate like a chessboard between the ground and 4600, so every cell edge slopes from a high node to a low one and the field stands on its low nodes, and every cell is folded on its diagonals into four triangles to a peak 6500 over its centre, the zigzag's front elevation; the two by two block in the middle carries one cross fold spanned diagonally with its ridges folded upwardly, its fold points 15000 high, its corners the block's corner nodes raised to 8000 and its edges' middles the block's low nodes, so the cells around the block rise to it: eighty triangles and the block's sixteen, 96 plates 120 mm thick, their edges on the field's sides cut flush. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_19_fold_composition.cpp

### 20 Triangle Square {#folding_example_20}

Step 20 of the folding templates: a structure system of folded triangular surfaces over a square plan. A square on a 12 m circumradius standing on its corners; every edge carries a vertical gable triangle up to its middle 5000 high, the lines joining the middles cut a triangle off every corner, folding down to the corner on the ground, and the cross from the centre at 6500 splits the square of the middles into four more: twelve planar triangles 120 mm thick, their corners cut back only where needed and their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_20_triangle_square.cpp

### 21 Triangle Triangle {#folding_example_21}

Step 21 of the folding templates: folded triangular surfaces over a triangular plan. An equilateral triangle on a 12 m circumradius standing on its corners; every edge carries a vertical gable triangle up to its middle 7500 high, the lines joining the middles cut the roof into four: the three corner triangles fold down from two peaks to the corner on the ground, the middle triangle stays flat at 7500, as the vertical lines's perspective show on the gables: seven plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_21_triangle_triangle.cpp

### 22 Triangle Pentagon {#folding_example_22}

Step 22 of the folding templates: folded triangular surfaces over a pentagonal plan. A regular pentagon on a 12 m circumradius standing on its corners; every edge carries a vertical gable triangle up to its middle 4800 high, a triangle at every corner folds down from two peaks to the corner, and the pentagon of the middles is split into five triangles to the centre at 6000: fifteen planar triangles 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_22_triangle_pentagon.cpp

### 23 Triangle Hexagon {#folding_example_23}

Step 23 of the folding templates: folded triangular surfaces over a hexagonal plan. A regular hexagon on a 12 m circumradius standing on its corners; every edge carries a vertical gable triangle up to its middle 4200 high, a triangle at every corner folds down from two peaks to the corner, and the hexagon of the middles is split into six triangles to the centre at 5300: eighteen planar triangles 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_23_triangle_hexagon.cpp

### 25 One Fold {#folding_example_25}

Step 25 of the folding templates: the system with one fold: two plates on a 12 m wide arch rising 3000, meeting at one ridge, carried straight over a 12 m span; the edge stresses at the common fold have the same tendency, so the stress distribution stays unchanged. Two 120 mm plates and a diaphragm at each end. Each step is a group: profile, folds, mesh, plates, diaphragms.

\include{lineno} templates_folding_25_one_fold.cpp

### 26 Three Folds {#folding_example_26}

Step 26 of the folding templates: the system with three folds: four plates with their corners on a 12 m wide arch rising 3500, carried straight over a 12 m span; the edge stresses at the side folds have opposite tendency and largely compensate each other through shear. Four 120 mm plates and a diaphragm at each end. Each step is a group: profile, folds, mesh, plates, diaphragms.

\include{lineno} templates_folding_26_three_folds.cpp

### 27 Counter Centre {#folding_example_27}

Step 27 of the folding templates: counter-running folding with the counter profile in the centre: the zigzag of four ridges at both ends of a 13.5 m span and its counter profile at mid-span, the same depth and elevation, so every fold line rises or falls by the depth to the centre and the centre profile is a crease at every fold; the warped strips fold ridge to valley, the diagonals running from the ends' valleys to the centre's, where the four of two strips cross in the stars's plan, one at every second fold line, and reach the corners of the plan singly: 32 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_27_counter_centre.cpp

### 28 Ridge To Valley {#folding_example_28}

Step 28 of the folding templates: counter-running folding with ridge-to-valley folding: the stations of step 5, every warped strip folded along the diagonal through its two low corners, a valley running from the valley of one end to the valley of the other, so the zigzag of diagonals in the plan starts a strip later and the end elevation shows the pyramids's drawing: 16 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_28_ridge_to_valley.cpp

### 29 Elevated Ridge To Valley {#folding_example_29}

Step 29 of the folding templates: the centre fold elevated over the edge folds with ridge-to-valley folding: the stations of step 6, every warped strip folded along the diagonal through its low corners, from the ends' odd fold lines, their valleys, to the centre's even ones, so the diagonals cross at the centre on the even lines and the centre station creases at every fold, the vertical line's plan: 32 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_29_elevated_ridge_to_valley.cpp

### 30 Alternate Fold {#folding_example_30}

Step 30 of the folding templates: the centre fold elevated over the edge folds with alternate folding: the stations of step 6, the folds about the even fold lines folded ridge to valley and ridge to ridge by turns, the edge fold first ridge to valley. The odd fold lines then lie straight in one plane on either side and vanish, as do the centre's creases of every ridge-to-ridge fold, so the surface is planar quads between the diagonals, which all slope one way on each half of the span, with the stars of the plan at every second even line and the diamonds's end elevation without a line through them: 20 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_30_alternate_fold.cpp

### 31 Deep Centre {#folding_example_31}

Step 31 of the folding templates: counter-running folding with the elevated centre profile deeper than the edge profiles, ridge to ridge: the construction of step 6 with the edge profiles 1000 deep and the centre's 1800, its valleys level with the ends' ridges, so the even fold lines run straight and the odd ones rise 2800 from the ends' valleys to the centre's ridges, the tall diamonds's end elevation; the diagonals from the ends' even lines to the centre's odd ones, the centre no crease on the even lines: 24 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_31_deep_centre.cpp

### 32 Deep Centre Valley {#folding_example_32}

Step 32 of the folding templates: the elevated centre profile deeper than the edge profiles with ridge-to-valley folding: the stations of step 31, the diagonals from the ends' valleys to the centre's, crossing at the centre on the even lines, where the centre station creases at every fold: 32 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

\include{lineno} templates_folding_32_deep_centre_valley.cpp

### 34 A Frame {#folding_example_34}

Step 34 of the folding templates: the two-hinged A-frame with ridge-to-valley folding. Four 3.85 m bays; every leg stands on point feet at the bay edges, bulges out 1450 at the knee 6000 high, and its fold line drops from the knee to the waist, 6100 in and 4000 high at the bay's centre, before climbing to the apex 12700 high at the bay edge and the ridge 18100 high over it: the diagonals from the feet to the apex cross at the waist, the star of six plates's plan, the kites of his side elevation hang from the ridge, and the legs' outer plates lean out over the feet: 48 planar plates 120 mm thick. Each step is a group: zigzag, mesh, folds, plates.

\include{lineno} templates_folding_34_a_frame.cpp

### 35 Pointed Arch {#folding_example_35}

Step 35 of the folding templates: the arch with a hinge at the crown. Four 4.95 m bays over a 17.9 m span, 26.1 m high at the proportions: from a point foot at the bay's centre a narrow trunk rises to its valley at a third of the height and its knee at 0.57, the haunch's valley at 0.885 of the height a fifth of the span in, the ridge peaks over 0.36 of the span with its valley straight below it, and dips to the hinge at mid-span, 0.93 of the height, where the two halves meet on the level valleys: 96 planar plates 120 mm thick. Each step is a group: zigzag, mesh, folds, plates.

\include{lineno} templates_folding_35_pointed_arch.cpp

### 36 Folded Truncated Pyramid {#folding_example_36}

Step 36 of the folding templates: the first variation for folding a given basic structure form. The truncated pyramid of step 15, an 18 m square with a 6 m top 6800 high; the middle of every base edge is pulled in a fifth of the side's depth and raised to 54 percent of the height, so every side fans out from it into three triangles to its corner lines and the top edge, and a steep fourth triangle leans from the base edge on the ground up to it, the X's elevations: sixteen triangles and the top, seventeen plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_36_folded_truncated_pyramid.cpp

### 38 Truncated Pyramid Corner Folds {#folding_example_38}

Step 38 of the folding templates: the second variation for folding a given basic structure form. The truncated pyramid of step 15, an 18 m square with a 6 m top 6800 high; every corner line from a base corner to a top corner is bent at a point a fifth of the way along it raised to half the height, and the middle of every base edge stays on the ground, pulled in a little, so every side fans out from that middle into five triangles over its two bent corner lines and the top edge, its base edge rising from the middle to the corners' bends and down to the corners, the kinked silhouette and the V to the ground's elevations: twenty triangles and the top, 21 plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_38_truncated_pyramid_corner_folds.cpp

### 39 Truncated Pyramid Corner Edge Folds {#folding_example_39}

Step 39 of the folding templates: the third variation for folding a given basic structure form. The truncated pyramid of step 15, an 18 m square with a 6 m top 6800 high; every corner line is bent at a point a fifth of the way along it raised to half the height, and the middle of every base edge is pulled in over a quarter of the side's depth and raised to the same half height, so a level fold runs from bend to middle to bend around the pyramid, every side fans out from its middle into five triangles above that fold and a steep sixth leans from the base edge on the ground up to it, the horizontal line through the X's elevations: 24 triangles and the top, 25 plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

\include{lineno} templates_folding_39_truncated_pyramid_corner_edge_folds.cpp
