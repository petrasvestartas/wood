# Cutting Gallery {#cutting_gallery}

[TOC]

Cutting gallery targets test 2D profile slots, blind pockets, tolerance-controlled cylindrical drills, exact NURBS bore BReps, and 3D polyhedral boolean operations. Each scene shows stock, cutter, and cut result side by side.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="examples.html#ex_15_profile_cuts">
    <img src="element_profile.webp" alt="Profile Cuts">
    <span class="wood-card-title">Profile Cuts</span>
  </a>
  <a class="wood-card" href="examples.html#ex_16_drill_solids">
    <img src="element_pin.png" alt="Drill Solids">
    <span class="wood-card-title">Drill Solids</span>
  </a>
  <a class="wood-card" href="examples.html#ex_17_solid_features">
    <img src="element_block.png" alt="Solid Features">
    <span class="wood-card-title">Solid Features</span>
  </a>
  <a class="wood-card" href="examples.html#ex_19_plate_joint_library">
    <img src="element_joint_plate_top_side.png" alt="Plate Joint Library">
    <span class="wood-card-title">Plate Joint Library</span>
  </a>
</div>
<div style="display:none">
![](elements/element_profile.webp)
![](elements/element_pin.png)
![](elements/element_block.png)
![](elements/element_joint_plate_top_side.png)
</div>

## Build and Run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target 15_profile_cuts 16_drill_solids 17_solid_features 18_cutting_gallery --parallel 4
./build/15_profile_cuts
./build/16_drill_solids
./build/17_solid_features
./build/18_cutting_gallery
```

## Cutter API

```cpp
auto profile = std::make_shared<Joint>(
    std::vector<Polyline>{outer, hole}, extrusion, SolidOperation::intersect);
auto drill = Joint::drill(axis, radius, chord_tolerance);
auto solid = std::make_shared<Joint>(closed_mesh, SolidOperation::subtract);
auto stock = std::make_shared<Block>(stock_mesh);
scene.add(profile);
scene.add_interaction(profile, plate, profile->interaction(0));
```

| Cutter | Geometric Rule |
| --- | --- |
| Profile cutter | Closed coplanar rings perpendicular to extrusion (`profiles[0]` outer boundary, `profiles[1..]` holes) |
| Planar and 3D booleans | Through-cuts use 2D polygon booleans and lofting; general polyhedra use double-precision Manifold booleans |
| Batch difference | Consecutive subtract cuts run in one batch and retain the largest connected solid |
| Cylindrical drill | Mesh segment count adapts to chord tolerance; BRep stores an exact rational NURBS cylinder bore |
