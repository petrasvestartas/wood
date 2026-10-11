# JointPlate ts_e_p {#elements_joint_plate_ts_e_p}

[TOC]

The top-to-side family: a design cut where a plate stands on the face of another, tenons on the bottom edge of the standing plate, the upright, and mortises through the plate it stands on, the base.

On a beam ending on another: [JointBeam ts_e_p](@ref elements_joint_beam_ts_e_p).

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

Every example joins two pairs, `Plate::pair_top_side(90.0)` and `Plate::pair_top_side(60.0)`: a 400 x 400 base and a 250 x 250 upright on the middle of its top, both 40 thick, upright and leaning to 60 degrees, the joint oriented on their face contact and passed to each plate with `add_interaction`, the upright first (target 0, the male of a top-side pair) and the base second, the upright lifted afterwards along the line between the plates' centres.

## ts_e_p_0: three fixed tenons

![ts_e_p_0](elements/element_joint_plate_ts_e_p_0.png)

![ts_e_p_0, the unit box: male pink, female blue](joint_library/ts_ts_e_p_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_0(); // a fixed design, no parameters
```

Three tenons on the upright's bottom edge over the middle five sevenths of the joint line, each the base's thickness deep, and three mortises through the base, 2024's literals with no parameters.

\include{lineno} elements/element_joint_plate_ts_e_p_0.cpp

## ts_e_p_1: the Annen tenons

![ts_e_p_1](elements/element_joint_plate_ts_e_p_1.png)

![ts_e_p_1, the unit box: male pink, female blue](joint_library/ts_ts_e_p_1_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_1(); // a fixed design, no parameters
```

The two fixed tenons of the Annen project on the upright's bottom edge, one run with the full-height edge marker, each the base's thickness deep, and two mortises through the base, 2024's literals with no parameters.

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

The parametric tenons that visit every interpolation point of the joint line, the division count geometric, made even and kept between 2 and 20, so one tenon on this 250 mm line, the sides straight at shift 0.5, and a mortise through the base per tenon.

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

The family default: the parametric tenons that skip every other point pair, a tenon per four divisions, the division count geometric, made a multiple of four and kept between 8 and 100, so two tenons on this 250 mm line, the sides straight at shift 0.5, and a mortise through the base per tenon.

\include{lineno} elements/element_joint_plate_ts_e_p_3.cpp

## ts_e_p_4: the milled wedge

![ts_e_p_4](elements/element_joint_plate_ts_e_p_4.png)

![ts_e_p_4, the unit box: male pink, female blue](joint_library/ts_ts_e_p_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_4(); // a fixed design, no parameters
```

The fixed wedge design 2024 kept as 240 literal points: four mill pockets through the base, and on the upright two wedge flanks, two walls and two caps milled along its faces and two slices, every outline a solid taken from its plate's stock, written twice as 2024 doubled them.

\include{lineno} elements/element_joint_plate_ts_e_p_4.cpp

## ts_e_p_5: the snap-fit tenon

![ts_e_p_5](elements/element_joint_plate_ts_e_p_5.png)

![ts_e_p_5, the unit box: male pink, female blue](joint_library/ts_ts_e_p_5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_5(
    0  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

The snap-fit tenon, 2024's literals, a copy per division spread along the joint line, one here, the copies run into one outline on the upright's bottom edge and a mortise through the base per copy.

\include{lineno} elements/element_joint_plate_ts_e_p_5.cpp

## ts_e_p_custom: your own outlines

![ts_e_p_custom](elements/element_joint_plate_ts_e_p_custom.png)

![ts_e_p_custom, the unit box: male pink, female blue](joint_library/ts_ts_e_p_custom_unit.png)

One tapered through tenon you write yourself: per upright face the tenon profile, half the joint line wide at the base's top and 0.3 at its bottom, and its end line, cut into its edge; per base face the mortise of the same widths, cut as a tapered hole, so the tenon wedges home from above.

\include{lineno} elements/element_joint_plate_ts_e_p_custom.cpp

## Parameters

**Divisions and shift.** Divisions set the number of tenons (two of every four divisions are tenons). The shift sets the tenon width against the gap and leans the tenon sides.

![ts_e_p_2 sweep](joint_library/sweep_ts_e_p_2.png)
![ts_e_p_3 sweep](joint_library/sweep_ts_e_p_3.png)

**At other angles.** The same tenons on an upright square to the base, skewed 75 degrees in plan and leaning 80 degrees, at 8, 16 and 24 divisions (the design rounds a count up to a multiple of four, at least eight, a tenon every four): the mortises follow the upright's footprint.

![ts_e_p_3 square, skewed and leaning](joint_library/sweep_ts_e_p_3_angles.png)

ts_e_p_5, the snap fit: each tenon ends in a hook that snaps under the base. The thesis names it only in the text (p.86, p.91). The oracle does not check it, because 2024's literals put its hook through the base by design; the datasets top_to_side_box and top_to_side_snap_fit prove it against the 2025 reference.


\image html joint_library/ts_ts_e_p_5.png "ts_e_p_5: snap fit, on the plates" width=70%
