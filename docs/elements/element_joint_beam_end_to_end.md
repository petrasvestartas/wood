# Beam End-to-End {#elements_joint_beam_end_to_end}

[TOC]

End-to-end splice and miter joints at the shared axis endpoint of two beams (`EdgeStraightElement::pair_end_to_end`), cutting into each beam's `extension` stock via `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<EdgeStraightElement>, 2> EdgeStraightElement::pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0);
static std::shared_ptr<JointBeam> from_contact(const EdgeStraightElement& source, const EdgeStraightElement& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings());
```

Collinear beams (`angle_degrees = 0.0`) use the in-plane scarf preset in `settings.joint_parameters[2]`; angled beams trim their extensions on the angle-bisector miter plane.

## Splayed Scarf
A collinear splayed scarf splice with end shoulders, key groove, and two vertical pin bores.

![](elements/element_joint_beam_side_side_inplane_splayed_scarf.png)

![](joint_library/ip_side_side_inplane_splayed_scarf_unit.png)

\include{lineno} elements/element_joint_beam_side_side_inplane_splayed_scarf.cpp

## Cross-Splayed Scarf
A collinear scarf whose upper and lower halves splay in opposite directions to lock the splice against lateral shear both ways.

![](elements/element_joint_beam_side_side_inplane_cross_splayed_scarf.png)

![](joint_library/ip_side_side_inplane_cross_splayed_scarf_unit.png)

\include{lineno} elements/element_joint_beam_side_side_inplane_cross_splayed_scarf.cpp

## Angled End Miter
A beam pair meeting at `60°` whose extensions are trimmed cleanly along the bisecting miter plane.

![](elements/element_joint_beam_side_side_rotated_miter.png)

\include{lineno} elements/element_joint_beam_side_side_rotated_miter.cpp

## Trait De Jupiter Scarf
A classical trait de Jupiter tension scarf with hooked tables, twin slopes, central key slot, and locking wedge key.

![](elements/element_joint_beam_jupiter_scarf.png)

![](joint_library/ip_jupiter_scarf_unit.png)

\include{lineno} elements/element_joint_beam_jupiter_scarf.cpp
