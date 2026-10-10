# Joint library {#joint_library}

[TOC]

Every plate joint of the library drawn as a **tile**, the way the Joinery Solver (Vestartas 2021, chapter 5) and the old compas_wood pages drew it: a joint is a pair of cuts in a unit box, the **male** cut for one plate and the **female** cut for the other, placed on the contact by a change of basis from the box to the joint's two volume rectangles. Each figure shows four views of one design:

- **male tile** (white): what the male plate keeps inside the joint's box;
- **female tile** (yellow): what the female plate keeps inside the same box;
- **both tiles**: together they fill the box and share no material;
- **on the plates**: the pair as the solver cuts it, with the joint's own piece (a key) in orange.

The pictures are not drawings of the design functions but of their result. Every design is built by `tests/joint_library.cpp` on its fixture pair and oriented on the plates' contact. Each plate's cut solid is then intersected with the joint's volume box (its `joint_volumes`), shrunk 1e-4 so that a face lying on the box is left out. That is why the two tiles fill the box exactly: the oracle's checks C4 (no overlap) and C6 (the box filled) hold for every design drawn here. The renders come from `tools/joint_library_figures/` (`make.sh` rebuilds all of them).

Parameters, as `JointPlate`'s constructors take them:

| Parameter | Designs | What it does |
| --- | --- | --- |
| `divisions` | ss_e_ip_1, ss_e_ip_2, ss_e_ip_5, ss_e_op_1, ss_e_op_2, ss_e_op_4 to 6, ss_e_op_17, ts_e_p_2, ts_e_p_3, ts_e_p_5, ss_e_r_2, ss_e_r_3, tt_e_p_2 | how many fingers, tenons, keys or drill rows along the joint line; 0 takes the joint line's length over the family's division length (300 mm in plane and rotated, 450 mm out of plane and top to side) |
| `shift` | ss_e_ip_1, ss_e_op_1, ss_e_op_2, ts_e_p_2, ts_e_p_3, ss_e_r_2, ss_e_r_3, cr_c_ip_1 to 5, tt_e_p_2 to 5 | in-plane fingers: the dovetail lean. Out-of-plane fingers lean only above 0.5, because 2024 reads a shift of 0 as no lean. Tenons: the tenon width against the gap. Cross joints: how narrow the half-lap's centre square is |
| `taper`, `chamfer`, `modify_outline`, `x`, `y`, `z` | ss_e_op_4 | the tenons' side lean, chamfered ends, the floor's edge cut around the tenons, and the tenon box inside the unit box |
| `disable_divisions` | ss_e_op_5 | the second linked joint (Vidy) without divisions |
| `division_length`, `radius` | tt_e_p_0 to 5 | the drill spacing along the contour and the drill radius |

## ss_e_ip: side to side, in plane {#joint_library_ss_e_ip}

Two plates in one plane meeting on a side face; ids 1 to 9. The box spans both thicknesses across the seam and the joint line along it.

![ss_e_ip_0: fixed dovetail](joint_library/tile_ip_ss_e_ip_0.png)
![ss_e_ip_1: parametric dovetails, 8 divisions](joint_library/tile_ip_ss_e_ip_1_8_0.5.png)
![ss_e_ip_2: butterfly pockets, 4 divisions](joint_library/tile_ip_ss_e_ip_2_4.png)
![ss_e_ip_3: milled key](joint_library/tile_ip_ss_e_ip_3.png)
![ss_e_ip_4: milled key with pins](joint_library/tile_ip_ss_e_ip_4.png)
![ss_e_ip_5: butterfly pockets, 4 divisions](joint_library/tile_ip_ss_e_ip_5_4.png)
![ss_e_ip_custom](joint_library/tile_ip_ss_e_ip_custom.png)
![side_removal in plane](joint_library/tile_ip_side_removal_0_0.5.png)

**Divisions and shift.** Divisions set the number of dovetails; the shift leans them: straight at 0.5, dovetailed one way at 0 and the other at 1.

![ss_e_ip_1 sweep](joint_library/sweep_ss_e_ip_1.png)

ss_e_ip_2 and ss_e_ip_5 cut butterfly pockets across the seam, one per division. The butterfly key itself is not modelled as a piece, as in 2024.

![ss_e_ip_2 and ss_e_ip_5 sweep](joint_library/sweep_ss_e_ip_2_5.png)

## ss_e_op: side to side, out of plane {#joint_library_ss_e_op}

Two plates at an angle on a mitred side face, a floor and a wall; ids 10 to 19. The box is the mitre's strip, with the floor's thickness along x, the wall's along y and the joint line along z.

![ss_e_op_0: three fingers](joint_library/tile_op_ss_e_op_0.png)
![ss_e_op_1: fingers, 8 divisions](joint_library/tile_op_ss_e_op_1_8_0.5.png)
![ss_e_op_2: fingers with a non-uniform shift, 8 divisions](joint_library/tile_op_ss_e_op_2_8_0.5.png)
![ss_e_op_3: one tenon](joint_library/tile_op_ss_e_op_3.png)
![ss_e_op_4: tenons, 8 divisions](joint_library/tile_op_ss_e_op_4_8_0_0_1.png)
![ss_e_op_5: linked tenons, 8 divisions](joint_library/tile_op_ss_e_op_5_8_0.png)
![ss_e_op_6: Vidy wall, 8 divisions](joint_library/tile_op_ss_e_op_6_8.png)
![ss_e_op_17](joint_library/tile_op_ss_e_op_17_4.png)
![ss_e_op_tutorial](joint_library/tile_op_ss_e_op_tutorial.png)
![ss_e_op_custom](joint_library/tile_op_ss_e_op_custom.png)
![side_removal out of plane](joint_library/tile_op_side_removal_1_0.5.png)

**Divisions and shift.** Divisions set the number of fingers. The shift skews the fingers through the thickness into a dovetail only above 0.5: 2024 reads a shift of 0 as no shift, so 0 and 0.5 give the same straight fingers.

![ss_e_op_1 sweep](joint_library/sweep_ss_e_op_1.png)

ss_e_op_2 moves the central pairs twice as far as the outer ones. At shift 1 its outline crosses itself and no solid is built, in 2024 as here.

![ss_e_op_2 sweep](joint_library/sweep_ss_e_op_2.png)

**ss_e_op_4's own parameters.** The taper narrows the tenons; the chamfer bevels their ends inside the floor.

![ss_e_op_4 sweep](joint_library/sweep_ss_e_op_4.png)

![ss_e_op_5, ss_e_op_6 and ss_e_op_17 sweep](joint_library/sweep_ss_e_op_5_6_17.png)

## ts_e_p: top to side {#joint_library_ts_e_p}

An upright standing on a base's top face; ids 20 to 29. The box is the base's thickness under the upright: the upright's thickness along x, the base's along y, the joint line along z. The male tile is the tenons; the female tile is the base strip with their mortises.

![ts_e_p_0](joint_library/tile_ts_ts_e_p_0.png)
![ts_e_p_1](joint_library/tile_ts_ts_e_p_1.png)
![ts_e_p_2: 8 divisions](joint_library/tile_ts_ts_e_p_2_8_0.5.png)
![ts_e_p_3: 8 divisions](joint_library/tile_ts_ts_e_p_3_8_0.5.png)
![ts_e_p_4](joint_library/tile_ts_ts_e_p_4.png)
![ts_e_p_custom](joint_library/tile_ts_ts_e_p_custom.png)
![side_removal top to side](joint_library/tile_ts_side_removal_0_0.5.png)

**Divisions and shift.** Divisions set the number of tenons (two of every four divisions are tenons). The shift sets the tenon width against the gap and leans the tenon sides.

![ts_e_p_2 sweep](joint_library/sweep_ts_e_p_2.png)
![ts_e_p_3 sweep](joint_library/sweep_ts_e_p_3.png)

ts_e_p_5, the snap fit, is not drawn here: the oracle skips it because 2024's literals put its hook through the base by design. The datasets top_to_side_box and top_to_side_snap_fit prove it against the 2025 reference.

## ss_e_r: side to side, rotated {#joint_library_ss_e_r}

Two plates folded on a shared side face; ids 50 to 59. ss_e_r_2 and ss_e_r_3 each mill one pocket per plate across the seam and own the loose key that fills both (orange).

![ss_e_r_0](joint_library/tile_r_ss_e_r_0.png)
![ss_e_r_2: keys, 4 divisions](joint_library/tile_r_ss_e_r_2_4_0.5.png)
![ss_e_r_3: keys, 4 divisions](joint_library/tile_r_ss_e_r_3_4_0.5.png)
![ss_e_r_custom](joint_library/tile_r_ss_e_r_custom.png)
![side_removal rotated](joint_library/tile_r_side_removal_0_0.5.png)
![side_removal merged with the joint](joint_library/tile_r_side_removal_1_0.5.png)
![side_removal_ss_e_r_1](joint_library/tile_r_side_removal_ss_e_r_1_0_0.5.png)

**Divisions.** One key per division.

![ss_e_r sweep](joint_library/sweep_ss_e_r.png)

## cr_c_ip: cross {#joint_library_cr_c_ip}

Two plates crossing through each other's slots; ids 30 to 39. The box is the crossing: each plate keeps one half of it. cr_c_ip_0 is the plain half-lap. cr_c_ip_1 to 5 are the conic half-laps of the thesis (Fig. 5.51, 5.55), whose side cuts let the plates slide in at an angle; cr_c_ip_3 to 5 add drills.

![cr_c_ip_0: half-lap](joint_library/tile_cr_cr_c_ip_0.png)
![cr_c_ip_1: conic half-lap](joint_library/tile_cr_cr_c_ip_1_0.5.png)
![cr_c_ip_2](joint_library/tile_cr_cr_c_ip_2.png)
![cr_c_ip_3](joint_library/tile_cr_cr_c_ip_3.png)
![cr_c_ip_4](joint_library/tile_cr_cr_c_ip_4.png)
![cr_c_ip_5](joint_library/tile_cr_cr_c_ip_5.png)
![cr_c_ip_custom](joint_library/tile_cr_cr_c_ip_custom.png)

**Shift.** The shift narrows the half-lap's centre square, 0 the widest and 1 the narrowest. Above 0.85 the slanted side cuts of cr_c_ip_2 to 5 cross over and the ring folds onto itself, in 2024 as here.

![cr_c_ip sweep](joint_library/sweep_cr_c_ip.png)

## tt_e_p: top to top {#joint_library_tt_e_p}

Two plates lying on each other; ids 40 to 49. The joint is a pattern of drills through both plates, so both tiles are the plates' overlap with the same holes.

![tt_e_p_0: one drill](joint_library/tile_tt_tt_e_p_0_8.png)
![tt_e_p_1](joint_library/tile_tt_tt_e_p_1_8.png)
![tt_e_p_2: a ring of drills](joint_library/tile_tt_tt_e_p_2_6_0.95_8.png)
![tt_e_p_3: drills along the contour](joint_library/tile_tt_tt_e_p_3_60_12_8.png)
![tt_e_p_4: a lattice of drills](joint_library/tile_tt_tt_e_p_4_60_12_8.png)
![tt_e_p_5](joint_library/tile_tt_tt_e_p_5_60_0.95_8.png)
![tt_e_p_custom](joint_library/tile_tt_tt_e_p_custom.png)

![tt_e_p_2 sweep](joint_library/sweep_tt_e_p_2.png)
![tt_e_p_3, tt_e_p_4 and tt_e_p_5 sweep](joint_library/sweep_tt_e_p_3_4_5.png)

## b: boundary

b_0 joins one plate on its border contact and cuts nothing: it writes the slices, as 2024 does. `tests/joint_border.cpp` checks it on the dataset boundary_side.
