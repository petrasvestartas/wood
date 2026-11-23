# Plate Joints {#elements_joint_plate}

[TOC]

Parametric plate-to-plate and boundary-edge joinery defined in the unit box `[-0.5, 0.5]^3` and oriented onto `InteractionContactFace` or `InteractionContactCross`.

![](diagrams/wood_joint_unit_box.webp)

## Families

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint_plate_top_side.html">
    <img src="assembly_element_joint_plate_top_side_three_tenons.gif" alt="Plate Top-to-Side">
    <span class="wood-card-title">Top-to-Side</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_inplane.html">
    <img src="assembly_element_joint_plate_side_side_inplane_dovetails.gif" alt="Plate In-Plane">
    <span class="wood-card-title">In-Plane</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_outplane.html">
    <img src="assembly_element_joint_plate_side_side_outplane_fingers.gif" alt="Plate Out-of-Plane">
    <span class="wood-card-title">Out-of-Plane</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_rotated.html">
    <img src="assembly_element_joint_plate_side_side_rotated_hooks.gif" alt="Plate Rotated">
    <span class="wood-card-title">Rotated</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_cross.html">
    <img src="assembly_element_joint_plate_cross_slot.gif" alt="Plate Cross">
    <span class="wood-card-title">Cross</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_top.html">
    <img src="assembly_element_joint_plate_top_top_circle_pins.gif" alt="Plate Top-to-Top">
    <span class="wood-card-title">Top-to-Top</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_border.html">
    <img src="assembly_element_joint_plate_border.gif" alt="Plate Border">
    <span class="wood-card-title">Border</span>
  </a>
</div>

## Constructors

```cpp
static std::shared_ptr<JointPlateElement> side_side_inplane_three_fingers() .. side_side_inplane_tooth_keys(int divisions = 0), side_side_inplane_dovetails(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlateElement> side_side_inplane_hilti(const FacePlanarElement& a, const FacePlanarElement& b, const InteractionContactFace& contact, ...);
static std::shared_ptr<JointPlateElement> side_side_outplane_three_fingers() .. side_side_outplane_linked_tenons_wall(int divisions = 0), side_side_outplane_capped_fingers(int divisions = 4), side_side_outplane_annen_fingers(int divisions = 7, double angle = 15.0), side_side_outplane_notch();
static VidyDoubleLayerNode JointVidyElement::folded_double_layer_node(double dihedral_degrees = 110.0, double seam_width = 1300.0, double wing_length = 900.0, double plate_thickness = 45.0, double total_offset = 300.0, double pin_pitch = 500.0, bool cutaway_front = true);
static std::shared_ptr<JointPlateElement> top_side_three_tenons(), top_side_annen_tenons(double tab_length = 80.0), top_side_annen_shifted_tenons(int divisions = 2, double long_edge = 700.0), top_side_milled_wedge(), top_side_snap_fit(int divisions = 0), top_side_dense_tenons(int divisions = 0, double shift = 0.5), top_side_tenons(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlateElement> side_side_rotated_miter(), side_side_rotated_hooks(int divisions = 0, double shift = 0.5), side_side_rotated_keys(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlateElement> cross_slot(), cross_sliced_half_lap(double shift = 0.5) .. cross_brussels_half_lap(double shift = 0.5);
static std::shared_ptr<JointPlateElement> top_top_centre_pin(double radius = 1.0), top_top_polylabel_pin(double radius = 1.0), top_top_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0), top_top_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0) .. top_top_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);
static std::shared_ptr<JointPlateElement> border();
static std::shared_ptr<JointPlateElement> side_removal(bool merge_with_joint = false, double shift = 0.5), jupiter_scarf();

void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<FacePlanarElement>>& elements);
void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<FacePlanarElement>>& elements);
std::shared_ptr<Interaction> interaction(size_t target) const;
```

## Tenons

Target `0` receives the male outline and target `1` the female outline.

| assembly | joint | tile |
|---|---|---|
| ![](joint_library/assembly_element_joint_plate_top_side_tenons.gif) | ![](joint_library/assembly_element_joint_plate_top_side_tenons.png) |  |

\include{lineno} elements/element_joint_plate_top_side_tenons.cpp

## Parameters

Parameter sweep of `top_side_tenons` varying `divisions = {8, 16, 24}` in the back row and `shift = {0.0, 0.25, 1.0}` in the front row.

| assembly | joint | tile |
|---|---|---|
| ![](joint_library/assembly_element_joint_plate_parameters.gif) | ![](joint_library/assembly_element_joint_plate_parameters.png) |  |

\include{lineno} elements/element_joint_plate_parameters.cpp

