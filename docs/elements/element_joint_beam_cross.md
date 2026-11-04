# Beam Cross {#elements_joint_beam_cross}

[TOC]

Cross-lap joints on two crossing beams (`EdgeStraightElement::pair_crossing(60.0)`) cut as 3D feature volumes by `JointBeam::from_contact` using `settings.joint_parameters[11]`.

## Constructors

```cpp
static std::array<std::shared_ptr<EdgeStraightElement>, 2> EdgeStraightElement::pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const EdgeStraightElement& source, const EdgeStraightElement& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings());
```

Every example shows one pair crossing at `60°` with the lower beam shifted downward out of its lap notch.

## Plain Half-Lap Slot
A straight half-depth rectangular lap slot cut into both crossing beams.

![](elements/element_joint_beam_cross_slot.png)

![](joint_library/cr_cross_slot_unit.png)

\include{lineno} elements/element_joint_beam_cross_slot.cpp

## Sliced Half-Lap
A nine-ring half-lap per beam where the centre and top sides are milled while the bottom sides and corner wedges are sliced.

![](elements/element_joint_beam_cross_sliced_half_lap.png)

![](joint_library/cr_cross_sliced_half_lap_0.5_unit.png)

\include{lineno} elements/element_joint_beam_cross_sliced_half_lap.cpp

## Milled Half-Lap
A five-ring milled half-lap per beam with sheer upper walls and extended lower bearing shelves. It is one joint, `cross_half_lap`, with an option `CrossDrills` saying which drills it carries: none (the default), two diagonal pin bores through both beams, or one pin bore down the crossing axis. The option is stored in the joint id, so `cross_half_lap_id(drills)` gives 32, 33 or 34, the numbers saved datasets use.

### No drills

![](elements/element_joint_beam_cross_half_lap.png)

![](joint_library/cr_cross_half_lap_unit.png)

\include{lineno} elements/element_joint_beam_cross_half_lap.cpp

### Two diagonal drills
`CrossDrills::two_diagonal`: two diagonal pin bores through both beams.

![](elements/element_joint_beam_cross_half_lap_diagonal_drills.png)

![](joint_library/cr_cross_half_lap_diagonal_drills_unit.png)

\include{lineno} elements/element_joint_beam_cross_half_lap_diagonal_drills.cpp


### One drill
`CrossDrills::one`: a single vertical pin bore down the crossing axis.

![](elements/element_joint_beam_cross_half_lap_drill.png)

![](joint_library/cr_cross_half_lap_drill_unit.png)

\include{lineno} elements/element_joint_beam_cross_half_lap_drill.cpp


## Brussels Half-Lap
An asymmetric milled half-lap with a vertical 50 mm counterbore and a horizontal 10 mm cross-pin bore.

![](elements/element_joint_beam_cross_brussels_half_lap.png)

![](joint_library/cr_cross_brussels_half_lap_unit.png)

\include{lineno} elements/element_joint_beam_cross_brussels_half_lap.cpp
