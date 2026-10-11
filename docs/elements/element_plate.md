# Plate {#elements_plate}

[TOC]

A timber plate: a bottom and a top outline, one side face per edge; joints come as `InteractionFeaturePlate` or `InteractionFeatureSolid`.

## Constructors

```cpp
Plate(const Polyline& bottom, const Polyline& top, const std::string& name = "plate")
static std::shared_ptr<Plate> from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, double thickness, const std::string& name = "plate")
static std::vector<std::shared_ptr<Plate>> row_between(const std::array<Polyline, 2>& bottom, const std::array<Polyline, 2>& top, const std::string& name = "plates")

// pairs, one per joint family's configuration, for examples and tests
static std::array<std::shared_ptr<Plate>, 2> pair_in_plane(double angle_degrees = 180.0, double length = 300.0, double width = 400.0, double thickness = 40.0)      // ss_e_ip: edge to edge, in line or in a shallow V
static std::array<std::shared_ptr<Plate>, 2> pair_folded(double angle_degrees = 90.0, double length = 300.0, double width = 400.0, double height = 300.0, double thickness = 40.0)   // ss_e_op: mitred fold
static std::array<std::shared_ptr<Plate>, 2> pair_rotated(double twist_degrees = 10.0, double length = 300.0, double width = 400.0, double thickness = 100.0)     // ss_e_r: end to end, twisted
static std::array<std::shared_ptr<Plate>, 2> pair_top_side(double angle_degrees = 90.0, double base_size = 400.0, double upright_size = 250.0, double thickness = 40.0)  // ts_e_p: upright or leaning on a base
static std::array<std::shared_ptr<Plate>, 2> pair_crossing(double angle_degrees = 90.0, double length = 400.0, double height = 200.0, double thickness = 40.0)    // cr_c_ip: crossing uprights
static std::array<std::shared_ptr<Plate>, 2> pair_stacked(double twist_degrees = 0.0, double length = 400.0, double width = 300.0, double thickness = 40.0)       // tt_e_p: stacked, shifted, turned
```

## Parameters

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `bottom`, `top` | | the two faces: any closed polygons with one point count; a smaller or shifted top tilts the side faces |
| `origin`, `x_axis`, `y_axis` | | where `from_rectangle` puts the bottom corner and which way its sides run |
| `width`, `height` | | the rectangle's sides |
| `thickness` | | how far the top stands off the bottom, along `x_axis` × `y_axis` |
| `bottom`, `top` rails (`row_between`) | | two rails per face; one plate per rail segment |

## From two polylines

![From two polylines](elements/element_plate.png)

Its bottom outline, any closed polygon, and the same outline 40 above; the two outlines are its features.

\include{lineno} elements/element_plate.cpp

## With holes

![With holes](elements/element_plate_holes.png)

Four hidden `Joint::drill` holes taken away through `add_interaction(hole, plate, InteractionFeatureSolid(drills, radius))`.

\include{lineno} elements/element_plate_holes.cpp

## Lofted between two rails

![Lofted between two rails](elements/element_plate_session.png)

A row of plates lofted between two bottom and two top rails by `Plate::row_between(bottom, top)`.

\include{lineno} elements/element_plate_session.cpp

## Joint types by points and lines

![Joint types by points and lines](elements/element_plate_assign_joints.png)

The eight `annen_box_pair` plates, joints set by `WoodSession::assign_joint_types_by_points` and `WoodSession::assign_insertion_vectors_by_lines`.

\include{lineno} elements/element_plate_assign_joints.cpp
