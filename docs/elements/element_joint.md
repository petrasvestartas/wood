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

## Usage

Add the joint to the scene and wire `interaction(0)` to the target element via `add_interaction`:

```cpp
scene.add(drill);
scene.add_interaction(drill, block, drill->interaction(0));
```

## Cylindrical drill

A 25 mm radius vertical bore drilled cleanly through a timber block.

![](elements/element_joint_drill.png)

```cpp
const std::shared_ptr<Joint> drill = Joint::drill(Line::from_points({150.0, 150.0, -20.0}, {150.0, 150.0, 220.0}), 25.0);
scene.add(drill);
scene.add_interaction(drill, block, drill->interaction(0));
```

| Parameter | Default | Role |
| --- | --- | --- |
| `axis` | — | Bore centreline extending past the faces it pierces |
| `radius` | — | Bore radius |
| `chord_tolerance` | `0.05` | Maximum facet chord deviation (exact analytical cylinder on BRep targets) |

<details><summary>Example code: elements/element_joint_drill.cpp</summary>

\include{lineno} elements/element_joint_drill.cpp

</details>

## Solid and extruded profile cutters

A box mesh subtracted from a beam's top face (`SolidOperation::subtract`) and an extruded 2D profile that trims the beam's underside to a V (`SolidOperation::intersect`).

![](elements/element_joint_cutter.png)

```cpp
const std::shared_ptr<Joint> notch = std::make_shared<Joint>(box_mesh, SolidOperation::subtract);
const std::shared_ptr<Joint> trim = std::make_shared<Joint>(profile_loop, Vector(0.0, 400.0, 0.0));
scene.add_interaction(notch, beam, notch->interaction(0));
scene.add_interaction(trim, beam, trim->interaction(0));
```

| Parameter | Default | Role |
| --- | --- | --- |
| `cutter` | — | Closed mesh subtracted (`subtract`), united (`add`), or intersected (`intersect`) |
| `operation` | `subtract` (`intersect` for profile) | Boolean operation applied to the target element |
| `profile`, `direction` | — | Closed 2D loop and extrusion direction vector |

<details><summary>Example code: elements/element_joint_cutter.cpp</summary>

\include{lineno} elements/element_joint_cutter.cpp

</details>

## Support seat

`Joint::support(support, column)` recesses a [Support](@ref elements_support)'s circular head plate and inclined pin bores into a column base (see [Column](@ref elements_column)).
