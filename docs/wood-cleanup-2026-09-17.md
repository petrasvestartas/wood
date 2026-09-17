# Wood cleanup - 2026-09-17

One hierarchy, one loft, one session, readable files. Every phase was checked against a
baseline taken from a clean checkout of `44813ff`: the 44-dataset sweep's
`WoodF2F_<name>.pb_coords.txt` / `_meta.txt` dumps plus the `WOOD_F2F_DUMP` type-13 log, 89
files, compared byte for byte after every commit. The kernel work was checked with the
minitests of all three languages.

## Elements

parity : `Plate`, `Column`, `Block` derive directly from `session_cpp::Element`
(`wood_element_plate.h`, `wood_element_column.h`, `wood_element_block.h`) and register factories,
so `Session::pb_load` returns them typed; `WoodSession::pb_load` is that and nothing else.
`TaggedElement`, the `WoodGeometry` variant, `WoodSession::elements`, `to_element` /
`from_element` / `sync_element` / `to_session` / `from_session`, `fill_session` and
`wood_element.h` are gone. `WoodJoint`, `FaceContact`, `ContactPair`, `ContactType` live in
`wood_joint.h`; the globals in `wood_globals.h`; `CrossJoint` / `plane_to_face` in
`wood_joint_detection.h`.

safety : the solver works in place on `std::vector<std::shared_ptr<Plate>>&`; no plate is
copied, no guid re-minted. Contact detection reads every element through one
`ContactElement` snapshot instead of three template instantiations.

tests : `main_session_round_trip` and `main_element_mapping_check` exit 0 (guids, thickness,
joint types, features and the registry rebuild checked); `1_io` loads a 237-element session as
153 plates, 4 columns, 80 blocks.

ran : sweep identical after each of the three commits.

## Loft and session

parity : `Plate::compute_geometry()` is the only loft, once per plate per run (was three); the
Element geometry is the plate with its cuts, so the viewer adds nothing beside it.
`add_outlines`, `add_contacts`, `add_joints`, `write(name)` are `WoodSession` members;
`write` carries the parity dumps and re-attaches every joint feature to its host element.

safety : `3_joint_detection` wrote its joints into one session and dumped another; fixed. The
dataset loader no longer overwrites the output name the yml chose.

tests : the 43 dataset tests collapsed onto `run_dataset(name)`; ctest `wood_assign`,
`wood_solver`, `wood_dataset_runner` pass.

ran : sweep identical.

## Polyline

parity : kernel additions in session_cpp, session_py and session_rust, tested in each:
`Vector::average_normal(Polyline)`, `Mesh::from_polylines(std::vector<Polyline>)`,
`Plane::squared_distance(Point)` (Python and Rust siblings `average_normal_polyline`,
`from_polylines_polyline`, `squared_distance`). Wood reads polylines through their own API;
`std::vector<Point>` remains only inside the merge and linked-joint algorithms.

ran : minitests cpp 801/801, rust 801/801, python vector 22/22, mesh 51/51, plane 16/16;
sweep identical.

## Splits and style

parity : `face_to_face_wood` (896 lines), `get_connection_zones` (699), `merge_joints_for_element`
(594) split by pure extraction into stage functions of ~60 lines each. Hand-coded coordinate
arithmetic replaced by `Point` / `Vector` / `Line` / `Plane` operators everywhere an
operation-for-operation identical form exists; lengths keep `std::sqrt(v.magnitude_squared())`
because the kernel's `magnitude()` uses a scaled algorithm.

style : no file headers, `═` banners only, one-line docstrings, no prints in library code
except single-line warnings about bad input, one `constexpr bool TRACE` per file,
`WOOD_F2F_DUMP` the only environment flag.

ran : sweep identical after each of the seven commits.

## Datasets and joint volume extension

parity : yml keys are now `obj`, the sidecars, `search_type`, `joints_parameters_and_types`,
`joint_volume_extension` (3 entries, or one triple per joint class), `joint_scale`, the
tolerances, and `beams` for beam datasets; `output_geometry_type`, `existing_types`,
`run_count`, `path_and_file_for_joints`, `data_set_output_database`, `data_set_output_file`
are gone, the output name is `WoodF2F_<yml stem>.pb`. `vidy_folding` got its legacy
`[0, 0, -20]` back, `vidy_one_layer` its 0.1 duplicate tolerance, `phanomema_node` its beam block.

safety : cross joints accept negative extensions (`!= 0` gates instead of `> 0`); top-side
joints re-derive their quads after the line extension, so the length now reaches them; the
reject guard only fires for a shrinking extension; the triple is chosen by joint class
(side-side, top-side, top-top, cross), not by adjacency-pair index. Not done: pushing the
length extension into `apply_unit_scale`; a unit-scale joint spans the plate thickness by
design and the dataset values would collapse it. The kernel's yaml reader dropped the minus of
a negative list item (`- -20` read as `20`), so every negative extension in the ymls had been
applied with the wrong sign since the ymls replaced the C++ test bodies; fixed in
`session_cpp/src/yaml/yaml.cpp` (session commit 0cdb2bda).

ran : with the yaml sign fixed the sweep differs from the baseline in 19 datasets, all of
them carrying a negative extension: `cross_ibois_pavilion`, `hexboxes`,
`inplane_differentdirections`, `inplane_hexshell`, `outofplane_box`, `outofplane_box_miter`,
`outofplane_dodecahedron`, `outofplane_icosahedron`, `outofplane_octahedron`,
`outofplane_tetra`, `simple_corners`, `simple_corners_combined`, `simple_corners_diff_lengths`,
`vda_floor_2`, `vidy_corner`, `vidy_folding`, `vidy_full`, `vidy_one_axis_two_layers`,
`vidy_one_layer`. In eleven of them the outline vertex counts drop as well, because a
shortened joint line gets fewer divisions; in five (`hexboxes`, `outofplane_icosahedron`,
`vidy_full`, `vidy_one_axis_two_layers`, `vidy_one_layer`) whole joints disappear, because the
reject guard drops a joint whose line is shorter than twice the negative length extension, as
the 2024 code did with the same values set in C++. Measured on `inplane_hexshell`, width -5:
the seam edge is 39.4 mm against 49.4 mm at 0 and 59.4 mm when the sign was dropped.

## Left for a follow-up

- `wood_nano` binds the old API (`std::vector<WoodElement>`, `loft_mesh`, `_wood_element.cpp`,
  `_joinery_solver.cpp`, its CMake source list) and needs the same rename.
- `tests/cpp_runner_test.py` and the `wood_cpp_runner` ctest are deleted: they tested a `bash/cpp.sh`
  that does not exist. Also deleted: `main_export_xml` (legacy XML input), `main_json_session`
  (legacy JSON input), `main_cdt_probe` and `main_loft_holes` (kernel-only probes), and the four
  `src/templates/temp/` mains with the chevron reference files.
- The `.claude/skills/wood-kernel` skill and `wood-agent` read `memory/wood_kernel.md`, which
  moved to `docs/wood_kernel.md`.
