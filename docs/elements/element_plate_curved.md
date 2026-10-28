# PlateCurved {#elements_plate_curved}

[TOC]

Closed 6-patch NURBS solid strip (`PlateCurved`) bounded by four `NurbsCurve` meridians (`outer_a`, `outer_b`, `inner_a`, `inner_b`). Two adjacent curved plates sharing a meridian pair share an exact ruled miter face.

## Constructors

```cpp
PlateCurved(
    const NurbsCurve& outer_a,
    const NurbsCurve& outer_b,
    const NurbsCurve& inner_a,
    const NurbsCurve& inner_b,
    const std::string& name = "plate_curved"
)
```

## Parameters

| Parameter | Role |
| --- | --- |
| `outer_a`, `outer_b` | Outer meridian curves spanning the outer ruled `NurbsSurface` face |
| `inner_a`, `inner_b` | Inner meridian curves offset by plate thickness, spanning the inner ruled `NurbsSurface` face |

## Barrel Vault Strip

![A strip of a barrel vault](elements/element_plate_curved.png)

Two outer quarter-arcs (`R = 2000 mm`, `600 mm` apart) over two inner quarter-arcs (`R = 1880 mm`): `120 mm` curved plate capped at springing and crown.

\include{lineno} elements/element_plate_curved.cpp
