# Beam Butt {#elements_joint_beam_butt}

[TOC]

Order-independent planar butt, miter, and half-lap joints (`JointBeamElement::butt`) between beams, columns, or plates.

![](diagrams/wood_beam_joints_connectors.webp)

| Configuration | Endpoint Condition | Plane Cut Applied |
| --- | --- | --- |
| `top_side` | One end meets the other's face, or both meet at a `90°` corner | Butting end cut on the through-member's face; at a square corner, member `0` runs to the outer face of member `1` |
| `side_side_outplane` | Both ends meet at an oblique corner | Both members cut on their angle-bisector miter plane |
| `side_side_inplane` | Both ends meet in line or coplanar | Both members cut on the perpendicular midpoint plane |
| `cross` | Interior crossing (neither end at contact) | Beams cut to half-depth lap; plates uncut |

## Constructors

```cpp
static std::shared_ptr<JointBeamElement> butt(const Element& a, const Element& b, double reach = 0.0);
static std::shared_ptr<JointBeamElement> butt_side_side(const Element& a, const Element& b, double reach = 0.0);
static std::shared_ptr<JointBeamElement> butt_top_side(const Element& top, const Element& side, double reach = 0.0);
static std::shared_ptr<FacePlanarElement> FacePlanarElement::from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");
```

## Beams

A mid-span post and a corner post resting on a sill beam: both posts stop on the sill's top face while the sill runs flush to the corner post's outer face.

![](elements/element_joint_beam_butt.png)

\include{lineno} elements/element_joint_beam_butt.cpp

## Plates

Two wall plates meeting at a corner on top of a floor slab: the floor slab runs beneath both walls while the long wall runs flush to the outer face of the short wall.

![](elements/element_joint_beam_butt_plates.png)

\include{lineno} elements/element_joint_beam_butt_plates.cpp
