# Beam {#elements_beam}

[TOC]

A beam on a polyline axis: a square of a radius, or a profile swept along it, segment by segment.

## Constructors

```cpp
Beam(const Polyline& axis, double radius, const std::string& name = "beam")
Beam(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions = {}, const std::string& name = "beam")
```

## A profile along an axis

![A profile along an axis](elements/element_beam.png)

A 120 x 200 rectangle swept along a two-segment axis; the axis and the sections are its features.

\include{lineno} elements/element_beam.cpp
