
# Wood

Timber joinery on the session kernel: plates and beams as elements, joints as elements that cut them, models built by templates.

| Page | What it holds |
| --- | --- |
| [Elements](@ref elements) | every element and joint, with its constructors, a picture and an example |
| [Templates](@ref templates) | generators that turn a surface, a mesh or a plan into elements and joints |
| [Examples](@ref examples) | short programs, one behaviour each |

A model is built with two calls: `add(element, group)` puts an element in the scene, and `add_interaction(source, target, interaction)` puts what one element does to another on their edge.

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons();
joint->orient(contact, {base, upright});
scene.add(joint);
scene.add_interaction(joint, upright, joint->interaction(0));
scene.add_interaction(joint, base, joint->interaction(1));
```
