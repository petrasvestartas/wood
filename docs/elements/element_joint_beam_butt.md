# JointBeam butt {#elements_joint_beam_butt}

[TOC]

A flat butt of two members, beams or plates drawn on lines and mid-planes: the butting member is cut on the running member's face, and at a corner the running member runs on to the butting member's outer face.

## Constructors

```cpp
static std::shared_ptr<JointBeam> butt(Element& running, Element& butting);   // target 0 runs, target 1 butts
static std::shared_ptr<Plate> Plate::from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");   // a wall centred on its footprint line
```

Each butt reads its running member as the earlier butts left it, cut and run on: join slabs to walls before walls to walls, so a wall corner runs on from what the wall keeps.

## Beams

![Beams](elements/element_joint_beam_butt.png)

A post on the middle of a sill and one on its end: each post is cut on the sill's top, the sill runs on to the end post's outer face.

\include{lineno} elements/element_joint_beam_butt.cpp

## Plates

![Plates](elements/element_joint_beam_butt_plates.png)

Two walls on a floor meeting at a corner: the walls are cut on the floor's top and the floor runs on under them; the short wall is cut on the long wall's inner face and the long wall runs on to its outer face.

\include{lineno} elements/element_joint_beam_butt_plates.cpp
