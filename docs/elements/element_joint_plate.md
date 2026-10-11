# JointPlate {#elements_joint_plate}

[TOC]

A joint between two plates: a design of the joint library, oriented on the plates' contact, that cuts its profile into both outlines.

## Constructors

```cpp
// the library designs, each by its own name and parameters
static std::shared_ptr<JointPlate> in_plane_three_fingers() .. in_plane_tooth_keys(int divisions = 0), in_plane_dovetails(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line; see the in_plane page
static std::shared_ptr<JointPlate> in_plane_hilti(const Plate& a, const Plate& b, const InteractionContactFace& contact, ...)   // the Hilti connector, side to side only
static std::shared_ptr<JointPlate> corner_three_fingers() .. corner_linked_tenons_wall(int divisions = 0), corner_capped_fingers(int divisions = 4), corner_notch()   // 0 divisions: one every 450 mm of the joint line, shift 0.64; see the corner page
static std::shared_ptr<JointPlate> tee_three_tenons(), tee_annen_tenons(), tee_milled_wedge(), tee_snap_fit(int divisions = 0), tee_dense_tenons(int divisions = 0, double shift = 0.5), tee_tenons(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 450 mm of the joint line, shift 0.5; see the tee page
static std::shared_ptr<JointPlate> rotated_mitre(), rotated_hooks(int divisions = 0, double shift = 0.5), rotated_keys(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line
static std::shared_ptr<JointPlate> cross_slot(), cross_sliced_half_lap(double shift = 0.5) .. cross_brussels_half_lap(double shift = 0.5)   // no divisions; the shift narrows the half-lap's centre square, 0 the widest, 1 the narrowest; the half-laps on the JointBeam cross page
static std::shared_ptr<JointPlate> stacked_centre_pin(double radius = 1.0), stacked_polylabel_pin(double radius = 1.0), stacked_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0), stacked_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0) .. stacked_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0)   // the family defaults, each design reading them its own way; the radius is the pin's
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
const std::shared_ptr<JointPlate> joint = JointPlate::tee_tenons(16, 0.5);
joint->orient(contact, {base, upright});
scene.add(joint);
scene.add_interaction(joint, upright, joint->interaction(0));
scene.add_interaction(joint, base, joint->interaction(1));
```

## Families

| Family | Contact | Pair |
| --- | --- | --- |
| [in_plane](@ref elements_joint_plate_in_plane) | side to side, in plane | two plates edge to edge |
| [corner](@ref elements_joint_plate_corner) | side to side, out of plane | a floor and a wall at a corner |
| [tee](@ref elements_joint_plate_tee) | top to side | a plate standing on another |
| [rotated](@ref elements_joint_plate_rotated) | side to side, rotated | two plates twisted about their seam |
| [cross](@ref elements_joint_plate_cross) | cross | two plates passing through each other |
| [stacked](@ref elements_joint_plate_stacked) | top to top | two plates stacked face on face |
| [b](@ref elements_joint_plate_b) | border | one plate's side face |

## b: boundary {#elements_joint_plate_b}

![border on a side face](elements/element_joint_plate_border.png)

`border` on one side face of a plate, its border contact made by `WoodSession::compute_border_contact(plate, 2)`.

\include{lineno} elements/element_joint_plate_border.cpp

## Parameters

![parameters of tee_tenons](elements/element_joint_plate_parameters.png)

`tee_tenons` six times: back row 8, 16 and 24 divisions, front row shift 0, 0.25 and 1.

| Parameter | Default | What it changes |
| --- | --- | --- |
| `divisions` | 0 | how many fingers, tenons or keys along the seam; 0: one per 300 mm (in plane, rotated) or 450 mm (out of plane, top to side) |
| `shift` | 0.5 | the lean of the finger or tenon sides; for cross joints, how narrow the half-lap |
| `male`, `female` (`*_custom`) | | your own outlines in the unit box |

\include{lineno} elements/element_joint_plate_parameters.cpp
