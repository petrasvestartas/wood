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

WoodSession scene = WoodSession::yaml_load(globals::Dataset::inplane_hexshell);
scene.compute_joints();      // search type and every tunable come from the yml
scene.add_joints();          // coloured joint rings, one group per joint type
scene.write();               // data/output/pb/live.pb, the file session_viewer watches
```

`WoodSession` is a `session_cpp::Session`; every plate in it is a `Plate`, every joint
sits on the graph edge between its two plates, and `write()` puts each joint back on its
host element as an `ElementFeature`. `pb_load(name)` reads a session back with its plates,
columns and blocks as the classes below, through the kernel's element registry.

| Method | What it does |
|---|---|
| `yaml_load(name)` | data/`name`.yml globals, then the obj it names as plates |
| `pb_load(name)` | data/`name`.pb, elements rebuilt as `Plate` / `Column` / `Block` |
| `compute_contacts()` | coplanar face overlaps between every pair, onto the graph edges |
| `compute_cross_contacts()`, `compute_line_contacts()` | plates passing through each other, outline crossings |
| `compute_joints(search)` | the solver over the plates, in place; each plate lofted once with its cuts |
| `add_outlines()`, `add_contacts()`, `add_joints()` | viewer geometry, grouped by class or type |
| `write(name)` | `data/output/pb/<name>.pb`; a name ending in `.pb` goes to `data/output/` with the outline dumps beside it |

## Types

| Type | File | What it is |
|---|---|---|
| `Plate` | `src/joinery_solver/wood_element_plate.h` | bottom + top outline, one side face per edge, thickness, joint types, merged cut outlines; lofts itself once |
| `Column` | `src/joinery_solver/wood_element_column.h` | a solid with an axis and a section |
| `Block` | `src/joinery_solver/wood_element_block.h` | a solid, one face per closed loop, contact detection only |
| `WoodJoint` | `src/joinery_solver/wood_joint.h` | one connection: type, area, lines, volumes, male and female cut outlines |
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
datasets a `beams` block. `globals::Dataset::<name>` names every shipped dataset.

`main_all_datasets` runs all of them and writes `data/output/WoodF2F_<name>.pb` with
`_meta.txt` and `_coords.txt` beside each: the record a refactor is diffed against.

## Targets

| Target | Source |
|---|---|
| `1_io`, `2_contact_detection`, `3_joint_detection` | `examples/` walk-throughs of load, contacts, joints |
| `main_hello` | plates and a custom butterfly joint built in code |
| `main_all_datasets`, `main_dataset_runner` | the sweep, and one dataset of it |
| `main_session_round_trip`, `main_element_mapping_check` | round-trip checks, exit code = failures |
| `main_json_session`, `main_joint_types`, `main_cross_corners`, `main_loft_holes`, `main_cdt_probe`, `main_export_xml` | smaller probes |
| `main_translation_shell`, `main_reflex_fold`, `main_chevron`, `main_reciprocal_*`, `main_annen_chevron`, `main_beam_reciprocal`, `main_chevron_test`, `main_vda_mesh` | `src/templates/` generators |

Tests: `ctest --test-dir build`. Architecture notes: `docs/wood_kernel.md`.
