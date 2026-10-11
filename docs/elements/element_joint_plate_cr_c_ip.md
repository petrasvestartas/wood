# JointPlate cr_c_ip {#elements_joint_plate_cr_c_ip}

[TOC]

The cross family: a slot or half-lap where two plates pass through each other, found by `compute_cross_contact`.

Plates take the plain slot and custom outlines; the half-laps are beam joints: [JointBeam cr_c_ip](@ref elements_joint_beam_cr_c_ip).

## Constructors

```cpp
// no divisions; the shift is the family's 0.5, and it narrows the half-lap's centre square in cr_c_ip_1 to cr_c_ip_5,
// 0 the widest and 1 the narrowest; cr_c_ip_2 to cr_c_ip_5 fold their bottom sides onto themselves from the shift 0.85 up
static std::shared_ptr<JointPlate> cr_c_ip_0();                    // the plain slot, a rectangle merged into each plate's outline over half the cross's depth; the family default
static std::shared_ptr<JointPlate> cr_c_ip_1(double shift = 0.5);  // the sliced half-lap, nine rings per plate: the centre and the two top sides milled, the two bottom sides and the four corner wedges sliced
static std::shared_ptr<JointPlate> cr_c_ip_2(double shift = 0.5);  // the milled half-lap, five rings per plate, the bottom sides extended 0.15 along the plate, no drill
static std::shared_ptr<JointPlate> cr_c_ip_3(double shift = 0.5);  // the milled half-lap and two diagonal drills through the lap
static std::shared_ptr<JointPlate> cr_c_ip_4(double shift = 0.5);  // the milled half-lap and one vertical drill down its centre
static std::shared_ptr<JointPlate> cr_c_ip_5(double shift = 0.5);  // the Brussels half-lap: the bottom sides extended 0.27 and shortened 0.075, a vertical 50 mm bit and a horizontal 10 mm bit
static std::shared_ptr<JointPlate> cr_c_ip_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' cross contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows two pairs, `Plate::pair_crossing(90.0)` and `Plate::pair_crossing(60.0)`: two plates crossing square and at 60 degrees, the second lifted to show the slots.

## cr_c_ip_0: the plain slot

![cr_c_ip_0](elements/element_joint_plate_cr_c_ip_0.png)

![cr_c_ip_0, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_0(); // a fixed design, no parameters
```

The family default: a slot over half the cross's depth, from the top of the first plate and the bottom of the second.

\include{lineno} elements/element_joint_plate_cr_c_ip_0.cpp

## cr_c_ip_custom: your own outlines

![cr_c_ip_custom](elements/element_joint_plate_cr_c_ip_custom.png)

![cr_c_ip_custom, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_custom_unit.png)

Your own outlines in the unit box: the male's pairs on the y faces, the female's on the x faces.

\include{lineno} elements/element_joint_plate_cr_c_ip_custom.cpp
