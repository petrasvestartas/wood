# Examples {#examples}

Every program under `examples/` is one target of the CMake project; build it with `cmake --build build --target <name> --parallel 4` and run it from the `wood` directory. Each one ends with a comment block that says what it does and how to run it. The sources are below, in the order to read them.

| Example | What it does |
|---|---|
| `1_io` | Loads a session `.pb`, prints it, writes it back through `pb_dump` |
| `2_contact_detection` | A dataset through `compute_contacts`; contacts drawn per element for the viewer |
| `3_joint_detection` | A dataset through `compute_features`; joints drawn per element for the viewer |
| `4_wood_session_api` | The WoodSession API end to end: elements, interactions, contacts and joints, the two geometries of a plate, the file |
| `main_hello` | Plates built in code, custom joint outlines in the settings, one solve |
| `main_joint_types` | The same dataset solved with different joint ids from `settings.joint_parameters` |
| `main_cross_corners` | Cross joints on plates built in code, with a joint volume extension |
| `main_dataset_runner` | One dataset from `data/`, chosen in `main` |
| `main_all_datasets` | Every dataset in `data/`, the regression sweep; each writes `data/output/WoodF2F_<name>.pb` with its outline dumps |
| `main_session_round_trip` | Load, solve, write, read back; checks every record survives the file, exit code is the failure count |
| `main_element_mapping_check` | One plate through the kernel's element registry and back; checks the fields the registry rebuilds |

## 1_io

\include{lineno} 1_io.cpp

## 2_contact_detection

\include{lineno} 2_contact_detection.cpp

## 3_joint_detection

\include{lineno} 3_joint_detection.cpp

## 4_wood_session_api

\include{lineno} 4_wood_session_api.cpp

## main_hello

\include{lineno} main_hello.cpp

## main_joint_types

\include{lineno} main_joint_types.cpp

## main_cross_corners

\include{lineno} main_cross_corners.cpp

## main_dataset_runner

\include{lineno} main_dataset_runner.cpp

## main_all_datasets

\include{lineno} main_all_datasets.cpp

## main_session_round_trip

\include{lineno} main_session_round_trip.cpp

## main_element_mapping_check

\include{lineno} main_element_mapping_check.cpp
