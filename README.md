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
| `compute_contacts(level)` | coplanar face overlaps between elements under the same tree node at that depth (0 = all), onto the graph edges |
| `compute_cross_contacts()`, `compute_line_contacts()` | plates passing through each other, outline crossings |
| `compute_features(search)` | the solver over the plates, in place; the plates stay outlines, nothing is lofted |
| `set_features_visible(type, on)` | shows or hides one feature kind (`contact`, `joint`, `outline`, ...); the viewer draws every visible feature |
| `instance_by_key()` | every repeated column, beam, block or plate becomes an instance of one definition, guid, name, tree node, edges and features kept |
| `pb_dump(pb_path(name))` | lofts every plate not yet lofted, syncs contacts and joints onto the elements as features, then the kernel's writer; `write_parity_dumps(scene, pb)` adds the outline dumps the sweep is diffed against |

## Types

| Type | File | What it is |
|---|---|---|
| `Plate` | `src/joinery_solver/wood_elements/wood_element_plate.h` | bottom + top outline (`Plate::from_rectangle` for the simple case), one side face per edge, thickness, joint types, merged cut outlines; element and model geometry as mesh or brep, lazy |
| `Beam` | `src/joinery_solver/wood_elements/wood_element_beam.h` | a profile swept along a polyline axis, cut by the planes in `cuts` |
| `Column` | `src/joinery_solver/wood_elements/wood_element_column.h` | a profile lofted along a line axis, cut by the planes in `cuts` |
| `Block` | `src/joinery_solver/wood_elements/wood_element_block.h` | the capped loft between bottom and top loops, cut by the planes in `cuts` |
| `Interaction` subclasses | `src/joinery_solver/wood_interaction/wood_interaction.h` | what joins two elements, a list per graph edge in the kernel's `Session::interactions`: contacts (`InteractionContactFace`, `InteractionContactAxis`, `InteractionContactCross`), features (`InteractionFeaturePlate`, `InteractionFeatureBeam`), structure; see `src/docs.md` |
| `InteractionFeaturePlate` | `src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_plate.h` | one plate joint: the pair, its contact, type, lines, volumes, male and female cut outlines |
| `WoodSession` | `src/joinery_solver/wood_session.h` | the scene |

All four element classes derive from `session_cpp::Element` and register a factory, so any
`Session` that holds them reads and writes them without knowing wood. Profiles come from
`wood_profile.h`: rectangle, round, W, HSS, double, slab band, T.

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

`src/docs.md` is the description of the structure with its diagrams, and the main page of the docs. Doxygen builds the site from it, `docs/examples.md`, `docs/templates.md` (every template and grid example with its screenshot from `docs/images/templates/` and its code) and the headers' docstrings, nothing else needed:

```bash
cmake --build build --target docs && xdg-open build/docs/html/index.html
```

Every push builds the same site in CI and publishes it at https://petrasvestartas.github.io/wood/.

## Targets

| Target | Source |
|---|---|
| `1_elements` … `12_cross_joints` | `examples/`: one behaviour each, in reading order; the list is in `docs/examples.md` and on the docs site |
| `1_elements_flat`, `1_elements_tree` | one floor bay from the grid template under the root, and three bays under tree branches; `compute_contacts(0)` and `(1)`, `instance_by_key` |
| `main_all_datasets`, `main_dataset_runner` | the sweep, and one dataset of it |
| `main_session_round_trip`, `main_element_mapping_check` | round-trip checks, exit code = failures |
| `templates_translation_shell`, `templates_reflex_fold`, `templates_chevron`, `templates_diamond_mesh`, `templates_vda_mesh`, `templates_reciprocal_move`, `templates_reciprocal_rotation` | each shell template built with its defaults and written to `live.pb` as a mesh plus its plates |
| `templates_gridshell` | `src/templates/shells/lamella_gridshell.h`: asymptotic and iso-curve gridshells on a saddle surface, then asymptotic ones on three minimal meshes (a relaxed disk, a catenoid annulus, an Enneper disk); two upright BRep boards per lamella (four smooth rails, ruled faces) in two layers and a hexagonal stud at every crossing; prints per scene the minimality of the mesh, the normal curvature and unrolled straightness of the lamellas and the largest overlap, exits 1 unless every board is a valid BRep solid, every stud touches its four boards and nothing overlaps |
| `templates_grid_footprint` | `src/templates/grid/grid.h` on footprints with patterns (workflow B), side by side: orthogonal with purlins and facade, skewed, radial, triangular, hex, irregular, courtyard, pentagon, U with two cores; every joint a plane cut, written to `live.pb` |
| `templates_grid_solid` | the same from massing solids sliced at the elevations (workflow A): box, prism, taper, setback, atrium, curved |
| `templates_grid_lines` | the same from drawn lines and surfaces (workflow C): the crea dataset and a braced frame |
| `templates_grid_reference`, `templates_grid_framing` | the Branch3D and FAST+EPP configurations reproduced; heads (column section, conical, stepped), node flush/through, purlin joints, the profile library, and a clash check that exits 1 on any overlap |

Architecture notes: `docs/wood_kernel.md`; every template with a screenshot: `docs/templates.md`.
