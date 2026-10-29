# Cutting Gallery {#cutting_gallery}

[TOC]

The cutting gallery targets (`15_profile_cuts` through `18_cutting_gallery`) test 2D profile slots, blind pockets, tolerance-controlled cylindrical drills, exact NURBS bore BReps, and 3D polyhedral boolean operations. Each case displays **stock → cutter → cut result** from left to right.

![](diagrams/wood_boolean_cutting.webp)

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="examples.html#ex_15_profile_cuts">
    <img src="element_profile.webp" alt="15_profile_cuts">
    <span class="wood-card-title">15_profile_cuts</span>
    <span class="wood-card-sub">Concave L profile, hole, through-slot &amp; blind pocket</span>
  </a>
  <a class="wood-card" href="examples.html#ex_16_drill_solids">
    <img src="element_pin.png" alt="16_drill_solids">
    <span class="wood-card-title">16_drill_solids</span>
    <span class="wood-card-sub">Chord-tolerance bores, exact NURBS BReps &amp; tilted bore</span>
  </a>
  <a class="wood-card" href="examples.html#ex_17_solid_features">
    <img src="element_block.png" alt="17_solid_features">
    <span class="wood-card-title">17_solid_features</span>
    <span class="wood-card-sub">Boolean difference, intersection, union &amp; mesh cutter</span>
  </a>
  <a class="wood-card" href="examples.html#ex_19_plate_joint_library">
    <img src="element_joint_plate_top_side.png" alt="19_plate_joint_library">
    <span class="wood-card-title">19_plate_joint_library</span>
    <span class="wood-card-sub">Six top_side plate joint presets across division &amp; shift</span>
  </a>
</div>
<div style="display:none">
![](elements/element_profile.webp)
![](elements/element_pin.png)
![](elements/element_block.png)
![](elements/element_joint_plate_top_side.png)
</div>

| Target | Cases |
| --- | --- |
| `15_profile_cuts` | Concave L profile, profile with hole, through-slot splitting two pieces, concave blind pocket |
| `16_drill_solids` | `R = 35` bores with chord tolerances `2`, `0.2`, `0.02`; exact cylindrical NURBS BReps; tilted `R = 28` bore |
| `17_solid_features` | Boolean difference, intersection, and union of sloped solids; arbitrary tetrahedral mesh cutter |
| `18_cutting_gallery` | Combined side-by-side gallery of `15_profile_cuts`, `16_drill_solids`, and `17_solid_features` |
| `19_plate_joint_library` | Six `top_side` joint presets across division and shift parameters |

## Build and Run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target 15_profile_cuts 16_drill_solids 17_solid_features 18_cutting_gallery --parallel 4
./build/15_profile_cuts
./build/16_drill_solids
./build/17_solid_features
./build/18_cutting_gallery
```

## Cutter API and Invariants

```cpp
auto profile = std::make_shared<Joint>(
    std::vector<Polyline>{outer, hole}, extrusion, SolidOperation::intersect);
auto drill = Joint::drill(axis, radius, chord_tolerance);
auto solid = std::make_shared<Joint>(closed_mesh, SolidOperation::subtract);
auto stock = std::make_shared<Block>(stock_mesh);
scene.add(profile);
scene.add_interaction(profile, plate, profile->interaction(0));
```

| Cutter / Stage | Geometric Invariant |
| --- | --- |
| `Joint(profiles, extrusion, op)` | Simple closed coplanar rings perpendicular to `extrusion` (`profiles[0]` outer boundary, `profiles[1..]` holes); finite extrusion vector controls through-cuts vs blind pockets |
| Planar and 3D Booleans | Compatible through-cuts use 2D polygon booleans + loft; general closed polyhedra use [Manifold v3.5.4](https://github.com/elalish/manifold) (double precision) with coplanar triangle re-merging |
| Batch Difference Rule | Consecutive `subtract` cuts run in a single batch and retain the largest connected solid (dropping separated offcuts); `intersect` and `add` retain all pieces |
| `Joint::drill(axis, r, tol)` | Mesh segment count (8 to 65536) adapts to chord tolerance (default `0.05`); BRep stores an exact rational NURBS cylinder with circular or elliptical trim loops |
| Exact BRep Bore Fallback | Planar faces crossed by an isolated drill receive exact analytical circle or ellipse trim loops and a cylindrical face; drills intersecting edges or other drills fall back to the faceted mesh BRep |
