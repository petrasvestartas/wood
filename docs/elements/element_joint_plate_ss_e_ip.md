# JointPlate ss_e_ip {#elements_joint_plate_ss_e_ip}

[TOC]

The side-to-side in-plane family: a design cut into the edges of two plates that meet edge to edge in one plane, so the fingers interlock along the plates' normal and a key, where the design has one, drops into both pockets; the 2024 library's joint type 12, ids 1 to 9 (1 `ss_e_ip_1`, the family default, 2 `ss_e_ip_0`, 3 `ss_e_ip_2`, 4 `ss_e_ip_3`, 5 `ss_e_ip_4`, 6 `ss_e_ip_5`, 8 `side_removal`, 9 `ss_e_ip_custom`), used on its in-plane datasets: the hexshell, the butterflies, the Hilti and the different-directions sets.

## Constructors

```cpp
// a zero division count is geometric: one division every 300 mm of the joint line, the family's 2024 division length;
// the shift is the family's 0.5, square fingers
static std::shared_ptr<JointPlate> ss_e_ip_0();                                       // three fixed fingers, 20 mm deep on each plate
static std::shared_ptr<JointPlate> ss_e_ip_1(int divisions = 0, double shift = 0.5);  // a zigzag of fingers, at least two divisions and even
static std::shared_ptr<JointPlate> ss_e_ip_2(int divisions = 0);                      // a butterfly key per division, scaled to the male plate's thickness
static std::shared_ptr<JointPlate> ss_e_ip_3();                                       // one slanted key groove milled the length of the seam, four drills
static std::shared_ptr<JointPlate> ss_e_ip_4();                                       // two crossed key grooves, four drills
static std::shared_ptr<JointPlate> ss_e_ip_5(int divisions = 0);                      // a reversed eight-point tooth key per division, scaled to the male plate's thickness
static std::shared_ptr<JointPlate> ss_e_ip_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example is the same pair: two 300 x 400 plates 40 thick edge to edge in one plane, the joint oriented on their face contact and passed to each plate with `add_interaction`, the right plate moved off along the seam's normal afterwards, in the plates' plane, so both edges read. The joint's unit box maps x across the seam to the plate thickness (the fingers are 20 deep on each side), y through the thickness and z along the joint line. A design whose male and female runs fill each other (`ss_e_ip_0`, `ss_e_ip_1`) owns no piece and the joint stays hidden; a key design (`ss_e_ip_2`, `ss_e_ip_5`) owns the keys, drawn as the joint's own solids, so the example turns `is_visible` on.

## ss_e_ip_0: three fingers

![ss_e_ip_0](elements/element_joint_plate_ss_e_ip_0.png)

The fixed three-finger zigzag merged into both edges, each plate's fingers in the other's notches; the right plate moved 120 off.

\include{lineno} elements/element_joint_plate_ss_e_ip_0.cpp

## ss_e_ip_1: parametric fingers

![ss_e_ip_1](elements/element_joint_plate_ss_e_ip_1.png)

Eight divisions at the shift 0.5: four square fingers on each plate, `ss_e_ip_0` being this zigzag fixed at six; on its defaults the count is geometric, one division every 300 of the joint line and at least two, which on this 400 seam is a single tongue; the right plate moved 120 off.

\include{lineno} elements/element_joint_plate_ss_e_ip_1.cpp

## ss_e_ip_2: butterfly keys

![ss_e_ip_2](elements/element_joint_plate_ss_e_ip_2.png)

A butterfly pocket into each edge per division, two on this seam, and the joint's own butterfly keys that fill both pockets, drawn half way between them; the right plate moved 160 off, the keys 80.

\include{lineno} elements/element_joint_plate_ss_e_ip_2.cpp

## ss_e_ip_3: milled key groove and drills

![ss_e_ip_3](elements/element_joint_plate_ss_e_ip_3.png)

A slanted key groove milled the length of the seam into each edge, its profile projected from face to face, and a drill through the thickness of each plate, a third of the way across the seam (2024 listed each of the two lines twice); the joint owns no piece, each plate hosts its groove and its bore, an exact cylinder; the right plate moved 200 off.

\include{lineno} elements/element_joint_plate_ss_e_ip_3.cpp

## ss_e_ip_4: two crossed key grooves and drills

![ss_e_ip_4](elements/element_joint_plate_ss_e_ip_4.png)

Two slanted key grooves crossing each other milled into each edge, each projected from face to face, and a drill through the thickness of each plate; its axis lies in the end face of the first groove, as 2024 placed it, so the groove halves the bore and it is drawn faceted, the one bore of the library the exact builder cannot make; the right plate moved 200 off.

\include{lineno} elements/element_joint_plate_ss_e_ip_4.cpp

## ss_e_ip_5: reversed-tooth keys

![ss_e_ip_5](elements/element_joint_plate_ss_e_ip_5.png)

An eight-point tooth pocket into each edge per division, each reversed, two on this seam, and the joint's own keys that fill both pockets, drawn half way between them; the right plate moved 200 off, the keys 100.

\include{lineno} elements/element_joint_plate_ss_e_ip_5.cpp

## ss_e_ip_custom: your own outlines

![ss_e_ip_custom](elements/element_joint_plate_ss_e_ip_custom.png)

A tile you draw yourself: three jigsaw tabs, each a round head of radius 10 on a neck 9 wide, drawn in the unit box as the library's own in-plane designs draw theirs, per face an open profile from one end of the joint line to the other and the seam as a two-point line. The box spans about a plate thickness across the seam (x) and the whole joint line along it (z), so a round head is sized in millimetres against each: 10 / 40 across, 10 / 400 along. Each plate merges its side of the profile into its outline, as the 2024 library merged a custom pair.

\include{lineno} elements/element_joint_plate_ss_e_ip_custom.cpp

## Angles

![ss_e_ip_1 on a seam at 75 degrees](elements/element_joint_plate_ss_e_ip_angle_75.png)

Two plates whose seam runs at 75 degrees to their edges: the fingers of `ss_e_ip_1` keep their shape along the slanted seam. The oracle runs every design of the family on its defaults on that seam and on a seam shorter than both plates (`ip@trapezoid`, `ip@short`), and checks the loose keys of `ss_e_ip_2` and `ss_e_ip_5` rigid: every key on those fixtures has the volume and the edge lengths of a key on the straight seam.

\include{lineno} elements/element_joint_plate_ss_e_ip_angle_75.cpp
