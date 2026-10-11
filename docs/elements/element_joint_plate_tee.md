# JointPlate tee {#elements_joint_plate_tee}

[TOC]

The top-to-side family: tenons on the bottom edge of an upright plate and mortises through the base it stands on.

`tee_milled_wedge`, the milled wedge, is a beam joint: [JointBeam tee](@ref elements_joint_beam_tee).

## Constructors

```cpp
// a zero division count is geometric: one division every 450 mm of the joint line, the family's division length;
// the shift is the family's 0.5, the tenon sides straight
static std::shared_ptr<JointPlate> tee_three_tenons();                                       // three fixed tenons
static std::shared_ptr<JointPlate> tee_annen_tenons();                                       // the two fixed tenons of the Annen project
static std::shared_ptr<JointPlate> tee_dense_tenons(int divisions = 0, double shift = 0.5);  // parametric tenons visiting every interpolation point, 2 to 20 divisions made even, the sides leaning by the shift
static std::shared_ptr<JointPlate> tee_tenons(int divisions = 0, double shift = 0.5);  // parametric tenons skipping every other point pair, 8 to 100 divisions made a multiple of four, a tenon per four; the family default
static std::shared_ptr<JointPlate> tee_milled_wedge();                                       // the milled wedge, on beams
static std::shared_ptr<JointPlate> tee_snap_fit(int divisions = 0);                      // the snap-fit tenon, a copy per division; unit scale, its unit z the upright's thickness
static std::shared_ptr<JointPlate> tee_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows `Plate::pair_top_side(90.0)` and `Plate::pair_top_side(60.0)`. The upright is target 0, the male.

## tee_three_tenons: three fixed tenons

![tee_three_tenons](elements/element_joint_plate_tee_three_tenons.png)

![tee_three_tenons, the unit box: male pink, female blue](joint_library/ts_tee_three_tenons_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::tee_three_tenons(); // a fixed design, no parameters
```

Three fixed tenons on the upright's bottom edge and three mortises through the base.

\include{lineno} elements/element_joint_plate_tee_three_tenons.cpp

## tee_annen_tenons: the Annen tenons

![tee_annen_tenons](elements/element_joint_plate_tee_annen_tenons.png)

![tee_annen_tenons, the unit box: male pink, female blue](joint_library/ts_tee_annen_tenons_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::tee_annen_tenons(); // a fixed design, no parameters
```

The two fixed Annen tenons on the upright's bottom edge and two mortises through the base.

\include{lineno} elements/element_joint_plate_tee_annen_tenons.cpp

## tee_dense_tenons: every point visited

![tee_dense_tenons](elements/element_joint_plate_tee_dense_tenons.png)

![tee_dense_tenons, the unit box: male pink, female blue](joint_library/ts_tee_dense_tenons_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::tee_dense_tenons(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the tenon width against the gap
);
```

Parametric tenons at every interpolation point, one on this 250 mm line, with a mortise per tenon.

\include{lineno} elements/element_joint_plate_tee_dense_tenons.cpp

## tee_tenons: a tenon per four divisions

![tee_tenons](elements/element_joint_plate_tee_tenons.png)

![tee_tenons, the unit box: male pink, female blue](joint_library/ts_tee_tenons_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::tee_tenons(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the tenon width against the gap
);
```

The family default: a tenon per four divisions, two on this 250 mm line, with a mortise per tenon.

\include{lineno} elements/element_joint_plate_tee_tenons.cpp

## tee_snap_fit: the snap-fit tenon

![tee_snap_fit](elements/element_joint_plate_tee_snap_fit.png)

![tee_snap_fit, the unit box: male pink, female blue](joint_library/ts_tee_snap_fit_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::tee_snap_fit(
    0  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

The snap-fit tenon, one copy per division, with a mortise per copy.

\include{lineno} elements/element_joint_plate_tee_snap_fit.cpp

## tee_custom: your own outlines

![tee_custom](elements/element_joint_plate_tee_custom.png)

![tee_custom, the unit box: male pink, female blue](joint_library/ts_tee_custom_unit.png)

A custom tapered through tenon and its tapered mortise, so the tenon wedges home from above.

\include{lineno} elements/element_joint_plate_tee_custom.cpp

## Parameters

**Divisions and shift.** Divisions set the number of tenons; the shift sets their width and leans their sides.

![tee_dense_tenons sweep](joint_library/sweep_tee_dense_tenons.png)
![tee_tenons sweep](joint_library/sweep_tee_tenons.png)

**At other angles.** The same tenons on a square, a skewed and a leaning upright at 8, 16 and 24 divisions.

![tee_tenons square, skewed and leaning](joint_library/sweep_tee_tenons_angles.png)

tee_snap_fit, the snap fit: each tenon ends in a hook that snaps under the base.


\image html joint_library/ts_tee_snap_fit.png "tee_snap_fit: snap fit, on the plates" width=70%
