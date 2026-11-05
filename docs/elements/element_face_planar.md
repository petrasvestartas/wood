# Face Planar {#elements_face_planar}

[TOC]

A flat timber plate bounded by a bottom and a top outline, one side face per edge.

## Constructors

```cpp
FacePlanarElement(const Polyline& bottom, const Polyline& top, const std::string& name = "plate");

static std::shared_ptr<FacePlanarElement> from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, double thickness, const std::string& name = "plate");
static std::shared_ptr<FacePlanarElement> from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");
static std::vector<std::shared_ptr<FacePlanarElement>> row_between(const std::array<Polyline, 2>& bottom, const std::array<Polyline, 2>& top, const std::string& name = "plate");

// Standard pairs for examples and tests
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_in_plane(double angle_degrees = 180.0, double length = 300.0, double width = 400.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_folded(double angle_degrees = 90.0, double length = 300.0, double width = 400.0, double height = 300.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_rotated(double twist_degrees = 10.0, double length = 300.0, double width = 400.0, double thickness = 100.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_top_side(double angle_degrees = 90.0, double base_size = 400.0, double upright_size = 250.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_crossing(double angle_degrees = 90.0, double length = 400.0, double height = 200.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_stacked(double twist_degrees = 0.0, double length = 400.0, double width = 300.0, double thickness = 40.0);
```

## From two polylines

A four-sided plate lofted between a bottom polygon and the same polygon lifted 40 mm.

![](elements/element_face_planar.png)

\include{lineno} elements/element_face_planar.cpp

## A session of several elements

A row of bed plates made between a near and a far parabolic rail.

![](elements/element_face_planar_session.png)

\include{lineno} elements/element_face_planar_session.cpp

## Holes

Four through-holes, each a drill, a joint, its interaction bores the plate.

![](elements/element_face_planar_holes.png)

\include{lineno} elements/element_face_planar_holes.cpp

## Cut by a plane

A cutting plane is a joint; the plate keeps the side its normal points to.

![](elements/element_face_planar_cut_plane.png)

\include{lineno} elements/element_face_planar_cut_plane.cpp

## Assign joints

Eight box plates whose joint types and insertion directions are set by points and lines.

![](elements/element_face_planar_joints.png)

\include{lineno} elements/element_face_planar_joints.cpp
