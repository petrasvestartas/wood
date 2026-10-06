# BeamVariable {#elements_beam_variable}

[TOC]

A beam whose section changes along its axis: one section per station, lofted between them. Its axis, sections, `top()` and `bottom()` outlines are its features, all cut by the planes other elements cut it by.

## Constructors

```cpp
BeamVariable(const Line& axis, const std::vector<Polyline>& sections, const std::string& name = "beam_variable")
static std::shared_ptr<BeamVariable> between(const Polyline& first, const Polyline& last, const std::string& name = "beam_variable")
Polyline top() const
Polyline bottom() const
```

## Like the outer rib

![Like the outer rib](elements/element_beam_variable.png)

Seven 120 wide rectangles hanging from a straight 3000 axis along its top, 730 deep at the start and 300 at the end on a parabola, as the floor's outer rib.

\include{lineno} elements/element_beam_variable.cpp

## Cut by plane elements

![Cut by plane elements](elements/element_beam_variable_cut.png)

The same rib cut at both ends by two `CutPlane` elements through `add_interaction(plane, rib, plane->feature())`; the rib, its sections and its top and bottom outlines keep the side the normals point to.

\include{lineno} elements/element_beam_variable_cut.cpp
