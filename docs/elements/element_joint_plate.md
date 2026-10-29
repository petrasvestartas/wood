# JointPlate {#elements_joint_plate}

[TOC]

Parametric plate-to-plate and single-edge joinery defined in the unit box `[-0.5, 0.5]^3` and oriented onto `InteractionContactFace` or `InteractionContactCross`.

![](diagrams/wood_joint_unit_box.webp)

## Constructors

```cpp
// Library presets across 7 families
static std::shared_ptr<JointPlate> side_side_inplane_three_fingers() .. side_side_inplane_tooth_keys(int divisions = 0), side_side_inplane_dovetails(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> side_side_inplane_hilti(const Plate& a, const Plate& b, const InteractionContactFace& contact, ...);
static std::shared_ptr<JointPlate> side_side_outplane_three_fingers() .. side_side_outplane_linked_tenons_wall(int divisions = 0), side_side_outplane_capped_fingers(int divisions = 4), side_side_outplane_notch();
static std::shared_ptr<JointPlate> top_side_three_tenons(), top_side_annen_tenons(), top_side_milled_wedge(), top_side_snap_fit(int divisions = 0), top_side_dense_tenons(int divisions = 0, double shift = 0.5), top_side_tenons(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> side_side_rotated_miter(), side_side_rotated_hooks(int divisions = 0, double shift = 0.5), side_side_rotated_keys(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> cross_slot(), cross_sliced_half_lap(double shift = 0.5) .. cross_brussels_half_lap(double shift = 0.5);
static std::shared_ptr<JointPlate> top_top_centre_pin(double radius = 1.0), top_top_polylabel_pin(double radius = 1.0), top_top_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0), top_top_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0) .. top_top_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);
static std::shared_ptr<JointPlate> border();
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5), jupiter_scarf();
static std::shared_ptr<JointPlate> <family>_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

// Orientation and target interaction access
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements);
void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements);
std::shared_ptr<Interaction> interaction(size_t target) const;
```

## Usage

Target `0` receives the male outline (pink in unit-box views) and target `1` receives the female outline (blue in unit-box views):

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(16, 0.5);
joint->orient(contact, {base, upright});
scene.add(joint);
scene.add_interaction(joint, upright, joint->interaction(0));
scene.add_interaction(joint, base, joint->interaction(1));
```

## Families

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint_plate_side_side_inplane.html">
    <img src="element_joint_plate_side_side_inplane.png" alt="JointPlate side_side_inplane">
    <span class="wood-card-title">side_side_inplane</span>
    <span class="wood-card-sub">Coplanar edge-to-edge dovetails, butterfly/tooth keys &amp; Hilti</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_outplane.html">
    <img src="element_joint_plate_side_side_outplane.png" alt="JointPlate side_side_outplane">
    <span class="wood-card-title">side_side_outplane</span>
    <span class="wood-card-sub">Folded corner miters, fingers, tenons &amp; Vidy linked tenons</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_side.html">
    <img src="element_joint_plate_top_side.png" alt="JointPlate top_side">
    <span class="wood-card-title">top_side</span>
    <span class="wood-card-sub">T-joint edge-to-face tenons, snap-fit &amp; milled wedge</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_rotated.html">
    <img src="element_joint_plate_side_side_rotated.png" alt="JointPlate side_side_rotated">
    <span class="wood-card-title">side_side_rotated</span>
    <span class="wood-card-sub">Twisted seam miter slices, hook/diamond keys &amp; side removal</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_cross.html">
    <img src="element_joint_plate_cross.png" alt="JointPlate cross">
    <span class="wood-card-title">cross</span>
    <span class="wood-card-sub">Intersecting plate cross slots &amp; half-laps</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_top.html">
    <img src="element_joint_plate_top_top.png" alt="JointPlate top_top">
    <span class="wood-card-title">top_top</span>
    <span class="wood-card-sub">Stacked face-to-face drilled pin patterns</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_border.html">
    <img src="element_joint_plate_border.png" alt="JointPlate border">
    <span class="wood-card-title">border</span>
    <span class="wood-card-sub">Single-plate boundary edge profile joinery</span>
  </a>
</div>

| Family | Contact | Geometry |
| --- | --- | --- |
| [side_side_inplane](@ref elements_joint_plate_side_side_inplane) | Side-to-side, in-plane | Coplanar plates edge-to-edge (dovetails, butterfly/tooth keys, Hilti) |
| [side_side_outplane](@ref elements_joint_plate_side_side_outplane) | Side-to-side, out-of-plane | Folded corner miters (fingers, tenons, Vidy linked tenons) |
| [top_side](@ref elements_joint_plate_top_side) | Top-to-side | T-joint edge-to-face (tenons, snap-fit, milled wedge) |
| [side_side_rotated](@ref elements_joint_plate_side_side_rotated) | Side-to-side, rotated | Twisted seam (miter slices, hook/diamond keys, side removal) |
| [cross](@ref elements_joint_plate_cross) | Cross (`InteractionContactCross`) | Plates passing through each other (`cross_slot`, half-laps) |
| [top_top](@ref elements_joint_plate_top_top) | Top-to-top | Stacked faces (drilled pin patterns) |
| [border](@ref elements_joint_plate_border) | Border | Single-plate side face (`compute_border_contact`) |

## Border {#joint_plate_border_section}

A single-plate edge profile (`JointPlate::border()`) oriented onto side face `2` via `WoodSession::compute_border_contact(plate, 2)`.

![](elements/element_joint_plate_border.png)

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_border_contact(plate, 2);
const std::shared_ptr<JointPlate> joint = JointPlate::border();
joint->orient(contact, {plate});
scene.add_interaction(joint, plate, joint->interaction(0));
```

<details><summary>Example code: elements/element_joint_plate_border.cpp</summary>

\include{lineno} elements/element_joint_plate_border.cpp

</details>

## Parameters

A parameter sweep of `top_side_tenons` varying `divisions = {8, 16, 24}` in the back row and `shift = {0.0, 0.25, 1.0}` in the front row.

![](elements/element_joint_plate_parameters.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(divisions, shift);
joint->orient(contact, {base, upright});
```

| Parameter | Default | Role |
| --- | --- | --- |
| `divisions` | `0` | Number of teeth or keys along the seam; `0` computes `1` per `300 mm` (in-plane, rotated) or `450 mm` (out-of-plane, top-side) |
| `shift` | `0.5` | Normalized lean of finger or tenon sides (`[0, 1]`), or centre width of cross half-laps |
| `male`, `female` (`*_custom`) | — | Custom polyline pairs in the unit box `[-0.5, 0.5]^3` |

<details><summary>Example code: elements/element_joint_plate_parameters.cpp</summary>

\include{lineno} elements/element_joint_plate_parameters.cpp

</details>
