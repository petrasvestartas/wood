# Plate Rotated {#elements_joint_plate_side_side_rotated}

[TOC]

Side-to-side rotated plate joints connecting two plates whose side faces meet along a twisted seam (`Plate::pair_rotated(0.0)` and `Plate::pair_rotated(20.0)`).

## Constructors

```cpp
static std::shared_ptr<JointPlate> side_side_rotated_miter();
static std::shared_ptr<JointPlate> side_side_rotated_hooks(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> side_side_rotated_keys(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> side_side_rotated_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5);

void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
std::shared_ptr<Interaction> interaction(size_t target) const;
```

## Four-Way Miter Slices
Splits the shared contact volume through its thickness and slices the twisted seam corners off both plates.

![](elements/element_joint_plate_side_side_rotated_miter.png)

![](joint_library/r_side_side_rotated_miter_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_rotated_miter();
```

## Hook Pockets and Key
Cuts one hook pocket per division into each plate's side face and generates the interlocking hook keys between them.

![](elements/element_joint_plate_side_side_rotated_hooks.png)

![](joint_library/r@120_side_side_rotated_hooks_4_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_rotated_hooks(4, 0.5);
```

## Diamond Pockets and Key
Cuts one diamond pocket per division into each plate's side face and generates the diamond keys that bridge the seam.

![](elements/element_joint_plate_side_side_rotated_keys.png)

![](joint_library/r@120_side_side_rotated_keys_4_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_rotated_keys(4, 0.5);
```

## Custom Outlines
A custom half-lap step along the seam where the male plate's tongue fits into the female plate's rebate.

![](elements/element_joint_plate_side_side_rotated_custom.png)

![](joint_library/r_side_side_rotated_custom_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_rotated_custom(male, female);
```

## Planar Side-Face Trim
Mills each plate's side face along its neighbour's plane so the twisted pair meets flush across the entire seam.

![](elements/element_joint_plate_side_side_rotated_side_removal.png)

![](joint_library/r_side_removal_0_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_removal(false, 0.5);
```

## Parameters

Divisions. Places one hook or diamond key per division along the seam.

![](joint_library/sweep_side_side_rotated.png)
