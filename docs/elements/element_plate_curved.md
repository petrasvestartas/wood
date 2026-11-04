# Curved Plate {#elements_plate_curved}

[TOC]

A closed 6-patch NURBS solid strip bounded by four `NurbsCurve` meridians (`outer_a`, `outer_b`, `inner_a`, `inner_b`). Adjacent curved plates that share a meridian pair meet along an exact ruled miter face.

## Constructors

```cpp
FaceCurvedElement(
    const NurbsCurve& outer_a,
    const NurbsCurve& outer_b,
    const NurbsCurve& inner_a,
    const NurbsCurve& inner_b,
    const std::string& name = "plate_curved"
);
```

## Barrel Vault Strip

A 120 mm thick barrel-vault strip bounded by two outer quarter-arcs (`R = 2000 mm`, `600 mm` apart) and two inner quarter-arcs (`R = 1880 mm`).

![](elements/element_plate_curved.png)

\include{lineno} elements/element_plate_curved.cpp
