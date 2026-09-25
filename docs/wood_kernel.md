# Wood Kernel Reference

Architecture reference for the wood timber-joinery solver as it is today: a flat C++ tree in
`src/joinery_solver/` built on the `session_cpp` geometry kernel at `../session/session_cpp/src`.
There is no CGAL, no XML input and no `shapes/` pipeline any more. Every path and function
named here exists in the current tree.

## 1. Repository layout

| Path | Contents |
|---|---|
| `src/joinery_solver/wood_elements/wood_element_{plate,column,block,beam}.h/.cpp` | `Plate`, `Column`, `Block`, `Beam` : `session_cpp::Element`; `Plate::flip` is the recorded mid-run face swap; `cuts` on a beam, column or block are the planes its model solid is cut by; `element_data` is `wood_proto.{Plate,Beam,Column,Block}` |
| `src/joinery_solver/wood_elements/wood_element_geometry.h/.cpp` | what the elements share: `sweep_sections`, `brep_sections`, `compute_newell`, `face_planes`, `compute_volume`, `is_geometry_feature` |
| `src/joinery_solver/wood_elements/wood_profile.h/.cpp` | `profile_rectangle`, `profile_round`, `profile_w`, `profile_hss`, `profile_double`, `profile_slab_band`, `profile_t`, `compute_size`, `profile_section` |
| `src/templates/grid/grid_plan.h/.cpp` | the plan algorithms of the grid template: `compute_section` of a solid at a height, ring booleans, offsets and mitres, `compute_crossings` and `compute_arrangement` of lines into a `Mesh` arrangement, direction polygons at a node |
| `src/joinery_solver/wood_instance.h/.cpp` | `element_key`: the class key and frame `WoodSession::instance_by_key` dedups by |
| `src/joinery_solver/wood_interaction/**` | the connectivity classes, one per file (see `src/docs.md`), all derived from the kernel's abstract `session_cpp::Interaction`: abstract `InteractionContact` ← `InteractionContactFace` / `InteractionContactAxis` / `InteractionContactCross`, abstract `InteractionFeature` ← `InteractionFeaturePlate` / `InteractionFeatureBeam` / `InteractionFeaturePlateBeam`, abstract `InteractionStructure` with no leaf yet; each leaf registers a factory with the kernel and writes its fields as `interaction_data` |
| `src/joinery_solver/wood_algorithms/wood_feature_construction.h/.cpp` | `apply_unit_scale`, `joint_orient_to_connection_area`, `merge_linked_joints`, `joint_get_divisions`, `joint_volume_extension`, `index_of` over a `InteractionFeaturePlate` |
| `src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_fabrication_type.h` | `wood_session::FabricationType`, one per cut outline |
| `src/joinery_solver/wood_settings.h/.cpp` | `Settings`: every solver tunable, from the yml, held by the scene, passed by reference, written with the file |
| `src/joinery_solver/wood_config.h/.cpp` | `wood_session::config`: `Dataset::` names, `load_yaml` (returns a `Settings`, sets the dataset paths), `reset_defaults`, the paths |
| `src/joinery_solver/wood_io.h/.cpp` | `io::load_obj`, the four sidecar readers, `io::pb_path`, `io::write_parity_dumps` |
| `src/joinery_solver/wood_view.h/.cpp` | contact and joint names and colours |
| `src/joinery_solver/wood_serialization.h/.cpp` | `json_of` / `message_from_json`: JSON derived from any proto message |
| `src/joinery_solver/wood_algorithms/wood_contact_detection.h/.cpp` | `adjacency_search`, `faces_coplanar`, `face_overlap_area`, `face_contacts_for_pair`, `face_contacts`, `plane_to_face` (a `InteractionContactCross`) over kernel elements |
| `src/joinery_solver/wood_algorithms/wood_feature_detection.h/.cpp` | `face_to_face_wood`: one plate pair to one `InteractionFeaturePlate`; `DetectionTrace` for the counts |
| `src/joinery_solver/wood_algorithms/wood_feature_detection_beam.h/.cpp` | `beam_to_beam`: one beam pair and its axis contact to one `InteractionFeatureBeam` |
| `src/joinery_solver/wood_algorithms/wood_assignment.h/.cpp` | `assign_feature_types`, `assign_insertion_vectors`: points and lines placed on plates into their slots |
| `src/joinery_solver/wood_algorithms/wood_feature_solver.cpp` | `WoodSession::compute_features` pipeline: `adjacent_pairs`, `detect_features`, `build_feature_geometry`, `merge_features`; `joint_create_geometry` dispatcher; `get_connection_zones` shims |
| `src/joinery_solver/wood_algorithms/wood_merge_modifier.h/.cpp` | `MergeModifier::apply` |
| `src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_plate_joints.h`, `wood_interaction_feature_plate_joints/*.h` | aggregator + one static constructor per joint variant, `tt_e_p_*` and `side_removal` included |
| `src/joinery_solver/wood_session.h/.cpp` | `WoodSession` (`pb_load`, `obj_load`, `yaml_load`, `load_sidecars`, `add_interaction` / `remove_interaction` with the wood rules over the kernel's `interactions` store keyed by edge guid, `adjacency`, `three_valence`, `get_plate_features`), `SearchType`, `type_plates_name_*` decls |
| `src/proto/*.proto`, `generated/` | one `wood_proto` message per class and the committed protoc output (`tools/regen_proto.sh`); `wood_session.proto` is the file format, a superset of `session_proto.Session` |
| `src/joinery_solver/wood_test.h/.cpp` | dataset runners, one per `data/*.yml` |
| `src/templates/` | the generators, one folder per family, one page with screenshots in `docs/templates.md`: header-only Plates in `shells/` (`translation_shell.h`, `chevron.h`), `folding/` (`reflex_fold.h`, `diamond_mesh.h`), `cross/` (`vda_mesh.h`) and `reciprocal/` (`reciprocal_*.h`); the building template in `grid/` (`grid.h/.cpp`: `Pattern`, `Framing`, `Building`; columns, heads, girders, beams, purlins, braces, decks and walls) |
| `examples/` | `1_elements` … `12_cross_joints` (`docs/examples.md`), `main_dataset_runner`, `main_all_datasets`, `main_session_round_trip`, `main_element_mapping_check`, one `templates_*` main per template and per grid workflow and pattern |
| `data/` | `<name>.yml` + `<name>.obj` + optional `<name>_{adjacency,three_valence,insertion_vectors,joints_types}.txt`; `output/` |
| `tools/` | `run_guarded.sh` / `.ps1` (always use), `xml_to_dataset.py` (legacy XML → yml/obj/txt) |

`CMakeLists.txt` builds `wood_core` (OBJECT, C++23) and one `ADD_EXE` per example and test,
each linking `wood_core` + `session_core`. Run anything through `tools/run_guarded.sh`.

## 2. Core data types

### Elements (`namespace wood_session`)

All three derive directly from `session_cpp::Element` and register a factory with
`Element::register_type` so `Session::pb_load` rebuilds the derived type from the `.pb`
(`register_element_types()` in `wood_session.cpp` calls all three; the `WoodSession`
constructor runs it).

| Class | File | Tag | Holds |
|---|---|---|---|
| `Plate` | `wood_element_plate.h` | `"Plate"` (legacy `"WoodElement"`) | `polylines` ([0] bottom, [1] top, [2..] sides), `planes` (one per outline), `feature_types` (per face; empty = auto), `reversed`, `thickness`, `features` (`Features{top, bottom}`: merged cut outlines, [0] outer, [1..] holes), `insertion_vectors()` |
| `Beam` | `wood_element_beam.h` | `"Beam"` | `axis` (Polyline), `radius`, `profile` (loops in the section frame, empty the square of the radius), `cuts` |
| `Column` | `wood_element_column.h` | `"Column"` | `axis` (Line), `profile`, `cuts`; the solid the profile lofted along the axis, cut by every plane |
| `Block` | `wood_element_block.h` | `"Solid"` (legacy `"BlockElement"`) | bottom and top loops, `cuts`; the capped loft between them cut by every plane |

A plate has two geometries, as a compas_model element does. `element_geometry_mesh()` is the
parametric shape alone, the loft of the two raw outlines, never cut. `model_geometry_mesh()` is the
shape with its joints applied, `Mesh::loft(features.bottom, features.top)` once the merge has
filled `features`, else the element geometry. Both are lazy: nothing lofts until one is
asked for, and the result is cached on the plate until `invalidate_geometry()`, which the merge
calls after filling `features`. `Plate::compute_geometry_mesh()` writes the model geometry onto the
Element slot the session file and the viewer read, then `set_dimensions` and
`set_features(face_features())`; `WoodSession::pb_dump` runs it for every plate whose slot is
stale, so a solve of N plates lofts exactly N times, at write time. `model_geometry_mesh()` is the
full featured one, compas_model's `modelgeometry`; `element_geometry_mesh()` is the plate alone. Each
stage also exists as a boundary representation, `element_geometry_brep()` and
`model_geometry_brep()`, built through `BRep::from_polylines` with holes, cached the same way,
opt-in: the file keeps the mesh by default. Plates, beams, columns and blocks all expose these
four methods, returning `const Mesh&` or `const BRep&` directly. They are virtual methods
on the base `Element` too, so calls through `Element*` or `Element&` dispatch to the
same derived caches. For a plain `Element`, element geometry is the supplied shape,
and model mesh geometry includes its in-memory geometry operations. Each result has its own typed
cache: requesting a mesh never builds a BRep, and requesting a BRep never builds a mesh.
Model geometry may reuse the corresponding element geometry. Serialization retains these
caches; `invalidate_geometry()` and `place()` clear all four. `Plate::face_features()` emits one `ElementFeature` per face:
`"joint_type_<code>"` for a face with a joint type, `"cut"` for a face with outlines.

### Cuts and instances

A beam, column or block carries `cuts`, planes applied to its parametric solid by the kernel's
`Mesh::cut_by_plane` / `BRep::cut_by_plane`, each keeping the side its normal points to; assign
them and call `invalidate_geometry()`. The grid template (`src/templates/grid/grid.h`) resolves every
joint into such planes. `WoodSession::instance_by_key()` replaces every repeated element by an
instance of one definition, keeping guid, name, tree node, edges and features; the passes read
`world_elements()`, every element and instance as world geometry.

### Manual interactions

The kernel's `Session` stores `interactions`: an edge guid to that edge's list of
`std::shared_ptr<Interaction>`. Four methods reach it, each taking two
`std::shared_ptr<Element>` in either order: `add_interaction(a, b, interaction)` makes or reuses
the edge (both elements registered and distinct; an existing edge keeps its attributes) and
appends; `get_interaction(a, b)` returns the list, empty when there is none;
`has_interaction(a, b)` and `remove_interaction(a, b)` test and drop the edge with its list.

`WoodSession::add_interaction` adds the wood rules before calling the kernel's: a contact is
oriented to the stored edge and one coinciding with a stored contact is not stored again, the
stored one is returned; a new contact goes onto the edge's first element as a "contact" feature,
a plate or beam feature onto its elements as "joint" features. `remove_interaction` also takes
those features off. A feature names its contact by `contact_guid`. `Interaction` and the three mid-level classes are
abstract, so every record is a leaf; any leaf can carry a name, e.g. a face contact named "glue".
The plate-to-beam feature is a placeholder with no solver parameters yet, and `InteractionStructure`
has no leaf.

These manual operations store authored data; they do not run detection, solve joints or undo
cuts already merged into an element's geometry. See `examples/1_elements.cpp`.

Removing an element or instance drops its edges' interaction lists with the edges; undo puts
them back as it puts the edges back. A `.pb` written before the kernel held interactions kept
them in `wood_proto.WoodSession` field 100, which is now reserved: those files still load, but
without their interactions; run `compute_contacts` / `compute_features` again to rebuild them.
An interaction whose type has no registered factory is not dropped: it loads as the kernel's
`InteractionUnknown`, which writes its type and data back unchanged on the next save.

### `InteractionFeaturePlate` (`wood_interaction/wood_interaction_feature/wood_interaction_feature_plate.h`)

Fields that matter downstream:

- `element_a` / `element_b` — guids; a is male, b female (the solver may swap). `index_of(elements, guid)` maps to a position.
- `contact` — `InteractionContactFace{face_a, face_b, type, polygon}`; `cross_faces` — second side face per element, type 30 only. `compute_features` stores the whole record on the pair's edge, beside the contact it names by `contact_guid`.
- `joint_type` — 11/12/13/20/30/40 (solver vocabulary, see below).
- `joint_lines[2]`, `joint_volumes[4]` (optional quads; [0],[1] male, [2],[3] female).
- `m_outlines[2]` / `f_outlines[2]` — cut polylines per plate face, unit-box until oriented; `male_fabrication_types` / `female_fabrication_types` — one `FabricationType` per outline.
- `divisions`, `shift`, `length`, `division_length`, `scale[3]`, `unit_scale`, `unit_scale_distance`.
- `linked_joints`, `linked_joints_seq`, `link` (shadow joints from three-valence), `no_orient`.
- `element_features[2]` / `feature_guids[2]` — the kernel view (`sync_features()`, `to_features()`), refreshed at the end of `get_connection_zones`.

### `ContactType` vs `joint_type`

`ContactType` (`wood_joint.h`) is what a face pair says about itself from indices alone:
`side_side=0`, `side_top=1`, `top_top=2`, `cross=3`, `line=4`, `unknown=-1`. `joint_type`
is what `face_to_face_wood` decided with geometry. They are different number spaces:

| joint_type | Meaning | Refines from |
|---|---|---|
| 11 | side-side, out-of-plane (dihedral <= threshold) | side_side |
| 12 | side-side, in-plane | side_side |
| 13 | side-side, rotated / perpendicular | side_side |
| 20 | top-side | side_top |
| 30 | cross (plane_to_face, `CrossJoint`) | cross |
| 40 | top-top | top_top |

`joint_type_name()` in `wood_session.cpp` maps these to `ss_op_11`, `ss_ip_12`, `ss_rot_13`,
`ts_20`, `cross_30`, `tt_40`.

### `wood_session::FabricationType` (`wood_interaction/wood_interaction_feature_fabrication_type.h`)

`nothing=0, hole=1, edge_insertion=2, insert_between_multiple_edges=3, slice=4,
slice_projectsheer=5, mill=6, mill_project=7, mill_projectsheer=8, cut=9, cut_project=10,
cut_projectsheer=11, cut_reverse=12, conic=13, conic_reverse=14, drill=15`. An empty
`*_fabrication_types` array means every outline is `edge_insertion`.

### Settings (`wood_settings.h`, filled by `config::load_yaml("<name>")` from `data/<name>.yml`)

| yml key | Settings field | Meaning |
|---|---|---|
| `joints_parameters_and_types` | `joint_parameters` | 7 rows x (division_length, shift, joint_type_id); rows 0..6 = ss_e_ip, ss_e_op, ts_e_p, cr_c_ip, tt_e_p, ss_e_r, b |
| `joint_volume_extension` | `joint_volume_extension` | (width, height, length) mm added to each joint volume |
| `joint_scale` | `joint_scale` | multiplicative (sx, sy, sz); used by ss_e_ip_2, ss_e_r_*, ts_e_p_5 |
| `distance`, `distance_squared`, `angle` | `distance`, `distance_squared`, `angle` | AABB inflate (mm), coplanarity (mm^2), angular tolerance in **radians** (0.11 ~ 6.3 deg) |
| `limit_min_joint_length`, `duplicate_pts_tol` | `limit_min_joint_length`, `duplicate_points_tolerance` | joint length filter; consecutive-duplicate removal in `load_plates` |
| `face_to_face_side_to_side_joints_dihedral_angle` / `_all_treated_as_rotated` / `_rotated_joint_as_average` | `dihedral_angle`, `all_treated_as_rotated`, `rotated_joint_as_average` | 11-vs-13 split (degrees) and the rotated branch switches |
| `clipper_scale`, `clipper_area` | `clipper_scale`, `clipper_area` | int64 grid (1e6) and minimum overlap area for `face_overlap_area` |
| `obj`, `adjacency`, `three_valence`, `insertion_vectors`, `joints_types` | `DATA_SET_OBJ`, `DATA_SET_ADJACENCY`, … | sidecar files, resolved relative to the yml; empty = derive |
| `search_type` | `SEARCH_TYPE` | `face_to_face`, `cross_joint` or `face_to_face_then_cross`; the default of `compute_features()` |
| `beams` | `BEAMS` | beam datasets only: radius, allowed type, min_distance, volume_length, cross_or_side_to_end, flip_male |

`config::Dataset::<name>` (and `Dataset::Face::` / `::Cross::` / `::Curves::`) give every
dataset name as a constant; `DATASET_NAMES` is the sweep order. `reset_defaults()` restores the
baseline; `load_yaml` calls it first. `CUSTOM_JOINTS_*` are runtime-only (not in yml).

## 3. Detection pipeline (`WoodSession::compute_features`, `wood_joint_solver.cpp`)

```cpp
std::vector<InteractionFeaturePlate> get_connection_zones(std::vector<std::shared_ptr<Plate>>&, SearchType);
enum SearchType { face_to_face = 0, cross_joint = 1, face_to_face_then_cross = 2 };
```

The plate vector is in-out: each `Plate` gets its `features`, its `insertion_vectors()` and
(when `reversed`) its swapped outline/plane pair written back. Stages, in code order:

1. **Sidecars.** Read `DATA_SET_ADJACENCY`, `_THREE_VALENCE`, `_INSERTION_VECTORS`,
   `_JOINTS_TYPES` (set by the yml). Formats: adjacency = one `i j` pair per line; insertion
   vectors = one line per element, `x y z` per face (faces 0/1 are zero); joints_types = one
   int per face per line; three_valence = first line is the instruction flag, then one group
   `s0 s1 e20 e31` per line.
2. **Adjacency.** File pairs if given, else `wood_session::adjacency_search(view, DISTANCE)`
   (`wood_face_to_face.cpp`: OBB per element, BVH over inflated AABBs, OBB/OBB SAT).
3. **Insertion vectors** are moved onto each `Plate::insertion_vectors()` (skipped when the
   caller pre-set them); a reversed plate has its vector order reversed.
4. **Per pair: `face_to_face_wood(...)`** (`wood_face_to_face.cpp`). Scans face pairs with
   `faces_coplanar` + `face_overlap_area` (Clipper2 on int64), then classifies: side-side splits
   on `Point::dihedral_angle_deg` into 11 (<= threshold), 12 (in-plane) or 13 (rotated,
   or forced by `all_treated_as_rotated`); top-side = 20; top-top = 40. With `cross_joint` /
   `face_to_face_then_cross` it calls `plane_to_face` (`wood_joint_detection.cpp`) for type 30.
   If it reports `swap_planes_b`, element b's faces 0 and 1 are swapped in place immediately.
   Output per joint: `contact.polygon`, `joint_lines`, `joint_volumes`.
5. **Three-valence.** Flag 1 → `three_valence_joint_addition_vidy` (creates shadow joints,
   `link = true`); flag 0 → `three_valence_joint_alignment_annen` (shortens overlapping joint
   lines). Joints are never sorted: the first joint with a given cache key fixes the geometry
   for the rest.
6. **Joint type id.** `id_representing_joint_name = max(|jt[a][face_a]|, |jt[b][face_b]|)`,
   read through the pre-reversal face index; 0 or no file → the row default from
   `settings.joint_parameters[row*3+2]` (3, 15, 20, 30, 40, 58, 60). The sign of the id is
   dropped (`std::abs`).
7. **`joint_create_geometry(joint, div_dist, shift, id, context)`** looks the id up in the
   library table (family by tens: 1-9, 10-19, 20-29, 30-39, 40-49, 50-59, 60-69), checks it agrees
   with `joint_type` (mismatch → no outlines), then dispatches by exact id to a `wood_interaction_feature_plate_joints/*.h`
   constructor. `joint_get_divisions` runs first so the cache key `"id;shift;divisions"`
   matches; the cache is applied only to type 12 with no linked joints. Types 12/13 pre-set
   `unit_scale_distance = plate thickness`.
8. **Orient.** `joint_orient_to_connection_area` (`wood_algorithms/wood_feature_construction.cpp`): `apply_unit_scale` (moves
   the two volume quads to `unit_scale_distance` apart when `unit_scale`), then
   `Xform::from_change_of_basis(vols[0], vols[1])` for male and `(vols[2], vols[3])` for
   female outlines. Ids 15/16 (ss_e_op_5) orient their shadow joints and call
   `merge_linked_joints`. `no_orient` skips this.
9. **Merge.** Build `j_mf[element][face] = [(joint_idx, is_male)]` (extra last slot for shadow
   joints), call `merge_joints_for_element` (`wood_merge.cpp`) per plate, and de-interleave
   its `[hole_top, hole_bot, …, outer_top, outer_bot]` result into `Plate::features`
   (outer first, then holes). Finally `sync_features()` on every joint.

`WoodSession::load_sidecars` reads the adjacency and three-valence sidecars onto the scene and
the insertion vectors and joint types onto every plate that carries none; set those fields
yourself to solve without files.

Diagnostics: `WOOD_F2F_DUMP=<path>` (environment) appends every type-13 volume to that file;
every other trace is a `constexpr bool TRACE = false;` at the top of its own file.

## 4. Joint library (`wood_interaction_feature_plate_joints.h` + `wood_interaction_feature_plate_joints/*.h`)

Each `<name>.h` is a plain header (no include guard, no includes) holding one
`static void <name>(InteractionFeaturePlate&)`; the aggregator includes them in order and is itself
included inside `wood_joint_solver.cpp` (which must include `wood_session.h` first). The `tt_e_p_*`
and `side_removal*` constructors take the plate vector too and have their own headers there.

| joint_type | group | id range | prefix | ids wired in `joint_create_geometry` |
|---|---|---|---|---|
| 12 | 0 | 1-9 | `ss_e_ip` | 1→ss_e_ip_1, 2→_0, 3→_2, 4→_3, 5→_4, 6→_5, 8→side_removal, 9→ss_e_ip_custom |
| 11 | 1 | 10-19 | `ss_e_op` | 10→_1, 11→_2, 12→_0, 13→_3, 14→_4, 15/16→_5 (with/without divisions), 17→_17, 18→_tutorial, 19→_custom |
| 20 | 2 | 20-29 | `ts_e_p` | 20/22→_3, 21→_2, 23→_0, 25→_5, 28→side_removal, 29→_custom |
| 30 | 3 | 30-39 | `cr_c_ip` | 30-35→_0.._5, 38→side_removal, 39→_custom |
| 40 | 4 | 40-49 | `tt_e_p` | 40-45→_0.._5 (no custom slot wired) |
| 13 | 5 | 50-59 | `ss_e_r` | 54→_3, 55→_2, 56→_0, 57→side_removal, 58→side_removal_ss_e_r_1_port, 59→_custom |
| 60 | 6 | 60-69 | `b` | 60→b_0, 69→b_custom |

Note the id → function number is not identity (id 10 is `ss_e_op_1`, id 12 is `ss_e_op_0`).
An id with no case prints `not ported, using family default` once and falls back.
`joints/ss_e_r_1.h` exists but is not included by the aggregator.

### Unit-box convention

Constructors write outlines in a unit cube: x in [-0.5, 0.5] across the joint, y in [-0.5, 0.5]
through the plate (face 0 at y=-0.5, face 1 at y=+0.5), z in [-0.5, 0.5] along the joint line.
`m_outlines[k]` / `f_outlines[k]` hold the cut polylines for plate face k, and the **last**
polyline of each list is a 2-point endpoint marker (`{front, back}`) that the merge reads at
index 1 to decide top/bottom (`wood_merge.cpp`: a marker with fewer than 2 points skips the
joint). `joint.name` must be set to the function name. Tiling along z uses `joint.divisions`,
`joint.length * joint.scale[2]` and `unit_scale_distance` (default 40 when unset).

### Adding a joint variant

1. Pick the family from the table and a free id in its range.
2. Create `wood_interaction_feature_plate_joints/<prefix>_N.h` with `static void <prefix>_N(InteractionFeaturePlate& joint)`.
3. Fill `m_outlines[0..1]`, `f_outlines[0..1]` in unit-box space, end each list with the
   endpoint marker, fill `male_fabrication_types` / `female_fabrication_types` with one `FabricationType::` value per outline,
   and set `joint.name = "<prefix>_N"`.
4. Set `joint.unit_scale = true` if the tooth size must follow plate thickness (see
   `ss_e_ip_2`, `ss_e_r_core`, `ts_e_p_5`).
5. `#include "joints/<prefix>_N.h"` in `wood_joint_lib.h`, after any constructor it calls.
6. Add `case <id>: <prefix>_N(joint); break;` to that group in `joint_create_geometry`
   (`wood_joint_solver.cpp`).
7. Exercise it: set the id in a dataset's `joints_parameters_and_types` row (col 2) or a
   `<name>_joints_types.txt` sidecar, and run `main_dataset_runner` through the guard.

### Custom joints at runtime

The `*_custom` constructors read `config::CUSTOM_JOINTS_<FAMILY>_MALE` / `_FEMALE`
(`wood_config.h`): pairs `(i, i+1)` = (face-0 polyline, face-1 polyline) of one base tooth,
tiled `divisions` times along z (`ss_e_ip_custom.h` documents the tiling). Fill the vectors in
C++ (the yml loader skips them), then select the family's custom id (9, 19, 29, 39, 59, 69).
`reset_defaults()` clears them.

## 5. session_cpp integration

What the kernel (`../session/session_cpp/src`) provides and wood uses:

- **`session_cpp::Element`** (`element.h`): geometry variant (Mesh/BRep), guid, `features()`
  (`ElementFeature{name, feature_type, face_index, outlines}`), `insertion_vectors()`,
  `dimensions()`, virtual `compute_polylines()` / `compute_planes()`, `clone()`,
  `element_type_name()` / `element_data_dumps()`, and the factory registry
  (`register_type`, `pb_loads_polymorphic`). `Objects` loading calls `pb_loads_polymorphic`,
  so a registered tag comes back as the derived class.
- **`session_cpp::Session`** (`session.h`): objects, tree, graph, `add_element`, `pb_dump` /
  `pb_load`. `WoodSession` derives from it (non-virtual; never delete through `Session*`).
- **`Mesh::loft(polylines0, polylines1, cap, fix_collinear)`** (`mesh.h`): the plate solid,
  called once per plate in `Plate::compute_geometry`.
- **`Polyline`, `Plane`, `Line`, `Point`, `Vector`, `Xform`, `Intersection`**: all joint
  and contact geometry; `std::vector<Point>` appears only inside the merge and linked-joint code.

How wood uses it (`wood_session.h/.cpp`):

- `WoodSession::yaml_load(name)` → `load_yaml`, `internal::load_plates(DATA_SET_OBJ)`
  (pairs consecutive OBJ loops bottom/top), one `Plate` per pair added by guid.
- `compute_contacts()` / `compute_face_contacts()` → `face_contacts` over every element type;
  `compute_cross_contacts()` → `plane_to_face`; `compute_line_contacts()`.
- `compute_features(search_type)` → `get_connection_zones` on `plates()` in place, then
  `compute_geometry_mesh()` on each plate, then each joint's contact and the joint itself onto
  its pair's edge through `add_interaction`.
- Features land on the elements as they are stored, never in a later pass: `add_interaction` puts
  a new contact on its edge's first element as a `contact` feature (guid = the contact's guid),
  a plate joint's two sides on its hosts, a beam joint on the first beam, outlines coloured by type; the clears and `erase_contacts` take them off again. An
  element's `compute_geometry_mesh()` keeps them, so the model geometry and its features travel together.
  The viewer draws every visible feature; nothing is copied into the tree, which stays as the caller
  built it (`add` without a parent = under the root). `set_features_visible(type, bool)` switches one
  kind off.
- Writing is the kernel's own `pb_dump(path)`; `pb_path(name)` gives `data/output/pb/<name>.pb`
  (`"live"` is what session_viewer watches). `write_parity_dumps(scene, pb)` writes
  `<pb>_meta.txt` / `_coords.txt`, every plate's merged outlines, the files to diff to prove a
  refactor changed nothing; the sweep (`run_dataset`) writes them beside `data/output/WoodF2F_<name>.pb`.
- `WoodSession::pb_load(name)` reads `data/<name>.pb` (or a path) and gets Plates, Columns
  and Blocks back through the registry.

## 6. Known pitfalls

| # | Pitfall | Fix |
|---|---|---|
| 1 | `angle` / `ANGLE` is radians, used as `cos(angle)`; `distance_squared` is mm^2 | Do not treat 0.11 as degrees; `Vector::is_parallel_to` (0.11 deg) is too strict for contacts, which is why `faces_coplanar` spells the test out |
| 2 | Joint id vs family mismatch (e.g. id 20 on a type-11 joint) | `joint_create_geometry` returns without outlines; keep the id in the range of the detected `joint_type` |
| 3 | Id 0 means "skip" upstream but is remapped to the row default here; the sign of an id is stripped with `std::abs` | Choose variants with positive ids in `joints_parameters_and_types` col 2 or the joints_types sidecar |
| 4 | Endpoint marker missing or 1-point in `*_outlines[k][1]` | Merge skips the joint (or, before the guard, relocated vertices to the origin); always end each outline list with `{front, back}` |
| 5 | `unit_scale` false on a thickness-dependent tooth | Geometry is stretched by the change of basis; set `unit_scale = true` and let `apply_unit_scale` size the volumes |
| 6 | Cache key is `"id;shift;divisions"`, not edge length; first joint wins | Do not sort joints before the geometry loop; `divisions` must differ for a different tooth count |
| 7 | `face_to_face_wood` asks for element b's faces 0/1 to be flipped mid-run | `detect_features` calls `Plate::flip`, which resets every cache; never keep face indices of a plate across pairs |
| 8 | `get_connection_zones` on a copied plate vector | The result lives on the plates (`features`, `insertion_vectors`, `reversed`); pass the scene's own `shared_ptr` vector |
| 9 | `settings.joint_parameters` shorter than 21 entries | Falls back to built-in defaults with a warning; keep 7 x 3 entries in the yml |
| 10 | `Session` has no virtual destructor | Do not own a `WoodSession` through a `Session*` |
| 11 | Copying an `ElementFeature` or `Element` mints a new guid | Joint identity lives in `InteractionFeaturePlate::feature_guids`; read features through `to_features()` |
| 12 | Unbounded runs | Every solver/example goes through `tools/run_guarded.sh`; one run at a time; `--parallel 4` |
