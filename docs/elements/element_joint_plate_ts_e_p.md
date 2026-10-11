# JointPlate ts_e_p {#elements_joint_plate_ts_e_p}

[TOC]

The top-to-side family: tenons on the bottom edge of an upright plate and mortises through the base it stands on.

`ts_e_p_4`, the milled wedge, is a beam joint: [JointBeam ts_e_p](@ref elements_joint_beam_ts_e_p).

## Constructors

```cpp
// a zero division count is geometric: one division every 450 mm of the joint line, the family's division length;
// the shift is the family's 0.5, the tenon sides straight
static std::shared_ptr<JointPlate> ts_e_p_0();                                       // three fixed tenons
static std::shared_ptr<JointPlate> ts_e_p_1();                                       // the two fixed tenons of the Annen project
static std::shared_ptr<JointPlate> ts_e_p_2(int divisions = 0, double shift = 0.5);  // parametric tenons visiting every interpolation point, 2 to 20 divisions made even, the sides leaning by the shift
static std::shared_ptr<JointPlate> ts_e_p_3(int divisions = 0, double shift = 0.5);  // parametric tenons skipping every other point pair, 8 to 100 divisions made a multiple of four, a tenon per four; the family default
static std::shared_ptr<JointPlate> ts_e_p_4();                                       // the milled wedge, on beams
static std::shared_ptr<JointPlate> ts_e_p_5(int divisions = 0);                      // the snap-fit tenon, a copy per division; unit scale, its unit z the upright's thickness
static std::shared_ptr<JointPlate> ts_e_p_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows `Plate::pair_top_side(90.0)` and `Plate::pair_top_side(60.0)`. The upright is target 0, the male.

## ts_e_p_0: three fixed tenons

![ts_e_p_0](elements/element_joint_plate_ts_e_p_0.png)

![ts_e_p_0, the unit box: male pink, female blue](joint_library/ts_ts_e_p_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_0(); // a fixed design, no parameters
```

Three fixed tenons on the upright's bottom edge and three mortises through the base.

\include{lineno} elements/element_joint_plate_ts_e_p_0.cpp

## ts_e_p_1: the Annen tenons

![ts_e_p_1](elements/element_joint_plate_ts_e_p_1.png)

![ts_e_p_1, the unit box: male pink, female blue](joint_library/ts_ts_e_p_1_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_1(); // a fixed design, no parameters
```

The two fixed Annen tenons on the upright's bottom edge and two mortises through the base.

\include{lineno} elements/element_joint_plate_ts_e_p_1.cpp

## ts_e_p_2: every point visited

![ts_e_p_2](elements/element_joint_plate_ts_e_p_2.png)

![ts_e_p_2, the unit box: male pink, female blue](joint_library/ts_ts_e_p_2_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_2(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the tenon width against the gap
);
```

Parametric tenons at every interpolation point, one on this 250 mm line, with a mortise per tenon.

\include{lineno} elements/element_joint_plate_ts_e_p_2.cpp

## ts_e_p_3: a tenon per four divisions

![ts_e_p_3](elements/element_joint_plate_ts_e_p_3.png)

![ts_e_p_3, the unit box: male pink, female blue](joint_library/ts_ts_e_p_3_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_3(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the tenon width against the gap
);
```

The family default: a tenon per four divisions, two on this 250 mm line, with a mortise per tenon.

\include{lineno} elements/element_joint_plate_ts_e_p_3.cpp

## ts_e_p_5: the snap-fit tenon

![ts_e_p_5](elements/element_joint_plate_ts_e_p_5.png)

![ts_e_p_5, the unit box: male pink, female blue](joint_library/ts_ts_e_p_5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_5(
    0  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

The snap-fit tenon, one copy per division, with a mortise per copy.

\include{lineno} elements/element_joint_plate_ts_e_p_5.cpp

## ts_e_p_custom: your own outlines

![ts_e_p_custom](elements/element_joint_plate_ts_e_p_custom.png)

![ts_e_p_custom, the unit box: male pink, female blue](joint_library/ts_ts_e_p_custom_unit.png)

A custom tapered through tenon and its tapered mortise, so the tenon wedges home from above.

\include{lineno} elements/element_joint_plate_ts_e_p_custom.cpp

## Parameters

**Divisions and shift.** Divisions set the number of tenons; the shift sets their width and leans their sides.

![ts_e_p_2 sweep](joint_library/sweep_ts_e_p_2.png)
![ts_e_p_3 sweep](joint_library/sweep_ts_e_p_3.png)

**At other angles.** The same tenons on a square, a skewed and a leaning upright at 8, 16 and 24 divisions.

![ts_e_p_3 square, skewed and leaning](joint_library/sweep_ts_e_p_3_angles.png)

ts_e_p_5, the snap fit: each tenon ends in a hook that snaps under the base.


\image html joint_library/ts_ts_e_p_5.png "ts_e_p_5: snap fit, on the plates" width=70%
