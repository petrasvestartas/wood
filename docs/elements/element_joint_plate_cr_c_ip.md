# JointPlate cr_c_ip {#elements_joint_plate_cr_c_ip}

[TOC]

The cross family: a design cut where two plates pass through each other in their planes, a slot or a half-lap into each over its share of the cross's depth, found by `compute_cross_contact`.

The plates take the plain slot and your own outlines; the half-laps `cr_c_ip_1` to `cr_c_ip_5` are beam joints, shown on two crossing beams: [JointBeam cr_c_ip](@ref elements_joint_beam_cr_c_ip).

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

Every example joins two pairs, `Plate::pair_crossing(90.0)` and `Plate::pair_crossing(60.0)`: two upright 400 x 200 plates 40 thick crossing at their middles, square and at 60 degrees, the joint oriented on their cross contact and passed to each plate with `add_interaction`, the second plate lifted 250 afterwards, the direction the plates slide into each other.

## cr_c_ip_0: the plain slot

![cr_c_ip_0](elements/element_joint_plate_cr_c_ip_0.png)

![cr_c_ip_0, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_0(); // a fixed design, no parameters
```

The family default: a rectangle merged into each plate's outline over half the cross's depth, a slot from the top of the first plate and from the bottom of the second, clipped into the outline by Clipper2 at two decimals as 2024 clipped it, which leaves the slot on a 1/128 mm grid in the frame of the face.

\include{lineno} elements/element_joint_plate_cr_c_ip_0.cpp

## cr_c_ip_custom: your own outlines

![cr_c_ip_custom](elements/element_joint_plate_cr_c_ip_custom.png)

![cr_c_ip_custom, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_custom_unit.png)

Your own outlines in the unit box, pairs (face 0, face 1) per side, the male's on the faces at y = -0.5 and 0.5, the first plate's on this pair, and the female's on the faces at x = -0.5 and 0.5, the second's, kept pair by pair as the 2024 library kept a custom design: each pair is inserted between the plate's edges as `cr_c_ip_0` is.

\include{lineno} elements/element_joint_plate_cr_c_ip_custom.cpp
