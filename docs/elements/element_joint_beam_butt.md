# JointBeam butt {#elements_joint_beam_butt}

[TOC]

Order-independent plane butt joints (`JointBeam::butt`) between beams, columns, or plates.

![JointBeam Axis Contacts, Butt Planes & Connectors](diagrams/wood_beam_joints_connectors.svg)

| Configuration | Endpoint Condition | Plane Cut Applied |
| --- | --- | --- |
| `top_side` | One end meets the other's face, or both meet at a `90°` corner | Butting end cut on the through-member's face; at a square corner, member `0` runs to the outer face of member `1` |
| `side_side_outplane` | Both ends meet at an oblique corner | Both members cut on their angle-bisector miter plane |
| `side_side_inplane` | Both ends meet in line or coplanar | Both members cut on the perpendicular midpoint plane |
| `cross` | Interior crossing (neither end at contact) | Beams cut to half-depth lap; plates uncut |

## Constructors

```cpp
static std::shared_ptr<JointBeam> butt(const Element& a, const Element& b, double reach = 0.0);              // automatic classification
static std::shared_ptr<JointBeam> butt_side_side(const Element& a, const Element& b, double reach = 0.0);    // force miter at every corner
static std::shared_ptr<JointBeam> butt_top_side(const Element& top, const Element& side, double reach = 0.0); // force second end onto first
static std::shared_ptr<Plate> Plate::from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");
```

## Beams

![Beams](elements/element_joint_beam_butt.png)

Mid-span post and corner post on a sill: posts stop on the sill's top face; the sill runs flush to the corner post's outer face.

\include{lineno} elements/element_joint_beam_butt.cpp

## Plates

![Plates](elements/element_joint_beam_butt_plates.png)

Two walls on a floor slab meeting at a corner: floor runs under both walls; long wall runs to the outer face of the short wall.

\include{lineno} elements/element_joint_beam_butt_plates.cpp
