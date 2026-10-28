# JointBeam butt {#elements_joint_beam_butt}

[TOC]

Order-independent planar butt, miter, and half-lap joints (`JointBeam::butt`) between beams, columns, or plates.

![](diagrams/wood_beam_joints_connectors.webp)

| Configuration | Endpoint Condition | Plane Cut Applied |
| --- | --- | --- |
| `top_side` | One end meets the other's face, or both meet at a `90°` corner | Butting end cut on the through-member's face; at a square corner, member `0` runs to the outer face of member `1` |
| `side_side_outplane` | Both ends meet at an oblique corner | Both members cut on their angle-bisector miter plane |
| `side_side_inplane` | Both ends meet in line or coplanar | Both members cut on the perpendicular midpoint plane |
| `cross` | Interior crossing (neither end at contact) | Beams cut to half-depth lap; plates uncut |

## Constructors

```cpp
static std::shared_ptr<JointBeam> butt(const Element& a, const Element& b, double reach = 0.0);
static std::shared_ptr<JointBeam> butt_side_side(const Element& a, const Element& b, double reach = 0.0);
static std::shared_ptr<JointBeam> butt_top_side(const Element& top, const Element& side, double reach = 0.0);
static std::shared_ptr<Plate> Plate::from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");
```

## Beams

A mid-span post and a corner post resting on a sill beam: both posts stop on the sill's top face while the sill runs flush to the corner post's outer face.

![](elements/element_joint_beam_butt.png)

```cpp
const std::shared_ptr<JointBeam> joint = JointBeam::butt(*sill, *post);
scene.add_interaction(joint, sill, joint->interaction(0));
scene.add_interaction(joint, post, joint->interaction(1));
```

<details><summary>Example code: elements/element_joint_beam_butt.cpp</summary>

\include{lineno} elements/element_joint_beam_butt.cpp

</details>

## Plates

Two wall plates meeting at a corner on top of a floor slab: the floor slab runs beneath both walls while the long wall runs flush to the outer face of the short wall.

![](elements/element_joint_beam_butt_plates.png)

```cpp
const std::shared_ptr<JointBeam> corner = JointBeam::butt(*long_wall, *short_wall);
scene.add_interaction(corner, long_wall, corner->interaction(0));
scene.add_interaction(corner, short_wall, corner->interaction(1));
```

<details><summary>Example code: elements/element_joint_beam_butt_plates.cpp</summary>

\include{lineno} elements/element_joint_beam_butt_plates.cpp

</details>
