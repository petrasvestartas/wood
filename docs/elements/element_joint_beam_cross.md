# Beam Cross {#elements_joint_beam_cross}

[TOC]

Cross-lap joints on two crossing beams (`Beam::pair_crossing(90.0)` and `Beam::pair_crossing(60.0)`) cut as 3D feature volumes by `JointBeam::from_contact` using `settings.joint_parameters[11]`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings());
```

Every example shows two crossing pairs (`90°` and `60°`) with the lower beam shifted downward out of its lap notch.

## Plain Half-Lap Slot
A straight half-depth rectangular lap slot cut into both crossing beams.

![](elements/element_joint_beam_cross_slot.png)

![](joint_library/cr_cross_slot_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_slot();
```

## Sliced Half-Lap
A nine-ring half-lap per beam where the centre and top sides are milled while the bottom sides and corner wedges are sliced.

![](elements/element_joint_beam_cross_sliced_half_lap.png)

![](joint_library/cr_cross_sliced_half_lap_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_sliced_half_lap(0.5);
```

## Milled Half-Lap
A five-ring milled half-lap per beam with sheer upper walls and extended lower bearing shelves.

![](elements/element_joint_beam_cross_half_lap.png)

![](joint_library/cr_cross_half_lap_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_half_lap(0.5);
```

## Milled Half-Lap with Diagonal Drills
The milled half-lap of `cross_half_lap` reinforced by two diagonal pin bores through both beams.

![](elements/element_joint_beam_cross_half_lap_diagonal_drills.png)

![](joint_library/cr_cross_half_lap_diagonal_drills_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_half_lap_diagonal_drills(0.5);
```

## Milled Half-Lap with Vertical Drill
The milled half-lap of `cross_half_lap` with a single vertical pin bore down the crossing axis.

![](elements/element_joint_beam_cross_half_lap_drill.png)

![](joint_library/cr_cross_half_lap_drill_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_half_lap_drill(0.5);
```

## Brussels Half-Lap
An asymmetric milled half-lap with a vertical 50 mm counterbore and a horizontal 10 mm cross-pin bore.

![](elements/element_joint_beam_cross_brussels_half_lap.png)

![](joint_library/cr_cross_brussels_half_lap_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cross_brussels_half_lap(0.5);
```
