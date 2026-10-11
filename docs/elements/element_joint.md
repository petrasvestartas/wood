# Joint {#elements_joint}

[TOC]

What one element cuts, drills or adds to another; the base of JointPlate, JointBeam and Pin.

## Constructors

```cpp
explicit Joint(const std::vector<Polyline>& loops, const std::string& name = "Joint")
explicit Joint(const Plane& cutter)
explicit Joint(const Mesh& cutter, SolidOperation operation = SolidOperation::subtract)
Joint(const Polyline& profile, const Vector& direction)
Joint(const std::vector<Polyline>& profile, const Vector& direction, SolidOperation operation = SolidOperation::intersect)
static std::shared_ptr<Joint> drill(const Line& axis, double radius, double chord_tolerance = 0.05)
static std::shared_ptr<Joint> support(const Support& support, const Column& column)
std::shared_ptr<Interaction> interaction(size_t target) const
```

## How it is used

A joint is added, usually hidden, and `interaction(0)` passes it to its target as one `InteractionFeatureSolid`:

```cpp
scene.add(drill);
scene.add_interaction(drill, block, drill->interaction(0));
```

## A drill

![A drill](elements/element_joint_drill.png)

A 25 radius hole along a vertical axis through a block.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `axis` | | where the hole runs, and how far: past the element's faces it goes through |
| `radius` | | the hole's radius |
| `chord_tolerance` | 0.05 | how far a facet of the hole may stray from the circle; a BRep keeps it exact |

\include{lineno} elements/element_joint_drill.cpp

## A solid and a profile

![A solid and a profile](elements/element_joint_cutter.png)

A box subtracted from a beam's top, and a closed profile that trims the underside to a V.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `cutter` | | the closed mesh taken away (`subtract`), added (`add`) or kept (`intersect`) |
| `operation` | subtract, or intersect for a profile | what the solid does to its target |
| `profile`, `direction` | | a closed loop and the vector it is extruded along |

\include{lineno} elements/element_joint_cutter.cpp

## A support seat

`Joint::support(support, column)` lets a @ref elements_support "Support"'s head plate into the column end; shown on @ref elements_column.
