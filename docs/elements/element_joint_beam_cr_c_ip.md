# JointBeam cr_c_ip {#elements_joint_beam_cr_c_ip}

[TOC]

Crossing joints on two beams that cross: the plate library's cross designs, chosen by `settings.joint_parameters[11]`, cut into both beams as solids by `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())
```

Every example shows two pairs, `Beam::pair_crossing(90.0)` and `Beam::pair_crossing(60.0)`: two beams crossing square and at 60 degrees, the second moved down out of its notch.

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

A half-lap of nine rings per beam: the centre and top sides milled, the bottom sides and corner wedges sliced.

\include{lineno} elements/element_joint_beam_cr_c_ip_1.cpp

## cr_c_ip_2: the milled half-lap

![cr_c_ip_2](elements/element_joint_beam_cr_c_ip_2.png)

![cr_c_ip_2, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_2_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_2(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

A half-lap of five rings: the centre square and bottom sides milled, the top sides as sheer walls.

\include{lineno} elements/element_joint_beam_cr_c_ip_2.cpp

## cr_c_ip_3: two diagonal drills

![cr_c_ip_3](elements/element_joint_beam_cr_c_ip_3.png)

![cr_c_ip_3, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_3_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_3(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The milled half-lap of `cr_c_ip_2` with two diagonal drills through both beams.

\include{lineno} elements/element_joint_beam_cr_c_ip_3.cpp

## cr_c_ip_4: one vertical drill

![cr_c_ip_4](elements/element_joint_beam_cr_c_ip_4.png)

![cr_c_ip_4, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_4(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

The milled half-lap of `cr_c_ip_2` with one vertical drill down its centre.

\include{lineno} elements/element_joint_beam_cr_c_ip_4.cpp

## cr_c_ip_5: the Brussels half-lap

![cr_c_ip_5](elements/element_joint_beam_cr_c_ip_5.png)

![cr_c_ip_5, the unit box: male pink, female blue](joint_library/cr_cr_c_ip_5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_5(
    0.5  // shift: how narrow the half-lap's centre square is: 0 widest, 1 narrowest
);
```

An asymmetric milled half-lap with a vertical 50 mm drill and a horizontal 10 mm drill (`drill_50`, `drill_10`).

\include{lineno} elements/element_joint_beam_cr_c_ip_5.cpp

## Parameters

**Shift.** The shift narrows the centre square, 0 the widest; above 0.85 the slanted cuts cross and the ring folds.

![cr_c_ip sweep](joint_library/sweep_cr_c_ip.png)
