# Examples {#examples}

[TOC]

## Build and run

Needs Git, CMake 3.22 or newer and a C++23 compiler (Visual Studio 2022, GCC 13 or Clang 17). The first configure downloads the kernel and its libraries, so it takes a few minutes.

1. Clone: `git clone https://github.com/petrasvestartas/wood.git` and `cd wood`
2. Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
3. Build one example: `cmake --build build --config Release --target two_plates --parallel 6`
4. Run it: `./build/two_plates` on Linux, `build\Release\two_plates.exe` on Windows
5. View: the run writes `data/output/pb/live.pb`; `bash ../bash/publish-scene.sh --target two_plates` publishes it to the [viewer](https://petrasvestartas.github.io/session/)

Every other example builds and runs the same way with its own target name.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#ex_two_plates">
    <img src="two_plates.png" alt="Two Plates">
    <span class="wood-card-title">Two Plates</span>
  </a>
  <a class="wood-card" href="#ex_2_contacts">
    <img src="ex_2_contacts.webp" alt="Contacts">
    <span class="wood-card-title">Contacts</span>
  </a>
  <a class="wood-card" href="#ex_3_features">
    <img src="ex_3_features.webp" alt="Plate Joinery">
    <span class="wood-card-title">Plate Joinery</span>
  </a>
  <a class="wood-card" href="#ex_4_traversal">
    <img src="ex_4_traversal.webp" alt="Graph Traversal">
    <span class="wood-card-title">Graph Traversal</span>
  </a>
  <a class="wood-card" href="#ex_5_settings">
    <img src="ex_5_settings.webp" alt="Solver Settings">
    <span class="wood-card-title">Solver Settings</span>
  </a>
  <a class="wood-card" href="#ex_6_custom_joint">
    <img src="ex_6_custom_joint.webp" alt="Custom Joint">
    <span class="wood-card-title">Custom Joint</span>
  </a>
  <a class="wood-card" href="#ex_7_assignment">
    <img src="ex_7_assignment.webp" alt="Joint Assignment">
    <span class="wood-card-title">Joint Assignment</span>
  </a>
  <a class="wood-card" href="#ex_8_beams">
    <img src="ex_8_beams.webp" alt="Beam Contacts">
    <span class="wood-card-title">Beam Contacts</span>
  </a>
  <a class="wood-card" href="#ex_9_cross_joints">
    <img src="ex_9_cross_joints.webp" alt="Cross Joints">
    <span class="wood-card-title">Cross Joints</span>
  </a>
  <a class="wood-card" href="#ex_10_profile_cuts">
    <img src="ex_10_profile_cuts.webp" alt="Profile Cuts">
    <span class="wood-card-title">Profile Cuts</span>
  </a>
  <a class="wood-card" href="#ex_11_drill_solids">
    <img src="ex_11_drill_solids.webp" alt="Drill Solids">
    <span class="wood-card-title">Drill Solids</span>
  </a>
  <a class="wood-card" href="#ex_12_solid_features">
    <img src="ex_12_solid_features.webp" alt="Solid Booleans">
    <span class="wood-card-title">Solid Booleans</span>
  </a>
</div>

---

## Two Plates {#ex_two_plates}

Two plates and one joint: the contact between them and a tenon joint cut into both.

![](two_plates.png)

\include{lineno} two_plates.cpp


## Contacts {#ex_2_contacts}

Detects face, axis, and cross contacts across an in-plane hexagonal shell dataset and inspects every contact edge in the session graph.

![](ex_2_contacts.webp)

\include{lineno} 2_contacts.cpp

---

## Plate Joinery {#ex_3_features}

Runs the plate joinery solver on a hexagonal shell, orienting finger joints across shared edges and evaluating boolean-cut plate meshes. The fingers are sized by two settings: `joint_parameters[0]` is the length along the seam that one division takes, and `joint_depth_scale` is their depth across it as a share of the joint volume, whose width is the plate thickness.

![](ex_3_features.webp)

\include{lineno} 3_features.cpp

---

## Graph Traversal {#ex_4_traversal}

Traverses the session graph from a solved plate joint to its connected male and female plates, edge interactions, and underlying face contact.

![](ex_4_traversal.webp)

\include{lineno} 4_traversal.cpp

---

## Solver Settings {#ex_5_settings}

Configures division counts, tolerances, and joint extensions through WoodSession settings on a folded plate box and round-trips them through protobuf bytes.

![](ex_5_settings.webp)

\include{lineno} 5_settings.cpp

---

## Custom Joint {#ex_6_custom_joint}

Defines custom butterfly dovetail outlines in unit-box space and tiles them along the seam between two coplanar plates.

![](ex_6_custom_joint.webp)

\include{lineno} 6_custom_joint.cpp

---

## Joint Assignment {#ex_7_assignment}

Assigns in-plane finger joints and top-to-side tenon-mortise joints from points on shared plate edges, and alters both finger and tenon-mortise directions from lines placed on those edges.

![](ex_7_assignment.webp)

\include{lineno} 7_assignment.cpp

---

## Beam Contacts {#ex_8_beams}

Detects axis intersections between three orthogonal timber beams and solves their oriented crossing and end-to-side joint volumes.

![](ex_8_beams.webp)

\include{lineno} 8_beams.cpp

---

## Cross Joints {#ex_9_cross_joints}

Solves half-lap cross joints between intersecting plates using cross-only and face-then-cross contact search modes.

![](ex_9_cross_joints.webp)

\include{lineno} 9_cross_joints.cpp

---

## Profile Cuts {#ex_10_profile_cuts}

Cuts rectangular timber plates with extruded 2D polyline profiles including concave L-outlines, interior holes, through-slots, and blind pockets.

![](ex_10_profile_cuts.webp)

\include{lineno} 10_profile_cuts.cpp

---

## Drill Solids {#ex_11_drill_solids}

Compares chord-tolerance-controlled cylindrical drill meshes against exact rational NURBS bore BReps and cuts a tilted bore through a timber block.

![](ex_11_drill_solids.webp)

\include{lineno} 11_drill_solids.cpp

---

## Solid Booleans {#ex_12_solid_features}

Applies 3D polyhedral boolean difference, intersection, union, and tetrahedral mesh cutters to sloped and lofted timber blocks.

![](ex_12_solid_features.webp)

\include{lineno} 12_solid_features.cpp
