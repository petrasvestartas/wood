# JointBeam end to end {#elements_joint_beam_end_to_end}

[TOC]

Joints at the shared axis end of two beams, cutting back each `Beam::extension`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())
```

In line, the splice is the in-plane design of `settings.joint_parameters[2]`; at an angle, the beams are trimmed on the mitre.

## side_side_inplane_splayed_scarf: the splayed scarf

![side_side_inplane_splayed_scarf](elements/element_joint_beam_side_side_inplane_splayed_scarf.png)

![side_side_inplane_splayed_scarf, the unit box: male pink, female blue](joint_library/ip_side_side_inplane_splayed_scarf_unit.png)

`Beam::pair_end_to_end(0.0)`: a splayed scarf with shoulders and two pins; the second beam moved 300 along the axis.

\include{lineno} elements/element_joint_beam_side_side_inplane_splayed_scarf.cpp

## side_side_inplane_cross_splayed_scarf: the cross-splayed scarf

![side_side_inplane_cross_splayed_scarf](elements/element_joint_beam_side_side_inplane_cross_splayed_scarf.png)

![side_side_inplane_cross_splayed_scarf, the unit box: male pink, female blue](joint_library/ip_side_side_inplane_cross_splayed_scarf_unit.png)

The same scarf with the upper and lower halves of the depth splayed in opposite directions, so the beams lock sideways both ways.

\include{lineno} elements/element_joint_beam_side_side_inplane_cross_splayed_scarf.cpp

## side_side_rotated_mitre: the mitre

![side_side_rotated_mitre](elements/element_joint_beam_side_side_rotated_mitre.png)

`Beam::pair_end_to_end(90.0)` and `Beam::pair_end_to_end(60.0)`: each extension trimmed on the plane through the shared end bisecting the axes; the second beam is moved 300.

\include{lineno} elements/element_joint_beam_side_side_rotated_mitre.cpp
