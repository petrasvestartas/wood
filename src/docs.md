# Wood

Timber joinery solver and parametric structure templates on [`session_cpp`](https://github.com/petrasvestartas/session_cpp).

![Wood Kernel Architecture](diagrams/wood_architecture.svg)

| Section | Scope |
| --- | --- |
| [Elements](@ref elements) | Timber solids (`Plate`, `PlateCurved`, `Beam`, `BeamVariable`, `Column`, `Block`, `Pin`, `Support`), cutters (`CutPlane`, `Profile`), and parametric joints (`Joint`, `JointPlate`, `JointBeam`) |
| [Templates](@ref templates) | 11 parametric timber systems (`Shells`, `Folding`, `Folding cylindrical`, `Folding developable`, `Chevron`, `Cross`, `Reciprocal`, `Module`, `Grid`, `Truss`, `Floor`) |
| [Examples](@ref examples) | Self-contained `examples/*.cpp` targets and the [Cutting gallery](@ref cutting_gallery) |
| [Architecture](@ref architecture) | Dual scene graph (`WoodSession`), contact detection (`Face`, `Cross`, `Axis`), lazy `Mesh`/`BRep` evaluation, and analytical NURBS bore cutting |

## Core Pattern

```cpp
WoodSession session("model");
const std::shared_ptr<InteractionContactFace> contact = session.compute_face_contact(a, b)[0];
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_fingers(8, 0.5);
joint->orient(contact, {a, b});
session.add(joint, joints_group);
session.add_interaction(joint, a, joint->interaction(0));
session.add_interaction(joint, b, joint->interaction(1));
session.pb_dump(pb_path("live"));
```
