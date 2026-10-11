# JointPlate tt_e_p {#elements_joint_plate_tt_e_p}

[TOC]

The top-to-top family: two plates stacked face on face, pinned through their contact by holes drilled one plate thickness into each, as the 2024 library laid them.

## Constructors

```cpp
// the family's 2024 defaults are division length 6 and shift 0.95, each design reading them its own way;
// the radius is the pin's, a property of the hole and no 2024 parameter
static std::shared_ptr<JointPlate> tt_e_p_0(double radius = 1.0);                                                       // one hole at the centre of the contact
static std::shared_ptr<JointPlate> tt_e_p_1(double radius = 1.0);                                                       // one hole at the polylabel, the centre of the largest inscribed circle
static std::shared_ptr<JointPlate> tt_e_p_2(int divisions = 6, double shift = 0.95, double radius = 1.0);               // `divisions` holes on the inscribed circle scaled by the shift
static std::shared_ptr<JointPlate> tt_e_p_3(double division_length = 6.0, double shift = 0.95, double radius = 1.0);    // the contact offset inward by the shift (mm), a hole every division length along that ring
static std::shared_ptr<JointPlate> tt_e_p_4(double division_length = 6.0, double shift = 0.95, double radius = 1.0);    // a lattice of the division length filling the offset ring
static std::shared_ptr<JointPlate> tt_e_p_5(double division_length = 6.0, double shift = 0.95, double radius = 1.0);    // the largest empty rectangle inset by 1 - shift, a hole every division length along its edges, a grid when negative
static std::shared_ptr<JointPlate> tt_e_p_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example joins two pairs, `Plate::pair_stacked(0.0)` and `Plate::pair_stacked(30.0)`: two 400 x 300 plates 40 thick stacked, the upper one's centre shifted, square and turned 30 degrees about the vertical, the joint oriented on their face contact and passed to each plate with `add_interaction`, the upper plate lifted afterwards.

## tt_e_p_0: the centre

![tt_e_p_0](elements/element_joint_plate_tt_e_p_0.png)

![tt_e_p_0, the unit box: male pink, female blue](joint_library/tt_tt_e_p_0_8_unit.png)
![tt_e_p_0, the plan](elements/element_joint_plate_tt_e_p_0_plan.png)

One hole at the centre of the contact, drilled through both plates.

\include{lineno} elements/element_joint_plate_tt_e_p_0.cpp

## tt_e_p_1: the polylabel

![tt_e_p_1](elements/element_joint_plate_tt_e_p_1.png)

![tt_e_p_1, the unit box: male pink, female blue](joint_library/tt_tt_e_p_1_8_unit.png)
![tt_e_p_1, the plan](elements/element_joint_plate_tt_e_p_1_plan.png)

One hole at the polylabel of the contact, the centre of the largest circle inscribed in it, which on this octagon lies apart from the centre of `tt_e_p_0`.

\include{lineno} elements/element_joint_plate_tt_e_p_1.cpp

## tt_e_p_2: holes on the inscribed circle

![tt_e_p_2](elements/element_joint_plate_tt_e_p_2.png)

![tt_e_p_2, the unit box: male pink, female blue](joint_library/tt_tt_e_p_2_6_0.95_8_unit.png)
![tt_e_p_2, the plan](elements/element_joint_plate_tt_e_p_2_plan.png)

Six holes on the largest inscribed circle, its radius scaled by the shift 0.95, from 45 degrees on in a frame along the contact edge nearest the centre.

\include{lineno} elements/element_joint_plate_tt_e_p_2.cpp

## tt_e_p_3: holes along the offset ring

![tt_e_p_3](elements/element_joint_plate_tt_e_p_3.png)

![tt_e_p_3, the unit box: male pink, female blue](joint_library/tt_tt_e_p_3_60_12_8_unit.png)
![tt_e_p_3, the plan](elements/element_joint_plate_tt_e_p_3_plan.png)

The contact offset inward by the shift, 20 mm, as 2024's Clipper inset it, and a hole every 60 along that ring.

\include{lineno} elements/element_joint_plate_tt_e_p_3.cpp

## tt_e_p_4: a lattice in the offset ring

![tt_e_p_4](elements/element_joint_plate_tt_e_p_4.png)

![tt_e_p_4, the unit box: male pink, female blue](joint_library/tt_tt_e_p_4_60_12_8_unit.png)
![tt_e_p_4, the plan](elements/element_joint_plate_tt_e_p_4_plan.png)

The contact offset inward by 20 and filled with a lattice of holes 60 apart from the ring's first corner.

\include{lineno} elements/element_joint_plate_tt_e_p_4.cpp

## tt_e_p_5: holes along the largest rectangle

![tt_e_p_5](elements/element_joint_plate_tt_e_p_5.png)

![tt_e_p_5, the unit box: male pink, female blue](joint_library/tt_tt_e_p_5_60_0.95_8_unit.png)
![tt_e_p_5, the plan](elements/element_joint_plate_tt_e_p_5_plan.png)

The largest empty rectangle 2024 inscribed in the contact, inset by 1 - 0.95 of its shorter extent, and a hole every 60 along its edges; a negative division length fills it with a grid of that step instead.

\include{lineno} elements/element_joint_plate_tt_e_p_5.cpp

## tt_e_p_custom: your own outlines

![tt_e_p_custom](elements/element_joint_plate_tt_e_p_custom.png)

![tt_e_p_custom, the unit box: male pink, female blue](joint_library/tt_tt_e_p_custom_unit.png)
![tt_e_p_custom, the plan](elements/element_joint_plate_tt_e_p_custom_plan.png)

Your own outlines, given as pairs (face 0, face 1) in the unit box, x across the contact, y along the plates' normal from the lower plate's bottom at -0.5 through the contact at 0 to the upper plate's top at 0.5, z along the contact. Here a hidden butterfly key: a bow-tie pocket milled into each plate from the contact face, and the key, the joint's own piece, set in both.

\include{lineno} elements/element_joint_plate_tt_e_p_custom.cpp

## Parameters

A pattern (`tt_e_p_2` to `tt_e_p_5`) is laid on the contact less a border of two drill radii, so one radius of wood stays between every hole and the plate's edge.

![tt_e_p_2 sweep](joint_library/sweep_tt_e_p_2.png)
![tt_e_p_3, tt_e_p_4 and tt_e_p_5 sweep](joint_library/sweep_tt_e_p_3_4_5.png)
