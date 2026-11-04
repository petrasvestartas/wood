# Joint {#elements_joint}

[TOC]

Base cutter and modifier element that drills, subtracts, unions, or intersects geometry on target elements; also the base class of `JointPlate`, `JointBeam`, and `Pin`.

## Constructors

```cpp
explicit Joint(const std::vector<Polyline>& loops, const std::string& name = "Joint");
explicit Joint(const Plane& cutter);
explicit Joint(const Mesh& cutter, SolidOperation operation = SolidOperation::subtract);
Joint(const Polyline& profile, const Vector& direction);
Joint(const std::vector<Polyline>& profile, const Vector& direction, SolidOperation operation = SolidOperation::intersect);
static std::shared_ptr<Joint> drill(const Line& axis, double radius, double chord_tolerance = 0.05);
static std::shared_ptr<Joint> support(const Support& support, const Column& column);
std::shared_ptr<Interaction> interaction(size_t target) const;
```

## Cylindrical drill

A 25 mm radius vertical bore drilled cleanly through a timber block.

![](elements/element_joint_drill.png)

\include{lineno} elements/element_joint_drill.cpp

| Parameter | Default | Role |
| --- | --- | --- |
| `axis` | — | Bore centreline extending past the faces it pierces |
| `radius` | — | Bore radius |
| `chord_tolerance` | `0.05` | Maximum facet chord deviation (exact analytical cylinder on BRep targets) |

## Solid and extruded profile cutters

A box mesh subtracted from a beam's top face (`SolidOperation::subtract`) and an extruded 2D profile that trims the beam's underside to a V (`SolidOperation::intersect`).

![](elements/element_joint_cutter.png)

\include{lineno} elements/element_joint_cutter.cpp

| Parameter | Default | Role |
| --- | --- | --- |
| `cutter` | — | Closed mesh subtracted (`subtract`), united (`add`), or intersected (`intersect`) |
| `operation` | `subtract` (`intersect` for profile) | Boolean operation applied to the target element |
| `profile`, `direction` | — | Closed 2D loop and extrusion direction vector |

## Support seat

`Joint::support(support, column)` recesses a [Support](@ref elements_support)'s circular head plate and inclined pin bores into a column base (see [Column](@ref elements_column)).
