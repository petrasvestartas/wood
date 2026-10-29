# Elements {#elements}

[TOC]

Timber members, auxiliary cutters, and parametric joints in `src/joinery_solver/wood_elements/`. Each element stores uncut stock geometry and evaluates incoming interactions in `WoodSession` to produce cut meshes and exact NURBS BReps.

## Shape Elements

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_plate.html">
    <img src="element_plate.png" alt="Plate">
    <span class="wood-card-title">Plate</span>
  </a>
  <a class="wood-card" href="elements_plate_curved.html">
    <img src="element_plate_curved.png" alt="Curved Plate">
    <span class="wood-card-title">Curved Plate</span>
  </a>
  <a class="wood-card" href="elements_beam.html">
    <img src="element_beam.png" alt="Beam">
    <span class="wood-card-title">Beam</span>
  </a>
  <a class="wood-card" href="elements_beam_curved.html">
    <img src="element_beam_curved.png" alt="Curved Beam">
    <span class="wood-card-title">Curved Beam</span>
  </a>
  <a class="wood-card" href="elements_beam_variable.html">
    <img src="element_beam_variable.png" alt="Variable Beam">
    <span class="wood-card-title">Variable Beam</span>
  </a>
  <a class="wood-card" href="elements_column.html">
    <img src="element_column.png" alt="Column">
    <span class="wood-card-title">Column</span>
  </a>
  <a class="wood-card" href="elements_block.html">
    <img src="element_block.png" alt="Block">
    <span class="wood-card-title">Block</span>
  </a>
  <a class="wood-card" href="elements_support.html">
    <img src="element_support.png" alt="Support">
    <span class="wood-card-title">Support</span>
  </a>
  <a class="wood-card" href="elements_cut_plane.html">
    <img src="element_cut_plane.png" alt="Cut Plane">
    <span class="wood-card-title">Cut Plane</span>
  </a>
  <a class="wood-card" href="elements_profile.html">
    <img src="element_profile.webp" alt="Profiles">
    <span class="wood-card-title">Profiles</span>
  </a>
  <a class="wood-card" href="elements_pin.html">
    <img src="element_pin.png" alt="Pin">
    <span class="wood-card-title">Pin</span>
  </a>
</div>

## Joint Elements

Joint elements cut pairs of timber members and optionally attach connector blocks, keys, side plates, or dowel pins.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint.html">
    <img src="element_joint.png" alt="Joint">
    <span class="wood-card-title">Joint</span>
  </a>
  <a class="wood-card" href="elements_joint_plate.html">
    <img src="element_joint_plate.png" alt="Plate Joints">
    <span class="wood-card-title">Plate Joints</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_side.html">
    <img src="element_joint_plate_top_side.png" alt="Plate Top-to-Side">
    <span class="wood-card-title">Plate Top-to-Side</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_inplane.html">
    <img src="element_joint_plate_side_side_inplane.png" alt="Plate In-Plane">
    <span class="wood-card-title">Plate In-Plane</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_outplane.html">
    <img src="element_joint_plate_side_side_outplane.png" alt="Plate Out-of-Plane">
    <span class="wood-card-title">Plate Out-of-Plane</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_rotated.html">
    <img src="element_joint_plate_side_side_rotated.png" alt="Plate Rotated">
    <span class="wood-card-title">Plate Rotated</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_cross.html">
    <img src="element_joint_plate_cross.png" alt="Plate Cross">
    <span class="wood-card-title">Plate Cross</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_top.html">
    <img src="element_joint_plate_top_top.png" alt="Plate Top-to-Top">
    <span class="wood-card-title">Plate Top-to-Top</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_border.html">
    <img src="element_joint_plate_border.png" alt="Plate Border">
    <span class="wood-card-title">Plate Border</span>
  </a>
  <a class="wood-card" href="elements_joint_beam.html">
    <img src="element_joint_beam.png" alt="Beam Joints">
    <span class="wood-card-title">Beam Joints</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_cross.html">
    <img src="element_joint_beam_cross.png" alt="Beam Cross">
    <span class="wood-card-title">Beam Cross</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_top_side.html">
    <img src="element_joint_beam_top_side.png" alt="Beam Top-to-Side">
    <span class="wood-card-title">Beam Top-to-Side</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_end_to_end.html">
    <img src="element_joint_beam_end_to_end.png" alt="Beam End-to-End">
    <span class="wood-card-title">Beam End-to-End</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_butt.html">
    <img src="element_joint_beam_butt.png" alt="Beam Butt">
    <span class="wood-card-title">Beam Butt</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_connectors.html">
    <img src="element_joint_beam_connectors.png" alt="Beam Connectors">
    <span class="wood-card-title">Beam Connectors</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_pins.html">
    <img src="element_joint_beam_pins.png" alt="Beam Pins">
    <span class="wood-card-title">Beam Pins</span>
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
![](elements/element_plate.png)
![](elements/element_plate_curved.png)
![](elements/element_beam.png)
![](elements/element_beam_curved.png)
![](elements/element_beam_variable.png)
![](elements/element_column.png)
![](elements/element_block.png)
![](elements/element_support.png)
![](elements/element_cut_plane.png)
![](elements/element_profile.webp)
![](elements/element_pin.png)
![](elements/element_joint.png)
![](elements/element_joint_plate.png)
![](elements/element_joint_plate_top_side.png)
![](elements/element_joint_plate_side_side_inplane.png)
![](elements/element_joint_plate_side_side_outplane.png)
![](elements/element_joint_plate_side_side_rotated.png)
![](elements/element_joint_plate_cross.png)
![](elements/element_joint_plate_top_top.png)
![](elements/element_joint_plate_border.png)
![](elements/element_joint_beam.png)
![](elements/element_joint_beam_cross.png)
![](elements/element_joint_beam_top_side.png)
![](elements/element_joint_beam_end_to_end.png)
![](elements/element_joint_beam_butt.png)
![](elements/element_joint_beam_connectors.png)
![](elements/element_joint_beam_pins.png)

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
- @subpage elements_pin
</div>
