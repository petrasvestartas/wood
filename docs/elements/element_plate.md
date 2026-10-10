# Plate {#elements_plate}

[TOC]

A timber plate: a bottom and a top outline, one side face per edge. Its joints come as merged outline features (`InteractionFeaturePlate`), solids added or taken away as `InteractionFeatureSolid`.

## Constructors

```cpp
Plate(const Polyline& bottom, const Polyline& top, const std::string& name = "plate")
static std::shared_ptr<Plate> from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, double thickness, const std::string& name = "plate")
static std::vector<std::shared_ptr<Plate>> row_between(const std::array<Polyline, 2>& bottom, const std::array<Polyline, 2>& top, const std::string& name = "plates")
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

Four hidden hole elements (`Joint::drill`) each take a 30 hole away through `add_interaction(hole, plate, InteractionFeatureSolid(drills, radius))`, a solid feature of drills alone; written as BReps, the holes are exact cylinders.

\include{lineno} elements/element_plate_holes.cpp

## Lofted between two rails

![Lofted between two rails](elements/element_plate_session.png)

Several plates from two rails, as a bed row of the floor: `Plate::row_between(bottom, top)` lofts one plate per rail segment between two bottom rails and two top rails and returns them; the example adds them to a WoodSession.

\include{lineno} elements/element_plate_session.cpp

## Joint types by points and lines

![Joint types by points and lines](elements/element_plate_assign_joints.png)

The eight plates of `annen_box_pair` read from the dataset's obj without its sidecars: `WoodSession::assign_joint_types_by_points(points, types, snap_radius)` writes each point's type on the side face whose middle line lies nearest (a negative type on the bottom or top face), as the plugin's dots set them, and `WoodSession::assign_insertion_vectors_by_lines(lines, snap_radius)` writes each line's direction on the face its start lies on; `compute_features` then makes the 13 joints the sidecars `*_joints_types.txt` and `*_insertion_vectors.txt` give. A face several points reach takes the largest type, as a joint takes the larger type of its two faces; `tests/plate_assignment.cpp` proves both functions against every sidecar dataset, Annen and Vidy.

\include{lineno} elements/element_plate_assign_joints.cpp
