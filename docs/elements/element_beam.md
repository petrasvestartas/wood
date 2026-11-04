# Beam {#elements_beam}

[TOC]

A straight timber beam along its axis, with either a square half-width (`radius`) or a section profile (`profile`). For a curved member see [Curved Beam](@ref elements_beam_curved), for a varying section [Variable Beam](@ref elements_beam_variable).

## Constructors

```cpp
EdgeStraightElement(const Polyline& axis, double radius, const std::string& name = "beam");
EdgeStraightElement(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions = {}, const std::string& name = "beam");

// Standard beam pairs for examples and tests
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0);
```

## A profile along an axis

A 120 × 200 mm rectangular profile swept along a straight axis.

![](elements/element_beam.png)

\include{lineno} elements/element_beam.cpp
