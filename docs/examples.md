# Examples {#examples}

Fourteen short programs under `examples/`, one behaviour each, in the order to read them. Every one is a CMake target: `cmake --build build --target <name> --parallel 4 && ./build/<name>` from the `wood` directory. Each ends with a comment that says what it does and how to run it; the ones that write `live.pb` show in the viewer at https://petrasvestartas.github.io/session/.

| Example | Behaviour |
|---|---|
| `1_elements` | The four element kinds built in code and added to a scene, one authored interaction record; `get_element` and the typed lists |
| `1_elements_flat` | One floor bay from the grid template under the root: columns, conical heads, girders and beams mitred at the corners, a deck; `compute_contacts(0)` over the whole scene |
| `1_elements_tree` | The same bay three times, each under its own tree branch, so `compute_contacts(1)` stays inside a branch; `instance_by_key` for one definition per repeated element |
| `2_datasets` | The three loaders: a dataset yml, an obj alone, a session `.pb` |
| `3_contacts` | Face, axis and cross contacts on one dataset, each read through the interaction of its edge |
| `4_features` | The joinery pipeline: every plate joint, a plate's geometry alone and cut, the joint features on the hosts |
| `5_traversal` | From a joint to its plates, the interactions on their edge, the contact it names by guid, the edge |
| `6_settings` | The solver settings as a value on the scene, set in code and read back from the file |
| `7_custom_joint` | A joint variant supplied as outlines through the settings |
| `8_assignment` | Feature types and insertion vectors filled from points and lines placed on the plates |
| `9_beams` | Beams, axis contacts and one beam feature per pair with its four volume rectangles |
| `10_serialization` | Bytes and back, the kernel reading the same bytes, one record as JSON and as protobuf |
| `11_viewer` | The scene arranged for the viewer, the colour tables, the files `pb_dump` writes |
| `12_cross_joints` | Cross joints and the search type chosen per solve |

Elements go under the tree node `add(element, parent)` names, under the root without one; there is no separate tree call. The generators under `src/templates/` have their own page, [Templates](@ref templates), with a screenshot and the code of every `templates_*` example.

The regression programs stay beside them: `main_all_datasets` runs every dataset in `data/` and writes the outline dumps a refactor is diffed against, `main_dataset_runner` one dataset, `main_session_round_trip` and `main_element_mapping_check` check the file and the element registry with an exit code.

## 1_elements

\include{lineno} 1_elements.cpp

## 1_elements_flat

\include{lineno} 1_elements_flat.cpp

## 1_elements_tree

\include{lineno} 1_elements_tree.cpp

## 2_datasets

\include{lineno} 2_datasets.cpp

## 3_contacts

\include{lineno} 3_contacts.cpp

## 4_features

\include{lineno} 4_features.cpp

## 5_traversal

\include{lineno} 5_traversal.cpp

## 6_settings

\include{lineno} 6_settings.cpp

## 7_custom_joint

\include{lineno} 7_custom_joint.cpp

## 8_assignment

\include{lineno} 8_assignment.cpp

## 9_beams

\include{lineno} 9_beams.cpp

## 10_serialization

\include{lineno} 10_serialization.cpp

## 11_viewer

\include{lineno} 11_viewer.cpp

## 12_cross_joints

\include{lineno} 12_cross_joints.cpp
