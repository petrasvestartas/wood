# Examples {#examples}

[TOC]

Self-contained example targets under `examples/*.cpp`. Build and run any target with `cmake --build build --target <name> && ./build/<name>`.

![](diagrams/wood_architecture.webp)

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#ex_1_session_contacts">
    <img src="wood_joint_unit_box.webp" alt="Session &amp; Joinery Pipeline">
    <span class="wood-card-title">1–14. Session &amp; Joinery</span>
    <span class="wood-card-sub">Contacts, features, tree branches, datasets, traversal &amp; serialization</span>
  </a>
  <a class="wood-card" href="#ex_cutting_gallery">
    <img src="wood_boolean_cutting.webp" alt="Cutting Gallery">
    <span class="wood-card-title">15–19. Cutting Gallery</span>
    <span class="wood-card-sub">2D profile cuts, cylindrical NURBS drills, 3D solid booleans &amp; joint library</span>
  </a>
  <a class="wood-card" href="#ex_20_butt_frame">
    <img src="20_butt_frame.png" alt="20_butt_frame &amp; 21_butt_house">
    <span class="wood-card-title">20–21. Butt Frame &amp; House</span>
    <span class="wood-card-sub">Braced timber-frame wall and plate-framed house with butt cuts</span>
  </a>
  <a class="wood-card" href="#ex_22_butt_pairs">
    <img src="butt_brace_frame.png" alt="22_butt_pairs &amp; 23_butt_assemblies">
    <span class="wood-card-title">22–23. Butt Pairs &amp; Assemblies</span>
    <span class="wood-card-sub">18 pairwise butt configurations &amp; 5 multi-member assemblies</span>
  </a>
</div>
<div style="display:none">
![](diagrams/wood_joint_unit_box.webp)
</div>

| Example | Behaviour |
| --- | --- |
| `1_session_contacts` | Plates, a beam, a column, and a block in one scene; face contacts computed by the session, a `JointPlate` oriented to one, and its two features added with `add_interaction` |
| `2_session_hard_coded_contacts` | The same scene with its contacts written by hand as `InteractionContactFace` records instead of computed |
| `3_elements_tree` | The same bay three times, each under its own tree branch, so `compute_face_contacts(1)` stays inside a branch; `instance_by_key` for one definition per repeated element |
| `4_datasets` | The three loaders: a dataset YAML, an OBJ mesh, and a session `.pb` file |
| `5_contacts` | Face, axis, and cross contacts on one dataset, each read through the interaction of its edge |
| `6_features` | The joinery pipeline: every plate joint, a plate's geometry alone and cut, and the joint features on the hosts |
| `7_traversal` | From a joint to its plates, the interactions on their edge, and the contact on the plate-pair edge |
| `8_settings` | The solver settings as a value on the scene, set in code and read back from the file |
| `9_custom_joint` | A joint variant supplied as outlines through the settings |
| `10_assignment` | Feature types and insertion vectors filled from points and lines placed on the plates |
| `11_beams` | Beams, axis contacts, and one beam feature per pair with its four volume rectangles |
| `12_serialization` | Bytes and back, the kernel reading the same bytes, one record as JSON and as protobuf |
| `13_viewer` | The scene arranged for the viewer, the colour tables, and the files `pb_dump` writes |
| `14_cross_joints` | Cross joints and the search type chosen per solve |
| `15_profile_cuts` | Concave profiles, holes, disconnected pieces, and pockets |
| `16_drill_solids` | Tolerance-controlled drill meshes, exact cylindrical BReps, and a tilted bore |
| `17_solid_features` | Solid difference, intersection, union, and an arbitrary mesh cutter |
| `18_cutting_gallery` | Loads the three saved cutting scenes into one viewer gallery |
| `19_plate_joint_library` | Named side-to-top factories with different division and shift parameters |
| `20_butt_frame` | A timber-frame wall from lines: sill, head, posts, and braces butted flat |
| `21_butt_house` | A small house from plates on mid-planes: floor, walls, and roof butted flat |
| `22_butt_pairs` | Butt pairs of beams, columns, and plates: `top_side`, `side_side` out of plane and in plane, and `cross` |
| `23_butt_assemblies` | Butt pairs combined: a braced frame, a curved sill, a pentagon floor, zigzag walls, and a folded roof |

See also [Templates](@ref templates) for `templates_*` targets and [Cutting Gallery](@ref cutting_gallery) for `15_*` through `18_*`.

## 1_session_contacts {#ex_1_session_contacts}

Computes face contacts between timber elements in a `WoodSession`, orients a parametric `JointPlate` to the contact zone, and attaches cutting interactions to both plates.

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(plate0, plate1);
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(8, 0.5);
joint->orient(contact, {plate0, plate1}, scene.settings);
scene.add(joint);
scene.add_interaction(joint, plate0, joint->interaction(0));
scene.add_interaction(joint, plate1, joint->interaction(1));
```

- `ContactType`: `side_side`, `side_top`, `top_top`, `end_side`, `end_end`, `end_top`, or `unknown`.
- `top_side` joints automatically place the side member first (`target 0`); other families preserve supplied element order (`contact->flip()` reverses it).

| Library family | Named factories | Parameters vary by design |
| --- | --- | --- |
| Side-side in plane | `side_side_inplane_three_fingers` through `side_side_inplane_tooth_keys`, `side_side_inplane_custom` | Divisions, shift, custom outlines |
| Side-side out of plane | `side_side_outplane_three_fingers` through `side_side_outplane_linked_tenons_wall`, `side_side_outplane_capped_fingers`, `side_side_outplane_notch`, `side_side_outplane_custom` | Divisions, taper, chamfer, outline modification, and bounds |
| Side-to-top | `top_side_three_tenons`, `top_side_annen_tenons`, `top_side_dense_tenons`, `top_side_tenons`, `top_side_snap_fit`, `top_side_custom` | Divisions and shift on parametric designs |
| Rotated side-side | `side_side_rotated_miter` through `side_side_rotated_keys`, `side_side_rotated_custom` | Design-specific dimensions and outlines |
| Cross | `cross_slot` through `cross_brussels_half_lap`, `cross_custom` | Design-specific dimensions and outlines |
| Top-to-top | `top_top_centre_pin` through `top_top_rectangle_pins` | Drill radius, chord tolerance, count, or spacing |
| Other | `border`, `border_custom`, `side_removal`, `jupiter_scarf_port` | Outlines, merge, and shift options |

<details><summary>Example code: 1_session_contacts.cpp</summary>

\include{lineno} 1_session_contacts.cpp

</details>

## 2_session_hard_coded_contacts

Constructs explicit `InteractionContactFace` records directly in code without running automatic contact detection.

<details><summary>Example code: 2_session_hard_coded_contacts.cpp</summary>

\include{lineno} 2_session_hard_coded_contacts.cpp

</details>

## 3_elements_tree

Organizes repeated structural bays into separate scene-tree branches so contact detection stays scoped within each branch and shared definitions are instanced with `instance_by_key`.

<details><summary>Example code: 3_elements_tree.cpp</summary>

\include{lineno} 3_elements_tree.cpp

</details>

## 4_datasets

Loads timber plate and mesh datasets from YAML, OBJ, and `.pb` scene files into `WoodSession`.

<details><summary>Example code: 4_datasets.cpp</summary>

\include{lineno} 4_datasets.cpp

</details>

## 5_contacts

Detects face, axis, and cross contacts across a hexagonal shell dataset and inspects the resulting contact edges in the session graph.

<details><summary>Example code: 5_contacts.cpp</summary>

\include{lineno} 5_contacts.cpp

</details>

## 6_features

Runs the full plate joinery pipeline on a hexagonal shell, orienting finger joints across all shared edges and evaluating both uncut stock and boolean-cut plate meshes.

<details><summary>Example code: 6_features.cpp</summary>

\include{lineno} 6_features.cpp

</details>

## 7_traversal

Traverses `Session::graph` from a joint element to its connected plates, reading the feature interactions and underlying face contacts along each directed edge.

<details><summary>Example code: 7_traversal.cpp</summary>

\include{lineno} 7_traversal.cpp

</details>

## 8_settings

Configures solver tolerances, division counts, and joint parameters via `WoodSession::settings` and round-trips them through serialization.

<details><summary>Example code: 8_settings.cpp</summary>

\include{lineno} 8_settings.cpp

</details>

## 9_custom_joint

Defines a custom joint profile as explicit male and female polyline outlines passed through the solver settings.

<details><summary>Example code: 9_custom_joint.cpp</summary>

\include{lineno} 9_custom_joint.cpp

</details>

## 10_assignment

Assigns joint types and insertion directions to plate faces from geometric marker points and direction lines placed on the elements.

<details><summary>Example code: 10_assignment.cpp</summary>

\include{lineno} 10_assignment.cpp

</details>

## 11_beams

Computes axis contacts between crossing and meeting timber beams and generates the oriented contact volume boxes for each beam pair.

<details><summary>Example code: 11_beams.cpp</summary>

\include{lineno} 11_beams.cpp

</details>

## 12_serialization

Serializes elements, interactions, and complete `WoodSession` scenes to both JSON and protobuf bytes and reconstructs them identically.

<details><summary>Example code: 12_serialization.cpp</summary>

\include{lineno} 12_serialization.cpp

</details>

## 13_viewer

Arranges scene layers, color tables, and `.pb` outputs for interactive inspection in `session_viewer`.

<details><summary>Example code: 13_viewer.cpp</summary>

\include{lineno} 13_viewer.cpp

</details>

## 14_cross_joints

Solves cross-lap joints between intersecting plates using face and cross contact search modes.

<details><summary>Example code: 14_cross_joints.cpp</summary>

\include{lineno} 14_cross_joints.cpp

</details>

## Cutting gallery {#ex_cutting_gallery}

See @subpage cutting_gallery for cutter specifications.

![](diagrams/wood_boolean_cutting.webp)

### 15_profile_cuts {#ex_15_profile_cuts}

Cuts plates with extruded 2D polyline profiles including concave L-outlines, interior holes, through-slots, and blind pockets.

<details><summary>Example code: 15_profile_cuts.cpp</summary>

\include{lineno} 15_profile_cuts.cpp

</details>

### 16_drill_solids {#ex_16_drill_solids}

Compares chord-tolerance-controlled cylindrical drill meshes against exact rational NURBS bore BReps and cuts a tilted bore through a solid block.

<details><summary>Example code: 16_drill_solids.cpp</summary>

\include{lineno} 16_drill_solids.cpp

</details>

### 17_solid_features {#ex_17_solid_features}

Applies 3D boolean difference, intersection, union, and custom tetrahedral mesh cutters to timber stock blocks.

<details><summary>Example code: 17_solid_features.cpp</summary>

\include{lineno} 17_solid_features.cpp

</details>

### 18_cutting_gallery {#ex_18_cutting_gallery}

Combines the profile, drill, and solid cutting scenes side-by-side into a single gallery scene for `session_viewer`.

<details><summary>Example code: 18_cutting_gallery.cpp</summary>

\include{lineno} 18_cutting_gallery.cpp

</details>

### 19_plate_joint_library {#ex_19_plate_joint_library}

Instantiates six `top_side` plate joint presets side-by-side across different division counts and shift parameters.

<details><summary>Example code: 19_plate_joint_library.cpp</summary>

\include{lineno} 19_plate_joint_library.cpp

</details>

## Butt joints {#ex_butt_joints}

Members drawn as lines and mid-planes overlap where they meet. `JointBeam::butt(a, b)` detects how their ends meet and cuts each member along a shared contact plane so they meet face to face (@ref elements_joint_beam_butt). Trusses are generated by the [Truss](@ref templates_truss) template.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#ex_20_butt_frame">
    <img src="20_butt_frame.png" alt="20_butt_frame">
    <span class="wood-card-title">20_butt_frame</span>
    <span class="wood-card-sub">Braced timber-frame wall from centerlines</span>
  </a>
  <a class="wood-card" href="#ex_21_butt_house">
    <img src="21_butt_house.png" alt="21_butt_house">
    <span class="wood-card-title">21_butt_house</span>
    <span class="wood-card-sub">Plate-framed house from floor, wall &amp; roof mid-planes</span>
  </a>
  <a class="wood-card" href="#ex_22_butt_pairs">
    <img src="butt_pair_beam_top_side_oblique.png" alt="22_butt_pairs">
    <span class="wood-card-title">22_butt_pairs</span>
    <span class="wood-card-sub">18 pairwise beam, column &amp; plate butt configurations</span>
  </a>
  <a class="wood-card" href="#ex_23_butt_assemblies">
    <img src="butt_folded_roof.png" alt="23_butt_assemblies">
    <span class="wood-card-title">23_butt_assemblies</span>
    <span class="wood-card-sub">5 multi-member butt-jointed timber assemblies</span>
  </a>
</div>

### 20_butt_frame {#ex_20_butt_frame}

Builds a braced timber-frame wall from centerlines with sill, head, posts, and diagonal braces trimmed flat against each other.

![](examples/20_butt_frame.png)

<details><summary>Example code: 20_butt_frame.cpp</summary>

\include{lineno} 20_butt_frame.cpp

</details>

### 21_butt_house {#ex_21_butt_house}

Constructs a small plate-framed house from floor, wall, and roof mid-planes joined by automatic butt cuts.

![](examples/21_butt_house.png)

<details><summary>Example code: 21_butt_house.cpp</summary>

\include{lineno} 21_butt_house.cpp

</details>

### 22_butt_pairs {#ex_22_butt_pairs}

Tests every pairwise beam, column, and plate butt configuration (`top_side`, `side_side` out-of-plane and in-plane, and `cross`) and writes `butt_pair_<case>.pb` for each pair.

#### beam_top_side

A post landing square on a continuous beam.

![](examples/butt_pair_beam_top_side.png)

#### beam_top_side_oblique

A strut meeting a continuous beam at 60 degrees.

![](examples/butt_pair_beam_top_side_oblique.png)

#### beam_top_side_end

Two beams meeting at their ends, where the first runs through and the second stops against its side face.

![](examples/butt_pair_beam_top_side_end.png)

#### beam_top_side_end_past

Two corner beams whose centerlines extend past the corner intersection before trimming.

![](examples/butt_pair_beam_top_side_end_past.png)

#### beam_top_side_end_short

Two corner beams whose centerlines stop short of the corner intersection and extend to meet cleanly.

![](examples/butt_pair_beam_top_side_end_short.png)

#### beam_side_side_outplane

Two beams meeting at 120 degrees, cut on the bisector miter plane.

![](examples/butt_pair_beam_side_side_outplane.png)

#### beam_side_side_outplane_square

Two beams meeting at 90 degrees, mitered on the 45-degree bisector by `butt_side_side`.

![](examples/butt_pair_beam_side_side_outplane_square.png)

#### beam_side_side_inplane

Two collinear beams spliced end to end.

![](examples/butt_pair_beam_side_side_inplane.png)

#### beam_cross

Two crossing beams joined with a half-lap cut.

![](examples/butt_pair_beam_cross.png)

#### plate_top_side

An interior wall partitioning another wall along its mid-span.

![](examples/butt_pair_plate_top_side.png)

#### plate_top_side_end

Two walls meeting at an L-corner, where the first wall runs through.

![](examples/butt_pair_plate_top_side_end.png)

#### plate_side_side_outplane

Two walls meeting at 120 degrees, trimmed on the bisector miter plane.

![](examples/butt_pair_plate_side_side_outplane.png)

#### plate_side_side_inplane

Two coplanar walls joined edge to edge in line.

![](examples/butt_pair_plate_side_side_inplane.png)

#### plate_top_side_floor

A vertical wall standing on a horizontal floor slab.

![](examples/butt_pair_plate_top_side_floor.png)

#### plate_cross

Two crossing walls left uncut at their intersection.

![](examples/butt_pair_plate_cross.png)

#### column_top_side_plate

A square column supporting a horizontal floor slab.

![](examples/butt_pair_column_top_side_plate.png)

#### plate_top_side_beam

A wall panel bearing on top of a timber beam.

![](examples/butt_pair_plate_top_side_beam.png)

#### beam_top_side_plate

A beam framing into a wall panel at 60 degrees in plan.

![](examples/butt_pair_beam_top_side_plate.png)

<details><summary>Example code: 22_butt_pairs.cpp</summary>

\include{lineno} 22_butt_pairs.cpp

</details>

### 23_butt_assemblies {#ex_23_butt_assemblies}

Combines pairwise butt joints into complete multi-member assemblies (`butt_<assembly>.pb`).

#### brace_frame

A post on a sill with a diagonal knee brace trimmed between them.

![](examples/butt_brace_frame.png)

#### curved_sill

Two vertical posts landing on a segmented arc sill of eight beams.

![](examples/butt_curved_sill.png)

#### polygon_floor

Five perimeter walls (one with a sloped top edge) standing on a pentagonal floor plate.

![](examples/butt_polygon_floor.png)

#### zigzag_walls

Folded zigzag walls in plan standing on a flat floor plate.

![](examples/butt_zigzag_walls.png)

#### folded_roof

A zigzag folded-plate roof spanning over two gable end walls with overhanging eaves.

![](examples/butt_folded_roof.png)

<details><summary>Example code: 23_butt_assemblies.cpp</summary>

\include{lineno} 23_butt_assemblies.cpp

</details>
