# Joint Cut Plane {#elements_joint_cut_plane}

[TOC]

A cutting plane, drawn as a square. It is a joint like any other modifier: `add_interaction(cut, target, cut->interaction(0))`, and the target keeps the side the normal points to.

## Constructors

```cpp
explicit JointCutPlaneElement(const Plane& plane, double size = 1000.0, const std::string& name = "cut_plane");

const Plane& plane() const;
std::shared_ptr<Interaction> interaction(size_t target) const override;
```

## The plane alone

A cutting plane drawn as a square of its size.

![](elements/element_joint_cut_plane.png)

\include{lineno} elements/element_joint_cut_plane.cpp

## Cutting an edge

One plane on a beam.

![](elements/element_edge_straight_cut_plane.png)

\include{lineno} elements/element_edge_straight_cut_plane.cpp

## Cutting both ends

Two inclined planes trimming a parabolic rib.

![](elements/element_edge_variable_cut_plane.png)

\include{lineno} elements/element_edge_variable_cut_plane.cpp
