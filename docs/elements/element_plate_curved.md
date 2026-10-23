# PlateCurved {#elements_plate_curved}

[TOC]

A curved plate: the strip between two meridians, a closed solid of six exact NURBS patches, its outer and its inner face ruled between the outer and the inner pair of meridians, a side face along each meridian and a cap at each end; two plates built on the same meridian share their side face, as mitered planar plates do.

## Constructors

```cpp
PlateCurved(const NurbsCurve& outer_a, const NurbsCurve& outer_b, const NurbsCurve& inner_a, const NurbsCurve& inner_b, const std::string& name = "plate_curved")
```

## Parameters

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `outer_a`, `outer_b` | | the two edges of the outer face, the ruled surface between them |
| `inner_a`, `inner_b` | | the two edges of the inner face, under `outer_a` and `outer_b`; their distance from the outer ones is the thickness |

All four curves run the same way; the caps join their starts and their ends.

## A strip of a barrel vault

![A strip of a barrel vault](elements/element_plate_curved.png)

Two quarter arcs of radius 2000, 600 apart, over two of radius 1880: a curved plate 120 thick, capped at the springing and at the crown.

\include{lineno} elements/element_plate_curved.cpp
