# wood

Timber joinery over the `session_cpp` kernel: plates touch, the solver finds where, decides
which joint fits, and cuts it into both plates.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
```

The kernel is resolved from `../session/session_cpp` (or `-DSESSION_CPP_LOCAL=<dir>`).
Run every solver and example through the guard, never directly:

```bash
tools/run_guarded.sh -t 10 -m 4 -- build/3_joint_detection
```

## Use

```cpp
#include "wood_session.h"
using namespace wood_session;

WoodSession scene = WoodSession::yaml_load(config::Dataset::inplane_hexshell);
scene.compute_features();      // search type and every tunable come from the yml
scene.add_to_tree();         // one group per plate: the plate, its outlines, its contacts, its joints
scene.pb_dump(pb_path("live").string());   // the file session_viewer watches
```

`WoodSession` is a `session_cpp::Session`; every plate in it is a `Plate`, every joint
sits on the graph edge between its two plates and on both host elements as an
`ElementFeature`. `pb_load(name)` reads a session back with its plates,
columns and blocks as the classes below, through the kernel's element registry.

| Method | What it does |
|---|---|
| `yaml_load(name)` | data/`name`.yml globals, then the obj it names as plates |
| `pb_load(name)` | data/`name`.pb, elements rebuilt as `Plate` / `Column` / `Block` |
| `compute_contacts()` | coplanar face overlaps between every pair, onto the graph edges |
| `compute_cross_contacts()`, `compute_line_contacts()` | plates passing through each other, outline crossings |
| `compute_features(search)` | the solver over the plates, in place; the plates stay outlines, nothing is lofted |
| `add_to_tree(geometry, outlines, contacts, joints)` | one group per element with those child groups; each flag adds or leaves out that part |
| `pb_dump(pb_path(name))` | lofts every plate not yet lofted, then the kernel's writer; `write_parity_dumps(scene, pb)` adds the outline dumps the sweep is diffed against |

## Types

| Type | File | What it is |
|---|---|---|
| `Plate` | `src/joinery_solver/wood_elements/wood_element_plate.h` | bottom + top outline (`Plate::from_rectangle` for the simple case), one side face per edge, thickness, joint types, merged cut outlines; element and model geometry as mesh or brep, lazy |
| `Column` | `src/joinery_solver/wood_elements/wood_element_column.h` | a solid with an axis and a section |
| `Block` | `src/joinery_solver/wood_elements/wood_element_block.h` | a solid, one face per closed loop, contact detection only |
| `Interaction` | `src/joinery_solver/wood_interaction/wood_interaction.h` | everything between two elements, keyed by their graph edge: contacts (`ContactFace`, `ContactAxis`, `ContactCross`), features (`FeaturePlate`, `FeatureBeam`), structure; see `src/docs.md` |
| `FeaturePlate` | `src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_plate.h` | one plate joint: the pair, its contact, type, lines, volumes, male and female cut outlines |
| `WoodSession` | `src/joinery_solver/wood_session.h` | the scene |

All three element classes derive from `session_cpp::Element` and register a factory, so any
`Session` that holds them reads and writes them without knowing wood.

Joint type codes: 11 side-side out of plane, 12 side-side in plane, 13 side-side rotated,
20 top-side, 30 cross, 40 top-top.

## Datasets

Every dataset is `data/<name>.yml` plus the obj it names, with optional `adjacency`,
`three_valence`, `insertion_vectors` and `joints_types` text sidecars. The yml carries every
tunable the solver reads: `search_type`, `joints_parameters_and_types` (7 families x
division length, shift, joint id), `joint_volume_extension` (width, height, length in mm, one
triple for all families or one per family), `joint_scale`, the tolerances, and for beam
datasets a `beams` block. `config::Dataset::<name>` names every shipped dataset.

`main_all_datasets` runs all of them and writes `data/output/WoodF2F_<name>.pb` with
`_meta.txt` and `_coords.txt` beside each: the record a refactor is diffed against.

## Docs

`src/docs.md` is the one-page description of the structure. The site around it, one page per section with diagrams and the API generated from the headers, builds with:

```bash
pip install -r requirements-docs.txt
python3 tools/api_docs.py && mkdocs serve
```

## Targets

| Target | Source |
|---|---|
| `1_io`, `2_contact_detection`, `3_joint_detection`, `4_wood_session_api` | `examples/` walk-throughs of load, contacts, joints, and the whole API in compas_model order |
| `main_hello` | plates and a custom butterfly joint built in code |
| `main_all_datasets`, `main_dataset_runner` | the sweep, and one dataset of it |
| `main_session_round_trip`, `main_element_mapping_check` | round-trip checks, exit code = failures |
| `main_joint_types`, `main_cross_corners` | joint type and cross corner probes |
| `main_translation_shell`, `main_reflex_fold`, `main_chevron`, `main_reciprocal_move`, `main_reciprocal_rotation` | `examples/templates/`: each template built with its defaults and written to `live.pb` as a mesh plus its plates |

Tests: `ctest --test-dir build`. Architecture notes: `docs/wood_kernel.md`.
