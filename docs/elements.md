# Elements {#elements}

[TOC]

Timber members, auxiliary cutters, and parametric joints in `src/joinery_solver/wood_elements/`. Each element stores uncut stock geometry and evaluates incoming interactions in `WoodSession` to produce cut meshes and exact NURBS BReps.

## Elements

Timber members and the auxiliary elements that shape them: plates, beams, columns, blocks, supports, cut planes, profiles and pins.

![](diagrams/wood_elements_sheet_1_elements.webp)

- [Plate](elements_plate.html)
- [Curved Plate](elements_plate_curved.html)
- [Beam](elements_beam.html)
- [Curved Beam](elements_beam_curved.html)
- [Variable Beam](elements_beam_variable.html)
- [Column](elements_column.html)
- [Block](elements_block.html)
- [Support](elements_support.html)
- [Cut Plane](elements_cut_plane.html)
- [Profiles](elements_profile.html)
- [Pin](elements_pin.html)

## Plate Joints

Plate joints cut pairs of plates meeting top to side, side to side in plane, out of plane or rotated, crossing, stacked top to top, or along a border.

![](diagrams/wood_elements_sheet_2_plate_joints.webp)

- [Plate Joints](elements_joint_plate.html)
- [Plate Top-to-Side](elements_joint_plate_top_side.html)
- [Plate In-Plane](elements_joint_plate_side_side_inplane.html)
- [Plate Out-of-Plane](elements_joint_plate_side_side_outplane.html)
- [Plate Rotated](elements_joint_plate_side_side_rotated.html)
- [Plate Cross](elements_joint_plate_cross.html)
- [Plate Top-to-Top](elements_joint_plate_top_top.html)
- [Plate Border](elements_joint_plate_border.html)

## Beam Joints

Beam joints cut pairs of beams into each other: cross laps, scarfs, miters, top-to-side tenons and butt joints.

![](diagrams/wood_elements_sheet_3_beam_joints.webp)

- [Beam Joints](elements_joint_beam.html)
- [Beam Cross](elements_joint_beam_cross.html)
- [Beam Top-to-Side](elements_joint_beam_top_side.html)
- [Beam End-to-End](elements_joint_beam_end_to_end.html)
- [Beam Butt](elements_joint_beam_butt.html)

## Connectors and Pins

Joints that add parts between members, such as wedges, butterfly keys, side plates and pins, and the plain joint with its cutters and drills.

![](diagrams/wood_elements_sheet_4_connectors.webp)

- [Joint](elements_joint.html)
- [Beam Connectors](elements_joint_beam_connectors.html)
- [Beam Pins](elements_joint_beam_pins.html)

## Joint Wiring

![](diagrams/wood_joint_unit_box.webp)

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
const std::shared_ptr<JointBeam> pins = JointBeam::centred_pins(*lower, *upper, *contact);
scene.add(pins);
scene.add_interaction(pins, lower, pins->interaction(0));
scene.add_interaction(pins, upper, pins->interaction(1));
```

<div style="display:none">
- @subpage elements_plate
- @subpage elements_plate_curved
- @subpage elements_beam
- @subpage elements_beam_curved
- @subpage elements_beam_variable
- @subpage elements_column
- @subpage elements_block
- @subpage elements_support
- @subpage elements_cut_plane
- @subpage elements_profile
- @subpage elements_pin
- @subpage elements_joint
- @subpage elements_joint_plate
- @subpage elements_joint_plate_top_side
- @subpage elements_joint_plate_side_side_inplane
- @subpage elements_joint_plate_side_side_outplane
- @subpage elements_joint_plate_side_side_rotated
- @subpage elements_joint_plate_cross
- @subpage elements_joint_plate_top_top
- @subpage elements_joint_plate_border
- @subpage elements_joint_beam
- @subpage elements_joint_beam_cross
- @subpage elements_joint_beam_top_side
- @subpage elements_joint_beam_end_to_end
- @subpage elements_joint_beam_butt
- @subpage elements_joint_beam_connectors
- @subpage elements_joint_beam_pins
</div>
