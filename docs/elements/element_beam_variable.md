# BeamVariable {#elements_beam_variable}

[TOC]

A beam whose section changes along its axis, lofted between one section per station.

## Constructors

```cpp
BeamVariable(const Line& axis, const std::vector<Polyline>& sections, const std::string& name = "beam_variable")
static std::shared_ptr<BeamVariable> between(const Polyline& first, const Polyline& last, const std::string& name = "beam_variable")
static std::shared_ptr<BeamVariable> box(const Point& start, const Vector& direction, double length, double width, double height, const std::string& name = "beam")
static std::array<std::shared_ptr<BeamVariable>, 2> pair_side_by_side(double length = 1600.0, double width = 200.0, double height = 300.0)
static std::array<std::shared_ptr<BeamVariable>, 2> pair_end_to_end(double length = 1000.0, double width = 200.0, double height = 300.0)
static std::array<std::shared_ptr<BeamVariable>, 2> pair_tee(double length = 600.0, double width = 120.0, double joist_width = 160.0, double height = 300.0)
Polyline top() const
Polyline bottom() const
```

## Parameters

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `axis` | | the straight reference line; contacts tell end faces from side faces by it |
| `sections` | | one closed ring per station, all with one point count, lofted in order: a deeper ring makes the beam deeper there |
| `first`, `last` (`between`) | | a straight beam lofted between two end sections |

## Like the outer rib

![Like the outer rib](elements/element_beam_variable.png)

Seven 120 wide sections on a 3000 axis, 730 deep at the start and 300 at the end on a parabola.

\include{lineno} elements/element_beam_variable.cpp

## Cut by plane elements

![Cut by plane elements](elements/element_beam_variable_cut.png)

The same rib cut at both ends by two `CutPlane` elements through `add_interaction(plane, rib, plane->feature())`.

\include{lineno} elements/element_beam_variable_cut.cpp
