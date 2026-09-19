
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
    - `wood_interaction.h/.cpp` (`Interaction`, with `InteractionStructure` inside until it has content)
    - `wood_interaction_contact/`: `wood_interaction_contact` (the envelope), `wood_interaction_contact_face` (+ `_type`), `wood_interaction_contact_axis`, `wood_interaction_contact_cross`
    - `wood_interaction_feature/`: `wood_interaction_feature` (the envelope, with `FeaturePlateBeam` inside until it has content), `wood_interaction_feature_plate` (+ `_fabrication_type`), `wood_interaction_feature_beam`, `wood_interaction_feature_plate_joints.h` and `wood_interaction_feature_plate_joints/` (one header per joint, plus `custom_outlines`, `tt_e_p_drills`, `cr_c_ip_core` and `ss_e_r_core` for code several joints share)
- `wood_algorithms/`: the computations, kept apart from the data classes and named by what they produce: `wood_contact_detection` (face contacts and cross contacts), `wood_feature_detection` (one FeaturePlate from one plate pair), `wood_feature_construction` (unit scale, orientation, linked joints, divisions), `wood_feature_solver` (the `compute_joints` pipeline), `wood_three_valence`, `wood_merge_modifier`. Functions, not classes, wherever a function is enough.
- `proto/`: one protobuf message per class, same names without the `wood_` prefix; `generated/` holds the C++ protoc output, committed, regenerated by `tools/regen_proto.sh` with the protoc the kernel pins.

## Serialization

- Every class above has both `jsondump` / `jsonload` and `pb_dumps` / `pb_loads`, like the kernel classes. Kernel geometry inside a message (polylines, lines, element features) is nested as the kernel's own message.
- The element payload the kernel carries opaquely in `element_data` is the class's protobuf message (`wood_proto.Plate`, `Beam`, `Column`); a payload written as JSON by older files is still read.
- A scene file is a `wood_proto.WoodSession`: fields 1..7 are `session_proto.Session` field for field, then `interactions` at field 100. The viewer and the py/rust kernels open it as a plain Session and drop the interactions as an unknown field; `WoodSession::pb_load` reads both.
- Interactions are written in guid order as a repeated field, not a protobuf map, so the bytes are identical across languages.
