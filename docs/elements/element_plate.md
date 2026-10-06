# Plate {#elements_plate}

A timber plate: a bottom and a top outline, one side face per edge. Its joints come as merged outline features (`InteractionFeaturePlate`), solids added or taken away as `InteractionFeatureSolid`.

```cpp
Plate(const Polyline& bottom, const Polyline& top, const std::string& name = "plate")
static std::shared_ptr<Plate> from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, double thickness, const std::string& name = "plate")
```

![Plate](elements/element_plate.png)

A plate from two polylines: its bottom outline and the same outline 40 above.

\include{lineno} elements/element_plate.cpp
