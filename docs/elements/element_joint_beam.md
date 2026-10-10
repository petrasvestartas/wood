# JointBeam {#elements_joint_beam}

[TOC]

A joint between beams or members: the feature volumes of a beam-to-beam joint, or a connector with its own parts, cutters and pins.

## Constructors

```cpp
// beam to beam, on the axis contact of two beams
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())

// connectors, on the face contact of two members
static std::shared_ptr<JointBeam> wedge(const Element& a, const Element& b, const InteractionContactFace& contact, double length_margin, double pocket_depth, ...)
static std::shared_ptr<Plate> let_in_plate(const Element& rib, const InteractionContactFace& contact, double width = 30.0, double back = 220.0, double front = 265.0, double height = 250.0)
static std::shared_ptr<JointBeam> rectangle_plate(const Element& column, const Element& rib, const Plate& plate, const InteractionContactFace& contact, double pin_radius = 25.0, ...)   // each pin across its member face to face
static std::shared_ptr<JointBeam> tie(const Element& a, const Element& b, const InteractionContactFace& contact, ...)
static std::shared_ptr<JointBeam> centred_pins(const Element& a, const Element& b, const InteractionContactFace& contact, double radius = 4.0, double length = 30.0, double offset = 50.0, ...)
static std::shared_ptr<JointBeam> headed_pins(const Element& through, const Element& into, const InteractionContactFace& contact, PinLayout layout, size_t count = 2, double offset = 20.0, double shift = 0.0, double radius = 2.0, double length = 200.0, int sides = 16)

std::shared_ptr<Interaction> interaction(size_t target) const
std::vector<std::shared_ptr<Joint>> children() const
```

## How it is used

Every connector is made the same way: from the contact of the members it joins, added, and passed to each member in its target order. Its parts and pins nest under it as `ConnectorPart` and `Pin` elements, in `CONNECTOR_COLOR`:

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
const std::shared_ptr<JointBeam> wedge = JointBeam::wedge(*left, *right, *contact, 300.0, 133.0);
scene.add(wedge);
scene.add_interaction(wedge, left, wedge->interaction(0));
scene.add_interaction(wedge, right, wedge->interaction(1));
```

## Beam to beam

![Beam to beam](elements/element_joint_beam_from_contact.png)

A crossing and a side-to-end of 60 radius beams, each joint's feature volumes from the closest point of their axes.

The interaction keeps the axes' `normal` and their two directions where they meet (`axes`), and `insertion(target)` says how each member comes off the joint: a crossing along the normal away from the other box, a side or end pair along the axis that points apart.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `volume_length` | | how long the feature volumes run along each beam from the contact |
| `cross_or_side_to_end` | | where on the axes a contact stops being a crossing and becomes an end: a crossing, a side-to-end or an end-to-end |
| `flip_male` | 0 | turns which corners of the volumes are male |

\include{lineno} elements/element_joint_beam_from_contact.cpp

## Families

| Page | Contact | Designs |
| --- | --- | --- |
| [cr_c_ip](@ref elements_joint_beam_cr_c_ip) | two beams crossing | `cr_c_ip_0` to `cr_c_ip_5` |
| [ts_e_p](@ref elements_joint_beam_ts_e_p) | a beam ending on another | `ts_e_p_4` |
| [connectors](@ref elements_joint_beam_connectors) | the face contact of two members | `wedge`, `rectangle_plate`, `tie` |
| [pins](@ref elements_joint_beam_pins) | the face contact of two members | `centred_pins`, `headed_pins` |
