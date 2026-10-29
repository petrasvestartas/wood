# Variable Beam {#elements_beam_variable}

[TOC]

A variable-section beam lofted across a sequence of closed station polylines along a reference axis.

## Constructors

```cpp
BeamVariable(const Line& axis, const std::vector<Polyline>& sections, const std::string& name = "beam_variable");
static std::shared_ptr<BeamVariable> between(const Polyline& first, const Polyline& last, const std::string& name = "beam_variable");
static std::shared_ptr<BeamVariable> box(const Point& start, const Vector& direction, double length, double width, double height, const std::string& name = "beam");
static std::array<std::shared_ptr<BeamVariable>, 2> pair_side_by_side(double length = 1600.0, double width = 200.0, double height = 300.0);
static std::array<std::shared_ptr<BeamVariable>, 2> pair_end_to_end(double length = 1000.0, double width = 200.0, double height = 300.0);
static std::array<std::shared_ptr<BeamVariable>, 2> pair_tee(double length = 600.0, double width = 120.0, double joist_width = 160.0, double height = 300.0);
Polyline top() const;
Polyline bottom() const;
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `axis` | — | Straight reference line used to classify end faces and side faces |
| `sections` | — | Ordered closed station loops with equal vertex count lofted from start to end |
| `first`, `last` (`between`) | — | Two end sections for a two-station tapered beam |

## Parabolic rib

Seven 120 mm wide rectangular sections along a 3000 mm axis whose depth tapers parabolically from 730 mm to 300 mm.

![](elements/element_beam_variable.png)

```cpp
const std::shared_ptr<BeamVariable> rib = std::make_shared<BeamVariable>(
    Line::from_points({0.0, 0.0, 0.0}, {3000.0, 0.0, 0.0}), sections, "rib"
);
scene.add(rib);
```

## Cut by plane elements

The same variable-depth rib trimmed at both ends by two inclined `CutPlane` elements via `add_interaction`.

![](elements/element_beam_variable_cut.png)

```cpp
const std::shared_ptr<CutPlane> start = std::make_shared<CutPlane>(
    Plane::from_point_normal({250.0, 0.0, -300.0}, {1.0, 0.0, 0.5}), 900.0, "start"
);
scene.add_interaction(start, rib, start->feature());
```
