# Elements {#elements}

[TOC]

The timber elements under `src/joinery_solver/wood_elements/`, one page each: what it is, its constructors, and one picture and example per case. Every example is a CMake target under `examples/elements/` that writes `data/output/pb/live.pb` for the viewer; the pictures are drawn by `tools/render_element_docs.py` at opacity 0.75 with the element features and a layer panel of the scene.

An element is built from its own parameters only. Whatever another element does to it comes through `WoodSession::add_interaction(source, target, interaction)`: a contact, a plate joint's outline feature, an `InteractionFeatureSolid` that adds a solid to its stock or takes one away, or an `InteractionFeaturePlane` that cuts it by a plane.

| Element | Header | Examples |
| --- | --- | --- |
| [Plate](@ref elements_plate) | `wood_element_plate.h` | `element_plate`, `element_plate_holes`, `element_plate_session` |
| [Beam](@ref elements_beam) | `wood_element_beam.h` | `element_beam` |
| [BeamVariable](@ref elements_beam_variable) | `wood_element_beam_variable.h` | `element_beam_variable`, `element_beam_variable_cut` |
| [Column](@ref elements_column) | `wood_element_column.h` | `element_column`, `element_column_session` |
| [Block](@ref elements_block) | `wood_element_block.h` | `element_block` |
| [Support](@ref elements_support) | `wood_element_support.h` | `element_support` |
| [CutPlane](@ref elements_cut_plane) | `wood_element_cut_plane.h` | `element_beam_variable_cut` |
| [Profiles](@ref elements_profile) | `wood_profile.h` | `element_profile` |
| [Joint](@ref elements_joint) | `wood_element_joint.h` | `element_joint_drill`, `element_joint_cutter` |
| [JointPlate](@ref elements_joint_plate) | `wood_element_joint_plate.h` | `element_joint_plate_ts_e_p`, `_ss_e_op`, `_cr_c_ip`, `_tt_e_p`, `_ss_e_r_3`, `_parameters` |
| [JointPlate ss_e_ip](@ref elements_joint_plate_ss_e_ip) | `wood_element_joint_plate.h` | `element_joint_plate_ss_e_ip_0` to `_5`, `_custom` |
| [JointBeam](@ref elements_joint_beam) | `wood_element_joint_beam.h` | `element_joint_beam_from_contact`, `_wedge`, `_rectangle_plate`, `_tie`, `_centred_pins`, `_headed_pins` |
| [Pin](@ref elements_pin) | `wood_element_pin.h` | `element_pin` |
| [ConnectorPart](@ref elements_connector_part) | `wood_element_connector_part.h` | `element_connector_part` |

- @subpage elements_plate
- @subpage elements_beam
- @subpage elements_beam_variable
- @subpage elements_column
- @subpage elements_block
- @subpage elements_support
- @subpage elements_cut_plane
- @subpage elements_profile
- @subpage elements_joint
- @subpage elements_joint_plate
- @subpage elements_joint_plate_ss_e_ip
- @subpage elements_joint_beam
- @subpage elements_pin
- @subpage elements_connector_part

## How joints are used

A joint is an element, and what it does is an interaction. Every connection is made in three steps: the joint computes itself from the contact of the members it joins, it is added, and it is passed to each member with `add_interaction`, in its target order:

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
const std::shared_ptr<JointBeam> pins = JointBeam::centred_pins(*lower, *upper, *contact);
scene.add(pins);
scene.add_interaction(pins, lower, pins->interaction(0));
scene.add_interaction(pins, upper, pins->interaction(1));
```

| Joint | Made from | What it does to each target |
| --- | --- | --- |
| [Joint](@ref elements_joint) | an axis, a mesh, a profile | drills, subtracts, adds or keeps a solid |
| [JointPlate](@ref elements_joint_plate) | a face or cross contact of two plates, a library design by name | cuts the design's outline into both plates |
| [JointBeam](@ref elements_joint_beam), beam to beam | an axis contact of two beams | the feature volumes of a lap or a butt |
| [JointBeam](@ref elements_joint_beam), connector | a face contact of two members | pockets and bores; its parts ([ConnectorPart](@ref elements_connector_part)) and [Pins](@ref elements_pin) nest under it |

The interaction belongs to its target, the element the joint acts on: the target hosts the cut, the pocket and the holes; the joint keeps only what it is.
