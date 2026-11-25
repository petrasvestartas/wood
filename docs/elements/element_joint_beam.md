# Beam Joints {#elements_joint_beam}

[TOC]

Parametric beam and member joinery covering axis-contact feature volumes, automatic plane butts, and face-contact connectors.

![](diagrams/wood_beam_joints_connectors.webp)

## Families

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint_beam_cross.html">
    <img src="assembly_element_joint_beam_cross_half_lap.gif" alt="Beam Cross">
    <span class="wood-card-title">Cross</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_top_side.html">
    <img src="assembly_element_joint_beam_top_side_milled_wedge.gif" alt="Beam Top-to-Side">
    <span class="wood-card-title">Top-to-Side</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_end_to_end.html">
    <img src="assembly_element_joint_beam_jupiter_scarf.gif" alt="Beam End-to-End">
    <span class="wood-card-title">End-to-End</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_butt.html">
    <img src="assembly_element_joint_beam_butt.gif" alt="Beam Butt">
    <span class="wood-card-title">Butt</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_connectors.html">
    <img src="assembly_element_joint_beam_butterfly_key.gif" alt="Beam Connectors">
    <span class="wood-card-title">Connectors</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_pins.html">
    <img src="assembly_element_joint_beam_headed_pins.gif" alt="Beam Pins">
    <span class="wood-card-title">Pins</span>
  </a>
</div>

## Constructors

```cpp
static std::shared_ptr<JointBeamElement> from_contact(const EdgeStraightElement& source, const EdgeStraightElement& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings());

static std::shared_ptr<JointBeamElement> butt(const Element& a, const Element& b, double reach = 0.0);
static std::shared_ptr<JointBeamElement> butt_side_side(const Element& a, const Element& b, double reach = 0.0);
static std::shared_ptr<JointBeamElement> butt_top_side(const Element& top, const Element& side, double reach = 0.0);

static std::shared_ptr<JointBeamElement> wedge(const Element& a, const Element& b, const InteractionContactFace& contact, double length_margin, double pocket_depth, ...);
static std::shared_ptr<FacePlanarElement> let_in_plate(const Element& rib, const InteractionContactFace& contact, double width = 30.0, double back = 220.0, double front = 265.0, double height = 250.0);
static std::shared_ptr<JointBeamElement> rectangle_plate(const Element& column, const Element& rib, const FacePlanarElement& plate, const InteractionContactFace& contact, double pin_radius = 25.0, ...);
static std::shared_ptr<FacePlanarElement> node_plate(const std::vector<const EdgeStraightElement*>& members, const Point& node, const Vector& normal, const std::vector<double>& reaches, double thickness, double inset = 15.0, const std::string& name = "node_plate");
static std::shared_ptr<JointBeamElement> slotted_plate(const std::vector<const EdgeStraightElement*>& members, const FacePlanarElement& plate, const Point& node, const Vector& way, const std::vector<double>& clear, double pin_radius = 6.0, int pin_sides = 16);
static std::shared_ptr<JointBeamElement> butterfly_key(const Element& a, const Element& b, const InteractionContactFace& contact, ...);
static std::shared_ptr<JointBeamElement> centred_pins(const Element& a, const Element& b, const InteractionContactFace& contact, double radius = 4.0, double length = 30.0, double offset = 50.0, ...);
static std::shared_ptr<JointBeamElement> skewed_pins(const Element& a, const Element& b, const InteractionContactFace& contact, double radius = 4.0, double depth = 60.0, double lean = 45.0, double offset = 20.0, ...);
static std::shared_ptr<JointBeamElement> headed_pins(const Element& through, const Element& into, const InteractionContactFace& contact, PinLayout layout, size_t count = 2, double offset = 20.0, double shift = 0.0, double radius = 2.0, double length = 200.0, int sides = 16);

std::shared_ptr<Interaction> interaction(size_t target) const;
std::vector<std::shared_ptr<JointElement>> children() const;
```

## Wedge

A wedge between two beams that meet face to face: the joint is added, then given to each beam.

| assembly | joint | tile |
|---|---|---|
| ![](joint_library/assembly_element_joint_beam_wedge.gif) | ![](joint_library/assembly_element_joint_beam_wedge.png) |  |

\include{lineno} elements/element_joint_beam_wedge.cpp

