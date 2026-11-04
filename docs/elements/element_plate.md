# Plate {#elements_plate}

[TOC]

A flat timber plate bounded by a bottom and a top polygon with one side face per edge; joints modify it through `InteractionFeaturePlate` or `InteractionFeatureSolid`. Once its joints are merged, its trimmed mesh is the merged loops triangulated as a polygon with holes on the bottom and the top, joined by a quad wall on every loop edge: one closed mesh, usable as a solid.

## Constructors

```cpp
FacePlanarElement(const Polyline& bottom, const Polyline& top, const std::string& name = "plate");
static std::shared_ptr<FacePlanarElement> from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, double thickness, const std::string& name = "plate");
static std::shared_ptr<FacePlanarElement> from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");
static std::vector<std::shared_ptr<FacePlanarElement>> row_between(const std::array<Polyline, 2>& bottom, const std::array<Polyline, 2>& top, const std::string& name = "plates");

// Standard plate pairs for examples and tests
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_in_plane(double angle_degrees = 180.0, double length = 300.0, double width = 400.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_folded(double angle_degrees = 90.0, double length = 300.0, double width = 400.0, double height = 300.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_rotated(double twist_degrees = 10.0, double length = 300.0, double width = 400.0, double thickness = 100.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_top_side(double angle_degrees = 90.0, double base_size = 400.0, double upright_size = 250.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_crossing(double angle_degrees = 90.0, double length = 400.0, double height = 200.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_stacked(double twist_degrees = 0.0, double length = 400.0, double width = 300.0, double thickness = 40.0);
```

## From two polylines

A four-sided plate lofted between a bottom polygon and the same polygon lifted 40 mm.

![](elements/element_plate.png)

\include{lineno} elements/element_plate.cpp

## With holes

A rectangular plate with four vertical through-holes, each a `JointElement::drill` added as an interaction.

![](elements/element_plate_holes.png)

\include{lineno} elements/element_plate_holes.cpp

## Lofted between two rails

A row of bed plates made between a near and a far parabolic rail with `FacePlanarElement::row_between`.

![](elements/element_plate_session.png)

\include{lineno} elements/element_plate_session.cpp

## Joint types by points and lines

Eight box plates whose contact joint types and insertion directions are set by points and lines.

![](elements/element_plate_assign_joints.png)

\include{lineno} elements/element_plate_assign_joints.cpp
