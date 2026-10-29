# Beam Top-to-Side {#elements_joint_beam_top_side}

[TOC]

Tee joints where the end of one beam meets the side of another (`Beam::pair_tee(90.0)` and `Beam::pair_tee(60.0)`), selected via `settings.joint_parameters[8]` and cut by `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings());
```

## Milled Wedge Tenon
A through-tenon and mortise with a milled wedge slot on orthogonal (`90°`) and angled (`60°`) T-beam pairs.

![](elements/element_joint_beam_top_side_milled_wedge.png)

![](joint_library/ts_top_side_milled_wedge_unit.png)

```cpp
Settings settings;
settings.joint_parameters[8] = JointType::top_side_milled_wedge;
const std::shared_ptr<JointBeam> joint = JointBeam::from_contact(*a, *b, *contact, 300.0, 0.5, 0, settings);
```
