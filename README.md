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
| `compute_face_contacts(level)` | coplanar face overlaps between elements under the same tree node at that depth (0 = all), onto the graph edges |
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

Joints are elements: `JointPlate` dispatches the existing library and custom builders,
`JointBeam` owns beam joint volumes, and `JointAnnen` / `JointVidy` group connections
across several plates. Contacts remain on host-pair edges; feature edges run from
the joint to each target. `Joint` supports plane, concave profile, drill and closed
mesh cutters. Compatible extrusions use polygon booleans; other solids use polygon
splitting. Drill meshes use `line_radius` and `chord_tolerance`; their BReps retain
exact cylinders. Cut target BReps are faceted. See [the cutting gallery](docs/cutting_gallery.md)
for examples, limits and Cloudflare publishing commands.

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
| `1_elements` … `14_cross_joints` | `examples/`: one behaviour each, in reading order; the list is in `docs/examples.md` and on the docs site |
| `3_elements_tree` | a two storey L of five bays, a `wood_grid::Grid`: a branch per level and a group per kind of element; pyramid column heads, flush decks cut by the heads, `instance_by_key` |
| `main_all_datasets`, `main_dataset_runner` | the sweep, and one dataset of it |
| `main_session_round_trip`, `main_element_mapping_check` | round-trip checks, exit code = failures |
| `templates_shells_1_translation_shell`, `templates_folding_1_reflex_fold`, `templates_folding_2_diamond_mesh`, `templates_folding_3_prismatic_fold`, `templates_folding_4_folded_barrel`, `templates_folding_5_counter_fold`, `templates_folding_6_elevated_fold`, `templates_folding_7_conical_fold`, `templates_folding_8_two_hinged_frame`, `templates_folding_9_three_hinged_frame`, `templates_folding_10_three_hinged_arch`, `templates_folding_11_triangle_fold` ... `templates_folding_15_truncated_pyramid` ... `templates_folding_39_truncated_pyramid_corner_edge_folds`, `templates_curved_folding_1_curved_fold` ... `templates_curved_folding_19_vault_field`, `templates_curved_folding_20_curved_crease_unfolding`, `templates_curved_folding_21_catenary_unfolding`, `templates_curved_folding_22_crease_quadrants`, `templates_curved_folding_23_synclastic_unfolding`, `templates_curved_folding_24_anticlastic_unfolding`, `templates_curved_folding_25_apex_level_unfolding`, `templates_curved_folding_26_support_level_unfolding`, `templates_curved_folding_27_alternating_unfolding`, `templates_curved_folding_28_sideways_unfolding`, `templates_curved_folding_29_corrugated_dome`, `templates_curved_folding_30_corrugated_funnel`, `templates_chevron_1_chevron`, `templates_cross_1_vda_mesh`, `templates_reciprocal_2_move`, `templates_reciprocal_1_rotation` | each shell template built with its defaults and written to `live.pb` as a mesh plus its plates |
| `templates_module_1_wall` | `src/templates/module/wall.h`: a framed wall module with a door and a window, beams of one section with the studs on a 625 mm grid and jamb studs at every opening, one sheathing plate on each face with the openings cut out, a face contact between every pair that touches |
| `templates_module_2_cassette_shell` | `src/templates/module/cassette_shell.h`: a segmented shell of hollow cassettes on the VDA dome, two thin plates and mitered edge beams per cell cut on the bevel shared with each neighbour, every contact between cassettes detected |
| `templates_module_3_cassette_house` | `CassetteShell::house_mesh`: a 6000 x 4000 gable house of six cassettes, one per face, the door a notch in the front, each two panels with its windows cut out and a framed core, joined in T joints, every contact between cassettes detected |
| `templates_module_4_room_module`, `templates_module_5_module_block`, `templates_module_6_bank_pavilion` | `src/templates/module/room_module.h`: timber frame modules after Blumer Lehmann, 9 x 3 m with 2.7 m clear, floor and ceiling of long edge beams a wall wide over the whole length on and under the corner posts, joists across flush on both faces, the ceiling thinner with deep edge beams, the ceiling edge beam one deeper beam on the open sides, on corner posts, walls only on the closed sides, two forming one room open between them with a post at mid-length; a block of four side by side and three storeys like the ZM10 pavilions; and the bank pavilion of four modules from the Blumer Lehmann brochure, measured from its plan; every contact detected |
| `templates_grid_1_footprint` | `src/templates/grid/grid.h` on footprints with patterns (workflow B), side by side: orthogonal with purlins and facade, skewed, radial, triangular, hex, irregular, courtyard, pentagon, U with two cores; every joint a plane cut, written to `live.pb` |
| `templates_grid_2_solid` | the same from massing solids sliced at the elevations (workflow A): box, prism, taper, setback, atrium, curved |
| `templates_grid_3_lines` | the same from drawn lines and surfaces (workflow C): the crea dataset and a braced frame |
| `templates_grid_4_reference`, `templates_grid_5_framing` | the Branch3D and FAST+EPP configurations reproduced; heads (column section, conical, stepped), node flush/through, purlin joints, the profile library, and a clash check that exits 1 on any overlap |

Architecture notes: `docs/wood_kernel.md`; every template with a screenshot: `docs/templates.md`.
