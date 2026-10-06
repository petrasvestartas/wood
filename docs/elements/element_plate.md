# Plate {#elements_plate}

[TOC]

A timber plate: a bottom and a top outline, one side face per edge. Its joints come as merged outline features (`InteractionFeaturePlate`), solids added or taken away as `InteractionFeatureSolid`.

## Constructors

```cpp
Plate(const Polyline& bottom, const Polyline& top, const std::string& name = "plate")
static std::shared_ptr<Plate> from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, double thickness, const std::string& name = "plate")
```

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

Several plates from two rails, as a bed row of the floor: `wood_floor::PlateSession::between(bottom, top)` lofts one plate per rail segment between two bottom rails and two top rails and returns them as a session.

\include{lineno} elements/element_plate_session.cpp
