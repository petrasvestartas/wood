# Beam {#elements_beam}

A beam on a polyline axis: a square of a radius, or a profile swept along it, segment by segment.

```cpp
Beam(const Polyline& axis, double radius, const std::string& name = "beam")
Beam(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions = {}, const std::string& name = "beam")
```

![Beam](elements/element_beam.png)

Front: a straight axis with a 60 radius. Back: a two-segment axis swept by a 120 x 200 rectangle.

\include{lineno} elements/element_beam.cpp
