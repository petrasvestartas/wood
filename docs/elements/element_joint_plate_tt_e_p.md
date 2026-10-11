# JointPlate tt_e_p {#elements_joint_plate_tt_e_p}

[TOC]

The top-to-top family: two plates stacked face on face, pinned through their contact by drilled holes.

## Constructors

```cpp
// the family's defaults are division length 6 and shift 0.95, each design reading them its own way;
// the radius is the pin's
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

Every example shows two pairs, `Plate::pair_stacked(0.0)` and `Plate::pair_stacked(30.0)`: two stacked plates, square and turned 30 degrees, the upper one lifted.

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

One hole at the polylabel of the contact, the centre of its largest inscribed circle.

\include{lineno} elements/element_joint_plate_tt_e_p_1.cpp

## tt_e_p_2: holes on the inscribed circle

![tt_e_p_2](elements/element_joint_plate_tt_e_p_2.png)

![tt_e_p_2, the unit box: male pink, female blue](joint_library/tt_tt_e_p_2_6_0.95_8_unit.png)
![tt_e_p_2, the plan](elements/element_joint_plate_tt_e_p_2_plan.png)

Six holes on the largest inscribed circle, its radius scaled by the shift 0.95.

\include{lineno} elements/element_joint_plate_tt_e_p_2.cpp

## tt_e_p_3: holes along the offset ring

![tt_e_p_3](elements/element_joint_plate_tt_e_p_3.png)

![tt_e_p_3, the unit box: male pink, female blue](joint_library/tt_tt_e_p_3_60_12_8_unit.png)
![tt_e_p_3, the plan](elements/element_joint_plate_tt_e_p_3_plan.png)

The contact offset inward by the shift, 20 mm, and a hole every 60 along that ring.

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

The largest empty rectangle in the contact and a hole every 60 along its edges; a negative length fills a grid.

\include{lineno} elements/element_joint_plate_tt_e_p_5.cpp

## tt_e_p_custom: your own outlines

![tt_e_p_custom](elements/element_joint_plate_tt_e_p_custom.png)

![tt_e_p_custom, the unit box: male pink, female blue](joint_library/tt_tt_e_p_custom_unit.png)
![tt_e_p_custom, the plan](elements/element_joint_plate_tt_e_p_custom_plan.png)

Your own outlines in the unit box; here a hidden butterfly key in a bow-tie pocket milled into each plate.

\include{lineno} elements/element_joint_plate_tt_e_p_custom.cpp

## Parameters

A pattern (`tt_e_p_2` to `tt_e_p_5`) is laid on the contact less a border of two drill radii.

![tt_e_p_2 sweep](joint_library/sweep_tt_e_p_2.png)
![tt_e_p_3, tt_e_p_4 and tt_e_p_5 sweep](joint_library/sweep_tt_e_p_3_4_5.png)
