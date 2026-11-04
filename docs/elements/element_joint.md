# Joint {#elements_joint}

[TOC]

Base cutter and modifier element that drills, subtracts, unions, or intersects geometry on target elements; also the base class of `JointPlateElement`, `JointBeamElement`, and `JointPinElement`.

## Constructors

```cpp
explicit JointElement(const std::vector<Polyline>& loops, const std::string& name = "JointElement");
explicit JointElement(const Plane& cutter);
explicit JointElement(const Mesh& cutter, SolidOperation operation = SolidOperation::subtract);
JointElement(const Polyline& profile, const Vector& direction);
JointElement(const std::vector<Polyline>& profile, const Vector& direction, SolidOperation operation = SolidOperation::intersect);
static std::shared_ptr<JointElement> drill(const Line& axis, double radius, double chord_tolerance = 0.05);
static std::shared_ptr<JointElement> support(const NodeSupportElement& support, const EdgeStraightElement& column);
std::shared_ptr<Interaction> interaction(size_t target) const;
```

## Cylindrical drill

A 25 mm radius vertical bore drilled cleanly through a timber block.

![](elements/element_joint_drill.png)

\include{lineno} elements/element_joint_drill.cpp

## Solid and extruded profile cutters

A box mesh subtracted from a beam's top face (`SolidOperation::subtract`) and an extruded 2D profile that trims the beam's underside to a V (`SolidOperation::intersect`).

![](elements/element_joint_cutter.png)

\include{lineno} elements/element_joint_cutter.cpp

## Support seat

`JointElement::support(support, column)` recesses a [Support](@ref elements_support)'s circular head plate and inclined pin bores into a column base (see [Column](@ref elements_column)).
