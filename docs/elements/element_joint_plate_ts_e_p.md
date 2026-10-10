# JointPlate ts_e_p {#elements_joint_plate_ts_e_p}

[TOC]

The top-to-side family: a design cut where a plate stands on the face of another, tenons on the bottom edge of the standing plate, the upright, and mortises through the plate it stands on, the base; the 2024 library's contact type 20, ids 20 to 29 (20 `ts_e_p_3`, the family default id, 21 `ts_e_p_2`, 22 `ts_e_p_3`, 23 and 24 `ts_e_p_4`, 2024's case 23 ran `ts_e_p_0` and fell through into `ts_e_p_4`, 25 `ts_e_p_5`, 28 `side_removal`, 29 `ts_e_p_custom`; an id without an entry takes `ts_e_p_3`; `ts_e_p_0` and `ts_e_p_1` are reached by name only), used on the top-to-side datasets: the corners and the test (`data/top_to_side_corners.yml`, `data/top_to_side_test.yml`, id 20, `ts_e_p_3`), the box and the snap fit (`data/top_to_side_box.yml`, `data/top_to_side_snap_fit.yml`, id 25, `ts_e_p_5`).

## Constructors

```cpp
// a zero division count is geometric: one division every 450 mm of the joint line, the family's 2024 division length;
// the shift is the family's 0.5, the tenon sides straight
static std::shared_ptr<JointPlate> ts_e_p_0();                                       // three fixed tenons, 2024's literals
static std::shared_ptr<JointPlate> ts_e_p_1();                                       // the two fixed tenons of the Annen project, 2024's literals
static std::shared_ptr<JointPlate> ts_e_p_2(int divisions = 0, double shift = 0.5);  // parametric tenons visiting every interpolation point, 2 to 20 divisions made even, the sides leaning by the shift
static std::shared_ptr<JointPlate> ts_e_p_3(int divisions = 0, double shift = 0.5);  // parametric tenons skipping every other point pair, 8 to 100 divisions made a multiple of four, a tenon per four; the family default
static std::shared_ptr<JointPlate> ts_e_p_4();                                       // the milled wedge, 2024's literals: four pockets in the base, flanks, walls, caps and slices on the upright
static std::shared_ptr<JointPlate> ts_e_p_5(int divisions = 0);                      // the snap-fit tenon, 2024's literals, a copy per division; unit scale, its unit z the upright's thickness
static std::shared_ptr<JointPlate> ts_e_p_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example is the same pair: a 400 x 400 base plate and a 250 x 250 upright standing in the middle of its top face, both 40 thick, the joint oriented on their face contact and passed to each plate with `add_interaction`, the upright first (target 0, the male of a top-side pair, whichever plate the contact listed first) and the base second (target 1, the female), the upright lifted 120 along the base's normal afterwards, the direction it drops onto the base, so the tenons and the mortises both read; every design shows the pair and a close-up fitted on the upright. The joint's unit box puts the upright's thickness along x, the base's along y and the joint line along z; the tenons are merged into the upright's outline as edge insertions, the mortises are holes through the base, a loop on each of its faces, and the last female outline, the rectangle that bounds the mortises, is dropped by the merge as 2024 dropped it. No design of the family owns a piece: the joint stays hidden and only the plates are cut; `ts_e_p_4` leaves pockets for a loose wedge 2024 never modelled.

## ts_e_p_0: three fixed tenons

![ts_e_p_0](elements/element_joint_plate_ts_e_p_0.png)
![ts_e_p_0, the upright](elements/element_joint_plate_ts_e_p_0_joint.png)

Three tenons on the upright's bottom edge over the middle five sevenths of the joint line, each the base's thickness deep, and three mortises through the base, 2024's literals with no parameters.

\include{lineno} elements/element_joint_plate_ts_e_p_0.cpp

## ts_e_p_1: the Annen tenons

![ts_e_p_1](elements/element_joint_plate_ts_e_p_1.png)
![ts_e_p_1, the upright](elements/element_joint_plate_ts_e_p_1_joint.png)

The two fixed tenons of the Annen project on the upright's bottom edge, one run with the full-height edge marker, each the base's thickness deep, and two mortises through the base, 2024's literals with no parameters.

\include{lineno} elements/element_joint_plate_ts_e_p_1.cpp

## ts_e_p_2: every point visited

![ts_e_p_2](elements/element_joint_plate_ts_e_p_2.png)
![ts_e_p_2, the upright](elements/element_joint_plate_ts_e_p_2_joint.png)

The parametric tenons that visit every interpolation point of the joint line, the division count geometric, made even and kept between 2 and 20, so one tenon on this 250 mm line, the sides straight at shift 0.5, and a mortise through the base per tenon.

\include{lineno} elements/element_joint_plate_ts_e_p_2.cpp

## ts_e_p_3: a tenon per four divisions

![ts_e_p_3](elements/element_joint_plate_ts_e_p_3.png)
![ts_e_p_3, the upright](elements/element_joint_plate_ts_e_p_3_joint.png)

The family default: the parametric tenons that skip every other point pair, a tenon per four divisions, the division count geometric, made a multiple of four and kept between 8 and 100, so two tenons on this 250 mm line, the sides straight at shift 0.5, and a mortise through the base per tenon; `ts_e_p_3` six times with other divisions and shifts is on the [JointPlate page](@ref elements_joint_plate).

\include{lineno} elements/element_joint_plate_ts_e_p_3.cpp

## ts_e_p_4: the milled wedge

![ts_e_p_4](elements/element_joint_plate_ts_e_p_4.png)
![ts_e_p_4, the upright](elements/element_joint_plate_ts_e_p_4_joint.png)

The fixed wedge design 2024 kept as 240 literal points: four mill pockets through the base, and on the upright two wedge flanks, two walls and two caps milled along its faces and two slices, every outline a solid taken from its plate's stock, written twice as 2024 doubled them; the pockets are left for a loose wedge 2024 never modelled. The ids 23 and 24.

\include{lineno} elements/element_joint_plate_ts_e_p_4.cpp

## ts_e_p_5: the snap-fit tenon

![ts_e_p_5](elements/element_joint_plate_ts_e_p_5.png)
![ts_e_p_5, the upright](elements/element_joint_plate_ts_e_p_5_joint.png)

The snap-fit tenon, 2024's literals, a copy per division spread along the joint line, one here, the copies run into one outline on the upright's bottom edge and a mortise through the base per copy; the design is unit scale, its unit z the upright's thickness times the joint scale instead of the joint line, so the tenon keeps the upright's thickness and its hook runs through the base and out below it as 2024 drew it. The id 25, the box and the snap-fit datasets.

\include{lineno} elements/element_joint_plate_ts_e_p_5.cpp

## ts_e_p_custom: your own outlines

![ts_e_p_custom](elements/element_joint_plate_ts_e_p_custom.png)
![ts_e_p_custom, the upright](elements/element_joint_plate_ts_e_p_custom_joint.png)

Your own outlines in the unit box, pairs (face 0, face 1) per side, kept pair by pair as the 2024 library kept a custom design: the outlines carry the fabrication type nothing, and only a closed rectangle of five points, or a line of two, is merged into the plate's edge; here a rectangle on each face of the upright cuts a notch into its bottom edge, and the rectangles on the base's faces, where 2024 merged nothing, stay features and cut nothing.

\include{lineno} elements/element_joint_plate_ts_e_p_custom.cpp

## Angles

![ts_e_p_3 skewed 60 degrees](elements/element_joint_plate_ts_e_p_angle_60.png)

The upright's foot at 60 degrees to the base's edges in plan: the tenons and mortises of `ts_e_p_3` follow the skewed contact. The oracle runs every design of the family on its defaults skewed 60 and 75 degrees and leaning 80 degrees from the base (`ts@skew60`, `ts@skew75`, `ts@lean80`), the foot kept flat on the base.

\include{lineno} elements/element_joint_plate_ts_e_p_angle_60.cpp
