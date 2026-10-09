# JointPlate {#elements_joint_plate}

[TOC]

A joint between two plates: a design of the joint library, oriented on the plates' contact, that cuts its profile into both outlines.

## Constructors

```cpp
// the library designs, each by its own name and parameters
static std::shared_ptr<JointPlate> ss_e_ip_0() .. ss_e_ip_5(int divisions = 0), ss_e_ip_1(int divisions = 0, double shift = 0.5)   // 0 divisions: one every 300 mm of the joint line; see the ss_e_ip page
static std::shared_ptr<JointPlate> ss_e_op_0() .. ss_e_op_6(int divisions = 0)   // 0 divisions: one every 450 mm of the joint line, shift 0.64
static std::shared_ptr<JointPlate> ts_e_p_0() .. ts_e_p_5(int divisions = 4)
static std::shared_ptr<JointPlate> ss_e_r_0() .. ss_e_r_3(int divisions = 4, double shift = 0.5)
static std::shared_ptr<JointPlate> cr_c_ip_0() .. cr_c_ip_5()
static std::shared_ptr<JointPlate> tt_e_p_0(double radius = 1.0, double chord_tolerance = 0.05) .. tt_e_p_5(double spacing = 30.0, double radius = 1.0, double chord_tolerance = 0.05)
static std::shared_ptr<JointPlate> b_0()
static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5)
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
| `ss_e_op` | side to side, out of plane | two plates at an angle on a shared side face |
| `ts_e_p` | top to side | a plate standing on another's face |
| `ss_e_r` | side to side, rotated | two side faces whose edges cross |
| `cr_c_ip` | cross | two plates passing through each other (`compute_cross_contact`) |
| `tt_e_p` | top to top | two plates stacked face on face |
| `b` | boundary | found by the solver only |

## ts_e_p: top to side

![ts_e_p](elements/element_joint_plate_ts_e_p.png)

`ts_e_p_0` to `ts_e_p_3`, tenons on the upright and mortises through the base, the upright lifted off to show both.

\include{lineno} elements/element_joint_plate_ts_e_p.cpp

## ss_e_ip: in plane

`ss_e_ip_0` to `ss_e_ip_5` and `ss_e_ip_custom` on two plates edge to edge in one plane, each design with its own example and picture on the [ss_e_ip page](@ref elements_joint_plate_ss_e_ip): the finger designs `ss_e_ip_0` and `ss_e_ip_1`, the key designs `ss_e_ip_2` (butterflies) and `ss_e_ip_5` (reversed teeth), whose keys the joint owns, the milled grooves with drills `ss_e_ip_3` and `ss_e_ip_4`, and the custom outlines.

## ss_e_op: out of plane

![ss_e_op](elements/element_joint_plate_ss_e_op.png)

`ss_e_op_3` to `ss_e_op_5` on a floor and a wall mitred at a right angle, the wall moved off to show the fingers of both.

\include{lineno} elements/element_joint_plate_ss_e_op.cpp

## cr_c_ip: cross

![cr_c_ip](elements/element_joint_plate_cr_c_ip.png)

`cr_c_ip_0` to `cr_c_ip_5` on two upright plates crossing, oriented on their `InteractionContactCross`, the second plate lifted to show the slots.

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

- `ss_e_op_0`, `ss_e_op_1` and `ss_e_op_2` on a mitred right-angle corner merge an outline that leaves the plate's plane, so the faces draw wrong; they are left out until the library merges them right.
- `ss_e_r_*` need two side faces whose edges cross; on a flat board butting an upright one the contact is not read as rotated and nothing is cut, so the family is shown only through the solver's datasets.
- `side_removal` on the same corner removes the whole wall instead of its side; it is meant for the solver, where it merges with the joint on that edge.
- `b_0` (boundary) has no face contact to orient on: the solver places it.
- `JointAnnen` and `JointVidy` are built from a whole dataset's plates, connections and groups, not from one contact.
