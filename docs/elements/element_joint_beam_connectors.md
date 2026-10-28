# JointBeam connectors {#elements_joint_beam_connectors}

[TOC]

Face-contact timber connectors (`wedge`, `rectangle_plate`, `butterfly_key`) that cut pockets and pin bores into both members while nesting the connector `Block` and [`Pin`](@ref elements_pin) children under the joint.

![](diagrams/wood_beam_joints_connectors.webp)

## Wedge

A tapered hardwood wedge let into the top seam of two side-by-side beams and pinned across; the right beam is shifted aside to show the pocket and cross-pins.

![](elements/element_joint_beam_wedge.png)

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
const std::shared_ptr<JointBeam> wedge = JointBeam::wedge(*left, *right, *contact, 300.0, 133.0);
scene.add(wedge);
scene.add_interaction(wedge, left, wedge->interaction(0));
scene.add_interaction(wedge, right, wedge->interaction(1));
```

| Parameter | Default | Role |
| --- | --- | --- |
| `length_margin` | — | Setback of the wedge from each end of the contact's top edge |
| `pocket_depth` | — | Depth of the slanted pocket cut into each member |
| `end` | `none` | Optional end plane trimming the wedge instead of `length_margin` |
| `pin_radius`, `pin_spacing`, `pin_sides` | `10`, `320`, `8` | Cross-pin radius, spacing along the wedge, and polygon facet count |
| `overshoot` | `20` | Extra bore depth past each pin end |
| `profile` | `WEDGE_PROFILE` | Wedge cross-section (`63.5 mm` wide at top, apex `197 mm` below top edge) |
| `pin_offset` | `80`, `-100` | Longitudinal and vertical offset of the first pin from the top edge |

<details><summary>Example code: elements/element_joint_beam_wedge.cpp</summary>

\include{lineno} elements/element_joint_beam_wedge.cpp

</details>

## Let-in plate

A flush steel shear plate (`let_in_plate`) slotted into the top of a column and an abutting rib, fastened by two cross-pins in the column and two in the rib (`rectangle_plate`).

![](elements/element_joint_beam_rectangle_plate.png)

```cpp
const std::shared_ptr<Plate> plate = JointBeam::let_in_plate(*rib, *contact);
const std::shared_ptr<JointBeam> joint = JointBeam::rectangle_plate(*column, *rib, *plate, *contact);
scene.add(plate);
scene.add(joint);
```

| Parameter | Default | Role |
| --- | --- | --- |
| `width` | `30` | Steel plate thickness |
| `back`, `front` | `220`, `265` | Embedment length into the column (`back`) and into the rib (`front`) |
| `height` | `250` | Plate depth measured down from the contact's top edge |
| `pin_length`, `pin_radius` | auto, `25` | Dowel pin length and radius through the column, rib, and plate |
| `margin_x`, `margin_z` | `6.05`, `3` | Longitudinal and vertical pocket clearance around the steel plate |
| `overshoot`, `pin_sides` | `25`, `16` | Extra bore depth past the pins and cylindrical facet count |

<details><summary>Example code: elements/element_joint_beam_rectangle_plate.cpp</summary>

\include{lineno} elements/element_joint_beam_rectangle_plate.cpp

</details>

## Butterfly key

A double-dovetail butterfly key inlaid across the end-to-end seam of two collinear beams along with its mated pockets in both members.

![](elements/element_joint_beam_butterfly_key.png)

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
const std::shared_ptr<JointBeam> key = JointBeam::butterfly_key(*left, *right, *contact);
scene.add(key);
scene.add_interaction(key, left, key->interaction(0));
scene.add_interaction(key, right, key->interaction(1));
```

| Parameter | Default | Role |
| --- | --- | --- |
| `top` | `138.5` | Vertical recess of the key below the beam's top face (`0` is flush) |
| `length` | `800` | Total length of the key across the seam |
| `head_length`, `head_width`, `neck_width` | `200`, `40`, `20` | Flared head length, head width, and central neck width |
| `depth`, `end_depth` | `58.5`, `70.4` | Key thickness at the central seam and at the outer ends |
| `pocket_depth` | `80` | Pocket depth milled into the two beams |

<details><summary>Example code: elements/element_joint_beam_butterfly_key.cpp</summary>

\include{lineno} elements/element_joint_beam_butterfly_key.cpp

</details>
