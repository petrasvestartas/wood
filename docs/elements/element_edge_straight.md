# Edge Straight {#elements_edge_straight}

[TOC]

A straight timber member along an axis: a beam, or a column when it stands. Its section is a square, a profile, a world loop or a circle; holes, cuts and joints are interactions, not parts of the class.

## Constructors

```cpp
EdgeStraightElement(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions = {}, const std::string& name = "beam");

static std::shared_ptr<EdgeStraightElement> square(const Polyline& axis, double radius, const std::string& name = "beam");
static std::shared_ptr<EdgeStraightElement> corner_square(const Line& axis, const Plane& corner, double side, const std::string& name = "column");
static std::shared_ptr<EdgeStraightElement> from_section(const Line& axis, const Polyline& section, const std::string& name = "column");
static std::shared_ptr<EdgeStraightElement> round(const Line& axis, double radius, double chord_tolerance = 0.02, const std::string& name = "column");

// Standard pairs for examples and tests
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0);
```

## A profile along an axis

A 120 × 200 mm rectangular profile swept along a straight axis.

![](elements/element_edge_straight.png)

\include{lineno} elements/element_edge_straight.cpp

## Sections

Every profile, each swept along a short edge: rectangle, round, W, hollow, slab band and T. A profile is loops in the section frame, loop 0 the outline, then holes.

![](elements/element_edge_straight_sections.png)

\include{lineno} elements/element_edge_straight_sections.cpp

## A column

A 200 × 300 mm section swept up a 3500 mm vertical axis. A column is an edge named for what it does; nothing in the class says column.

![](elements/element_edge_straight_column.png)

\include{lineno} elements/element_edge_straight_column.cpp

## A column with its head and seat

A floor corner column: glued head blocks, a steel support and pocket cuts, each an interaction on the same edge.

![](elements/element_edge_straight_column_session.png)

\include{lineno} elements/element_edge_straight_column_session.cpp

## A session of several elements

Two columns and the beam on them, each added to its group.

![](elements/element_edge_straight_session.png)

\include{lineno} elements/element_edge_straight_session.cpp

## Holes

A hole is a drill, a joint; its interaction bores the edge.

![](elements/element_edge_straight_holes.png)

\include{lineno} elements/element_edge_straight_holes.cpp

## Cut by a plane

A cutting plane is a joint; the edge keeps the side its normal points to.

![](elements/element_edge_straight_cut_plane.png)

\include{lineno} elements/element_edge_straight_cut_plane.cpp

## Assign joints

A joint computes itself from its two edges and goes on each with `add_interaction`.

![](elements/element_joint_beam_cross_half_lap.png)

\include{lineno} elements/element_joint_beam_cross_half_lap.cpp
