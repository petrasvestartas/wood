# JointPlate {#elements_joint_plate}

[TOC]

A joint between two plates: a design of the joint library, oriented on the plates' contact, that cuts its profile into both outlines.

## Constructors

```cpp
// the library designs, each by its own name and parameters
static std::shared_ptr<JointPlate> side_side_inplane_three_fingers() .. side_side_inplane_tooth_keys(int divisions = 0), side_side_inplane_dovetails(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line; see the side_side_inplane page
static std::shared_ptr<JointPlate> side_side_inplane_hilti(const Plate& a, const Plate& b, const InteractionContactFace& contact, ...)   // the Hilti connector, side to side only
static std::shared_ptr<JointPlate> side_side_outplane_three_fingers() .. side_side_outplane_linked_tenons_wall(int divisions = 0), side_side_outplane_capped_fingers(int divisions = 4), side_side_outplane_notch()   // 0 divisions: one every 450 mm of the joint line, shift 0.64; see the side_side_outplane page
static std::shared_ptr<JointPlate> top_side_three_tenons(), top_side_annen_tenons(), top_side_milled_wedge(), top_side_snap_fit(int divisions = 0), top_side_dense_tenons(int divisions = 0, double shift = 0.5), top_side_tenons(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 450 mm of the joint line, shift 0.5; see the top_side page
static std::shared_ptr<JointPlate> side_side_rotated_miter(), side_side_rotated_hooks(int divisions = 0, double shift = 0.5), side_side_rotated_keys(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line
static std::shared_ptr<JointPlate> cross_slot(), cross_sliced_half_lap(double shift = 0.5) .. cross_brussels_half_lap(double shift = 0.5)   // no divisions; the shift narrows the half-lap's centre square, 0 the widest, 1 the narrowest; the half-laps on the JointBeam cross page
static std::shared_ptr<JointPlate> top_top_centre_pin(double radius = 1.0), top_top_polylabel_pin(double radius = 1.0), top_top_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0), top_top_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0) .. top_top_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0)   // the family defaults, each design reading them its own way; the radius is the pin's
static std::shared_ptr<JointPlate> border()
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5), jupiter_scarf()
static std::shared_ptr<JointPlate> <family>_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female)

// placed on a contact, then passed to each plate
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
std::shared_ptr<Interaction> interaction(size_t target) const
```

## How it is used

A design is made by name on the contact of two plates, added, and passed to each plate. The male (pink) is target 0, the female (blue) target 1.

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(16, 0.5);
joint->orient(contact, {base, upright});
scene.add(joint);
scene.add_interaction(joint, upright, joint->interaction(0));
scene.add_interaction(joint, base, joint->interaction(1));
```

## Families

| Family | Contact | Pair |
| --- | --- | --- |
| [side_side_inplane](@ref elements_joint_plate_side_side_inplane) | side to side, in plane | two plates edge to edge |
| [side_side_outplane](@ref elements_joint_plate_side_side_outplane) | side to side, out of plane | a floor and a wall at a corner |
| [top_side](@ref elements_joint_plate_top_side) | top to side | a plate standing on another |
| [side_side_rotated](@ref elements_joint_plate_side_side_rotated) | side to side, rotated | two plates twisted about their seam |
| [cross](@ref elements_joint_plate_cross) | cross | two plates passing through each other |
| [top_top](@ref elements_joint_plate_top_top) | top to top | two plates stacked face on face |
| [border](@ref elements_joint_plate_border) | border | one plate's side face |

## border {#elements_joint_plate_border}

![border on a side face](elements/element_joint_plate_border.png)

`border` on one side face of a plate, its border contact made by `WoodSession::compute_border_contact(plate, 2)`.

\include{lineno} elements/element_joint_plate_border.cpp

## Parameters

![parameters of top_side_tenons](elements/element_joint_plate_parameters.png)

`top_side_tenons` six times: back row 8, 16 and 24 divisions, front row shift 0, 0.25 and 1.

| Parameter | Default | What it changes |
| --- | --- | --- |
| `divisions` | 0 | how many fingers, tenons or keys along the seam; 0: one per 300 mm (in plane, rotated) or 450 mm (out of plane, top to side) |
| `shift` | 0.5 | the lean of the finger or tenon sides; for cross joints, how narrow the half-lap |
| `male`, `female` (`*_custom`) | | your own outlines in the unit box |

\include{lineno} elements/element_joint_plate_parameters.cpp
