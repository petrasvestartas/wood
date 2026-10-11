# JointPlate {#elements_joint_plate}

[TOC]

A joint between two plates: a design of the joint library, oriented on the plates' contact, that cuts its profile into both outlines.

## Constructors

```cpp
// the library designs, each by its own name and parameters
static std::shared_ptr<JointPlate> ss_e_ip_0() .. ss_e_ip_5(int divisions = 0), ss_e_ip_1(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line; see the ss_e_ip page
static std::shared_ptr<JointPlate> ss_e_ip_hilti(const Plate& a, const Plate& b, const InteractionContactFace& contact, ...)   // the Hilti connector, side to side only
static std::shared_ptr<JointPlate> ss_e_op_0() .. ss_e_op_6(int divisions = 0), ss_e_op_17(int divisions = 4), ss_e_op_tutorial()   // 0 divisions: one every 450 mm of the joint line, shift 0.64; see the ss_e_op page
static std::shared_ptr<JointPlate> ts_e_p_0(), ts_e_p_1(), ts_e_p_4(), ts_e_p_5(int divisions = 0), ts_e_p_2(int divisions = 0, double shift = 0.5), ts_e_p_3(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 450 mm of the joint line, shift 0.5; see the ts_e_p page
static std::shared_ptr<JointPlate> ss_e_r_0(), ss_e_r_2(int divisions = 0, double shift = 0.5), ss_e_r_3(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line
static std::shared_ptr<JointPlate> cr_c_ip_0(), cr_c_ip_1(double shift = 0.5) .. cr_c_ip_5(double shift = 0.5)   // no divisions; the shift narrows the half-lap's centre square, 0 the widest, 1 the narrowest; the half-laps on the JointBeam cr_c_ip page
static std::shared_ptr<JointPlate> tt_e_p_0(double radius = 1.0), tt_e_p_1(double radius = 1.0), tt_e_p_2(int divisions = 6, double shift = 0.95, double radius = 1.0), tt_e_p_3(double division_length = 6.0, double shift = 0.95, double radius = 1.0) .. tt_e_p_5(double division_length = 6.0, double shift = 0.95, double radius = 1.0)   // the family defaults, each design reading them its own way; the radius is the pin's
static std::shared_ptr<JointPlate> b_0()
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5), side_removal_ss_e_r_1()
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
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_3(16, 0.5);
joint->orient(contact, {base, upright});
scene.add(joint);
scene.add_interaction(joint, upright, joint->interaction(0));
scene.add_interaction(joint, base, joint->interaction(1));
```

## Families

| Family | Contact | Pair |
| --- | --- | --- |
| [ss_e_ip](@ref elements_joint_plate_ss_e_ip) | side to side, in plane | two plates edge to edge |
| [ss_e_op](@ref elements_joint_plate_ss_e_op) | side to side, out of plane | a floor and a wall at a corner |
| [ts_e_p](@ref elements_joint_plate_ts_e_p) | top to side | a plate standing on another |
| [ss_e_r](@ref elements_joint_plate_ss_e_r) | side to side, rotated | two plates twisted about their seam |
| [cr_c_ip](@ref elements_joint_plate_cr_c_ip) | cross | two plates passing through each other |
| [tt_e_p](@ref elements_joint_plate_tt_e_p) | top to top | two plates stacked face on face |
| [b](@ref elements_joint_plate_b) | border | one plate's side face |

## b: boundary {#elements_joint_plate_b}

![b_0 on a side face](elements/element_joint_plate_b_0.png)

`b_0` on one side face of a plate, its border contact made by `WoodSession::compute_border_contact(plate, 2)`.

\include{lineno} elements/element_joint_plate_b_0.cpp

## Parameters

![parameters of ts_e_p_3](elements/element_joint_plate_parameters.png)

`ts_e_p_3` six times: back row 8, 16 and 24 divisions, front row shift 0, 0.25 and 1.

| Parameter | Default | What it changes |
| --- | --- | --- |
| `divisions` | 0 | how many fingers, tenons or keys along the seam; 0: one per 300 mm (in plane, rotated) or 450 mm (out of plane, top to side) |
| `shift` | 0.5 | the lean of the finger or tenon sides; for cross joints, how narrow the half-lap |
| `male`, `female` (`*_custom`) | | your own outlines in the unit box |

\include{lineno} elements/element_joint_plate_parameters.cpp
