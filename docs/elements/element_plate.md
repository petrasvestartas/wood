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
