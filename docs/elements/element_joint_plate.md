# Plate Joints {#elements_joint_plate}

[TOC]

Parametric plate-to-plate and boundary-edge joinery defined in the unit box `[-0.5, 0.5]^3` and oriented onto `InteractionContactFace` or `InteractionContactCross`.

![](diagrams/wood_joint_unit_box.webp)

## Families

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint_plate_top_side.html">
    <img src="element_joint_plate_top_side.png" alt="Plate Top-to-Side">
    <span class="wood-card-title">Top-to-Side</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_inplane.html">
    <img src="element_joint_plate_side_side_inplane.png" alt="Plate In-Plane">
    <span class="wood-card-title">In-Plane</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_outplane.html">
    <img src="element_joint_plate_side_side_outplane.png" alt="Plate Out-of-Plane">
    <span class="wood-card-title">Out-of-Plane</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_rotated.html">
    <img src="element_joint_plate_side_side_rotated.png" alt="Plate Rotated">
    <span class="wood-card-title">Rotated</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_cross.html">
    <img src="element_joint_plate_cross.png" alt="Plate Cross">
    <span class="wood-card-title">Cross</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_top.html">
    <img src="element_joint_plate_top_top.png" alt="Plate Top-to-Top">
    <span class="wood-card-title">Top-to-Top</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_border.html">
    <img src="element_joint_plate_border.png" alt="Plate Border">
    <span class="wood-card-title">Border</span>
  </a>
</div>

## Constructors

```cpp
static std::shared_ptr<JointPlate> side_side_inplane_three_fingers() .. side_side_inplane_tooth_keys(int divisions = 0), side_side_inplane_dovetails(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> side_side_inplane_hilti(const Plate& a, const Plate& b, const InteractionContactFace& contact, ...);
static std::shared_ptr<JointPlate> side_side_outplane_three_fingers() .. side_side_outplane_linked_tenons_wall(int divisions = 0), side_side_outplane_capped_fingers(int divisions = 4), side_side_outplane_notch();
static std::shared_ptr<JointPlate> top_side_three_tenons(), top_side_annen_tenons(), top_side_milled_wedge(), top_side_snap_fit(int divisions = 0), top_side_dense_tenons(int divisions = 0, double shift = 0.5), top_side_tenons(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> side_side_rotated_miter(), side_side_rotated_hooks(int divisions = 0, double shift = 0.5), side_side_rotated_keys(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> cross_slot(), cross_sliced_half_lap(double shift = 0.5) .. cross_brussels_half_lap(double shift = 0.5);
static std::shared_ptr<JointPlate> top_top_centre_pin(double radius = 1.0), top_top_polylabel_pin(double radius = 1.0), top_top_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0), top_top_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0) .. top_top_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);
static std::shared_ptr<JointPlate> border();
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5), jupiter_scarf();

void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements);
void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements);
std::shared_ptr<Interaction> interaction(size_t target) const;
```

## Usage

Target `0` receives the male outline and target `1` receives the female outline.

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(16, 0.5);
joint->orient(contact, {base, upright});
scene.add(joint);
scene.add_interaction(joint, upright, joint->interaction(0));
scene.add_interaction(joint, base, joint->interaction(1));
```

## Parameters

Parameter sweep of `top_side_tenons` varying `divisions = {8, 16, 24}` in the back row and `shift = {0.0, 0.25, 1.0}` in the front row.

![](elements/element_joint_plate_parameters.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(divisions, shift);
joint->orient(contact, {base, upright});
```

| Parameter | Default | Role |
| --- | --- | --- |
| divisions | 0 | Number of teeth or keys along the seam (`0` selects automatic spacing) |
| shift | 0.5 | Normalized lean of finger or tenon sides in `[0, 1]`, or half-lap centre width |
| male, female | — | Custom polyline pairs in the unit box `[-0.5, 0.5]^3` |
