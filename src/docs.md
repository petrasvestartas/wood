
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

- `wood_elements/`: one element class per file, `wood_element_plate`, `wood_element_beam`, `wood_element_column`, `wood_element_block`.
- `wood_interaction/`: the folders nest as the data does, one class per file, the file name spelling the path down the tree:
    - `wood_interaction.h/.cpp` (`Interaction`)
    - `wood_interaction_contact/`: `wood_interaction_contact` (the envelope), `wood_interaction_contact_face` (+ `_type`), `wood_interaction_contact_axis`, `wood_interaction_contact_cross`
    - `wood_interaction_feature/`: `wood_interaction_feature` (the envelope), `wood_interaction_feature_plate` (+ `_fabrication_type`), `wood_interaction_feature_beam`, `wood_interaction_feature_plate_beam` (empty for now), `wood_interaction_feature_plate_joints.h` and `wood_interaction_feature_plate_joints/` (one header per joint, plus `custom_outlines`, `tt_e_p_drills`, `cr_c_ip_core` and `ss_e_r_core` for code several joints share)
    - `wood_interaction_structure/`: `wood_interaction_structure` (empty for now)
- `wood_algorithms/`: the computations, kept apart from the data classes and named by what they produce: `wood_contact_detection` (face contacts and cross contacts), `wood_feature_detection` (one FeaturePlate from one plate pair), `wood_feature_construction` (unit scale, orientation, linked joints, divisions), `wood_feature_solver` (the `compute_features` pipeline), `wood_three_valence`, `wood_merge_modifier`. Functions, not classes, wherever a function is enough.
- `proto/`: one protobuf message per class, same names without the `wood_` prefix; `generated/` holds the C++ protoc output, committed, regenerated by `tools/regen_proto.sh` with the protoc the kernel pins.

## Serialization

- Every class above has both `jsondump` / `jsonload` and `pb_dumps` / `pb_loads`, like the kernel classes. Kernel geometry inside a message (polylines, lines, element features) is nested as the kernel's own message.
- The element payload the kernel carries opaquely in `element_data` is the class's protobuf message (`wood_proto.Plate`, `Beam`, `Column`); a payload written as JSON by older files is still read.
- A scene file is a `wood_proto.WoodSession`: fields 1..7 are `session_proto.Session` field for field, then `interactions` at field 100. The viewer and the py/rust kernels open it as a plain Session and drop the interactions as an unknown field; `WoodSession::pb_load` reads both.
- Interactions are written in guid order as a repeated field, not a protobuf map, so the bytes are identical across languages.

## Pipeline

- `WoodSession::yaml_load` reads the dataset yml into `config::` globals, the obj into plates, and the four sidecars onto the scene (`adjacency`, `three_valence`) and the plates (insertion vectors, joint types).
- `compute_contacts` runs `wood_contact_detection` over every element pair the OBB/BVH search returns and stores one `ContactFace` per overlapping face pair on the pair's interaction; `compute_cross_contacts` and `compute_line_contacts` add `ContactCross` and `ContactAxis` the same way.
- `compute_features` runs `wood_feature_solver`: `adjacent_pairs` (the sidecar or the search), `wood_feature_detection` on each pair (one `FeaturePlate` or nothing, and possibly a face swap on the second plate), `wood_three_valence` (shadow joints, annen alignment), `wood_feature_construction` + the joint library (unit-box outlines, oriented onto the volumes), `wood_merge_modifier` (the cut outlines stitched into each plate's `features`), then every joint onto its interaction with `add_feature` and onto both hosts as `ElementFeature`s.
- `pb_dump` lofts every stale plate and writes the `wood_proto.WoodSession`; `add_to_tree` arranges elements, outlines, contacts and joints into viewer groups.
- Beams take a separate road: `Beam::joint_volumes` builds its own `WoodSession`, finds the closest axis segments, cuts four volume rectangles per pair and stores them as `ContactAxis` + `FeatureBeam`.

## Architecture review

### What is right

- The data model reads top down and matches the wire: session, edge, interaction, contact or feature, kind. One class per file, folders nested as the data is, one protobuf message per class, the file a superset of the kernel Session. A newcomer can find any record from its name.
- Composition and variants instead of class hierarchies on the edge side; inheritance only where the kernel registry demands it. New kinds are one struct, one message, one `std::get_if` branch.
- Algorithms are functions in files named by what they produce, separated from the data they produce.
- The regression net is strong: 44 datasets with byte-compared outline dumps, plus a full session round trip that exercises every serializer and the kernel reader.

### What is weak

- **Global mutable configuration.** `wood_config.h` exports 39 globals (`DISTANCE`, `JOINT_SCALE`, the joint parameter table, the custom joint polylines, the dataset paths). `load_yaml` writes them, `reset_defaults` resets them, and the algorithms, the joint library headers and `WoodSession` read them from anywhere. Every solve therefore depends on which yml was loaded last; two scenes with different settings cannot coexist, nothing is thread-safe, and a function's real inputs are invisible in its signature. `set_cross_joint_distance_squared` is a second global on top.
- **Dependencies point the wrong way in one place.** `wood_elements/wood_element_beam.cpp` includes `wood_session.h` and `wood_feature_detection.h`: an element class runs the solver. `Beam::joint_volumes` is an algorithm and belongs in `wood_algorithms`, operating on the scene's beams like `compute_features` operates on its plates, instead of minting a scene of its own.
- **The solver mutates its inputs.** `detect_features` swaps a plate's bottom and top (planes and outlines) when detection asks for it. That is why contact detection must read a plate's outlines live and not through the kernel's cache, and why `face_contacts_for_pair` is documented as "call it inside the loop". A detector should return an orientation and leave the plate alone, or the plate should own a `flip()` that also invalidates the kernel cache.
- **The joint library is a compile trick.** 39 headers with `static` functions, no include guards, included in a fixed order into the anonymous namespace of one translation unit, dispatched by a hand-written `switch` over integer ids whose families are ranges (1-9, 10-19, ...) known only from that switch and the docs. The ids also encode the family twice: in `joint_type` (11/12/13/20/30/40) and in the per-face id table. A registry (name to builder, family to id range in one table) would make a variant addable without touching the solver.
- **`FeaturePlate` still carries solver scratch.** `linked_joints` are indices into one run's joint vector and mean nothing once stored; `dbg_*` are trace counters; the joint carries its pair and its contact although the interaction has both. Two places for one fact will drift: nothing checks that `FeaturePlate::contact` equals `Interaction::contacts[feature.contact]`.
- **Two serializers per class, written by hand.** Every record has `jsondump/jsonload` and `pb_dumps/pb_loads`: four functions and two key vocabularies that must agree, for ten classes. The proto message is the schema; JSON can be produced from it (`google::protobuf::util::MessageToJsonString`) or dropped where nothing reads it.
- **`WoodSession` does too much.** It is the store, the solver entry, the sidecar reader, the viewer layout (`add_to_tree`, colours), the point-to-slot assignment tool, and its header declares the 44 dataset runners. Those runners and the colour tables are not part of a scene.
- **Vocabulary is split.** The records say feature, the pipeline says joint: `compute_features`, `get_plate_features`, `Plate::feature_types`, `joint_type`, `JOINTS_PARAMETERS_AND_TYPES`, `SearchType::face_to_face`. One word should win.
- **Two outputs from one call.** `compute_features` both stores the joints and returns them; callers can act on the return value while the store holds an older copy.
- **Kernel warts leak up.** `Graph::add_edge` copies an Edge into both directions before its guid exists, so wood mints the guid and writes it on the second copy; `Element::polylines()` caches with no public invalidation. Both belong in the kernel.
- **Templates sit in `src/`.** `src/templates/` is 5000 lines of header-only shape generators (reciprocal, chevron, folds) with their own namespaces; they are example inputs, not the solver, and they compile into every main that includes them.

### How I would plan it

- **Layers with one-way includes.** `wood_elements` knows the kernel. `wood_interaction` knows elements. `wood_algorithms` knows both and takes every input by argument. `wood_session` orchestrates and stores. Examples and templates sit on top. A check in CMake or a grep in CI keeps it that way.
- **Settings as a value.** One `Settings` struct built from the yml (tolerances, the joint parameter table, volume extension, scale, the custom joint outlines), held by the `WoodSession` and passed by `const&` into every algorithm and every joint builder. `config::` keeps only the dataset catalogue and the paths. No solver code reads a global.
- **One pipeline shape for every element kind.** `compute_contacts` then `compute_features` on the scene, for plates and for beams alike; each step reads the store and writes the store, and returns nothing or the guids it added. `Beam::joint_volumes` becomes `wood_algorithms/wood_feature_detection_beam`.
- **Detection that does not mutate.** `face_to_face_wood` reports the orientation it needs; the caller flips the plate through a `Plate::flip()` that invalidates the kernel cache, or the feature records the flip and nothing moves.
- **A joint registry.** The library headers become ordinary functions in `wood_session::joints`, each registered under its name with its family and id, `void ss_e_ip_2(FeaturePlate&, const Settings&, const std::vector<std::shared_ptr<Plate>>&)`; the solver looks the builder up, and the family table is the only place ids are defined. `FabricationType` per outline stays.
- **`FeaturePlate` holds only what is fabricated.** Links between features by guid, not by index; trace counters in a separate `DetectionTrace` returned by detection; the pair and the contact read through the interaction, unless the duplication is kept on purpose, in which case `add_feature` asserts they agree.
- **One serializer.** The proto message is the class on the wire; JSON is derived from it. Every record keeps `pb_dumps/pb_loads` and gets `jsondump/jsonload` for free.
- **A thinner session.** `WoodSession` = kernel Session + `interactions` + `adjacency`, `three_valence` + `compute_*` + `pb_*`. Viewer layout and colours in `wood_view`, sidecar and obj readers in `wood_io`, the dataset runners in `wood_test.h`, assignment tools with the templates that need them.
- **One word.** Feature everywhere the record is meant (`compute_features`, `get_features`, `Plate::feature_types`, `FeaturePlate::type`), joint only in the library names that are the domain's vocabulary (`ss_e_ip_2`).
- **Fix the kernel, not around it.** `Edge` gets its guid when it is added, so both copies share it; `Element` exposes `invalidate()`.
- **Templates and examples out of `src/`.** `templates/` next to `examples/`, each generator a source file built once, not a header rebuilt by every main.
