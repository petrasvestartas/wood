# JointPlate ss_e_r {#elements_joint_plate_ss_e_r}

[TOC]

The side-to-side rotated family: a design cut into the side faces of two plates whose seam is twisted.

## Constructors

```cpp
// a zero division count is geometric: one division every 300 mm of the joint line, the family's division length;
// the shift is the family's 0.5, a 60 mm tile
static std::shared_ptr<JointPlate> ss_e_r_0();                                       // the averaged volume split through its thickness, the halves sliced off four ways along the joint line
static std::shared_ptr<JointPlate> ss_e_r_2(int divisions = 0, double shift = 0.5);  // a hook pocket into each plate per division, and the key that fills both
static std::shared_ptr<JointPlate> ss_e_r_3(int divisions = 0, double shift = 0.5);  // a diamond pocket into each plate per division, and the key that fills both
static std::shared_ptr<JointPlate> ss_e_r_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5);           // each plate's side face milled off by the joint scale, the male's outlines doubled under merge_with_joint
static std::shared_ptr<JointPlate> side_removal_ss_e_r_1();  // the trait de Jupiter of two plates in line: abouts, slopes, the redan and its key

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows two pairs, `Plate::pair_rotated(0.0)` and `Plate::pair_rotated(20.0)`: two blocks end to end, side by side and twisted 20 degrees about the seam, drawn apart.

## ss_e_r_0: four slices

![ss_e_r_0](elements/element_joint_plate_ss_e_r_0.png)

![ss_e_r_0, the unit box: male pink, female blue](joint_library/r_ss_e_r_0_unit.png)

The averaged joint volume split through the thickness and sliced off both plates; here slivers at the seam corners.

\include{lineno} elements/element_joint_plate_ss_e_r_0.cpp

## ss_e_r_2: hook pockets and key

![ss_e_r_2](elements/element_joint_plate_ss_e_r_2.png)

![ss_e_r_2, the unit box: male pink, female blue](joint_library/r@120_ss_e_r_2_4_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_r_2(
    4,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the size of each key along the joint line
);
```

A hook pocket per division in each plate and the key that fills them, drawn between the plates.

\include{lineno} elements/element_joint_plate_ss_e_r_2.cpp

## ss_e_r_3: diamond pockets and key

![ss_e_r_3](elements/element_joint_plate_ss_e_r_3.png)

![ss_e_r_3, the unit box: male pink, female blue](joint_library/r@120_ss_e_r_3_4_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_r_3(
    4,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the size of each key along the joint line
);
```

A diamond pocket per division in each plate and the key that fills them, drawn between the plates.

\include{lineno} elements/element_joint_plate_ss_e_r_3.cpp

## ss_e_r_custom: your own outlines

![ss_e_r_custom](elements/element_joint_plate_ss_e_r_custom.png)

![ss_e_r_custom, the unit box: male pink, female blue](joint_library/r_ss_e_r_custom_unit.png)

A custom half-lap step along the seam: the male's tongue fills the female's rebate; shown on the side-by-side pair only.

\include{lineno} elements/element_joint_plate_ss_e_r_custom.cpp

## side_removal: the side faces milled off

![side_removal](elements/element_joint_plate_ss_e_r_side_removal.png)

![side_removal, the unit box: male pink, female blue](joint_library/r_side_removal_0_0.5_unit.png)

Each plate's side face, widened by the joint scale, milled off along its normal.

\include{lineno} elements/element_joint_plate_ss_e_r_side_removal.cpp

## side_removal_ss_e_r_1: the trait de Jupiter

![side_removal_ss_e_r_1](elements/element_joint_plate_ss_e_r_side_removal_ss_e_r_1.png)

![side_removal_ss_e_r_1, the unit box: male pink, female blue](joint_library/ip_side_removal_ss_e_r_1_unit.png)

`Plate::pair_in_plane()`: a scarf through the thickness with a redan and a key driven through both plates.

\include{lineno} elements/element_joint_plate_ss_e_r_side_removal_ss_e_r_1.cpp

## Parameters

**Divisions.** One key per division.

![ss_e_r sweep](joint_library/sweep_ss_e_r.png)
