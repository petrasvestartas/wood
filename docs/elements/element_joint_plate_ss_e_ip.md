# JointPlate ss_e_ip {#elements_joint_plate_ss_e_ip}

[TOC]

The side-to-side in-plane family: fingers or key pockets cut into the edges of two plates meeting in one plane.

`ss_e_ip_3` and `ss_e_ip_4` are beam joints: [JointBeam end to end](@ref elements_joint_beam_end_to_end).

## Constructors

```cpp
// a zero division count is geometric: one division every 300 mm of the joint line, the family's division length;
// the shift is the family's 0.5, square fingers
static std::shared_ptr<JointPlate> ss_e_ip_0();                                       // three fixed fingers, 20 mm deep on each plate
static std::shared_ptr<JointPlate> ss_e_ip_1(int divisions = 0, double shift = 0.5);  // a zigzag of fingers, at least two divisions and even
static std::shared_ptr<JointPlate> ss_e_ip_2(int divisions = 0);                      // a butterfly key per division, scaled to the male plate's thickness
static std::shared_ptr<JointPlate> ss_e_ip_3();                                       // a milled key groove and drills, on beams end to end
static std::shared_ptr<JointPlate> ss_e_ip_4();                                       // two crossed key grooves and drills, on beams end to end
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

Every example shows two pairs, `Plate::pair_in_plane(180.0)` and `Plate::pair_in_plane(165.0)`: two plates edge to edge, in line and in a shallow V, drawn apart.

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

Eight divisions at shift 0.5: four square fingers on each plate; the right plate moved 120 off.

\include{lineno} elements/element_joint_plate_ss_e_ip_1.cpp

## ss_e_ip_2: butterfly keys

![ss_e_ip_2](elements/element_joint_plate_ss_e_ip_2.png)

![ss_e_ip_2, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_2_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_2(
    4  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

A butterfly pocket per division in each edge and the keys that fill them; the right plate moved 160 off, the keys 80.

\include{lineno} elements/element_joint_plate_ss_e_ip_2.cpp

## ss_e_ip_5: reversed-tooth keys

![ss_e_ip_5](elements/element_joint_plate_ss_e_ip_5.png)

![ss_e_ip_5, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_5_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_5(
    4  // divisions: how many teeth along the joint line; 0 takes them from its length
);
```

A reversed eight-point tooth pocket per division and the keys that fill them; the right plate moved 200 off, the keys 100.

\include{lineno} elements/element_joint_plate_ss_e_ip_5.cpp

## ss_e_ip_custom: your own outlines

![ss_e_ip_custom](elements/element_joint_plate_ss_e_ip_custom.png)

![ss_e_ip_custom, the unit box: male pink, female blue](joint_library/ip_ss_e_ip_custom_unit.png)

A custom feather joint: eight sharp V teeth alternating across the seam, merged into both edges.

\include{lineno} elements/element_joint_plate_ss_e_ip_custom.cpp

## ss_e_ip_hilti: the Hilti connector

![the reference: 200 mm CLT folded 0 to 50 degrees, and the connector's parts](hilti_reference_photo.jpg)

![ss_e_ip_hilti](elements/element_joint_plate_ss_e_ip_hilti.png)

The Hilti joint: a bow-tie pocket milled into each slab and its key of plywood halves, steel discs and bolt; slabs folded 30 degrees.

\include{lineno} elements/element_joint_plate_ss_e_ip_hilti.cpp

## ss_e_ip_hilti at six fold angles

![ss_e_ip_hilti at six angles](elements/element_joint_plate_ss_e_ip_hilti_angles.png)

The same pair folded 0 to 50 degrees: the key stays the same, the pockets follow the slabs.

\include{lineno} elements/element_joint_plate_ss_e_ip_hilti_angles.cpp

## Parameters

**Divisions and shift.** Divisions set the number of dovetails; the shift leans them: straight at 0.5, dovetailed one way at 0 and the other at 1.

![ss_e_ip_1 sweep](joint_library/sweep_ss_e_ip_1.png)

**On other seams.** The same dovetails on a square, a 75 degree and a short seam, counted along the seam.

![ss_e_ip_1 on straight, slanted and short seams](joint_library/sweep_ss_e_ip_1_seams.png)

ss_e_ip_2 and ss_e_ip_5 cut butterfly pockets across the seam, one per division; the key is not modelled.

![ss_e_ip_2 and ss_e_ip_5 sweep](joint_library/sweep_ss_e_ip_2_5.png)
