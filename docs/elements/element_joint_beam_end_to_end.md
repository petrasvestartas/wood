# JointBeam end to end {#elements_joint_beam_end_to_end}

[TOC]

End-to-end splice and miter joints at the shared axis endpoint of two beams (`Beam::pair_end_to_end`), cutting into each beam's `extension` stock via `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings());
```

Collinear beams (`angle_degrees = 0.0`) use the in-plane scarf preset in `settings.joint_parameters[2]`; angled beams trim their extensions on the angle-bisector miter plane.

## side_side_inplane_splayed_scarf: splayed scarf

A collinear splayed scarf splice with end shoulders, key groove, and two vertical pin bores.

![](elements/element_joint_beam_side_side_inplane_splayed_scarf.png)

![](joint_library/ip_side_side_inplane_splayed_scarf_unit.png)

```cpp
Settings settings;
settings.joint_parameters[2] = JointType::side_side_inplane_splayed_scarf;
const std::shared_ptr<JointBeam> joint = JointBeam::from_contact(*a, *b, *contact, 300.0, 0.5, 0, settings);
```

<details><summary>Example code: elements/element_joint_beam_side_side_inplane_splayed_scarf.cpp</summary>

\include{lineno} elements/element_joint_beam_side_side_inplane_splayed_scarf.cpp

</details>

## side_side_inplane_cross_splayed_scarf: cross-splayed scarf

A collinear scarf whose upper and lower halves splay in opposite directions to lock the splice against lateral shear both ways.

![](elements/element_joint_beam_side_side_inplane_cross_splayed_scarf.png)

![](joint_library/ip_side_side_inplane_cross_splayed_scarf_unit.png)

```cpp
Settings settings;
settings.joint_parameters[2] = JointType::side_side_inplane_cross_splayed_scarf;
const std::shared_ptr<JointBeam> joint = JointBeam::from_contact(*a, *b, *contact, 300.0, 0.5, 0, settings);
```

<details><summary>Example code: elements/element_joint_beam_side_side_inplane_cross_splayed_scarf.cpp</summary>

\include{lineno} elements/element_joint_beam_side_side_inplane_cross_splayed_scarf.cpp

</details>

## side_side_rotated_miter: angled end miter

Two beam pairs meeting at `90°` and `60°` whose extensions are trimmed cleanly along the bisecting miter plane.

![](elements/element_joint_beam_side_side_rotated_miter.png)

```cpp
const auto [a, b] = Beam::pair_end_to_end(90.0);
const std::shared_ptr<JointBeam> joint = JointBeam::from_contact(*a, *b, *contact, 300.0, 0.5);
```

<details><summary>Example code: elements/element_joint_beam_side_side_rotated_miter.cpp</summary>

\include{lineno} elements/element_joint_beam_side_side_rotated_miter.cpp

</details>

## jupiter_scarf: trait de Jupiter scarf

A classical trait de Jupiter tension scarf with hooked tables, twin slopes, central key slot, and locking wedge key.

![](elements/element_joint_beam_jupiter_scarf.png)

![](joint_library/ip_jupiter_scarf_unit.png)

```cpp
Settings settings;
settings.joint_parameters[2] = JointType::jupiter_scarf;
const std::shared_ptr<JointBeam> joint = JointBeam::from_contact(*a, *b, *contact, 300.0, 0.5, 0, settings);
```

<details><summary>Example code: elements/element_joint_beam_jupiter_scarf.cpp</summary>

\include{lineno} elements/element_joint_beam_jupiter_scarf.cpp

</details>
