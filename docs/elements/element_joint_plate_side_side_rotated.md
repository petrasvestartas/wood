# JointPlate side_side_rotated {#elements_joint_plate_side_side_rotated}

[TOC]

The side-to-side rotated family: a design cut into the side faces of two plates whose seam is twisted.

## Constructors

```cpp
// a zero division count is geometric: one division every 300 mm of the joint line, the family's division length;
// the shift is the family's 0.5, a 60 mm tile
static std::shared_ptr<JointPlate> side_side_rotated_miter();                                       // the averaged volume split through its thickness, the halves sliced off four ways along the joint line
static std::shared_ptr<JointPlate> side_side_rotated_hooks(int divisions = 0, double shift = 0.5);  // a hook pocket into each plate per division, and the key that fills both
static std::shared_ptr<JointPlate> side_side_rotated_keys(int divisions = 0, double shift = 0.5);  // a diamond pocket into each plate per division, and the key that fills both
static std::shared_ptr<JointPlate> side_side_rotated_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5);           // each plate's side face milled off by the joint scale, the male's outlines doubled under merge_with_joint

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows two pairs, `Plate::pair_rotated(0.0)` and `Plate::pair_rotated(20.0)`: two blocks end to end, side by side and twisted 20 degrees about the seam, drawn apart.

## side_side_rotated_miter: four slices

![side_side_rotated_miter](elements/element_joint_plate_side_side_rotated_miter.png)

![side_side_rotated_miter, the unit box: male pink, female blue](joint_library/r_side_side_rotated_miter_unit.png)

The averaged joint volume split through the thickness and sliced off both plates; here slivers at the seam corners.

\include{lineno} elements/element_joint_plate_side_side_rotated_miter.cpp

## side_side_rotated_hooks: hook pockets and key

![side_side_rotated_hooks](elements/element_joint_plate_side_side_rotated_hooks.png)

![side_side_rotated_hooks, the unit box: male pink, female blue](joint_library/r@120_side_side_rotated_hooks_4_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_rotated_hooks(
    4,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the size of each key along the joint line
);
```

A hook pocket per division in each plate and the key that fills them, drawn between the plates.

\include{lineno} elements/element_joint_plate_side_side_rotated_hooks.cpp

## side_side_rotated_keys: diamond pockets and key

![side_side_rotated_keys](elements/element_joint_plate_side_side_rotated_keys.png)

![side_side_rotated_keys, the unit box: male pink, female blue](joint_library/r@120_side_side_rotated_keys_4_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_rotated_keys(
    4,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the size of each key along the joint line
);
```

A diamond pocket per division in each plate and the key that fills them, drawn between the plates.

\include{lineno} elements/element_joint_plate_side_side_rotated_keys.cpp

## side_side_rotated_custom: your own outlines

![side_side_rotated_custom](elements/element_joint_plate_side_side_rotated_custom.png)

![side_side_rotated_custom, the unit box: male pink, female blue](joint_library/r_side_side_rotated_custom_unit.png)

A custom half-lap step along the seam: the male's tongue fills the female's rebate; shown on the side-by-side pair only.

\include{lineno} elements/element_joint_plate_side_side_rotated_custom.cpp

## side_removal: the side faces milled off

![side_removal](elements/element_joint_plate_side_side_rotated_side_removal.png)

![side_removal, the unit box: male pink, female blue](joint_library/r_side_removal_0_0.5_unit.png)

Each plate cut along its neighbour's side face, back through the neighbour and across the whole face, so the twisted pair meets flush; the square pair only touches and loses nothing.

\include{lineno} elements/element_joint_plate_side_side_rotated_side_removal.cpp

## Parameters

**Divisions.** One key per division.

![rotated sweep](joint_library/sweep_side_side_rotated.png)
