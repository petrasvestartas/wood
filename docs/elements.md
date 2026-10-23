# Elements {#elements}

[TOC]

The timber elements under `src/joinery_solver/wood_elements/`, one page each: what it is, its constructors, and a picture and an example per case.

An element is built from its own parameters; what others do to it comes through `WoodSession::add_interaction(source, target, interaction)`.

| Element | Header | Examples |
| --- | --- | --- |
| [Plate](@ref elements_plate) | `wood_element_plate.h` | `element_plate`, `element_plate_holes`, `element_plate_session`, `element_plate_assign_joints` |
| [PlateCurved](@ref elements_plate_curved) | `wood_element_plate_curved.h` | `element_plate_curved` |
| [Beam](@ref elements_beam) | `wood_element_beam.h` | `element_beam` |
| [BeamVariable](@ref elements_beam_variable) | `wood_element_beam_variable.h` | `element_beam_variable`, `element_beam_variable_cut` |
| [Column](@ref elements_column) | `wood_element_column.h` | `element_column`, `element_column_session` |
| [Block](@ref elements_block) | `wood_element_block.h` | `element_block` |
| [Support](@ref elements_support) | `wood_element_support.h` | `element_support` |
| [CutPlane](@ref elements_cut_plane) | `wood_element_cut_plane.h` | `element_beam_variable_cut` |
| [Profiles](@ref elements_profile) | `wood_profile.h` | `element_profile` |
| [Joint](@ref elements_joint) | `wood_element_joint.h` | `element_joint_drill`, `element_joint_cutter` |
| [JointPlate](@ref elements_joint_plate) | `wood_element_joint_plate.h` | `element_joint_plate_parameters` |
| [JointPlate top_side](@ref elements_joint_plate_top_side) | `wood_element_joint_plate.h` | `element_joint_plate_top_side_three_tenons` to `_snap_fit`, `_custom` |
| [JointPlate side_side_inplane](@ref elements_joint_plate_side_side_inplane) | `wood_element_joint_plate.h` | `element_joint_plate_side_side_inplane_three_fingers` to `_tooth_keys`, `_custom`, `_hilti`, `_hilti_angles` |
| [JointPlate side_side_outplane](@ref elements_joint_plate_side_side_outplane) | `wood_element_joint_plate.h` | `element_joint_plate_side_side_outplane_three_fingers` to `_linked_tenons_wall`, `_capped_fingers`, `_notch`, `_custom` |
| [JointPlate side_side_rotated](@ref elements_joint_plate_side_side_rotated) | `wood_element_joint_plate.h` | `element_joint_plate_side_side_rotated_miter`, `_hooks`, `_keys`, `_custom`, `_side_removal` |
| [JointPlate cross](@ref elements_joint_plate_cross) | `wood_element_joint_plate.h` | `element_joint_plate_cross_slot`, `_custom` |
| [JointPlate top_top](@ref elements_joint_plate_top_top) | `wood_element_joint_plate.h` | `element_joint_plate_top_top_centre_pin` to `_rectangle_pins`, `_custom` |
| [JointPlate border](@ref elements_joint_plate_border) | `wood_element_joint_plate.h` | `element_joint_plate_border` |
| [JointBeam](@ref elements_joint_beam) | `wood_element_joint_beam.h` | `element_joint_beam_wedge` |
| [JointBeam cross](@ref elements_joint_beam_cross) | `wood_element_joint_beam.h` | `element_joint_beam_cross_slot` to `_brussels_half_lap` |
| [JointBeam top_side](@ref elements_joint_beam_top_side) | `wood_element_joint_beam.h` | `element_joint_beam_top_side_milled_wedge` |
| [JointBeam end to end](@ref elements_joint_beam_end_to_end) | `wood_element_joint_beam.h` | `element_joint_beam_side_side_inplane_splayed_scarf`, `_side_side_inplane_cross_splayed_scarf`, `_side_side_rotated_miter`, `_jupiter_scarf` |
| [JointBeam butt](@ref elements_joint_beam_butt) | `wood_element_joint_beam.h` | `element_joint_beam_butt`, `_butt_plates` |
| [JointBeam connectors](@ref elements_joint_beam_connectors) | `wood_element_joint_beam.h` | `element_joint_beam_wedge`, `_rectangle_plate`, `_butterfly_key` |
| [JointBeam pins](@ref elements_joint_beam_pins) | `wood_element_joint_beam.h` | `element_joint_beam_centred_pins`, `_headed_pins` |
| [Pin](@ref elements_pin) | `wood_element_pin.h` | `element_pin` |

- @subpage elements_plate
- @subpage elements_plate_curved
- @subpage elements_beam
- @subpage elements_beam_variable
- @subpage elements_column
- @subpage elements_block
- @subpage elements_support
- @subpage elements_cut_plane
- @subpage elements_profile
- @subpage elements_joint
- @subpage elements_joint_plate
- @subpage elements_joint_plate_top_side
- @subpage elements_joint_plate_side_side_inplane
- @subpage elements_joint_plate_side_side_outplane
- @subpage elements_joint_plate_side_side_rotated
- @subpage elements_joint_plate_cross
- @subpage elements_joint_plate_top_top
- @subpage elements_joint_beam
- @subpage elements_joint_beam_cross
- @subpage elements_joint_beam_top_side
- @subpage elements_joint_beam_end_to_end
- @subpage elements_joint_beam_butt
- @subpage elements_joint_beam_connectors
- @subpage elements_joint_beam_pins
- @subpage elements_pin

## How joints are used

A joint is an element, and what it does is an interaction: it is computed from the contact, added, and passed to each member:

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
| [JointBeam](@ref elements_joint_beam), connector | a face contact of two members | pockets and bores; its parts and [Pins](@ref elements_pin) nest under it |

The interaction belongs to its target, which hosts the cut, pocket and holes.
