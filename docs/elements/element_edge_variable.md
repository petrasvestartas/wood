# Edge Variable {#elements_edge_variable}

[TOC]

A member whose section changes along its axis, lofted through closed station sections.

## Constructors

```cpp
EdgeVariableElement(const Line& axis, const std::vector<Polyline>& sections, const std::string& name = "edge_variable");

static std::shared_ptr<EdgeVariableElement> between(const Polyline& first, const Polyline& last, const std::string& name = "edge_variable");
static std::shared_ptr<EdgeVariableElement> box(const Point& start, const Vector& direction, double length, double width, double height, const std::string& name = "beam");

// Standard pairs for examples and tests
static std::array<std::shared_ptr<EdgeVariableElement>, 2> pair_side_by_side(double length = 1600.0, double width = 200.0, double height = 300.0);
static std::array<std::shared_ptr<EdgeVariableElement>, 2> pair_end_to_end(double length = 1000.0, double width = 200.0, double height = 300.0);
static std::array<std::shared_ptr<EdgeVariableElement>, 2> pair_tee(double length = 600.0, double width = 120.0, double joist_width = 160.0, double height = 300.0);
```

## Parabolic rib

Seven sections along a 3000 mm axis, the depth falling parabolically from 730 mm to 300 mm.

![](elements/element_edge_variable.png)

\include{lineno} elements/element_edge_variable.cpp

## A session of several elements

The joist and the beam of a tee.

![](elements/element_edge_variable_session.png)

\include{lineno} elements/element_edge_variable_session.cpp

## Holes

A hole is a drill, a joint; its interaction bores the rib.

![](elements/element_edge_variable_holes.png)

\include{lineno} elements/element_edge_variable_holes.cpp

## Cut by a plane

The rib trimmed at both ends by two cutting planes, each a joint.

![](elements/element_edge_variable_cut_plane.png)

\include{lineno} elements/element_edge_variable_cut_plane.cpp

## Assign joints

Two ribs side by side, a wedge let into both.

![](elements/element_joint_beam_wedge.png)

\include{lineno} elements/element_joint_beam_wedge.cpp
