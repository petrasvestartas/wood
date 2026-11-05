# Joint Pin {#elements_joint_pin}

[TOC]

One cylinder along its axis: every screw, dowel or pin is a pin, alone or a child of its connector.

## Constructors

```cpp
JointPinElement(const Line& axis, double radius, double chord_tolerance = 0.05);

const Line& axis() const;
```

## Along an axis

A 10 mm radius pin along a 200 mm vertical axis.

![](elements/element_joint_pin.png)

\include{lineno} elements/element_joint_pin.cpp
