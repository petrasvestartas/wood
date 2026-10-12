# JointBeam butt {#elements_joint_beam_butt}

[TOC]

A butt of two members drawn on lines and mid-planes, beams, columns or plates: each end that reaches the other is run on and cut on one plane, so the two meet face to face. How the ends meet decides the configuration:

| Configuration | Ends | Cut |
|---|---|---|
| `top_side` | one end on the other's face, or both at a square corner | the end on the face; at a corner the first runs on to the second's outer face |
| `side_side_outplane` | both, at an oblique corner | both on the miter |
| `side_side_inplane` | both, in line or in plane | both on the plane halfway |
| `cross` | none | nothing |

The drawn members decide, so the order butts are added in does not matter. A plate's butted edges are run on and cut together, so its corners close and a valley keeps its wood.

## Constructors

```cpp
static std::shared_ptr<JointBeam> butt(const Element& a, const Element& b, double reach = 0.0);              // the configuration read from the ends
static std::shared_ptr<JointBeam> butt_side_side(const Element& a, const Element& b, double reach = 0.0);    // every corner on the miter
static std::shared_ptr<JointBeam> butt_top_side(const Element& top, const Element& side, double reach = 0.0); // the second's end on the first
static std::shared_ptr<Plate> Plate::from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");   // a wall centred on its footprint line
```

## Beams

![Beams](elements/element_joint_beam_butt.png)

A post on the middle of a sill and one on its end: each post stops on the sill's top, the sill runs on to the end post's outer face.

\include{lineno} elements/element_joint_beam_butt.cpp

## Plates

![Plates](elements/element_joint_beam_butt_plates.png)

Two walls on a floor meeting at a corner: the floor runs on under the walls, which stop on its top; the long wall runs on to the short wall's outer face.

\include{lineno} elements/element_joint_beam_butt_plates.cpp
