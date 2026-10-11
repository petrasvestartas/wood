# JointPlate ss_e_op {#elements_joint_plate_ss_e_op}

[TOC]

The side-to-side out-of-plane family: fingers and notches, or tenons and mortises, where two plates meet at an angle.

## Constructors

```cpp
// a zero division count is geometric: one division every 450 mm of the joint line, the family's division length;
// the shift is the family's 0.64
static std::shared_ptr<JointPlate> ss_e_op_0();                                        // three fixed fingers
static std::shared_ptr<JointPlate> ss_e_op_1(int divisions = 0, double shift = 0.64);  // a zigzag of fingers, at least two divisions and even
static std::shared_ptr<JointPlate> ss_e_op_2(int divisions = 0, double shift = 0.64);  // the zigzag with a non-uniform shift, the central pairs moved twice the outer, at least four divisions
static std::shared_ptr<JointPlate> ss_e_op_3();                                        // one fixed tenon on the wall through a mortise in the floor
static std::shared_ptr<JointPlate> ss_e_op_4(
    int divisions = 0,
    double taper = 0.0,                                                                 // the tenon sides' lean
    bool chamfer = true,                                                                // chamfered tenon ends
    bool modify_outline = true,                                                         // the floor's edge modified around the tenons
    const std::array<double, 2>& x = {-0.5, 0.5},                                      // the tenon box in the joint's unit frame
    const std::array<double, 2>& y = {-0.5, 0.5},
    const std::array<double, 2>& z = {-0.5, 0.5}
);                                                                                      // chamfered tenons on the wall, a through mortise per tenon in the floor
static std::shared_ptr<JointPlate> ss_e_op_5(int divisions = 0, bool disable_divisions = false);  // ss_e_op_4 on this joint and its one or two linked joints, the tenons lengthened to -0.75
static std::shared_ptr<JointPlate> ss_e_op_6(int divisions = 0);                       // ss_e_op_5 with the second link's divisions disabled, the Vidy wall
static std::shared_ptr<JointPlate> ss_e_op_17(int divisions = 4);                      // ss_e_op_0 with divisions / 2 fingers and flat mitre caps, by name only
static std::shared_ptr<JointPlate> ss_e_op_tutorial();                                 // one rectangular notch, the worked example of a new design, by name only
static std::shared_ptr<JointPlate> ss_e_op_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows two pairs, `Plate::pair_folded(90.0)` and `Plate::pair_folded(60.0)`: a floor and a wall on a mitred corner, square and folded to 60 degrees. The wall is target 0, the male.

## ss_e_op_0: three fingers

![ss_e_op_0](elements/element_joint_plate_ss_e_op_0.png)

![ss_e_op_0, the unit box: male pink, female blue](joint_library/op_ss_e_op_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_0(); // a fixed design, no parameters
```

The fixed three-finger joint merged into both mitred edges: the floor's three fingers reach under the wall, the wall notched for them over its thickness.

\include{lineno} elements/element_joint_plate_ss_e_op_0.cpp

## ss_e_op_1: parametric fingers

![ss_e_op_1](elements/element_joint_plate_ss_e_op_1.png)

![ss_e_op_1, the unit box: male pink, female blue](joint_library/op_ss_e_op_1_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_1(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the fingers' lean through the thickness, only above 0.5
);
```

Eight divisions at shift 0.64: four fingers on the floor in four notches of the wall.

\include{lineno} elements/element_joint_plate_ss_e_op_1.cpp

## ss_e_op_2: fingers with a non-uniform shift

![ss_e_op_2](elements/element_joint_plate_ss_e_op_2.png)

![ss_e_op_2, the unit box: male pink, female blue](joint_library/op_ss_e_op_2_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_2(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the fingers' lean through the thickness, only above 0.5
);
```

Eight divisions at shift 0.64: the zigzag of `ss_e_op_1` with splayed fingers.

\include{lineno} elements/element_joint_plate_ss_e_op_2.cpp

## ss_e_op_3: mitre tenon and mortise

![ss_e_op_3](elements/element_joint_plate_ss_e_op_3.png)

![ss_e_op_3, the unit box: male pink, female blue](joint_library/op_ss_e_op_3_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_3(); // a fixed design, no parameters
```

One tenon in the middle of the wall's edge, through a mortise in a lip of the floor.

\include{lineno} elements/element_joint_plate_ss_e_op_3.cpp

## ss_e_op_4: chamfered tenons and through mortises

![ss_e_op_4](elements/element_joint_plate_ss_e_op_4.png)

![ss_e_op_4, the unit box: male pink, female blue](joint_library/op_ss_e_op_4_8_0_0_1_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_4(
    8,     // divisions: how many teeth along the joint line; 0 takes them from its length
    0,     // taper: the tenons' side lean
    false  // chamfer: bevelled tenon ends inside the floor
);
```

On its defaults: one chamfered tenon on the wall and a through mortise in the floor; the merge into mitred outlines is not right yet.

\include{lineno} elements/element_joint_plate_ss_e_op_4.cpp

## ss_e_op_5: linked tenons

![ss_e_op_5](elements/element_joint_plate_ss_e_op_5.png)

![ss_e_op_5, the unit box: male pink, female blue](joint_library/op_ss_e_op_5_8_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_5(
    8,     // divisions: how many teeth along the joint line; 0 takes them from its length
    false  // disable_divisions: the second linked joint (Vidy) without divisions
);
```

`ss_e_op_4` with longer chamfered tenons on the wall and a through mortise per tenon in the floor.

\include{lineno} elements/element_joint_plate_ss_e_op_5.cpp

## ss_e_op_6: the Vidy wall

![ss_e_op_6](elements/element_joint_plate_ss_e_op_6.png)

![ss_e_op_6, the unit box: male pink, female blue](joint_library/op_ss_e_op_6_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_6(
    8  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

`ss_e_op_5` with its second linked joint's divisions disabled; with no linked joint, the same tenons and mortises.

\include{lineno} elements/element_joint_plate_ss_e_op_6.cpp

## A Vidy node: four plates

![a Vidy node, with the viewer's layers](elements/element_joint_plate_vidy_node.png)

Where ss_e_op_5 and ss_e_op_6 are used: a Vidy corner node of two wall plates and two roof plates.

\include{lineno} elements/element_joint_plate_vidy_node.cpp

## ss_e_op_17: fingers with flat mitre caps

![ss_e_op_17](elements/element_joint_plate_ss_e_op_17.png)

![ss_e_op_17, the unit box: male pink, female blue](joint_library/op_ss_e_op_17_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_17(
    4  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

Four divisions, two fingers, the mitre capped flat at both ends; the merge into mitred outlines is not right yet.

\include{lineno} elements/element_joint_plate_ss_e_op_17.cpp

## ss_e_op_tutorial: one notch

![ss_e_op_tutorial](elements/element_joint_plate_ss_e_op_tutorial.png)

![ss_e_op_tutorial, the unit box: male pink, female blue](joint_library/op_ss_e_op_tutorial_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_tutorial(); // a fixed design, no parameters
```

One rectangular notch over the middle half of the joint line: the floor's tongue under the wall.

\include{lineno} elements/element_joint_plate_ss_e_op_tutorial.cpp

## ss_e_op_custom: your own outlines

![ss_e_op_custom](elements/element_joint_plate_ss_e_op_custom.png)

![ss_e_op_custom, the unit box: male pink, female blue](joint_library/op_ss_e_op_custom_unit.png)

A custom tile: three fingers on the floor, wide, narrow, wide, and the sockets they fill in the wall.

\include{lineno} elements/element_joint_plate_ss_e_op_custom.cpp

## Parameters

**Divisions and shift.** Divisions set the number of fingers; a shift above 0.5 skews them into a dovetail.

![ss_e_op_1 sweep](joint_library/sweep_ss_e_op_1.png)

**At other angles.** The same fingers at 90, 120 and 150 degrees: they keep their count and lean with the mitre.

![ss_e_op_1 at 90, 120 and 150 degrees](joint_library/sweep_ss_e_op_1_angles.png)

**Where it is used.** Two plates from the Chevron template (`src/templates/folding`), the shift in rows and divisions in columns.

![ss_e_op_1 on a Chevron pair](joint_library/sweep_ss_e_op_1_chevron.png)

ss_e_op_2 moves the central pairs twice as far as the outer ones; at shift 1 its outline crosses itself and no solid is built.

![ss_e_op_2 sweep](joint_library/sweep_ss_e_op_2.png)

**ss_e_op_4's own parameters.** The taper narrows the tenons; the chamfer bevels their ends inside the floor.

![ss_e_op_4 sweep](joint_library/sweep_ss_e_op_4.png)

![ss_e_op_5, ss_e_op_6 and ss_e_op_17 sweep](joint_library/sweep_ss_e_op_5_6_17.png)
