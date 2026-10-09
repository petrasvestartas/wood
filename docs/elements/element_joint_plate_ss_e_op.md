# JointPlate ss_e_op {#elements_joint_plate_ss_e_op}

[TOC]

The side-to-side out-of-plane family: a design cut into the edges of two plates that meet at an angle on a shared side face, a floor and a wall at a corner, so the fingers of one plate sit in the notches of the other, or the wall's tenons pass through mortises in the floor; the 2024 library's joint type 11, ids 10 to 19 (10 `ss_e_op_1`, 11 `ss_e_op_2`, 12 `ss_e_op_0`, 13 `ss_e_op_3`, 14 `ss_e_op_4`, 15 `ss_e_op_5`, the family's default id, 16 `ss_e_op_6`, 18 `side_removal`, 19 `ss_e_op_custom`; 17 and any other id take `ss_e_op_1`), used on its out-of-plane datasets: the Annen corner, boxes, grids and arch, the Vidy chapel corner and layer, the folding and the mitred box, the tetra, octa, dodeca and icosahedron boxes, the simple corners and the hexboxes.

## Constructors

```cpp
// a zero division count is geometric: one division every 450 mm of the joint line, the family's 2024 division length;
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

Every example is the same pair: a 300 x 400 floor plate and a 300 high wall plate, both 40 thick, meeting at a right angle on a mitred side face, the joint oriented on their face contact and passed to each plate with `add_interaction`, the wall first: `orient` turns an out-of-plane contact so target 0 is the second plate of the contact, the one the detector makes the male. The wall is then moved 100 out and 100 up, along the normal of its contact face, the mitre, the direction it slides onto the floor, so both sides read; every page shows the pair and a close-up of the joint. The joint's unit box puts the floor's thickness along x, the wall's along y and the joint line along z. No design of this family owns a piece: the fingers, tenons, notches and mortises belong to the plates and the joint stays hidden. `ss_e_op_5` and `ss_e_op_6` also stitch the one or two joints linked to them on the solver's datasets, the Vidy wall; on one pair they have no link and build `ss_e_op_4` with longer tenons.

## ss_e_op_0: three fingers

![ss_e_op_0](elements/element_joint_plate_ss_e_op_0.png)
![ss_e_op_0, the joint](elements/element_joint_plate_ss_e_op_0_joint.png)

The fixed three-finger joint merged into both mitred edges: the floor's three fingers reach under the wall, the wall notched for them over its thickness.

\include{lineno} elements/element_joint_plate_ss_e_op_0.cpp

## ss_e_op_1: parametric fingers

![ss_e_op_1](elements/element_joint_plate_ss_e_op_1.png)
![ss_e_op_1, the joint](elements/element_joint_plate_ss_e_op_1_joint.png)

Eight divisions at the family's shift 0.64: four fingers on the floor in four notches of the wall; on its defaults the count is geometric, one division every 450 of the joint line and at least two, which on this 400 seam is one finger.

\include{lineno} elements/element_joint_plate_ss_e_op_1.cpp

## ss_e_op_2: fingers with a non-uniform shift

![ss_e_op_2](elements/element_joint_plate_ss_e_op_2.png)
![ss_e_op_2, the joint](elements/element_joint_plate_ss_e_op_2_joint.png)

Eight divisions at the shift 0.64: the zigzag of `ss_e_op_1` with its central pairs moved twice as far as the outer ones and the sign flipped past the middle, so the fingers splay, the wall's fingers hanging below its edge into the floor's splayed notches.

\include{lineno} elements/element_joint_plate_ss_e_op_2.cpp

## ss_e_op_3: mitre tenon and mortise

![ss_e_op_3](elements/element_joint_plate_ss_e_op_3.png)
![ss_e_op_3, the joint](elements/element_joint_plate_ss_e_op_3_joint.png)

One fixed tenon over the middle of the joint line on the wall, and in the floor its mortise, a see-through hole in a lip that the floor's edge extends under the wall around it.

\include{lineno} elements/element_joint_plate_ss_e_op_3.cpp

## ss_e_op_4: chamfered tenons and through mortises

![ss_e_op_4](elements/element_joint_plate_ss_e_op_4.png)
![ss_e_op_4, the joint](elements/element_joint_plate_ss_e_op_4_joint.png)

On its defaults: the geometric division count, one tenon on this 400 seam, chamfered at its end, on the wall, the floor's edge modified over the joint's length and a through mortise per tenon. As drawn today the wall's faces render as a frame around an empty middle, the tenon's chamfer bows and the floor shows two dots on its near side: the design's merge into the mitred outlines is not right yet.

\include{lineno} elements/element_joint_plate_ss_e_op_4.cpp

## ss_e_op_5: linked tenons

![ss_e_op_5](elements/element_joint_plate_ss_e_op_5.png)
![ss_e_op_5, the joint](elements/element_joint_plate_ss_e_op_5_joint.png)

On its defaults and with no linked joint: `ss_e_op_4` with the tenon box lengthened to -0.75 of the unit box, chamfered tenons on the wall and a through mortise per tenon in the floor; on the solver's datasets the design also builds the one or two joints linked to it and the merge sequences that stitch them.

\include{lineno} elements/element_joint_plate_ss_e_op_5.cpp

## ss_e_op_6: the Vidy wall

![ss_e_op_6](elements/element_joint_plate_ss_e_op_6.png)
![ss_e_op_6, the joint](elements/element_joint_plate_ss_e_op_6_joint.png)

`ss_e_op_5` with the divisions of its second linked joint disabled, the Vidy wall version that merges its tenons with one side only; with no linked joint, as here, it is the tenons and mortises of `ss_e_op_5`.

\include{lineno} elements/element_joint_plate_ss_e_op_6.cpp

## ss_e_op_17: fingers with flat mitre caps

![ss_e_op_17](elements/element_joint_plate_ss_e_op_17.png)
![ss_e_op_17, the joint](elements/element_joint_plate_ss_e_op_17_joint.png)

Four divisions, two fingers: `ss_e_op_0` with the finger count parametric and the mitre capped flat at both ends of the joint line. As drawn today both plates render as frames around an empty middle: the design's merge into the mitred outlines is not right yet.

\include{lineno} elements/element_joint_plate_ss_e_op_17.cpp

## ss_e_op_tutorial: one notch

![ss_e_op_tutorial](elements/element_joint_plate_ss_e_op_tutorial.png)
![ss_e_op_tutorial, the joint](elements/element_joint_plate_ss_e_op_tutorial_joint.png)

One rectangular notch over the middle half of the joint line, merged into both mitred edges: the floor's tongue reaches under the wall, the wall notched for it; the worked example of writing a new design.

\include{lineno} elements/element_joint_plate_ss_e_op_tutorial.cpp

## ss_e_op_custom: your own outlines

![ss_e_op_custom](elements/element_joint_plate_ss_e_op_custom.png)
![ss_e_op_custom, the joint](elements/element_joint_plate_ss_e_op_custom_joint.png)

A dovetail tongue on the floor's two faces and its mirror notched out of the wall on its two, given as pairs (face 0, face 1) in the unit box; as the 2024 library kept it, custom outlines carry the fabrication type nothing, so the plates stay uncut and each shows its side of the outlines as a feature on both faces, the box mapped onto the whole contact.

\include{lineno} elements/element_joint_plate_ss_e_op_custom.cpp
