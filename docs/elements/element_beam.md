# Beam {#elements_beam}

[TOC]

A beam on a polyline axis: a square of a radius, or a profile swept along it, segment by segment.

## Constructors

```cpp
Beam(const Polyline& axis, double radius, const std::string& name = "beam")
Beam(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions = {}, const std::string& name = "beam")
```

## Parameters

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `axis` | | the centreline; each segment is a straight span, its corners kinks |
| `radius` | | the half-width of the square section, when no profile is given |
| `profile` | square of the radius | the section loops swept along the axis (@ref elements_profile) |
| `directions` | the contact normal | the section's up direction per segment, turning it about the axis |

## A profile along an axis

![A profile along an axis](elements/element_beam.png)

A 120 x 200 rectangle swept along a two-segment axis; the axis and the sections are its features.

\include{lineno} elements/element_beam.cpp
