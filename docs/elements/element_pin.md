# Pin {#elements_pin}

[TOC]

A pin: one cylinder along its axis, every pin, screw or dowel of a connector, nested under it.

## Constructors

```cpp
Pin(const Line& axis, double radius, double chord_tolerance)
const Line& axis() const
```

## Along an axis

![Along an axis](elements/element_pin.png)

A pin 10 in radius along a 200 axis; connectors make theirs with `JointBeam::centred_pins` and `JointBeam::headed_pins` (@ref elements_joint_beam).

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `axis` | | where the pin stands and how long it is |
| `radius` | | its thickness |
| `chord_tolerance` | | how far a facet may stray from the circle |

\include{lineno} elements/element_pin.cpp
