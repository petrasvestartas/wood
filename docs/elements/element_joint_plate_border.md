# Plate Border {#elements_joint_plate_border}

[TOC]

Single-plate edge profile joinery (`JointPlate::border()`, `JointPlate::border_custom(...)`) oriented onto a plate side face via `WoodSession::compute_border_contact(plate, side_index)`.

![](elements/element_joint_plate_border.png)

## Constructors

```cpp
static std::shared_ptr<JointPlate> border();
static std::shared_ptr<JointPlate> border_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);
```

## Usage

`WoodSession::compute_border_contact(plate, side_index)` extracts an `InteractionContactFace` frame directly on the requested boundary edge of `plate`, allowing single-plate edge profiles, slots, or notches to be oriented and applied through `joint->interaction(0)`:

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_border_contact(plate, 2);
const std::shared_ptr<JointPlate> joint = JointPlate::border();
joint->orient(contact, {plate});
scene.add(joint);
scene.add_interaction(joint, plate, joint->interaction(0));
```
