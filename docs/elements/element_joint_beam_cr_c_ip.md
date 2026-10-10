# JointBeam cr_c_ip {#elements_joint_beam_cr_c_ip}

[TOC]

Crossing joints on two beams that cross: the plate library's cross designs, chosen by `settings.joint_parameters[11]`, cut into both beams as solids by `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())
```

Every example is the same pair, `Beam::pair_crossing(90.0)`, two 150 x 150 beams crossing at their middles, the second lifted 300.

## cr_c_ip_0: the plain slot

![cr_c_ip_0](elements/element_joint_beam_cr_c_ip_0.png)

\include{lineno} elements/element_joint_beam_cr_c_ip_0.cpp

## cr_c_ip_1: the sliced half-lap

![cr_c_ip_1](elements/element_joint_beam_cr_c_ip_1.png)

\include{lineno} elements/element_joint_beam_cr_c_ip_1.cpp

## cr_c_ip_2: the milled half-lap

![cr_c_ip_2](elements/element_joint_beam_cr_c_ip_2.png)

\include{lineno} elements/element_joint_beam_cr_c_ip_2.cpp

## cr_c_ip_3: two diagonal drills

![cr_c_ip_3](elements/element_joint_beam_cr_c_ip_3.png)

\include{lineno} elements/element_joint_beam_cr_c_ip_3.cpp

## cr_c_ip_4: one vertical drill

![cr_c_ip_4](elements/element_joint_beam_cr_c_ip_4.png)

\include{lineno} elements/element_joint_beam_cr_c_ip_4.cpp

## cr_c_ip_5: the Brussels half-lap

![cr_c_ip_5](elements/element_joint_beam_cr_c_ip_5.png)

\include{lineno} elements/element_joint_beam_cr_c_ip_5.cpp
