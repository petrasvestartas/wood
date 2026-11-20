# Beam Compendium Joints {#elements_joint_beam_compendium}

[TOC]

Beam joints rebuilt from Bauer and Pauli's *Joinery Compendium* as its drawings make them, one record each in the YAML library `data/joints/*.yaml` (the format is written at the top of every file) built by `JointBeamElement::from_library`: each keeps its cutting procedure step by step, A1, A2 on its first member and B1, B2 on its second, and the directions each member goes in along.

## Constructors

```cpp
static std::shared_ptr<JointBeamElement> JointBeamElement::from_library(const std::string& design, const EdgeStraightElement& a, const EdgeStraightElement& b, const std::map<std::string, double>& parameters = {}, double overshoot = 2.0);
JointClassification classify_joint(const EdgeStraightElement& a, const EdgeStraightElement& b, double tolerance = 1.0);
AssemblySequence(const WoodSession& session, const std::string& base, double step = 10.0, double clearance = 10.0);
```

## Parameters

- Every record names its parameters with their defaults, shares of the members' sizes: `depth` a share of the carrier's depth, `thickness` of the member's width, `inset` of its depth; `from_library` takes others by name.
- `overshoot`: how far every removal runs past the faces it opens, so the one boolean that takes them away leaves no skin.

Every step keeps its saw or chisel planes and the wood it takes away (`JointElement::steps`, `target_steps(i)`); a member loses all of them in one boolean (`removal(i)`), and the steps and the insertion directions (`target_insertions`) are written with the scene. `add_interaction` also adds the directions to the member's own `insertion_vectors`.

## Assembly

`classify_joint` reads what two beams need from their axes: an end where the closest point lies within the other's half section of an axis end, then `end_end_inplane` (splicing), `end_end_outplane` (corner), `side_end` (t when square, else angled), `cross` (cross lap) or `side_side`. `AssemblySequence` takes the members of a session apart: again and again one slides out along a direction one of its joints gives, swept its own diagonal against every member still in without entering one, the base last; reversed, that is the assembly. A member no direction frees is reported locked, and a joint whose every direction another joint's member blocks is reported as a conflict.

## Families

The records of the library so far, each on a square and an inclined corner where it exists there, the member drawn out along its insertion. Pictures by `python3 tools/render_joint_library.py`.

<!-- joint-table -->
| joint | compendium |
|---|---|
| ![](joint_library/beam_notched_oblique_thrust.png) | **Notched oblique thrust joint**<br>Stirnversatz, p. 584<br>`notched_oblique_thrust` |
| ![](joint_library/beam_notched_oblique_thrust_right_angled_end.png) | **Notched oblique thrust joint with right-angled end**<br>Stirnversatz mit rechtwinkliger Stirn, p. 586<br>`notched_oblique_thrust_right_angled_end` |
| ![](joint_library/beam_oblique_thrust_notched_heel.png) | **Oblique thrust joint with notched heel**<br>Fersenversatz, p. 590<br>`oblique_thrust_notched_heel` |
| ![](joint_library/beam_double_notched_oblique_thrust.png) | **Double notched oblique thrust joint**<br>Stirn- und Fersenversatz, p. 592<br>`double_notched_oblique_thrust` |
| ![](joint_library/beam_oblique_thrust_sloped_tenon_notched_heel.png) | **Oblique thrust joint with sloped tenon and notched heel**<br>Fersenversatz mit schrägem Zapfen, p. 594<br>`oblique_thrust_sloped_tenon_notched_heel` |
| ![](joint_library/beam_oblique_thrust_mortise_tenon.png) | **Oblique thrust joint with mortise and tenon**<br>Schräger Stoß mit Zapfen, p. 596<br>`oblique_thrust_mortise_tenon` |
| ![](joint_library/beam_oblique_thrust_sloped_mortise_tenon.png) | **Oblique thrust joint with sloped mortise and tenon**<br>schräger Stoß mit schrägem Zapfen, p. 598<br>`oblique_thrust_sloped_mortise_tenon` |
| ![](joint_library/beam_notched_oblique_thrust_mortise_tenon.png) | **Notched oblique thrust joint with mortise and tenon**<br>Stirnversatz mit Zapfen, p. 600<br>`notched_oblique_thrust_mortise_tenon` |
| ![](joint_library/beam_notched_oblique_thrust_pegged_mortise_tenon.png) | **Notched oblique thrust joint with pegged mortise and tenon**<br>Stirnversatz mit Zapfen und Dübelsicherung, p. 602<br>`notched_oblique_thrust_pegged_mortise_tenon` |
| ![](joint_library/beam_blind_mortise_tenon.png) | **Blind mortise and tenon**<br>verdeckter Zapfen, p. 496<br>`blind_mortise_tenon` |
<!-- /joint-table -->

## Library
Every record built on pairs of 120 x 120 beams standing in the xz plane, a group and a row per design: on a square corner and on one leaning at 50 degrees where the record exists there, each member drawn out along its first insertion direction.

![](elements/element_joint_beam_library.png)

\include{lineno} elements/element_joint_beam_library.cpp
