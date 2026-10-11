# JointPlate ss_e_ip {#elements_joint_plate_ss_e_ip}

[TOC]

The side-to-side in-plane family: a design cut into the edges of two plates that meet edge to edge in one plane, so the fingers interlock along the plates' normal and a key, where the design has one, drops into both pockets.

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

// the Hilti connector on the face contact of two slabs side to side, their seam faces mitred so the contact is the seam; null on any other contact
static std::shared_ptr<JointPlate> ss_e_ip_hilti(
    const Plate& a,
    const Plate& b,
    const InteractionContactFace& contact,
    double half_length = 120.0,     // the seam to the pocket's end, half the 240 mm cutout
    double neck_length = 27.7,      // the neck across the seam, half in each pocket
    double neck_width = 40.0,       // the neck
    double taper_end = 74.4,        // the seam to where the wing reaches its width
    double wing_width = 90.0,       // the wing, the cutout's width
    double depth = 93.0,            // the pocket, from the top down
    double height = 90.0,           // the key, its top depth - height under the top
    double rod_diameter = 12.0,     // the bolt
    double disc_diameter = 50.0,    // the steel disc under each nut, recessed in its wing's end
    double disc_thickness = 6.0,
    double router_radius = 20.0,    // the far corners of each pocket, a 40 mm router
    int sides = 32                  // the discs', the rounded corners' and the bolt's polygon
);

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example joins two pairs, `Plate::pair_in_plane(180.0)` and `Plate::pair_in_plane(150.0)`: two 300 x 400 plates 40 thick edge to edge in one plane, in line and bent to 150 degrees on a mitred seam, the joint oriented on their face contact and passed to each plate with `add_interaction`, the right plate moved off afterwards along the line between the plates' centres.

## ss_e_ip_0: three fingers

![ss_e_ip_0](elements/element_joint_plate_ss_e_ip_0.png)

![ss_e_ip_0, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_0(); // a fixed design, no parameters
```

The fixed three-finger zigzag merged into both edges, each plate's fingers in the other's notches; the right plate moved 120 off.

\include{lineno} elements/element_joint_plate_ss_e_ip_0.cpp

## ss_e_ip_1: parametric fingers

![ss_e_ip_1](elements/element_joint_plate_ss_e_ip_1.png)

![ss_e_ip_1, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_1_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_1(
    8,   // divisions: how many teeth along the joint line; 0 takes them from its length
    0.5  // shift: the dovetail lean: straight at 0.5, one way at 0, the other at 1
);
```

Eight divisions at the shift 0.5: four square fingers on each plate, `ss_e_ip_0` being this zigzag fixed at six; on its defaults the count is geometric, one division every 300 of the joint line and at least two, which on this 400 seam is a single tongue; the right plate moved 120 off.

\include{lineno} elements/element_joint_plate_ss_e_ip_1.cpp

## ss_e_ip_2: butterfly keys

![ss_e_ip_2](elements/element_joint_plate_ss_e_ip_2.png)

![ss_e_ip_2, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_2_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_2(
    4  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

A butterfly pocket into each edge per division, two on this seam, and the joint's own butterfly keys that fill both pockets, drawn half way between them; the right plate moved 160 off, the keys 80.

\include{lineno} elements/element_joint_plate_ss_e_ip_2.cpp

## ss_e_ip_3: milled key groove and drills

![ss_e_ip_3](elements/element_joint_plate_ss_e_ip_3.png)

![ss_e_ip_3, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_3_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_3(); // a fixed design, no parameters
```

A slanted key groove milled the length of the seam into each edge, its profile projected from face to face, and a drill through the thickness of each plate, a third of the way across the seam (2024 listed each of the two lines twice).

\include{lineno} elements/element_joint_plate_ss_e_ip_3.cpp

## ss_e_ip_4: two crossed key grooves and drills

![ss_e_ip_4](elements/element_joint_plate_ss_e_ip_4.png)

![ss_e_ip_4, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_4(); // a fixed design, no parameters
```

Two slanted key grooves crossing each other milled into each edge, each projected from face to face, and a drill through the thickness of each plate.

\include{lineno} elements/element_joint_plate_ss_e_ip_4.cpp

## ss_e_ip_5: reversed-tooth keys

![ss_e_ip_5](elements/element_joint_plate_ss_e_ip_5.png)

![ss_e_ip_5, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_5_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_5(
    4  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

An eight-point tooth pocket into each edge per division, each reversed, two on this seam, and the joint's own keys that fill both pockets, drawn half way between them; the right plate moved 200 off, the keys 100.

\include{lineno} elements/element_joint_plate_ss_e_ip_5.cpp

## ss_e_ip_custom: your own outlines

![ss_e_ip_custom](elements/element_joint_plate_ss_e_ip_custom.png)

![ss_e_ip_custom, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_custom_unit.png)

A feather joint you write yourself: eight sharp V teeth alternating across the seam, per face an open profile and its end line, as `ss_e_ip_0` is written; both plates merge their side of it into the edge.

\include{lineno} elements/element_joint_plate_ss_e_ip_custom.cpp

## ss_e_ip_hilti: the Hilti connector

![the reference: 200 mm CLT folded 0 to 50 degrees, and the connector's parts](hilti_reference_photo.jpg)

![ss_e_ip_hilti](elements/element_joint_plate_ss_e_ip_hilti.png)

The 2024 Hilti joint with its key, only on two slabs side to side: each slab, `interaction(i)`, is milled its half of the bow-tie pocket from the top, the neck across the seam widening into the wing, the far corners rounded by the router; the key, two plywood halves, two steel discs and the bolt, is the joint's own. Two 600 x 400 slabs 200 thick folded 30 degrees, the right one moved 300 off.

\include{lineno} elements/element_joint_plate_ss_e_ip_hilti.cpp

## ss_e_ip_hilti at six fold angles

![ss_e_ip_hilti at six angles](elements/element_joint_plate_ss_e_ip_hilti_angles.png)

The same pair folded 0 to 50 degrees, one row per angle as in the test series: the key is the same solid in every row, the pockets follow the slabs, so the wings stand out of the tops as the fold grows.

\include{lineno} elements/element_joint_plate_ss_e_ip_hilti_angles.cpp

## Parameters

**Divisions and shift.** Divisions set the number of dovetails; the shift leans them: straight at 0.5, dovetailed one way at 0 and the other at 1.

![ss_e_ip_1 sweep](joint_library/sweep_ss_e_ip_1.png)

**On other seams.** The same dovetails on a seam square to the plates, on a seam slanted to 75 degrees and on a seam shorter than both plates: the divisions are counted along the seam itself.

![ss_e_ip_1 on straight, slanted and short seams](joint_library/sweep_ss_e_ip_1_seams.png)

ss_e_ip_2 and ss_e_ip_5 cut butterfly pockets across the seam, one per division. The butterfly key itself is not modelled as a piece, as in 2024.

![ss_e_ip_2 and ss_e_ip_5 sweep](joint_library/sweep_ss_e_ip_2_5.png)
