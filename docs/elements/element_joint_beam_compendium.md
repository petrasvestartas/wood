# Beam Compendium Joints {#elements_joint_beam_compendium}

[TOC]

Beam joints rebuilt from Bauer and Pauli's *Joinery Compendium* as its drawings make them: each keeps its cutting procedure step by step, A1, A2 on its first member and B1, B2 on its second, and the directions each member goes in along.

## Constructors

```cpp
static std::shared_ptr<JointBeamElement> JointBeamElement::notched_oblique_thrust(const EdgeStraightElement& strut, const EdgeStraightElement& carrier, double depth = 0.2, double overshoot = 2.0);
static std::shared_ptr<JointBeamElement> JointBeamElement::blind_mortise_tenon(const EdgeStraightElement& tenon, const EdgeStraightElement& mortise, double thickness = 1.0 / 3.0, double inset = 1.0 / 6.0, double depth = 0.5, double overshoot = 2.0);
JointClassification classify_joint(const EdgeStraightElement& a, const EdgeStraightElement& b, double tolerance = 1.0);
AssemblySequence(const WoodSession& session, const std::string& base, double step = 10.0, double clearance = 10.0);
```

## Parameters

- `depth`: the notch's depth, a share of the carrier's depth across the face the strut arrives on (p. 584); the mortise's, a share of the mortise member's depth (p. 496).
- `thickness`: the tenon's thickness across the mortise member, a share of the tenon member's width; `inset`: how far its edges are set in along the mortise member, a share of the tenon member's depth.
- `overshoot`: how far every removal runs past the faces it opens, so the one boolean that takes them away leaves no skin.

Every step keeps its saw or chisel planes and the wood it takes away (`JointElement::steps`, `target_steps(i)`); a member loses all of them in one boolean (`removal(i)`), and the steps and the insertion directions (`target_insertions`) are written with the scene. `add_interaction` also adds the directions to the member's own `insertion_vectors`.

## Assembly

`classify_joint` reads what two beams need from their axes: an end where the closest point lies within the other's half section of an axis end, then `end_end_inplane` (splicing), `end_end_outplane` (corner), `side_end` (t when square, else angled), `cross` (cross lap) or `side_side`. `AssemblySequence` takes the members of a session apart: again and again one slides out along a direction one of its joints gives, swept its own diagonal against every member still in without entering one, the base last; reversed, that is the assembly. A member no direction frees is reported locked, and a joint whose every direction another joint's member blocks is reported as a conflict.

## Notched Oblique Thrust
The Stirnversatz: the strut's front face and seat sawn (A1, A2), the notch's front face sawn to depth and its seat sawn so the wedge comes out (B1, B2), the front face bisecting the obtuse angle; the strut drawn out along its insertion, the bisector of the notch's two faces.

![](elements/element_joint_beam_notched_oblique_thrust.png)

\include{lineno} elements/element_joint_beam_notched_oblique_thrust.cpp

## Blind Mortise and Tenon
The verdeckter Zapfen: the tenon crosscut to length (A1), its cheeks ripped and crosscut (A2), its edge shoulders sawn (A3), the mortise chopped (B1); the member drawn out along its axis, the one way it comes out.

![](elements/element_joint_beam_blind_mortise_tenon.png)

\include{lineno} elements/element_joint_beam_blind_mortise_tenon.cpp
