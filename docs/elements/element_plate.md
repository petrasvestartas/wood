# Plate {#elements_plate}

[TOC]

A flat timber plate bounded by a bottom and a top polygon with one side face per edge; joints modify it through `InteractionFeaturePlate` or `InteractionFeatureSolid`.

## Constructors

```cpp
Plate(const Polyline& bottom, const Polyline& top, const std::string& name = "plate");
static std::shared_ptr<Plate> from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, double thickness, const std::string& name = "plate");
static std::shared_ptr<Plate> from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");
static std::vector<std::shared_ptr<Plate>> row_between(const std::array<Polyline, 2>& bottom, const std::array<Polyline, 2>& top, const std::string& name = "plates");

// Standard plate pairs for examples and tests
static std::array<std::shared_ptr<Plate>, 2> pair_in_plane(double angle_degrees = 180.0, double length = 300.0, double width = 400.0, double thickness = 40.0);
static std::array<std::shared_ptr<Plate>, 2> pair_folded(double angle_degrees = 90.0, double length = 300.0, double width = 400.0, double height = 300.0, double thickness = 40.0);
static std::array<std::shared_ptr<Plate>, 2> pair_rotated(double twist_degrees = 10.0, double length = 300.0, double width = 400.0, double thickness = 100.0);
static std::array<std::shared_ptr<Plate>, 2> pair_top_side(double angle_degrees = 90.0, double base_size = 400.0, double upright_size = 250.0, double thickness = 40.0);
static std::array<std::shared_ptr<Plate>, 2> pair_crossing(double angle_degrees = 90.0, double length = 400.0, double height = 200.0, double thickness = 40.0);
static std::array<std::shared_ptr<Plate>, 2> pair_stacked(double twist_degrees = 0.0, double length = 400.0, double width = 300.0, double thickness = 40.0);
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `bottom`, `top` | — | Closed bottom and top polygons with equal vertex count; a shifted or smaller top bevels the side faces |
| `origin`, `x_axis`, `y_axis` | — | Corner point and local in-plane axes for `from_rectangle` |
| `width`, `height` | — | Side lengths of the rectangle |
| `thickness` | — | Offset from bottom to top along `x_axis` × `y_axis` |
| `bottom`, `top` rails (`row_between`) | — | Two rails per face, creating one plate per rail segment |

## From two polylines

A four-sided plate lofted between a bottom polygon and the same polygon offset 40 mm upward.

![](elements/element_plate.png)

```cpp
const Polyline bottom({{0.0, 0.0, 0.0}, {600.0, 0.0, 0.0}, {500.0, 400.0, 0.0}, {0.0, 300.0, 0.0}});
const Polyline top = bottom.translated({0.0, 0.0, 40.0});
scene.add(std::make_shared<Plate>(bottom.closed(), top.closed()));
```

## With holes

A rectangular plate with four vertical through-holes drilled via `Joint::drill` interactions.

![](elements/element_plate_holes.png)

```cpp
const std::shared_ptr<Plate> plate = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 600.0, 400.0, 40.0);
const std::shared_ptr<Joint> hole = Joint::drill(axis, 15.0);
scene.add_interaction(hole, plate, std::make_shared<InteractionFeatureSolid>(std::vector<Line>{axis}, 15.0));
```

## Lofted between two rails

A segmented row of bed plates created between near and far parabolic rail pairs using `Plate::row_between`.

![](elements/element_plate_session.png)

```cpp
for (const std::shared_ptr<Plate>& plate : Plate::row_between(
         {bottom_near, bottom_far},
         {bottom_near.translated({0.0, 0.0, 40.0}), bottom_far.translated({0.0, 0.0, 40.0})},
         "bed"
     )) {
    scene.add(plate);
}
```

## Joint types by points and lines

Eight box plates whose contact joint types and insertion directions are assigned spatially by `assign_joint_types_by_points` and `assign_insertion_vectors_by_lines`.

![](elements/element_plate_assign_joints.png)

```cpp
scene.compute_face_contacts();
scene.assign_joint_types_by_points(points, types);
scene.assign_insertion_vectors_by_lines(lines);
```
