# JointPlate {#elements_joint_plate}

[TOC]

A joint between two plates: a design of the joint library, oriented on the plates' contact, that cuts its profile into both outlines.

## Constructors

```cpp
// the library designs, each by its own name and parameters
static std::shared_ptr<JointPlate> ss_e_ip_0() .. ss_e_ip_5(int divisions = 0), ss_e_ip_1(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line; see the ss_e_ip page
static std::shared_ptr<JointPlate> ss_e_op_0() .. ss_e_op_6(int divisions = 0), ss_e_op_17(int divisions = 4), ss_e_op_tutorial()   // 0 divisions: one every 450 mm of the joint line, shift 0.64; see the ss_e_op page
static std::shared_ptr<JointPlate> ts_e_p_0(), ts_e_p_1(), ts_e_p_4(), ts_e_p_5(int divisions = 0), ts_e_p_2(int divisions = 0, double shift = 0.5), ts_e_p_3(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 450 mm of the joint line, shift 0.5; see the ts_e_p page
static std::shared_ptr<JointPlate> ss_e_r_0() .. ss_e_r_3(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line; ss_e_r_1 is the tile of side_removal_ss_e_r_1, not a design of its own
static std::shared_ptr<JointPlate> cr_c_ip_0(), cr_c_ip_1(double shift = 0.5) .. cr_c_ip_5(double shift = 0.5)   // no divisions; the shift narrows the half-lap's centre square, 0 the widest, 1 the narrowest
static std::shared_ptr<JointPlate> tt_e_p_0(double radius = 1.0, double chord_tolerance = 0.05) .. tt_e_p_5(double spacing = 30.0, double radius = 1.0, double chord_tolerance = 0.05)
static std::shared_ptr<JointPlate> b_0()
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5), side_removal_ss_e_r_1(bool merge_with_joint = false, double shift = 0.5)
static std::shared_ptr<JointPlate> <family>_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female)

// placed on a contact, then passed to each plate
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
std::shared_ptr<Interaction> interaction(size_t target) const
```

## How it is used

A design is made by name, oriented on the contact of its two plates, added, and passed to each plate in its target order:

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_3(16, 0.5);
joint->orient(contact, {base, upright});
scene.add(joint);
scene.add_interaction(joint, upright, joint->interaction(0));
scene.add_interaction(joint, base, joint->interaction(1));
```

The target order is the joint's, not the contact's: `orient` turns a side-to-top contact so target 0 is the plate standing on the other (its tenons) and target 1 the plate it stands on (its mortises), and an out-of-plane contact so target 0 is the second plate of the contact, the one the detector makes the male (the wall of a floor and wall corner); every other family keeps the contact's order. `add_interaction` refuses a side handed to the other plate, so the order cannot swap the cuts between them. What a side merges into its plate's outline (edge insertions, holes) and what it takes out as a solid (mills, slices, cuts, conics, drills) is decided by each outline's `FabricationType`. Each design belongs to one contact family, by the first letters of its name:

| Family | Contact | Plates |
| --- | --- | --- |
| [`ss_e_ip`](@ref elements_joint_plate_ss_e_ip) | side to side, in plane | two plates edge to edge in one plane, their fingers interlocking along the normal, a loose key on the key designs |
| [`ss_e_op`](@ref elements_joint_plate_ss_e_op) | side to side, out of plane | two plates at an angle on a shared side face, a floor and a wall at a corner, fingers on both edges or the wall's tenons through the floor's mortises |
| [`ts_e_p`](@ref elements_joint_plate_ts_e_p) | top to side | a plate standing on another's face, the upright's tenons through the base's mortises |
| `ss_e_r` | side to side, rotated | two side faces whose edges cross, or any side-to-side pair under `settings.all_treated_as_rotated` |
| `cr_c_ip` | cross | two plates passing through each other (`compute_cross_contact`) |
| `tt_e_p` | top to top | two plates stacked face on face |
| `b` | boundary | found by the solver only |

## ts_e_p: top to side

`ts_e_p_0` to `ts_e_p_5` and `ts_e_p_custom` on an upright standing in the middle of a base, each design with its own example and pictures on the [ts_e_p page](@ref elements_joint_plate_ts_e_p): the fixed three tenons of `ts_e_p_0` and the two Annen tenons of `ts_e_p_1`, the parametric tenons `ts_e_p_2` and `ts_e_p_3` (the family default, ids 20 and 22), the milled wedge `ts_e_p_4` (ids 23 and 24), the snap-fit tenon `ts_e_p_5` (id 25), which keeps the upright's thickness along the joint line instead of stretching to it, and the custom outlines, kept pair by pair as 2024 kept them.

## ss_e_ip: in plane

`ss_e_ip_0` to `ss_e_ip_5` and `ss_e_ip_custom` on two plates edge to edge in one plane, each design with its own example and picture on the [ss_e_ip page](@ref elements_joint_plate_ss_e_ip): the finger designs `ss_e_ip_0` and `ss_e_ip_1`, the key designs `ss_e_ip_2` (butterflies) and `ss_e_ip_5` (reversed teeth), whose keys the joint owns, the milled grooves with drills `ss_e_ip_3` and `ss_e_ip_4`, and the custom outlines.

## ss_e_op: out of plane

`ss_e_op_0` to `ss_e_op_6`, `ss_e_op_17`, `ss_e_op_tutorial` and `ss_e_op_custom` on a floor and a wall mitred at a right angle, each design with its own example and pictures on the [ss_e_op page](@ref elements_joint_plate_ss_e_op): the finger designs `ss_e_op_0`, `ss_e_op_1`, `ss_e_op_2` and `ss_e_op_17`, the one-notch `ss_e_op_tutorial`, the tenon and mortise designs `ss_e_op_3`, `ss_e_op_4`, `ss_e_op_5` and `ss_e_op_6`, whose tenons on the wall pass through mortises in the floor, and the custom outlines.

## cr_c_ip: cross

![cr_c_ip](elements/element_joint_plate_cr_c_ip.png)

`cr_c_ip_0` to `cr_c_ip_5` on two upright plates crossing, oriented on their `InteractionContactCross`, the second plate lifted to show the slots: the plain slot `cr_c_ip_0` (id 30, the family default, a rectangle merged into each plate's outline over half the cross's depth, clipped into it by Clipper2 at two decimals as 2024 clipped it, which leaves the slot on a 1/128 mm grid), and the milled half-lap `cr_c_ip_1` to `cr_c_ip_5` (ids 31 to 35), every outline a solid taken from the plate, its centre square narrowed by the shift, with 2024's drills: two diagonal in `cr_c_ip_3`, one vertical in `cr_c_ip_4`, a vertical 50 mm and a horizontal 10 mm bit in `cr_c_ip_5`, whose bottom sides are extended 0.27 on one segment and shortened 0.075 on the other as 2024 built the Brussels sports tower; `cr_c_ip_custom` (id 39) keeps your outlines pair by pair as 2024 kept them.

\include{lineno} elements/element_joint_plate_cr_c_ip.cpp

## tt_e_p: top to top

![tt_e_p](elements/element_joint_plate_tt_e_p.png)

`tt_e_p_0` to `tt_e_p_5` on two stacked plates, each drilling pins through both: one, a ring of six, or a grid at a spacing; the upper plate lifted to show the holes.

\include{lineno} elements/element_joint_plate_tt_e_p.cpp

## Parameters

![parameters of ts_e_p_3](elements/element_joint_plate_parameters.png)

`ts_e_p_3` six times: back row 8, 16 and 24 divisions, front row shift 0, 0.25 and 1.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `divisions` | per design | how many tenons, fingers or slots the contact is split into (`ts_e_p_3`: a tenon per four, at least eight) |
| `shift` | 0.5 | how the tenon sides lean: square at 0.5, a dovetail one way below and the other way above |
| `radius` (`tt_e_p`) | 1.0 | the radius of every pin hole |
| `spacing` (`tt_e_p_3..5`) | 30 | the distance between pins in the grid |
| `count`, `circle_radius` (`tt_e_p_2`) | 6, 20 | how many pins stand on a ring, and its radius |
| `taper`, `chamfer`, `x`, `y`, `z` (`ss_e_op_4`) | 0, true, ±0.5 | the finger taper, chamfered finger ends, the finger box in the joint's unit frame |
| `merge_with_joint` (`side_removal`) | false | the side removal merged into a joint already on that edge |
| `male`, `female` (`*_custom`) | | your own outlines in the joint's unit frame |

\include{lineno} elements/element_joint_plate_parameters.cpp

## Not shown

- `ss_e_r_*` need a rotated contact: `element_joint_plate_ss_e_r_3` shows the family on the in-plane pair with `settings.all_treated_as_rotated` on; the others and `side_removal_ss_e_r_1` are shown through the solver's datasets.
- `side_removal` on the same corner removes the whole wall instead of its side; it is meant for the solver, where it merges with the joint on that edge.
- `b_0` (boundary) has no face contact to orient on: the solver places it.
- `JointAnnen` and `JointVidy` are built from a whole dataset's plates, connections and groups, not from one contact.
