# Node Support {#elements_node_support}

[TOC]

A steel column base: base plate, anchors, threaded rod, nuts, head plate and inclined pins. Being steel with its own bores, it takes no holes or plane cuts.

## Constructors

```cpp
explicit NodeSupportElement(const Plane& plane, const std::string& name = "support");

Point column_foot() const;
Line column_axis(double top_z) const;
```

## On a plane

A support on the world `xy` plane, its exact BRep.

![](elements/element_node_support.png)

\include{lineno} elements/element_node_support.cpp

## A session of several elements

The floor's corner column, the support under it among its parts.

![](elements/element_edge_straight_column_session.png)

\include{lineno} elements/element_edge_straight_column_session.cpp

## Assign joints

The seat is a joint: it recesses the head plate and bores the pins into the column end.

![](elements/element_joint_support.png)

\include{lineno} elements/element_joint_support.cpp
