# Edge Curved {#elements_edge_curved}

[TOC]

A single-curved or double-curved glulam member swept along a `NurbsCurve` centreline as a stack of lamellae.

## Constructors

```cpp
EdgeCurvedElement(const NurbsCurve& centreline, double width, double height, const std::string& name = "beam_curved");

static std::shared_ptr<EdgeCurvedElement> single_curved(const NurbsCurve& centreline, double width, double height, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
static std::shared_ptr<EdgeCurvedElement> double_curved(const NurbsCurve& centreline, double width, double height, const std::vector<Vector>& directions = {}, const std::vector<double>& stations = {}, int num_width = 2, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
static std::shared_ptr<EdgeCurvedElement> from_surface(const NurbsCurve& centreline, const NurbsSurface& surface, double width, double height, int num_width = 2, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
```

## Single-curved and double-curved glulam

A planar arch of 6 lamellae trimmed by plumb cuts, beside a spatial S-curved member with a twisting frame.

![](elements/element_edge_curved.png)

\include{lineno} elements/element_edge_curved.cpp

## A session of several elements

Three arches side by side.

![](elements/element_edge_curved_session.png)

\include{lineno} elements/element_edge_curved_session.cpp

## Holes

A hole is a drill, a joint; its interaction bores the arch at its crown.

![](elements/element_edge_curved_holes.png)

\include{lineno} elements/element_edge_curved_holes.cpp

## Cut by a plane

A cutting plane is a joint; the arch keeps the side its normal points to.

![](elements/element_edge_curved_cut_plane.png)

\include{lineno} elements/element_edge_curved_cut_plane.cpp

## Assign joints

Two half arches cut flat at the crown, pins centred across their contact.

![](elements/element_edge_curved_joints.png)

\include{lineno} elements/element_edge_curved_joints.cpp
