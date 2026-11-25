# Code Structure {#architecture}

## 1 Session {#arch_session}

```cpp
WoodSession scene("two_plates");
```

<div class="wood-links">
  <a href="classwood__session_1_1WoodSession.html">WoodSession</a>
  <a href="examples.html#ex_4_traversal">Graph traversal</a>
  <a href="examples.html#ex_5_settings">Settings</a>
</div>

## 2 Elements {#arch_elements}

```cpp
const std::array<std::shared_ptr<FacePlanarElement>, 2> pair = FacePlanarElement::pair_top_side(90.0, 400.0);
const std::shared_ptr<FacePlanarElement> base = pair[0];
const std::shared_ptr<FacePlanarElement> upright = pair[1];
scene.add(base);
scene.add(upright);
```

<div class="wood-links">
  <a href="elements.html">Elements</a>
  <a href="elements_face_planar.html">Plates</a>
  <a href="elements_edge_straight.html">Beams</a>
  <a href="templates.html">Templates</a>
</div>

## 3 Contacts {#arch_contacts}

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
scene.add_interaction(base, upright, contact);
```

<div class="wood-links">
  <a href="examples.html#ex_2_contacts">Plate contacts</a>
  <a href="examples.html#ex_8_beams">Beam contacts</a>
  <a href="examples.html#ex_9_cross_joints">Cross contacts</a>
</div>

## 4 Joints {#arch_joints}

```cpp
const std::shared_ptr<JointPlateElement> joint = JointPlateElement::top_side_tenons();
joint->orient(contact, {base, upright});

scene.add(joint);
scene.add_interaction(joint, upright, joint->interaction(0));
scene.add_interaction(joint, base, joint->interaction(1));
```

<div class="wood-links">
  <a href="elements_joint_plate.html">Plate joints</a>
  <a href="elements_joint_beam.html">Beam joints</a>
  <a href="elements_joint_beam_compendium.html">Joint library</a>
  <a href="examples.html#ex_6_custom_joint">Custom joint</a>
</div>

## 5 Geometry {#arch_geometry}

```cpp
const Mesh& stock = upright->element_geometry_mesh(); // the member uncut
const Mesh& cut = upright->model_geometry_mesh();     // with its joints

scene.pb_dump(pb_path("live"));
```

<div class="wood-links">
  <a href="examples.html#ex_12_solid_features">Solid cuts</a>
  <a href="examples.html#ex_11_drill_solids">Drills</a>
  <a href="https://petrasvestartas.github.io/session/">Viewer</a>
</div>
