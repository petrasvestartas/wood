# JointBeam {#elements_joint_beam}

[TOC]

Parametric beam and member joinery covering axis-contact feature volumes (`from_contact`), automatic plane butts (`butt`), and face-contact connectors (`wedge`, `rectangle_plate`, `butterfly_key`, `centred_pins`, `headed_pins`).

![](diagrams/wood_beam_joints_connectors.webp)

## Constructors

```cpp
// Beam-to-beam on axis contact
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings());

// Automatic or forced plane butts (beams, columns, or plates)
static std::shared_ptr<JointBeam> butt(const Element& a, const Element& b, double reach = 0.0);
static std::shared_ptr<JointBeam> butt_side_side(const Element& a, const Element& b, double reach = 0.0);
static std::shared_ptr<JointBeam> butt_top_side(const Element& top, const Element& side, double reach = 0.0);

// Face-contact connectors
static std::shared_ptr<JointBeam> wedge(const Element& a, const Element& b, const InteractionContactFace& contact, double length_margin, double pocket_depth, ...);
static std::shared_ptr<Plate> let_in_plate(const Element& rib, const InteractionContactFace& contact, double width = 30.0, double back = 220.0, double front = 265.0, double height = 250.0);
static std::shared_ptr<JointBeam> rectangle_plate(const Element& column, const Element& rib, const Plate& plate, const InteractionContactFace& contact, double pin_radius = 25.0, ...);
static std::shared_ptr<JointBeam> butterfly_key(const Element& a, const Element& b, const InteractionContactFace& contact, ...);
static std::shared_ptr<JointBeam> centred_pins(const Element& a, const Element& b, const InteractionContactFace& contact, double radius = 4.0, double length = 30.0, double offset = 50.0, ...);
static std::shared_ptr<JointBeam> headed_pins(const Element& through, const Element& into, const InteractionContactFace& contact, PinLayout layout, size_t count = 2, double offset = 20.0, double shift = 0.0, double radius = 2.0, double length = 200.0, int sides = 16);

std::shared_ptr<Interaction> interaction(size_t target) const;
std::vector<std::shared_ptr<Joint>> children() const;
```

## Usage

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
const std::shared_ptr<JointBeam> wedge = JointBeam::wedge(*left, *right, *contact, 300.0, 133.0);
scene.add(wedge);
scene.add_interaction(wedge, left, wedge->interaction(0));
scene.add_interaction(wedge, right, wedge->interaction(1));
```

## Contact Joint Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `volume_length` | — | Feature box length along each beam axis from the contact point |
| `cross_or_side_to_end` | — | Axis threshold separating interior crossings from side-to-end and end-to-end contacts |
| `flip_male` | `0` | Rotates which feature volume corners are assigned male |

## Families

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="elements_joint_beam_cross.html">
    <img src="element_joint_beam_cross.png" alt="JointBeam cross">
    <span class="wood-card-title">cross</span>
    <span class="wood-card-sub">Crossing beam half-laps &amp; slots</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_top_side.html">
    <img src="element_joint_beam_top_side.png" alt="JointBeam top_side">
    <span class="wood-card-title">top_side</span>
    <span class="wood-card-sub">Beam ending on another member (`top_side_milled_wedge`)</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_end_to_end.html">
    <img src="element_joint_beam_end_to_end.png" alt="JointBeam end to end">
    <span class="wood-card-title">end to end</span>
    <span class="wood-card-sub">Splayed scarfs, miter scarfs &amp; Jupiter scarfs</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_butt.html">
    <img src="element_joint_beam_butt.png" alt="JointBeam butt">
    <span class="wood-card-title">butt</span>
    <span class="wood-card-sub">Automatic or forced plane butts (`butt`, `butt_side_side`, `butt_top_side`)</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_connectors.html">
    <img src="element_joint_beam_connectors.png" alt="JointBeam connectors">
    <span class="wood-card-title">connectors</span>
    <span class="wood-card-sub">Face-contact wedges, let-in plates, rectangle plates &amp; butterfly keys</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_pins.html">
    <img src="element_joint_beam_pins.png" alt="JointBeam pins">
    <span class="wood-card-title">pins</span>
    <span class="wood-card-sub">Centred pins &amp; headed pins across `InteractionContactFace`</span>
  </a>
</div>

| Page | Contact Input | Presets and Factories |
| --- | --- | --- |
| [cross](@ref elements_joint_beam_cross) | Two beams crossing | `cross_slot` .. `cross_brussels_half_lap` |
| [top_side](@ref elements_joint_beam_top_side) | Beam ending on another | `top_side_milled_wedge` |
| [end to end](@ref elements_joint_beam_end_to_end) | Two collinear or angled beams meeting at ends | `side_side_inplane_splayed_scarf`, `_cross_splayed_scarf`, `_side_side_rotated_miter`, `_jupiter_scarf` |
| [butt](@ref elements_joint_beam_butt) | Any two members (beams, columns, plates) | `butt`, `butt_side_side`, `butt_top_side` |
| [connectors](@ref elements_joint_beam_connectors) | `InteractionContactFace` | `wedge`, `let_in_plate`, `rectangle_plate`, `butterfly_key` |
| [pins](@ref elements_joint_beam_pins) | `InteractionContactFace` | `centred_pins`, `headed_pins` |
