# Pin {#elements_pin}

[TOC]

A cylindrical dowel, bolt, or screw element along a straight centreline, created standalone or nested as a child element inside [JointBeam](@ref elements_joint_beam) connectors.

## Constructors

```cpp
Pin(const Line& axis, double radius, double chord_tolerance = 0.05);
const Line& axis() const;
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `axis` | — | Start-to-end centreline defining position, direction, and length |
| `radius` | — | Cylinder radius in mm |
| `chord_tolerance` | `0.05` | Maximum chord deviation of the polygon facets from the true circle |

## Along an axis

A 10 mm radius cylindrical pin along a 200 mm vertical axis.

![](elements/element_pin.png)

```cpp
scene.add(std::make_shared<Pin>(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 200.0}), 10.0, 0.05));
```

<details><summary>Example code: elements/element_pin.cpp</summary>

\include{lineno} elements/element_pin.cpp

</details>
