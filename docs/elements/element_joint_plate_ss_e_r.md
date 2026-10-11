# JointPlate ss_e_r {#elements_joint_plate_ss_e_r}

[TOC]

The side-to-side rotated family: a design cut into the side faces of two plates that meet edge to edge, the contact read as rotated when the two alignment lines of the seam cross, before the dihedral test that sorts parallel seams into in-plane and out-of-plane, or when the scene reads every side-to-side contact so (`settings.all_treated_as_rotated`), the joint volumes averaged between the plates under `settings.rotated_joint_as_average`.

## Constructors

```cpp
// a zero division count is geometric: one division every 300 mm of the joint line, the family's 2024 division length;
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

Every example is the same pair, `Plate::pair_rotated(20.0)`: two 300 x 400 blocks 100 thick end to end, the right one twisted 20 degrees about the seam's normal, as the Rossinière hex blocks meet, drawn apart along the seam's normal.

## ss_e_r_0: four slices

![ss_e_r_0](elements/element_joint_plate_ss_e_r_0.png)

The averaged joint volume split in half through the thickness, each half sliced off the plates four ways along the joint line, the slices widened by the joint scale; no key, nothing is oriented, the slices lie in world space, so on this pair they are slivers at the seam corners.

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

A hook pocket milled into each plate per division, two on this seam, each pocket the unit profile projected from one face to the other, and the joint's own key that fills both pockets, drawn half way between the plates.

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

A diamond pocket milled into each plate per division, two on this seam, and the joint's own key that fills both pockets, drawn half way between the plates.

\include{lineno} elements/element_joint_plate_ss_e_r_3.cpp

## ss_e_r_custom: your own outlines

![ss_e_r_custom](elements/element_joint_plate_ss_e_r_custom.png)

Your own outlines, given as pairs (face 0, face 1) in the unit box, x across the seam, y through the thickness, z along the joint line.

\include{lineno} elements/element_joint_plate_ss_e_r_custom.cpp

## side_removal: the side faces milled off

![side_removal](elements/element_joint_plate_ss_e_r_side_removal.png)

Each plate's side face, widened at its convex corners and up and down by the joint scale, milled off along its normal, the female's by the scale plus 2, the male's by the scale, the sides swapped as 2024 swapped them.

\include{lineno} elements/element_joint_plate_ss_e_r_side_removal.cpp

## side_removal_ss_e_r_1: the trait de Jupiter

![side_removal_ss_e_r_1](elements/element_joint_plate_ss_e_r_side_removal_ss_e_r_1.png)

`Plate::pair_in_plane()`: a scarf through the thickness, an about at each end of the joint line, two slopes six thicknesses long and the redan between them, where the key drives through the slot both plates are notched around.

\include{lineno} elements/element_joint_plate_ss_e_r_side_removal_ss_e_r_1.cpp

## Parameters

**Divisions.** One key per division.

![ss_e_r sweep](joint_library/sweep_ss_e_r.png)

## Angles

![ss_e_r_0 at a 40 degree twist](elements/element_joint_plate_ss_e_r_angle_40.png)

Two plates folded 120 degrees on a mitred seam, the scene reading the contact as rotated: the slices of `ss_e_r_0` follow the seam.

\include{lineno} elements/element_joint_plate_ss_e_r_angle_40.cpp
