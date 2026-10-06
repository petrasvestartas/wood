# BeamVariable {#elements_beam_variable}

A beam whose section changes along its axis: one section per station, lofted between them.

```cpp
BeamVariable(const Line& axis, const std::vector<Polyline>& sections, const std::string& name = "beam_variable")
static std::shared_ptr<BeamVariable> between(const Polyline& first, const Polyline& last, const std::string& name = "beam_variable")
```

![BeamVariable](elements/element_beam_variable.png)

Like the floor's outer rib: seven 120 wide rectangles hanging from a straight 3000 axis along its top, 730 deep at the start and 300 at the end on a parabola. The lines are its features, the axis and every section.

\include{lineno} elements/element_beam_variable.cpp
