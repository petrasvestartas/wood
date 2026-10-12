# JointPlate cross {#elements_joint_plate_cross}

[TOC]

The cross family: a slot or half-lap where two plates pass through each other, found by `compute_cross_contact`.

Plates take the plain slot and custom outlines; the half-laps are beam joints: [JointBeam cross](@ref elements_joint_beam_cross).

## Constructors

```cpp
// no divisions; the shift is the family's 0.5, and it narrows the half-lap's centre square in cross_sliced_half_lap to cross_brussels_half_lap,
// 0 the widest and 1 the narrowest; cross_half_lap to cross_brussels_half_lap fold their bottom sides onto themselves from the shift 0.85 up
static std::shared_ptr<JointPlate> cross_slot();                    // the plain slot, a rectangle merged into each plate's outline over half the cross's depth; the family default
static std::shared_ptr<JointPlate> cross_sliced_half_lap(double shift = 0.5);  // the sliced half-lap, nine rings per plate: the centre and the two top sides milled, the two bottom sides and the four corner wedges sliced
static std::shared_ptr<JointPlate> cross_half_lap(double shift = 0.5);  // the milled half-lap, five rings per plate, the bottom sides extended 0.15 along the plate, no drill
static std::shared_ptr<JointPlate> cross_half_lap_diagonal_drills(double shift = 0.5);  // the milled half-lap and two diagonal drills through the lap
static std::shared_ptr<JointPlate> cross_half_lap_drill(double shift = 0.5);  // the milled half-lap and one vertical drill down its centre
static std::shared_ptr<JointPlate> cross_brussels_half_lap(double shift = 0.5);  // the Brussels half-lap: the bottom sides extended 0.27 and shortened 0.075, a vertical 50 mm bit and a horizontal 10 mm bit
static std::shared_ptr<JointPlate> cross_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' cross contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows two pairs, `Plate::pair_crossing(90.0)` and `Plate::pair_crossing(60.0)`: two plates crossing square and at 60 degrees, the second moved down out of its slot.

## cross_slot: the plain slot

![cross_slot](elements/element_joint_plate_cross_slot.png)

![cross_slot, the unit box: male pink, female blue](joint_library/cr_cross_slot_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_slot(); // a fixed design, no parameters
```

The family default: a slot over half the cross's depth, from the top of the first plate and the bottom of the second.

\include{lineno} elements/element_joint_plate_cross_slot.cpp

## cross_custom: your own outlines

![cross_custom](elements/element_joint_plate_cross_custom.png)

![cross_custom, the unit box: male pink, female blue](joint_library/cr_cross_custom_unit.png)

Your own outlines in the unit box: the male's pairs on the y faces, the female's on the x faces.

\include{lineno} elements/element_joint_plate_cross_custom.cpp
