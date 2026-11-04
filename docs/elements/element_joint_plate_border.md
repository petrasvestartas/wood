# Plate Border {#elements_joint_plate_border}

[TOC]

The boundary family, a joint on one plate alone: `JointPlateElement::border()` cuts a slab off a side face along the whole side and through the whole thickness, 15 mm deep, and `JointPlateElement::border_custom(...)` cuts what its outlines say; both oriented onto the side by `WoodSession::compute_border_contact(plate, side_index)`, the contact of a plate that has no neighbour on that side. The joint's scale extends the cut: x beyond the side's ends, y beyond the faces, z deeper, a millimetre each by default.

![](elements/element_joint_plate_border.png)

## Constructors

```cpp
static std::shared_ptr<JointPlateElement> border();
static std::shared_ptr<JointPlateElement> border_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);
```

## Usage

`WoodSession::compute_border_contact(plate, side_index)` extracts an `InteractionContactFace` frame directly on the requested boundary edge of `plate`, allowing single-plate edge profiles, slots, or notches to be oriented and applied through `joint->interaction(0)`:

![](elements/element_joint_plate_border.png)

\include{lineno} elements/element_joint_plate_border.cpp
