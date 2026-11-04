# Support {#elements_support}

[TOC]

A steel column base assembly comprising a square base plate, anchor bolts, threaded adjustment rod, nuts, circular head plate, and inclined column pins. `JointElement::support` recesses the head plate and pin bores into a column base.

## Constructors

```cpp
explicit NodeSupportElement(const Plane& plane, const std::string& name = "support");
Point column_foot() const;
static std::shared_ptr<JointElement> JointElement::support(const NodeSupportElement& support, const Column& column);
```

## On a plane

A steel column support placed on the world `xy` plane with its exact BRep geometry.

![](elements/element_support.png)

\include{lineno} elements/element_support.cpp
