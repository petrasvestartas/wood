# Pin {#elements_pin}

[TOC]

A cylindrical dowel, bolt, or screw element along a straight centreline, created standalone or nested as a child element inside [JointBeam](@ref elements_joint_beam) connectors.

## Constructors

```cpp
JointPinElement(const Line& axis, double radius, double chord_tolerance = 0.05);
const Line& axis() const;
```

## Along an axis

A 10 mm radius cylindrical pin along a 200 mm vertical axis.

![](elements/element_pin.png)

\include{lineno} elements/element_pin.cpp
