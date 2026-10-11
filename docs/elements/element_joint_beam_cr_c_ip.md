# JointBeam cr_c_ip {#elements_joint_beam_cr_c_ip}

[TOC]

Crossing joints on two beams that cross: the plate library's cross designs, chosen by `settings.joint_parameters[11]`, cut into both beams as solids by `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())
```

Every example joins two pairs, `Beam::pair_crossing(90.0)` and `Beam::pair_crossing(60.0)`: two 150 x 150 beams crossing at their middles, square and at 60 degrees, the second beam lifted 300.

## cr_c_ip_0: the plain slot

![cr_c_ip_0](elements/element_joint_beam_cr_c_ip_0.png)

![cr_c_ip_0, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_0_unit.png)

\include{lineno} elements/element_joint_beam_cr_c_ip_0.cpp

## cr_c_ip_1: the sliced half-lap

![cr_c_ip_1](elements/element_joint_beam_cr_c_ip_1.png)

![cr_c_ip_1, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_1_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_1(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The half-lap as nine rings per plate on the shift, every one a solid taken from the plate: the centre square and the two top sides milled, the two bottom sides and the four corner wedges sliced; face 1 is every ring offset along its normal, the male the female with its axes swapped and its depth flipped, as 2024 built it. Chosen on beams by `settings.joint_parameters[11] = JointPlate::library_id("cr_c_ip_1")`.

\include{lineno} elements/element_joint_beam_cr_c_ip_1.cpp

## cr_c_ip_2: the milled half-lap

![cr_c_ip_2](elements/element_joint_beam_cr_c_ip_2.png)

![cr_c_ip_2, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_2_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_2(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The five rings 2024 wrote four times for `cr_c_ip_2` to `cr_c_ip_5`, one body here: the centre square milled, the two top sides as the two sheer walls between each ring and its offset, the two bottom sides milled, their first and third segments extended 0.15 along the plate at both ends and their slanted segments 0.6 up and down so the cut clears the plate. Chosen on beams by `settings.joint_parameters[11] = JointPlate::library_id("cr_c_ip_2")`.

\include{lineno} elements/element_joint_beam_cr_c_ip_2.cpp

## cr_c_ip_3: two diagonal drills

![cr_c_ip_3](elements/element_joint_beam_cr_c_ip_3.png)

![cr_c_ip_3, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_3_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_3(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The milled half-lap of `cr_c_ip_2` and two diagonal drills through the lap, each a line in the unit box bored through both plates. Chosen on beams by `settings.joint_parameters[11] = JointPlate::library_id("cr_c_ip_3")`.

\include{lineno} elements/element_joint_beam_cr_c_ip_3.cpp

## cr_c_ip_4: one vertical drill

![cr_c_ip_4](elements/element_joint_beam_cr_c_ip_4.png)

![cr_c_ip_4, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_4(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The milled half-lap of `cr_c_ip_2` and one vertical drill down the centre of the lap, along the cross's depth through both plates; the line keeps where 2024 declared it, where 2024 offset it too, reading past its ring and length arrays. Chosen on beams by `settings.joint_parameters[11] = JointPlate::library_id("cr_c_ip_4")`.

\include{lineno} elements/element_joint_beam_cr_c_ip_4.cpp

## cr_c_ip_5: the Brussels half-lap

![cr_c_ip_5](elements/element_joint_beam_cr_c_ip_5.png)

![cr_c_ip_5, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_5(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The milled half-lap with the asymmetry 2024 gave the Brussels sports tower, the bottom sides extended 0.27 on their first segment and shortened 0.075 on their third, a vertical 50 mm bit down the centre of the lap and a horizontal 10 mm bit below it, each bored through both plates, the bits named in `FabricationType` as `drill_50` and `drill_10`. Chosen on beams by `settings.joint_parameters[11] = JointPlate::library_id("cr_c_ip_5")`.

\include{lineno} elements/element_joint_beam_cr_c_ip_5.cpp

## Parameters

**Shift.** The shift narrows the half-lap's centre square, 0 the widest and 1 the narrowest. Above 0.85 the slanted side cuts of cr_c_ip_2 to 5 cross over and the ring folds onto itself, in 2024 as here.

![cr_c_ip sweep](joint_library/sweep_cr_c_ip.png)
