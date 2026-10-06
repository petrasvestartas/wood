# CutPlane {#elements_cut_plane}

[TOC]

A cutting plane as an element, drawn as a square: it cuts other elements by its plane feature, `add_interaction(cut_plane, element, cut_plane->feature())`, the element keeping the side the normal points to.

## Constructors

```cpp
explicit CutPlane(const Plane& plane, double size = 1000.0, const std::string& name = "cut_plane")
std::shared_ptr<InteractionFeaturePlane> feature() const
```

## Cutting a variable beam

![Cutting a variable beam](elements/element_beam_variable_cut.png)

Two inclined planes drawn as 900 squares cutting the ends of a variable beam.

\include{lineno} elements/element_beam_variable_cut.cpp
