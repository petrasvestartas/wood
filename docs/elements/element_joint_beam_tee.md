# JointBeam tee {#elements_joint_beam_tee}

[TOC]

Tee joints where a beam ends on another's side, chosen by `settings.joint_parameters[8]` and cut by `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())
```

## tee_milled_wedge: the milled wedge

![tee_milled_wedge](elements/element_joint_beam_tee_milled_wedge.png)

![tee_milled_wedge, the unit box: male pink, female blue](joint_library/ts_tee_milled_wedge_unit.png)

`Beam::pair_tee(90.0)` and `Beam::pair_tee(60.0)`: a beam ending on another, square and at 60 degrees, its tenon through the other; lifted to show the mortise.

\include{lineno} elements/element_joint_beam_tee_milled_wedge.cpp
