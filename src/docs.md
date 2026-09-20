
# Structure of Wood

## Connectivity

- The session stores a graph that says which elements are connected. A graph edge (a, b) carries no payload: its guid is the key into the interaction collection, `WoodSession::interactions`, a map from guid to `Interaction`. The edge is the only place the two element guids are stored; every record below refers to "the first element" (edge v0) and "the second element" (edge v1) and never repeats them.
- `Interaction` is a plain struct, not a base class; it is composition. It has one guid (the edge's) and three attributes:
    - `contacts`, a list of `InteractionContact`: every place the two elements touch.
    - `features`, a list of `InteractionFeature`: every joint cut between them (what the wood project calls a joint).
    - `structure`, one optional `InteractionStructure`: how the two elements transfer forces; empty until that pass exists.
- `InteractionContact` stores a guid and exactly one contact, as a variant (a protobuf `oneof`): `ContactFace`, `ContactAxis` or `ContactCross`, never two at once. The kinds are plain structs, not subclasses.
    - `ContactFace` stores the two face indices, the class (`ContactType`: unknown, side_side, side_top, top_top) and `polygon`, the Clipper boolean intersection of the two face outlines, closed, in the first face's plane.
    - `ContactAxis` stores the closest segment between two polylines (beam axes, or plate outlines) and where its ends sit: the parameter and the polyline and segment index on each side.
    - `ContactCross` stores what the cross detection computed: the two side faces of each element, the mid-plane polygon, its two centrelines and the two bounding quads.
- `InteractionFeature` stores a guid, the index of the contact it was solved from (in `Interaction::contacts`) and exactly one feature, again a variant: `FeaturePlate`, `FeatureBeam` or `FeaturePlateBeam`.
    - `FeaturePlate` is a plate-to-plate joint: the pair (male `element_a`, female `element_b`), the `ContactFace` it was solved from, the variant name the joint library built (`ss_e_ip_2`, `tt_e_p_0`, ...), joint type, divisions, shift, scale, the two joint lines, the four volume rectangles, the cut outlines per element per face with a `FabricationType` per outline (`wood_interaction_feature_fabrication_type.h`: hole, drill, mill, conic, ...), and the two `session_cpp::ElementFeature` handed to the host elements. The solver builds it in place and the interaction stores it whole; there is no separate working joint class. The joint library stays one function per variant, one header each under `wood_interaction_feature_plate_joints/`; the variants differ by algorithm, not by data, so there is no subclass per joint.
    - `FeatureBeam` is a beam-to-beam joint: the end type (crossing, side to end, end to end) and the four volume rectangles.
    - `FeaturePlateBeam` is reserved, empty.
- `InteractionStructure` is reserved, empty.
- Inheritance is used only where the kernel forces it: `Plate`, `Beam`, `Column` and `Block` derive from `session_cpp::Element`, because `Session::pb_load` rebuilds them through the kernel's `element_type` registry. Everything on the edge side is data: no virtual method, no base class.
- The dataset sidecars are not a class of their own: `WoodSession::load_sidecars` puts the adjacency and the three-valence groups on the scene (`adjacency`, `three_valence`) and the insertion vectors and joint types on each plate. Detection reads the elements themselves; there is no detection view class.

## Files

- `wood_settings`: `Settings`, every tunable the solver reads, filled from the dataset yml by `config::load_yaml`, held by the scene, passed by reference into every algorithm and joint builder, written with the scene. `wood_config` keeps only the dataset catalogue and the paths.
- `wood_elements/`: one element class per file, `wood_element_plate`, `wood_element_beam`, `wood_element_column`, `wood_element_block`.
- `wood_interaction/`: the folders nest as the data does, one class per file, the file name spelling the path down the tree:
    - `wood_interaction.h/.cpp` (`Interaction`)
    - `wood_interaction_contact/`: `wood_interaction_contact` (the envelope), `wood_interaction_contact_face` (+ `_type`), `wood_interaction_contact_axis`, `wood_interaction_contact_cross`
    - `wood_interaction_feature/`: `wood_interaction_feature` (the envelope), `wood_interaction_feature_plate` (+ `_fabrication_type`), `wood_interaction_feature_beam`, `wood_interaction_feature_plate_beam` (empty for now), `wood_interaction_feature_plate_joints.h` and `wood_interaction_feature_plate_joints/` (one header per joint, plus `custom_outlines`, `tt_e_p_drills`, `cr_c_ip_core` and `ss_e_r_core` for code several joints share)
    - `wood_interaction_structure/`: `wood_interaction_structure` (empty for now)
- `wood_algorithms/`: the computations, kept apart from the data classes and named by what they produce, every input by argument: `wood_contact_detection` (face, cross and axis contacts), `wood_feature_detection` (one plate pair to one FeaturePlate), `wood_feature_detection_beam` (one beam pair to one FeatureBeam), `wood_feature_construction` (unit scale, orientation, linked joints, divisions), `wood_feature_solver` (the `compute_features` pipeline and the joint registry), `wood_three_valence`, `wood_merge_modifier`, `wood_assignment` (points and lines placed on plates into their feature types and insertion vectors). Functions, not classes, wherever a function is enough.
- `wood_session`: the scene, the store and the pipeline entry points, nothing else. `wood_view`: the viewer layout and the colours. `wood_io`: the sidecar and obj readers, `pb_path`, the parity dumps. `wood_serialization`: JSON from a proto message and back. `wood_test.h/.cpp`: the dataset runners.
- `proto/`: one protobuf message per class, same names without the `wood_` prefix; `generated/` holds the C++ protoc output, committed, regenerated by `tools/regen_proto.sh` with the protoc the kernel pins.

## Serialization

- Every class above has `pb_dumps` / `pb_loads`, and `jsondump` / `jsonload` derived from the same proto message through `wood_serialization` (`json_of`, `message_from_json`): the proto is the one schema, the JSON carries the proto field names and a `type` key. Kernel geometry inside a message (polylines, lines, element features) is nested as the kernel's own message.
- The element payload the kernel carries opaquely in `element_data` is the class's protobuf message (`wood_proto.Plate`, `Beam`, `Column`); a payload written in the kernel's JSON by older files is still read, the one hand-written JSON reader left.
- A scene file is a `wood_proto.WoodSession`: fields 1..7 are `session_proto.Session` field for field, then `interactions` at field 100 and `settings` at 101. The viewer and the py/rust kernels open it as a plain Session and drop the two as unknown fields; `WoodSession::pb_load` reads all of it.
- Interactions are written in guid order as a repeated field, not a protobuf map, so the bytes are identical across languages.

## Pipeline

- `WoodSession::yaml_load` reads the dataset yml into the scene's `settings` and the dataset paths, the obj into plates, and the four sidecars onto the scene (`adjacency`, `three_valence`) and the plates (insertion vectors, feature types).
- `compute_contacts` runs `wood_contact_detection` over every element pair the OBB/BVH search returns and stores one `ContactFace` per overlapping face pair on the pair's interaction; `compute_cross_contacts`, `compute_line_contacts` and `compute_axis_contacts` add `ContactCross` and `ContactAxis` the same way.
- `compute_features` runs `wood_feature_solver`: `adjacent_pairs` (the sidecar or the search), `wood_feature_detection` on each pair (one `FeaturePlate` or nothing; when a joint wants the other face first the second plate is flipped through `Plate::flip`, which resets every cache), `wood_three_valence` (shadow joints, annen alignment), `wood_feature_construction` + the joint registry (unit-box outlines, oriented onto the volumes), `wood_merge_modifier` (the cut outlines stitched into each plate's `features`), then every joint onto its interaction with `add_feature` and onto both hosts as `ElementFeature`s.
- `compute_beam_features` runs `wood_feature_detection_beam` on every axis contact between two beams: four volume rectangles per pair, one `FeatureBeam` each.
- `pb_dump` lofts every stale plate and writes the `wood_proto.WoodSession`; `add_to_tree` (in `wood_view`) arranges elements, outlines, contacts and features into viewer groups.

<!-- --8<-- [start:review] -->
## Architecture review

### What is right

- The data model reads top down and matches the wire: session, edge, interaction, contact or feature, kind. One class per file, folders nested as the data is, one protobuf message per class, the file a superset of the kernel Session. A newcomer can find any record from its name.
- Composition and variants instead of class hierarchies on the edge side; inheritance only where the kernel registry demands it. New kinds are one struct, one message, one `std::get_if` branch.
- Algorithms are functions in files named by what they produce, separated from the data they produce.
- The regression net is strong: 44 datasets with byte-compared outline dumps, plus a full session round trip that exercises every serializer and the kernel reader.

### What was wrong, and what was done about it (2026-09-20)

- **Global mutable configuration.** 39 globals in `wood_config.h` were the real inputs of every algorithm. Gone: `Settings` is a value the scene holds and every algorithm and builder takes by reference; `config` keeps the catalogue and the paths.
- **A dependency pointing the wrong way.** `Beam::joint_volumes` ran the solver from inside an element. Gone: `axis_contacts` and `beam_to_beam` live in the algorithms and `compute_axis_contacts` / `compute_beam_features` run on the scene's beams like the plate pipeline.
- **The solver mutating its inputs.** Detection swapped a plate's faces behind the kernel's cache. Removing the swap changes eight datasets, so it is a real step of the method, not a leak: `Plate::flip` owns it and resets every cache, and contact detection reads the kernel's cached outlines again.
- **The joint library dispatched by hand.** Seven switches over id ranges known only there. Gone: one table, id to family and builder, the family ranges and defaults beside it.
- **`FeaturePlate` carrying solver scratch.** Run indices and trace counters. Gone: joints link by guid, every feature has its own guid, the counters live in `DetectionTrace` for callers that ask. The pair and the contact are still stored on the joint as well as on the edge and in the interaction, by choice; `WoodSession::consistent` checks they agree and the round trip asserts it.
- **Two hand-written serializers per class.** Gone: JSON is derived from the proto message; the one reader left is for element payloads of older files.
- **`WoodSession` doing too much.** Viewer layout, readers, assignment tools and the dataset runners moved out to `wood_view`, `wood_io`, `wood_assignment` and `wood_test.h`.
- **Vocabulary split between joint and feature.** Feature is the word for the record and the pipeline; joint stays in the library vocabulary (`joint_type`, `joint_lines`, the builder names).

### What remains

- **Kernel: an edge's guid.** `Graph::add_edge` copies an Edge into both directions before it has a guid; wood mints it and stamps the second copy. The kernel fix is to mint at `add_edge`, but Python already shares one object both ways and Rust copies like C++, so all three kernels must change together for the files to stay byte-identical.
- **The joint headers are still `static` functions included into one translation unit.** The registry makes the solver blind to that, but a proper library would make them ordinary functions with include guards in their own translation unit.
- **`compute_features` both stores and returns.** Callers that act on the return value after a second run read a stale copy; the return should go once nothing depends on it.
- **Templates sit in `src/`.** `src/templates/` is 5000 lines of header-only generators compiled by every main that includes them; they belong next to `examples/`, each a source file built once. Left in place because they are being edited in a parallel branch.

### The layering, as it stands

- `wood_elements` knows the kernel. `wood_interaction` knows elements. `wood_algorithms` knows both and takes every input by argument. `wood_session` orchestrates and stores; `wood_view` and `wood_io` sit beside it. Examples and templates sit on top. A grep of the includes shows no arrow pointing up.
<!-- --8<-- [end:review] -->
