# JointBeam cross {#elements_joint_beam_cross}

[TOC]

Crossing joints on two beams that cross: the plate library's cross designs, chosen by `settings.joint_parameters[11]`, cut into both beams as solids by `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())
```

Every example shows two pairs, `Beam::pair_crossing(90.0)` and `Beam::pair_crossing(60.0)`: two beams crossing square and at 60 degrees, the second moved down out of its notch.

## cross_slot: the plain slot

![cross_slot](elements/element_joint_beam_cross_slot.png)

![cross_slot, the unit box: male pink, female blue](joint_library/cr_cross_slot_unit.png)

\include{lineno} elements/element_joint_beam_cross_slot.cpp

## cross_sliced_half_lap: the sliced half-lap

![cross_sliced_half_lap](elements/element_joint_beam_cross_sliced_half_lap.png)

![cross_sliced_half_lap, the unit box: male pink, female blue](joint_library/cr_cross_sliced_half_lap_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_sliced_half_lap(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

A half-lap of nine rings per beam: the centre and top sides milled, the bottom sides and corner wedges sliced.

\include{lineno} elements/element_joint_beam_cross_sliced_half_lap.cpp

## cross_half_lap: the milled half-lap

![cross_half_lap](elements/element_joint_beam_cross_half_lap.png)

![cross_half_lap, the unit box: male pink, female blue](joint_library/cr_cross_half_lap_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_half_lap(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

A half-lap of five rings: the centre square and bottom sides milled, the top sides as sheer walls.

\include{lineno} elements/element_joint_beam_cross_half_lap.cpp

## cross_half_lap_diagonal_drills: two diagonal drills

![cross_half_lap_diagonal_drills](elements/element_joint_beam_cross_half_lap_diagonal_drills.png)

![cross_half_lap_diagonal_drills, the unit box: male pink, female blue](joint_library/cr_cross_half_lap_diagonal_drills_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_half_lap_diagonal_drills(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The milled half-lap of `cross_half_lap` with two diagonal drills through both beams.

\include{lineno} elements/element_joint_beam_cross_half_lap_diagonal_drills.cpp

## cross_half_lap_drill: one vertical drill

![cross_half_lap_drill](elements/element_joint_beam_cross_half_lap_drill.png)

![cross_half_lap_drill, the unit box: male pink, female blue](joint_library/cr_cross_half_lap_drill_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_half_lap_drill(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The milled half-lap of `cross_half_lap` with one vertical drill down its centre.

\include{lineno} elements/element_joint_beam_cross_half_lap_drill.cpp

## cross_brussels_half_lap: the Brussels half-lap

![cross_brussels_half_lap](elements/element_joint_beam_cross_brussels_half_lap.png)

![cross_brussels_half_lap, the unit box: male pink, female blue](joint_library/cr_cross_brussels_half_lap_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_brussels_half_lap(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

An asymmetric milled half-lap with a vertical 50 mm drill and a horizontal 10 mm drill (`drill_50`, `drill_10`).

\include{lineno} elements/element_joint_beam_cross_brussels_half_lap.cpp
