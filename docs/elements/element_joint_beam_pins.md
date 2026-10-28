# JointBeam pins {#elements_joint_beam_pins}

[TOC]

Dowel and screw pin connectors placed across the `InteractionContactFace` of two members according to a `PinLayout`, boring into both targets and nesting [`Pin`](@ref elements_pin) child elements.

![](diagrams/wood_beam_joints_connectors.webp)

## Centred pins

Four dowel pins placed at the inset corners of the contact face between two stacked blocks, embedded half into each block.

![](elements/element_joint_beam_centred_pins.png)

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
const std::shared_ptr<JointBeam> pins = JointBeam::centred_pins(*lower, *upper, *contact);
scene.add(pins);
scene.add_interaction(pins, lower, pins->interaction(0));
scene.add_interaction(pins, upper, pins->interaction(1));
```

| Parameter | Default | Role |
| --- | --- | --- |
| `radius`, `length` | `4`, `30` | Radius and total length of each dowel pin |
| `offset` | `50` | Inset distance from the contact polygon corners |
| `overshoot`, `pin_sides` | `10`, `16` | Extra bore depth past each pin end and cylindrical facet count |

<details><summary>Example code: elements/element_joint_beam_centred_pins.cpp</summary>

\include{lineno} elements/element_joint_beam_centred_pins.cpp

</details>

## Headed pins

Long screws driven from a header beam's outer face through into an abutting joist across all three `PinLayout` modes: four `corners`, a `vertical` column, and a `horizontal` row.

![](elements/element_joint_beam_headed_pins.png)

```cpp
const std::shared_ptr<JointBeam> pins = JointBeam::headed_pins(
    *beam, *joist, *contact, PinLayout::horizontal, 3
);
scene.add(pins);
scene.add_interaction(pins, beam, pins->interaction(0));
scene.add_interaction(pins, joist, pins->interaction(1));
```

| Parameter | Default | Role |
| --- | --- | --- |
| `layout` | — | `PinLayout::corners`, `PinLayout::vertical`, or `PinLayout::horizontal` |
| `count` | `2` | Number of pins in a `vertical` column or `horizontal` row |
| `offset` | `20` | Edge inset distance from the contact boundary |
| `shift` | `0` | Offset along the contact to stagger opposing connectors |
| `radius`, `length`, `sides` | `2`, `200`, `16` | Pin radius, total length, and polygon facet count |

<details><summary>Example code: elements/element_joint_beam_headed_pins.cpp</summary>

\include{lineno} elements/element_joint_beam_headed_pins.cpp

</details>
