# JointPlate corner {#elements_joint_plate_corner}

[TOC]

The side-to-side out-of-plane family: fingers and notches, or tenons and mortises, where two plates meet at an angle.

## Constructors

```cpp
// a zero division count is geometric: one division every 450 mm of the joint line, the family's division length;
// the shift is the family's 0.64
static std::shared_ptr<JointPlate> corner_three_fingers();                                        // three fixed fingers
static std::shared_ptr<JointPlate> corner_fingers(int divisions = 0, double shift = 0.64);  // a zigzag of fingers, at least two divisions and even
static std::shared_ptr<JointPlate> corner_graded_fingers(int divisions = 0, double shift = 0.64);  // the zigzag with a non-uniform shift, the central pairs moved twice the outer, at least four divisions
static std::shared_ptr<JointPlate> corner_mitre_tenon();                                        // one fixed tenon on the wall through a mortise in the floor
static std::shared_ptr<JointPlate> corner_tenons(
    int divisions = 0,
    double taper = 0.0,                                                                 // the tenon sides' lean
    bool chamfer = true,                                                                // chamfered tenon ends
    bool modify_outline = true,                                                         // the floor's edge modified around the tenons
    const std::array<double, 2>& x = {-0.5, 0.5},                                      // the tenon box in the joint's unit frame
    const std::array<double, 2>& y = {-0.5, 0.5},
    const std::array<double, 2>& z = {-0.5, 0.5}
);                                                                                      // chamfered tenons on the wall, a through mortise per tenon in the floor
static std::shared_ptr<JointPlate> corner_linked_tenons(int divisions = 0, bool disable_divisions = false);  // corner_tenons on this joint and its one or two linked joints, the tenons lengthened to -0.75
static std::shared_ptr<JointPlate> corner_linked_tenons_wall(int divisions = 0);                       // corner_linked_tenons with the second link's divisions disabled, the Vidy wall
static std::shared_ptr<JointPlate> corner_capped_fingers(int divisions = 4);                      // corner_three_fingers with divisions / 2 fingers and flat mitre caps, by name only
static std::shared_ptr<JointPlate> corner_notch();                                 // one rectangular notch, the worked example of a new design, by name only
static std::shared_ptr<JointPlate> corner_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows two pairs, `Plate::pair_folded(90.0)` and `Plate::pair_folded(60.0)`: a floor and a wall on a mitred corner, square and folded to 60 degrees. The wall is target 0, the male.

## corner_three_fingers: three fingers

![corner_three_fingers](elements/element_joint_plate_corner_three_fingers.png)

![corner_three_fingers, the unit box: male pink, female blue](joint_library/op_corner_three_fingers_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_three_fingers(); // a fixed design, no parameters
```

The fixed three-finger joint merged into both mitred edges: the floor's three fingers reach under the wall, the wall notched for them over its thickness.

\include{lineno} elements/element_joint_plate_corner_three_fingers.cpp

## corner_fingers: parametric fingers

![corner_fingers](elements/element_joint_plate_corner_fingers.png)

![corner_fingers, the unit box: male pink, female blue](joint_library/op_corner_fingers_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_fingers(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the fingers' lean through the thickness, only above 0.5
);
```

Eight divisions at shift 0.64: four fingers on the floor in four notches of the wall.

\include{lineno} elements/element_joint_plate_corner_fingers.cpp

## corner_graded_fingers: fingers with a non-uniform shift

![corner_graded_fingers](elements/element_joint_plate_corner_graded_fingers.png)

![corner_graded_fingers, the unit box: male pink, female blue](joint_library/op_corner_graded_fingers_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_graded_fingers(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the fingers' lean through the thickness, only above 0.5
);
```

Eight divisions at shift 0.64: the zigzag of `corner_fingers` with splayed fingers.

\include{lineno} elements/element_joint_plate_corner_graded_fingers.cpp

## corner_mitre_tenon: mitre tenon and mortise

![corner_mitre_tenon](elements/element_joint_plate_corner_mitre_tenon.png)

![corner_mitre_tenon, the unit box: male pink, female blue](joint_library/op_corner_mitre_tenon_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_mitre_tenon(); // a fixed design, no parameters
```

One tenon, 0.3 of the seam wide, in the middle of the wall's edge, through a mortise in a lip of the floor.

\include{lineno} elements/element_joint_plate_corner_mitre_tenon.cpp

## corner_tenons: chamfered tenons and through mortises

![corner_tenons](elements/element_joint_plate_corner_tenons.png)

![corner_tenons, the unit box: male pink, female blue](joint_library/op_corner_tenons_8_0_0_1_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_tenons(
    8,     // divisions: how many teeth along the joint line; 0 takes them from its length
    0,     // taper: the tenons' side lean
    false  // chamfer: bevelled tenon ends inside the floor
);
```

On its defaults: one chamfered tenon on the wall and a through mortise in the floor; the merge into mitred outlines is not right yet.

\include{lineno} elements/element_joint_plate_corner_tenons.cpp

## corner_linked_tenons: linked tenons

![corner_linked_tenons](elements/element_joint_plate_corner_linked_tenons.png)

![corner_linked_tenons, the unit box: male pink, female blue](joint_library/op_corner_linked_tenons_8_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_linked_tenons(
    8,     // divisions: how many teeth along the joint line; 0 takes them from its length
    false  // disable_divisions: the second linked joint (Vidy) without divisions
);
```

`corner_tenons` with longer chamfered tenons on the wall and a through mortise per tenon in the floor.

\include{lineno} elements/element_joint_plate_corner_linked_tenons.cpp

## corner_linked_tenons_wall: the Vidy wall

![corner_linked_tenons_wall](elements/element_joint_plate_corner_linked_tenons_wall.png)

![corner_linked_tenons_wall, the unit box: male pink, female blue](joint_library/op_corner_linked_tenons_wall_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_linked_tenons_wall(
    8  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

`corner_linked_tenons` with its second linked joint's divisions disabled; with no linked joint, the same tenons and mortises.

\include{lineno} elements/element_joint_plate_corner_linked_tenons_wall.cpp

## A Vidy node: four plates

![a Vidy node, with the viewer's layers](elements/element_joint_plate_vidy_node.png)

Where corner_linked_tenons and corner_linked_tenons_wall are used: a Vidy corner node of two wall plates and two roof plates.

\include{lineno} elements/element_joint_plate_vidy_node.cpp

## corner_capped_fingers: fingers with flat mitre caps

![corner_capped_fingers](elements/element_joint_plate_corner_capped_fingers.png)

![corner_capped_fingers, the unit box: male pink, female blue](joint_library/op_corner_capped_fingers_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_capped_fingers(
    4  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

Four divisions, two fingers, the mitre capped flat at both ends; the merge into mitred outlines is not right yet.

\include{lineno} elements/element_joint_plate_corner_capped_fingers.cpp

## corner_notch: one notch

![corner_notch](elements/element_joint_plate_corner_notch.png)

![corner_notch, the unit box: male pink, female blue](joint_library/op_corner_notch_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::corner_notch(); // a fixed design, no parameters
```

One rectangular notch over the middle half of the joint line: the floor's tongue under the wall.

\include{lineno} elements/element_joint_plate_corner_notch.cpp

## corner_custom: your own outlines

![corner_custom](elements/element_joint_plate_corner_custom.png)

![corner_custom, the unit box: male pink, female blue](joint_library/op_corner_custom_unit.png)

A custom tile: three fingers on the floor, wide, narrow, wide, and the sockets they fill in the wall.

\include{lineno} elements/element_joint_plate_corner_custom.cpp

## Parameters

**Divisions and shift.** Divisions set the number of fingers; a shift above 0.5 skews them into a dovetail.

![corner_fingers sweep](joint_library/sweep_corner_fingers.png)

**At other angles.** The same fingers at 90, 120 and 150 degrees: they keep their count and lean with the mitre.

![corner_fingers at 90, 120 and 150 degrees](joint_library/sweep_corner_fingers_angles.png)

**Where it is used.** Two plates from the Chevron template (`src/templates/folding`), the shift in rows and divisions in columns.

![corner_fingers on a Chevron pair](joint_library/sweep_corner_fingers_chevron.png)

corner_graded_fingers moves the central pairs twice as far as the outer ones; at shift 1 its outline crosses itself and no solid is built.

![corner_graded_fingers sweep](joint_library/sweep_corner_graded_fingers.png)

**corner_tenons's own parameters.** The taper narrows the tenons; the chamfer bevels their ends inside the floor.

![corner_tenons sweep](joint_library/sweep_corner_tenons.png)

![corner_linked_tenons, corner_linked_tenons_wall and corner_capped_fingers sweep](joint_library/sweep_corner_linked_tenons_6_17.png)
