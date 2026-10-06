# Support {#elements_support}

[TOC]

A steel column base with the manufacturer's dimensions on a plane: base plate, tube, head plate, screws and anchors. `Joint::support` lets its head plate into a column end and drills the column screws, a subtract feature from the joint.

## Constructors

```cpp
explicit Support(const Plane& plane, const std::string& name = "support")
Point column_foot() const
static std::shared_ptr<Joint> Joint::support(const Support& support, const Column& column)
```

## On a plane

![On a plane](elements/element_support.png)

A support on the xy plane, written as its exact BRep: the round parts and the four anchor holes of the base plate are exact cylinders.

\include{lineno} elements/element_support.cpp
