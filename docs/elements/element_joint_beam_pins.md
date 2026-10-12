# JointBeam pins {#elements_joint_beam_pins}

[TOC]

Pins on the face contact of two members, laid out by `PinLayout`, each a `Pin` bored into both.

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
