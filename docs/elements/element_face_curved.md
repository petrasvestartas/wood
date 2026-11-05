# Face Curved {#elements_face_curved}

[TOC]

A closed curved plate of six NURBS patches, bounded by four meridians.

## Constructors

```cpp
FaceCurvedElement(const NurbsCurve& outer_a, const NurbsCurve& outer_b, const NurbsCurve& inner_a, const NurbsCurve& inner_b, const std::string& name = "plate_curved");
```

## Barrel vault strip

A 120 mm thick strip between two outer quarter arcs of radius 2000 mm, 600 mm apart, and two inner arcs of radius 1880 mm.

![](elements/element_face_curved.png)

\include{lineno} elements/element_face_curved.cpp

## A session of several elements

Three strips of a barrel vault side by side.

![](elements/element_face_curved_session.png)

\include{lineno} elements/element_face_curved_session.cpp
