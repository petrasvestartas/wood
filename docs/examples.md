# Examples {#examples}

[TOC]

Eighteen short programs under `examples/`, one behaviour each, in the order to read them. Every one is a CMake target: `buildslot ~/.local/bin/cmake --build build --target <name> --parallel 4` followed by `tools/run_guarded.sh -t 10 -m 4 -- build/<name>` from the `wood` directory. The cutting examples have [generation and publishing instructions](@ref cutting_gallery); the earlier examples end with run instructions; the ones that write `live.pb` show in the viewer at https://petrasvestartas.github.io/session/.

| Example | Behaviour |
|---|---|
| `1_session_contacts` | Plates, a beam, a column and a block in one scene; face contacts computed by the session, a `JointPlate` oriented to one and its two features added with `add_interaction` |
| `2_session_hard_coded_contacts` | The same scene with its contacts written by hand as `InteractionContactFace` records instead of computed |
| `3_elements_tree` | The same bay three times, each under its own tree branch, so `compute_face_contacts(1)` stays inside a branch; `instance_by_key` for one definition per repeated element |
| `4_datasets` | The three loaders: a dataset yml, an obj alone, a session `.pb` |
| `5_contacts` | Face, axis and cross contacts on one dataset, each read through the interaction of its edge |
| `6_features` | The joinery pipeline: every plate joint, a plate's geometry alone and cut, the joint features on the hosts |
| `7_traversal` | From a joint to its plates, the interactions on their edge, the contact on the plate-pair edge, the edge |
| `8_settings` | The solver settings as a value on the scene, set in code and read back from the file |
| `9_custom_joint` | A joint variant supplied as outlines through the settings |
| `10_assignment` | Feature types and insertion vectors filled from points and lines placed on the plates |
| `11_beams` | Beams, axis contacts and one beam feature per pair with its four volume rectangles |
| `12_serialization` | Bytes and back, the kernel reading the same bytes, one record as JSON and as protobuf |
| `13_viewer` | The scene arranged for the viewer, the colour tables, the files `pb_dump` writes |
| `14_cross_joints` | Cross joints and the search type chosen per solve |
| `15_profile_cuts` | Concave profiles, holes, disconnected pieces and pockets |
| `16_drill_solids` | Tolerance-controlled drill meshes, exact cylindrical BReps and a tilted bore |
| `17_solid_features` | Solid difference, intersection, union and an arbitrary mesh cutter |
| `18_cutting_gallery` | Loads the three saved cutting scenes into one viewer gallery |
| `19_plate_joint_library` | Named side-to-top factories with different division and shift parameters |

Elements go under the tree node `add(element, parent)` names, under the root without one; there is no separate tree call. The generators under `src/templates/` have their own page, [Templates](@ref templates), with a screenshot and the code of every `templates_*` example. `templates_shells_2_gridshell`, the lamella gridshell, exits 1 unless every stud touches its four boards and no two elements overlap.

The regression programs stay beside them: `main_all_datasets` runs every dataset in `data/` and writes the outline dumps a refactor is diffed against, `main_dataset_runner` one dataset, `main_session_round_trip` and `main_element_mapping_check` check the file and the element registry with an exit code.

## 1_session_contacts

Select a library design by its actual name and parameters:

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(plate0, plate1);
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_3(8, 0.5);
joint->orient(contact, {plate0, plate1}, scene.settings);
scene.add(joint);
scene.add_interaction(joint, plate0, joint->interaction(0));
scene.add_interaction(joint, plate1, joint->interaction(1));
```

`ContactType` describes the touching faces: `side_side`, `side_top`, `top_top`,
`end_side`, `end_end`, `end_top`, or `unknown`. A contact stores no library joint
code or male/female flag. Its faces, lines and volumes follow element order.
The selected joint determines its library family. Side-to-top joints automatically
place the side member first internally, using the face indices. Other contacts
preserve the supplied element order. To reverse a contact, call `contact->flip()`
and supply the elements in reversed order when orienting or adding the interaction.
`flipped()` returns a reversed copy.

| Library family | Named factories | Parameters vary by design |
| --- | --- | --- |
| Side-side in plane | `ss_e_ip_0` through `ss_e_ip_5`, `ss_e_ip_custom` | Divisions, shift, custom outlines |
| Side-side out of plane | `ss_e_op_0` through `ss_e_op_5`, `ss_e_op_17`, `ss_e_op_tutorial`, `ss_e_op_custom` | Divisions, taper, chamfer, outline modification and bounds |
| Side-to-top | `ts_e_p_0`, `ts_e_p_1`, `ts_e_p_2`, `ts_e_p_3`, `ts_e_p_5`, `ts_e_p_custom` | Divisions and shift on parametric designs |
| Rotated side-side | `ss_e_r_0` through `ss_e_r_3`, `ss_e_r_custom` | Design-specific dimensions and outlines |
| Cross | `cr_c_ip_0` through `cr_c_ip_5`, `cr_c_ip_custom` | Design-specific dimensions and outlines |
| Top-to-top | `tt_e_p_0` through `tt_e_p_5` | Drill radius, chord tolerance, count or spacing |
| Other | `b_0`, `b_custom`, `side_removal`, `side_removal_ss_e_r_1_port` | Outlines, merge and shift options |

Exact signatures are declared together in `wood_element_joint_plate.h`.
`19_plate_joint_library` demonstrates six side-to-top configurations and writes
`data/output/pb/19_plate_joint_library.pb` for the viewer.

\include{lineno} 1_session_contacts.cpp

## 2_session_hard_coded_contacts

\include{lineno} 2_session_hard_coded_contacts.cpp

## 3_elements_tree

\include{lineno} 3_elements_tree.cpp

## 4_datasets

\include{lineno} 4_datasets.cpp

## 5_contacts

\include{lineno} 5_contacts.cpp

## 6_features

\include{lineno} 6_features.cpp

## 7_traversal

\include{lineno} 7_traversal.cpp

## 8_settings

\include{lineno} 8_settings.cpp

## 9_custom_joint

\include{lineno} 9_custom_joint.cpp

## 10_assignment

\include{lineno} 10_assignment.cpp

## 11_beams

\include{lineno} 11_beams.cpp

## 12_serialization

\include{lineno} 12_serialization.cpp

## 13_viewer

\include{lineno} 13_viewer.cpp

## 14_cross_joints

\include{lineno} 14_cross_joints.cpp

## Cutting gallery

See @subpage cutting_gallery for the API, geometry limits and Cloudflare command.

### 15_profile_cuts

\include{lineno} 15_profile_cuts.cpp

### 16_drill_solids

\include{lineno} 16_drill_solids.cpp

### 17_solid_features

\include{lineno} 17_solid_features.cpp

### 18_cutting_gallery

\include{lineno} 18_cutting_gallery.cpp
