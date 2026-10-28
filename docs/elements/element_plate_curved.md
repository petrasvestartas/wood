# PlateCurved {#elements_plate_curved}

[TOC]

A closed 6-patch NURBS solid strip bounded by four `NurbsCurve` meridians (`outer_a`, `outer_b`, `inner_a`, `inner_b`). Adjacent curved plates that share a meridian pair meet along an exact ruled miter face.

## Constructors

```cpp
PlateCurved(
    const NurbsCurve& outer_a,
    const NurbsCurve& outer_b,
    const NurbsCurve& inner_a,
    const NurbsCurve& inner_b,
    const std::string& name = "plate_curved"
);
```

## Parameters

| Parameter | Role |
| --- | --- |
| `outer_a`, `outer_b` | Outer meridian curves spanning the outer ruled `NurbsSurface` face |
| `inner_a`, `inner_b` | Inner meridian curves offset by plate thickness, spanning the inner ruled `NurbsSurface` face |

## Barrel Vault Strip

A 120 mm thick barrel-vault strip bounded by two outer quarter-arcs (`R = 2000 mm`, `600 mm` apart) and two inner quarter-arcs (`R = 1880 mm`).

![](elements/element_plate_curved.png)

```cpp
const std::shared_ptr<PlateCurved> plate = std::make_shared<PlateCurved>(
    NurbsCurve::create_interpolated(arc(2000.0, 0.0)),
    NurbsCurve::create_interpolated(arc(2000.0, 600.0)),
    NurbsCurve::create_interpolated(arc(1880.0, 0.0)),
    NurbsCurve::create_interpolated(arc(1880.0, 600.0))
);
scene.add(plate);
```

<details><summary>Example code: elements/element_plate_curved.cpp</summary>

\include{lineno} elements/element_plate_curved.cpp

</details>
