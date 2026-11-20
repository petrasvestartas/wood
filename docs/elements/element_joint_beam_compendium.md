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

The records of the library so far, by the compendium's chapter, page and German name.

| design | joint | German | page |
|---|---|---|---|
| `notched_oblique_thrust` | Notched oblique thrust joint | Stirnversatz | 584 |
| `notched_oblique_thrust_right_angled_end` | Notched oblique thrust joint with right-angled end | Stirnversatz mit rechtwinkliger Stirn | 586 |
| `oblique_thrust_notched_heel` | Oblique thrust joint with notched heel | Fersenversatz | 590 |
| `double_notched_oblique_thrust` | Double notched oblique thrust joint | Stirn- und Fersenversatz | 592 |
| `oblique_thrust_sloped_tenon_notched_heel` | Oblique thrust joint with sloped tenon and notched heel | Fersenversatz mit schrägem Zapfen | 594 |
| `oblique_thrust_mortise_tenon` | Oblique thrust joint with mortise and tenon | Schräger Stoß mit Zapfen | 596 |
| `oblique_thrust_sloped_mortise_tenon` | Oblique thrust joint with sloped mortise and tenon | schräger Stoß mit schrägem Zapfen | 598 |
| `blind_mortise_tenon` | Blind mortise and tenon | verdeckter Zapfen | 496 |

## Library
Every record built on a pair of 120 x 120 beams, one group per design: the angled joints with the member leaning at 50 degrees, the T-joints standing square, each member drawn out along its first insertion direction.

![](elements/element_joint_beam_library.png)

\include{lineno} elements/element_joint_beam_library.cpp
