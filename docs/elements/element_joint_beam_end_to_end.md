# JointBeam end to end {#elements_joint_beam_end_to_end}

[TOC]

Joints at the shared axis end of two beams whose solids run past it by `Beam::extension`: the joint cuts each extension back, so the beams are modelled long and cut first.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())
```

In line, the splice is the in-plane design of `settings.joint_parameters[2]`: each beam loses what the design mills from it and the other beam's box past the joint. At an angle, the beams are trimmed on the mitre.

## ss_e_ip_3: the splayed scarf

![ss_e_ip_3](elements/element_joint_beam_ss_e_ip_3.png)

`Beam::pair_end_to_end(0.0)`: a splayed scarf with shoulders through the depth and two pins across; the second beam is moved 300 along the axis.

\include{lineno} elements/element_joint_beam_ss_e_ip_3.cpp

## ss_e_ip_4: the cross-splayed scarf

![ss_e_ip_4](elements/element_joint_beam_ss_e_ip_4.png)

The same scarf with the upper and lower halves of the depth splayed in opposite directions, so the beams lock sideways both ways.

\include{lineno} elements/element_joint_beam_ss_e_ip_4.cpp

## ss_e_r_0: the mitre

![ss_e_r_0](elements/element_joint_beam_ss_e_r_0.png)

`Beam::pair_end_to_end(90.0)`: each extension trimmed on the plane through the shared end bisecting the axes; the second beam is moved 300.

\include{lineno} elements/element_joint_beam_ss_e_r_0.cpp
