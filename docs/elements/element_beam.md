# Beam {#elements_beam}

[TOC]

A timber beam swept along a polyline centreline using either a square half-width (`radius`) or a section profile (`profile`), mitered cleanly at each interior kink.

## Constructors

```cpp
Beam(const Polyline& axis, double radius, const std::string& name = "beam");
Beam(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions = {}, const std::string& name = "beam");

// Standard beam pairs for examples and tests
static std::array<std::shared_ptr<Beam>, 2> pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::array<std::shared_ptr<Beam>, 2> pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::array<std::shared_ptr<Beam>, 2> pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0);
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `axis` | — | Centreline polyline; each segment is a straight span mitered at interior vertices |
| `radius` | — | Half-width of the square cross-section when no `profile` is given |
| `profile` | square of `radius` | Section loops swept along the axis ([Profiles](@ref elements_profile)) |
| `directions` | contact normal | Section up-vector per segment, orienting the profile around the axis |
| `extension` | `0` | Extra stock length past each axis endpoint for end-to-end scarf or miter cuts |

## A profile along an axis

A 120 × 200 mm rectangular profile swept along a two-segment kinked polyline axis.

![](elements/element_beam.png)

```cpp
const Polyline axis({{0.0, 0.0, 0.0}, {600.0, 0.0, 0.0}, {1000.0, 300.0, 0.0}});
const Polyline profile = Polyline::rectangle({-60.0, -100.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 120.0, 200.0);
scene.add(std::make_shared<Beam>(axis, std::vector<Polyline>{profile}, std::vector<Vector>{}, "beam"));
```

<details><summary>Example code: elements/element_beam.cpp</summary>

\include{lineno} elements/element_beam.cpp

</details>
