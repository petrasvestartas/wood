# Elements {#elements}

[TOC]

## Edge

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_beam.html">
    <img src="element_beam.png" alt="straight">
    <span class="wood-card-title">straight</span>
  </a>
  <a class="wood-card" href="elements_beam_curved.html">
    <img src="element_beam_curved.png" alt="curved">
    <span class="wood-card-title">curved</span>
  </a>
  <a class="wood-card" href="elements_beam_variable.html">
    <img src="element_beam_variable.png" alt="variable">
    <span class="wood-card-title">variable</span>
  </a>
</div>

## Face

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_plate.html">
    <img src="element_plate.png" alt="planar">
    <span class="wood-card-title">planar</span>
  </a>
  <a class="wood-card" href="elements_plate_curved.html">
    <img src="element_plate_curved.png" alt="curved">
    <span class="wood-card-title">curved</span>
  </a>
</div>

## Node

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_block.html">
    <img src="element_block.png" alt="block">
    <span class="wood-card-title">block</span>
  </a>
  <a class="wood-card" href="elements_support.html">
    <img src="element_support.png" alt="support">
    <span class="wood-card-title">support</span>
  </a>
</div>

## Joints

### Plates

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint_plate_side_side_inplane.html">
    <img src="element_joint_plate_side_side_inplane.png" alt="side to side, in plane">
    <span class="wood-card-title">side to side, in plane</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_outplane.html">
    <img src="element_joint_plate_side_side_outplane.png" alt="side to side, out of plane">
    <span class="wood-card-title">side to side, out of plane</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_rotated.html">
    <img src="element_joint_plate_side_side_rotated.png" alt="side to side, rotated">
    <span class="wood-card-title">side to side, rotated</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_side.html">
    <img src="element_joint_plate_top_side.png" alt="top to side">
    <span class="wood-card-title">top to side</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_top.html">
    <img src="element_joint_plate_top_top.png" alt="top to top">
    <span class="wood-card-title">top to top</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_cross.html">
    <img src="element_joint_plate_cross.png" alt="cross">
    <span class="wood-card-title">cross</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_border.html">
    <img src="element_joint_plate_border.png" alt="border">
    <span class="wood-card-title">border</span>
  </a>
</div>

### Beams

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint_beam_cross.html">
    <img src="element_joint_beam_cross.png" alt="cross">
    <span class="wood-card-title">cross</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_top_side.html">
    <img src="element_joint_beam_top_side.png" alt="top to side">
    <span class="wood-card-title">top to side</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_top_side_bridle.html">
    <img src="element_joint_beam_top_side_bridle.png" alt="bridle">
    <span class="wood-card-title">bridle</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_end_to_end.html">
    <img src="element_joint_beam_end_to_end.png" alt="end to end">
    <span class="wood-card-title">end to end</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_butt.html">
    <img src="element_joint_beam_butt.png" alt="butt">
    <span class="wood-card-title">butt</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_column_head.html">
    <img src="element_joint_beam_column_head.png" alt="column head">
    <span class="wood-card-title">column head</span>
  </a>
</div>

### Connectors

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint_beam_connectors.html">
    <img src="element_joint_beam_connectors.png" alt="connectors">
    <span class="wood-card-title">connectors</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_keyed_box.html">
    <img src="element_joint_beam_keyed_box.png" alt="keyed box">
    <span class="wood-card-title">keyed box</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_pins.html">
    <img src="element_joint_beam_pins.png" alt="pins">
    <span class="wood-card-title">pins</span>
  </a>
</div>

### Cuts

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_cut_plane.html">
    <img src="element_beam_variable_cut.png" alt="cut plane">
    <span class="wood-card-title">cut plane</span>
  </a>
  <a class="wood-card" href="elements_joint.html">
    <img src="element_joint_drill.png" alt="drill">
    <span class="wood-card-title">drill</span>
  </a>
</div>

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
- @subpage elements_joint_beam_top_side_bridle
- @subpage elements_joint_beam_column_head
- @subpage elements_joint_beam_keyed_box
- @subpage elements_joint_beam_end_to_end
- @subpage elements_joint_beam_butt
- @subpage elements_joint_beam_connectors
- @subpage elements_joint_beam_pins
</div>
