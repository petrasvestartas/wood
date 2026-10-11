# Beam {#elements_beam}

[TOC]

A beam on a polyline axis: a square of a radius, or a profile swept along it, segment by segment.

## Constructors

```cpp
Beam(const Polyline& axis, double radius, const std::string& name = "beam")
Beam(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions = {}, const std::string& name = "beam")

// pairs, one per joint family's configuration, for examples and tests
static std::array<std::shared_ptr<Beam>, 2> pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0)   // cr_c_ip: crossing at their middles
static std::array<std::shared_ptr<Beam>, 2> pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0)        // ts_e_p: the second ending on the first
static std::array<std::shared_ptr<Beam>, 2> pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0)   // splice or mitre at a shared axis end
```

## Parameters

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `axis` | | the centreline; each segment is a straight span, its corners kinks |
| `radius` | | the half-width of the square section, when no profile is given |
| `profile` | square of the radius | the section loops swept along the axis (@ref elements_profile) |
| `directions` | the contact normal | the section's up direction per segment, turning it about the axis |
| `extension` | 0 | how far the solid runs past each axis end: the stock an end joint at the axis end cuts back |

## A profile along an axis

![A profile along an axis](elements/element_beam.png)

A 120 x 200 rectangle swept along a two-segment axis; the axis and the sections are its features.

\include{lineno} elements/element_beam.cpp
