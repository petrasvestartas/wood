# JointBeam {#elements_joint_beam}

[TOC]

A joint between beams or members: the feature volumes of a beam-to-beam joint, or a connector with its own parts, cutters and pins.

## Constructors

```cpp
// beam to beam, on the axis contact of two beams
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0)

// connectors, on the face contact of two members
static std::shared_ptr<JointBeam> wedge(const Element& a, const Element& b, const InteractionContactFace& contact, double length_margin, double pocket_depth, ...)
static std::shared_ptr<Plate> let_in_plate(const Element& rib, const InteractionContactFace& contact, double width = 30.0, double back = 220.0, double front = 265.0, double height = 250.0)
static std::shared_ptr<JointBeam> rectangle_plate(const Element& column, const Element& rib, const Plate& plate, const InteractionContactFace& contact, double pin_length, ...)
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

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `volume_length` | | how long the feature volumes run along each beam from the contact |
| `cross_or_side_to_end` | | where on the axes a contact stops being a crossing and becomes an end: a crossing, a side-to-end or an end-to-end |
| `flip_male` | 0 | turns which corners of the volumes are male |

\include{lineno} elements/element_joint_beam_from_contact.cpp

## Wedge

![Wedge](elements/element_joint_beam_wedge.png)

A wedge between two beams side by side, pinned across, the right beam moved off to show its pocket.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `length_margin` | | how far short of each end of the contact's top edge the wedge stops |
| `pocket_depth` | | how deep the pocket under each slanted face cuts into each member |
| `end` | none | a plane the wedge runs on to instead of stopping short |
| `pin_radius`, `pin_spacing`, `pin_sides` | 10, 320, 8 | the pins across the wedge: thickness, distance, facets |
| `overshoot` | 20 | how far the holes run past the pins |
| `profile` | `WEDGE_PROFILE` | the wedge's section: apex 197 below the top edge, 63.5 wide at the top |
| `pin_offset` | 80, -100 | where the first pin stands along and below the top edge |

\include{lineno} elements/element_joint_beam_wedge.cpp

## Let-in plate

![Let-in plate](elements/element_joint_beam_rectangle_plate.png)

A plate let into a column and the rib ending on it, two pins in each, each through its member and the plate.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `width` | 30 | the plate's thickness |
| `back`, `front` | 220, 265 | how far the plate runs into the column and into the rib |
| `height` | 250 | how far the plate runs down from the contact's top edge |
| `pin_length`, `pin_radius` | , 25 | the two pins in the column and the two in the rib, each through the plate too |
| `margin_x`, `margin_z` | 6.05, 3 | the pocket's clearance around the plate |
| `overshoot`, `pin_sides` | 25, 16 | how far the holes run past the pins, their facets |

\include{lineno} elements/element_joint_beam_rectangle_plate.cpp

## Tie

![Tie](elements/element_joint_beam_tie.png)

A bow-tie key across two beams end to end, the second moved off to show its pocket.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `top` | 138.5 | how far below the contact's top edge the key starts |
| `length` | 800 | the key's length across the joint |
| `head_length`, `head_width`, `neck_width` | 200, 40, 20 | the bow-tie outline: its two heads and the neck between them |
| `depth`, `end_depth` | 58.5, 70.4 | the key's depth at the middle and at its ends |
| `pocket_depth`, `overshoot` | 80, 10 | the pockets in the members |

\include{lineno} elements/element_joint_beam_tie.cpp

## Centred pins

![Centred pins](elements/element_joint_beam_centred_pins.png)

Four pins at the inset corners of the contact of two stacked blocks, half in each.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `radius`, `length` | 4, 30 | each pin |
| `offset` | 50 | how far the pins stand in from the contact's corners |
| `overshoot`, `pin_sides` | 10, 16 | how far the holes run past the pins, their facets |

\include{lineno} elements/element_joint_beam_centred_pins.cpp

## Headed pins

![Headed pins](elements/element_joint_beam_headed_pins.png)

Pins from a beam's far face into the joist ending on it, in each `PinLayout`: corners, a vertical column, a horizontal row.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `layout` | | `corners`, `vertical` or `horizontal` |
| `count` | 2 | pins in a column or row |
| `offset` | 20 | how far the pins stand in from the contact's edges |
| `shift` | 0 | moves the pins along the contact, so two connectors' heads stay apart |
| `radius`, `length`, `sides` | 2, 200, 16 | each pin |

\include{lineno} elements/element_joint_beam_headed_pins.cpp
