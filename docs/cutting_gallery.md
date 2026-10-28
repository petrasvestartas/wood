# Cutting examples for the viewer {#cutting_gallery}

[TOC]

![Boolean & Exact BRep Cutting Pipeline](diagrams/wood_boolean_cutting.svg)

| Target | Cases |
| --- | --- |
| `15_profile_cuts` | Concave L profile, profile with hole, through-slot splitting two pieces, concave blind pocket |
| `16_drill_solids` | `R = 35` bores with chord tolerances `2`, `0.2`, `0.02`; exact cylindrical NURBS BReps; tilted `R = 28` bore |
| `17_solid_features` | Boolean difference, intersection, and union of sloped solids; arbitrary tetrahedral mesh cutter |
| `18_cutting_gallery` | Combined side-by-side gallery of `15_profile_cuts`, `16_drill_solids`, and `17_solid_features` |
| `19_plate_joint_library` | Six `top_side` joint presets across division and shift parameters |

Each row displays **stock $\to$ cutter $\to$ cut result** from left to right.

## Build & Run

```bash
~/.local/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
buildslot ~/.local/bin/cmake --build build --target 15_profile_cuts 16_drill_solids 17_solid_features 18_cutting_gallery --parallel 4
tools/run_guarded.sh -t 10 -m 4 -- build/15_profile_cuts
tools/run_guarded.sh -t 10 -m 4 -- build/16_drill_solids
tools/run_guarded.sh -t 10 -m 4 -- build/17_solid_features
tools/run_guarded.sh -t 10 -m 4 -- build/18_cutting_gallery
```

Publish to the live [session viewer](https://petrasvestartas.github.io/session/):

```bash
../bash/publish-scene.sh "$PWD/data/output/pb/18_cutting_gallery.pb" --no-notify
```

## Cutter API & Kernel Invariants

```cpp
auto profile = std::make_shared<Joint>(
    std::vector<Polyline>{outer, hole}, extrusion, SolidOperation::intersection);
auto drill = Joint::drill(axis, radius, chord_tolerance);
auto solid = std::make_shared<Joint>(closed_mesh, SolidOperation::difference);
auto stock = std::make_shared<Block>(stock_mesh);
scene.add(profile);
scene.add_interaction(profile, plate, profile->interaction(0));
```

| Cutter / Stage | Geometric Invariant |
| --- | --- |
| `Joint(profiles, extrusion, op)` | Simple closed coplanar rings $\perp$ `extrusion` (`profiles[0]` outer boundary, `profiles[1..]` holes); finite extrusion vector controls through-cuts vs blind pockets |
| Planar & 3D Booleans | Compatible through-cuts use 2D polygon booleans + loft; general closed polyhedra use [Manifold v3.5.4](https://github.com/elalish/manifold) (double precision) with coplanar triangle re-merging |
| Batch Difference Rule | Consecutive `difference` cuts run in a single batch and retain the largest connected solid (dropping separated offcuts); `intersection` and `union` retain all pieces |
| `Joint::drill(axis, r, tol)` | Mesh segment count $n \in [8, 65536]$ adapts to chord tolerance (default `0.05`); BRep stores exact rational NURBS cylinder with circular/elliptical trim loops |
| Exact BRep Bore Fallback | Planar faces crossed by an isolated drill receive exact analytical circle/ellipse trim loops + cylindrical face; drills intersecting edges or other drills fall back to the faceted mesh BRep |
