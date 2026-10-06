# BeamVariable {#elements_beam_variable}

A beam whose section changes along its axis: one section per station, lofted between them.

```cpp
BeamVariable(const Line& axis, const std::vector<Polyline>& sections, const std::string& name = "beam_variable")
static std::shared_ptr<BeamVariable> between(const Polyline& first, const Polyline& last, const std::string& name = "beam_variable")
```

![BeamVariable](elements/element_beam_variable.png)

Both taper from a 120 x 300 section to 120 x 120: with an axis, and `BeamVariable::between` from the two sections alone.

\include{lineno} elements/element_beam_variable.cpp
