# JointPlate cross {#elements_joint_plate_cross}

[TOC]

Cross-lap joints where two plates pass through each other along a crossing line found by `compute_cross_contact`. Plates use `cross_slot` and `cross_custom`; the multi-ring milled half-laps are used on crossing beams ([JointBeam cross](@ref elements_joint_beam_cross)).

## Constructors

```cpp
static std::shared_ptr<JointPlate> cross_slot();
static std::shared_ptr<JointPlate> cross_sliced_half_lap(double shift = 0.5);
static std::shared_ptr<JointPlate> cross_half_lap(double shift = 0.5);
static std::shared_ptr<JointPlate> cross_half_lap_diagonal_drills(double shift = 0.5);
static std::shared_ptr<JointPlate> cross_half_lap_drill(double shift = 0.5);
static std::shared_ptr<JointPlate> cross_brussels_half_lap(double shift = 0.5);
static std::shared_ptr<JointPlate> cross_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
std::shared_ptr<Interaction> interaction(size_t target) const;
```

Every example shows two crossing pairs (`Plate::pair_crossing(90.0)` and `Plate::pair_crossing(60.0)`) with the second plate shifted downward out of its slot.

## cross_slot: plain half-depth slot

The family default cutting a half-depth rectangular slot from the top of the first plate and the bottom of the second plate.

![](elements/element_joint_plate_cross_slot.png)

![](joint_library/cr_cross_slot_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_slot();
```

<details><summary>Example code: elements/element_joint_plate_cross_slot.cpp</summary>

\include{lineno} elements/element_joint_plate_cross_slot.cpp

</details>

## cross_custom: custom outlines

User-defined crossing slot outlines in the `[-0.5, 0.5]^3` unit box (`male` on the `y` faces, `female` on the `x` faces).

![](elements/element_joint_plate_cross_custom.png)

![](joint_library/cr_cross_custom_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_custom(male, female);
```

<details><summary>Example code: elements/element_joint_plate_cross_custom.cpp</summary>

\include{lineno} elements/element_joint_plate_cross_custom.cpp

</details>
