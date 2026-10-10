# JointPlate ss_e_r {#elements_joint_plate_ss_e_r}

[TOC]

The side-to-side rotated family: a design cut into the side faces of two plates that meet edge to edge, the contact read as rotated when the two alignment lines of the seam cross, before the dihedral test that sorts parallel seams into in-plane and out-of-plane, or when the scene reads every side-to-side contact so (`settings.all_treated_as_rotated`), the joint volumes averaged between the plates under `settings.rotated_joint_as_average`; the 2024 library's joint type 13, ids 50 to 59 (54 `ss_e_r_3`, 55 `ss_e_r_2`, 56 `ss_e_r_0`, 57 `side_removal`, 58 `side_removal` merged with the joint, the family default, 59 `ss_e_r_custom`; an id without an entry takes `side_removal`), used wherever a 2024 side-to-side seam's lines cross, and forced on every seam of the hex block of Rossinière (`data/hex_block_rossiniere.yml`, the merged side removal) and of the Hilti in-plane dataset (`data/inplane_hilti.yml`, `ss_e_r_2`). `ss_e_r_1` is no design of its own: it is the arc tenon tile `side_removal_ss_e_r_1` lays in the side face, with no id and no dispatch in 2024, and alone on a pair its male and female profiles coincide.

## Constructors

```cpp
// a zero division count is geometric: one division every 300 mm of the joint line, the family's 2024 division length;
// the shift is the family's 0.5, a 60 mm tile
static std::shared_ptr<JointPlate> ss_e_r_0();                                       // the averaged volume split through its thickness, the halves sliced off four ways along the joint line
static std::shared_ptr<JointPlate> ss_e_r_2(int divisions = 0, double shift = 0.5);  // a hook pocket into each plate per division, and the key that fills both
static std::shared_ptr<JointPlate> ss_e_r_3(int divisions = 0, double shift = 0.5);  // a diamond pocket into each plate per division, and the key that fills both
static std::shared_ptr<JointPlate> ss_e_r_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5);           // each plate's side face milled off by the joint scale, the male's outlines doubled under merge_with_joint
static std::shared_ptr<JointPlate> side_removal_ss_e_r_1(bool merge_with_joint = false, double shift = 0.5);  // side_removal and, under merge_with_joint, the ss_e_r_1 arc tenon tile at the middle of the joint line

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example is the same pair: two 300 x 400 plates 40 thick folded 120 degrees along their shared edge, their side faces mitred on the bisector, the scene set to read every side-to-side contact as rotated (the seam lines of a fold are parallel, so without the flag the detector would read it out of plane) and to average the joint volumes, the joint oriented on their face contact and passed to each plate with `add_interaction`, the right plate moved 150 along the mitre's normal afterwards, the direction the plates slide together, so both sides read; every page shows the pair and a close-up. The joint's unit box puts the seam across x, the thickness along y and the joint line along z. `ss_e_r_2` and `ss_e_r_3` are key designs: each plate loses a pocket on its side of the seam, a tile 120 * shift square per division stepped along the joint line in that size, and the joint owns the loose key that fills both, cut from the plates it is oriented on, so those examples turn `is_visible` on and move the key half way; every other design cuts the plates only and the joint stays hidden.

## ss_e_r_0: four slices

![ss_e_r_0](elements/element_joint_plate_ss_e_r_0.png)
![ss_e_r_0, the seam](elements/element_joint_plate_ss_e_r_0_joint.png)

The averaged joint volume split in half through the thickness, each half sliced off the plates four ways along the joint line, the slices widened by the joint scale; no key, nothing is oriented, the slices lie in world space, so on this pair they are slivers at the seam corners.

\include{lineno} elements/element_joint_plate_ss_e_r_0.cpp

## ss_e_r_2: hook pockets and key

![ss_e_r_2](elements/element_joint_plate_ss_e_r_2.png)
![ss_e_r_2, the keys](elements/element_joint_plate_ss_e_r_2_joint.png)

A hook pocket milled into each plate per division, two on this seam, each pocket the unit profile projected from one face to the other, and the joint's own key that fills both pockets, drawn half way between the plates.

\include{lineno} elements/element_joint_plate_ss_e_r_2.cpp

## ss_e_r_3: diamond pockets and key

![ss_e_r_3](elements/element_joint_plate_ss_e_r_3.png)
![ss_e_r_3, the keys](elements/element_joint_plate_ss_e_r_3_joint.png)

A diamond pocket milled into each plate per division, two on this seam, and the joint's own key that fills both pockets, drawn half way between the plates.

\include{lineno} elements/element_joint_plate_ss_e_r_3.cpp

## ss_e_r_custom: your own outlines

![ss_e_r_custom](elements/element_joint_plate_ss_e_r_custom.png)
![ss_e_r_custom, the seam](elements/element_joint_plate_ss_e_r_custom_joint.png)

Your own outlines, given as pairs (face 0, face 1) in the unit box, x across the seam, y through the thickness, z along the joint line; as the 2024 library kept a custom pair, they carry the fabrication type nothing and only a closed rectangle of five points, or a line of two, is merged into the plate, every other outline passing through uncut and shown as a feature: here a rectangle on each face of the male plate cuts a notch half a thickness deep into its mitred edge over one stretch of the joint line, and one on each face of the female cuts the same notch into hers over another.

\include{lineno} elements/element_joint_plate_ss_e_r_custom.cpp

## side_removal: the side faces milled off

![side_removal](elements/element_joint_plate_ss_e_r_side_removal.png)
![side_removal, the seam](elements/element_joint_plate_ss_e_r_side_removal_joint.png)

Each plate's side face, widened at its convex corners and up and down by the joint scale, milled off along its normal, the female's by the scale plus 2, the male's by the scale, the sides swapped as 2024 swapped them; the session cuts each removal as a solid, where 2024 clipped the rectangle into the outline, and at the default scale the removals are slivers along the seam.

\include{lineno} elements/element_joint_plate_ss_e_r_side_removal.cpp

## side_removal_ss_e_r_1: the arc tenon tile

![side_removal_ss_e_r_1](elements/element_joint_plate_ss_e_r_side_removal_ss_e_r_1.png)
![side_removal_ss_e_r_1, the seam](elements/element_joint_plate_ss_e_r_side_removal_ss_e_r_1_joint.png)

Under `merge_with_joint`, the side removal outlines of both faces and the `ss_e_r_1` arc tile oriented on two 20 x 20 rectangles at the middle of the joint line, offset by the conic allowance, cut out of the male's third outline and appended as conic, mill and reverse conic cuts; kept as 2024 wrote it, the merged form swaps the sides last and hands each plate its own side slab outside its stock, so no side is removed and only the tile's conic slivers cut. 2024 named it as id 58 but dispatched `side_removal(true)` for it, so no 2025 reference dataset reaches it; it is the 2024 function, not a design the solver chooses.

\include{lineno} elements/element_joint_plate_ss_e_r_side_removal_ss_e_r_1.cpp
