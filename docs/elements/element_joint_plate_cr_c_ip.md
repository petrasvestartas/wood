# JointPlate cr_c_ip {#elements_joint_plate_cr_c_ip}

[TOC]

The cross family: a design cut where two plates pass through each other in their planes, a slot or a half-lap into each over its share of the cross's depth, found by `compute_cross_contact`; the 2024 library's contact type 30, ids 30 to 39 (30 `cr_c_ip_0`, the family default id, 31 `cr_c_ip_1`, 32 `cr_c_ip_2`, 33 `cr_c_ip_3`, 34 `cr_c_ip_4`, 35 `cr_c_ip_5`, 38 `side_removal`, 39 `cr_c_ip_custom`; an id without an entry takes `cr_c_ip_0`), used on every cross dataset (`data/cross_*.yml`, id 30, `cr_c_ip_0`) but the Brussels sports tower (`data/cross_brussels_sports_tower.yml`, id 35, `cr_c_ip_5`).

## Constructors

```cpp
// no divisions; the shift is the family's 0.5, and it narrows the half-lap's centre square in cr_c_ip_1 to cr_c_ip_5,
// 0 the widest and 1 the narrowest; cr_c_ip_2 to cr_c_ip_5 fold their bottom sides onto themselves from the shift 0.85 up
static std::shared_ptr<JointPlate> cr_c_ip_0();                    // the plain slot, a rectangle merged into each plate's outline over half the cross's depth; the family default
static std::shared_ptr<JointPlate> cr_c_ip_1(double shift = 0.5);  // the sliced half-lap, nine rings per plate: the centre and the two top sides milled, the two bottom sides and the four corner wedges sliced
static std::shared_ptr<JointPlate> cr_c_ip_2(double shift = 0.5);  // the milled half-lap, five rings per plate, the bottom sides extended 0.15 along the plate, no drill
static std::shared_ptr<JointPlate> cr_c_ip_3(double shift = 0.5);  // the milled half-lap and two diagonal drills through the lap
static std::shared_ptr<JointPlate> cr_c_ip_4(double shift = 0.5);  // the milled half-lap and one vertical drill down its centre
static std::shared_ptr<JointPlate> cr_c_ip_5(double shift = 0.5);  // the Brussels half-lap: the bottom sides extended 0.27 and shortened 0.075, a vertical 50 mm bit and a horizontal 10 mm bit
static std::shared_ptr<JointPlate> cr_c_ip_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' cross contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings())
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example is the same pair: two upright 400 x 200 plates 40 thick crossing at their middles, the first along x and the second along y, the joint oriented on their cross contact and passed to each plate with `add_interaction`, the first plate its target 0 and the second its target 1, the second plate lifted 250 afterwards along the cross's depth, the direction the plates slide into each other, so both cuts read; every section shows the pair and a close-up on the lifted plate. The joint's unit box puts one plate's thickness along x, the other's along y and the cross's depth along z, its middle at z = 0, as 2024's library drew the family: the male side, target 0, is the first plate, its faces at y = -0.5 and 0.5, cut on the z < 0 half, from the middle of the cross to its top edge here; the female, the second, its faces at x = -0.5 and 0.5, is cut on the z > 0 half, to its bottom edge. No design owns a piece, so the joint stays hidden. `cr_c_ip_0` and `cr_c_ip_custom` merge their rectangles into the plates' outlines, so the slots are cut from the plates; the half-laps of `cr_c_ip_1` to `cr_c_ip_5` are solids taken from the plates, every ring drawn on its plate as a cut feature, with the drills as the lines 2024 declared them.

## cr_c_ip_0: the plain slot

![cr_c_ip_0](elements/element_joint_plate_cr_c_ip_0.png)
![cr_c_ip_0, the lifted plate](elements/element_joint_plate_cr_c_ip_0_joint.png)

The family default: a rectangle merged into each plate's outline over half the cross's depth, a slot from the top of the first plate and from the bottom of the second, clipped into the outline by Clipper2 at two decimals as 2024 clipped it, which leaves the slot on a 1/128 mm grid in the frame of the face. The id 30, every cross dataset but Brussels.

\include{lineno} elements/element_joint_plate_cr_c_ip_0.cpp

## cr_c_ip_1: the sliced half-lap

![cr_c_ip_1](elements/element_joint_plate_cr_c_ip_1.png)
![cr_c_ip_1, the lifted plate](elements/element_joint_plate_cr_c_ip_1_joint.png)

The half-lap as nine rings per plate on the shift, every one a solid taken from the plate: the centre square and the two top sides milled, the two bottom sides and the four corner wedges sliced; face 1 is every ring offset along its normal, the male the female with its axes swapped and its depth flipped, as 2024 built it. The id 31.

\include{lineno} elements/element_joint_plate_cr_c_ip_1.cpp

## cr_c_ip_2: the milled half-lap

![cr_c_ip_2](elements/element_joint_plate_cr_c_ip_2.png)
![cr_c_ip_2, the lifted plate](elements/element_joint_plate_cr_c_ip_2_joint.png)

The five rings 2024 wrote four times for `cr_c_ip_2` to `cr_c_ip_5`, one body here: the centre square milled, the two top sides as the two sheer walls between each ring and its offset, the two bottom sides milled, their first and third segments extended 0.15 along the plate at both ends and their slanted segments 0.6 up and down so the cut clears the plate; no drill. The id 32.

\include{lineno} elements/element_joint_plate_cr_c_ip_2.cpp

## cr_c_ip_3: two diagonal drills

![cr_c_ip_3](elements/element_joint_plate_cr_c_ip_3.png)
![cr_c_ip_3, the lifted plate](elements/element_joint_plate_cr_c_ip_3_joint.png)

The milled half-lap of `cr_c_ip_2` and two diagonal drills through the lap, each a line in the unit box bored through both plates. The id 33.

\include{lineno} elements/element_joint_plate_cr_c_ip_3.cpp

## cr_c_ip_4: one vertical drill

![cr_c_ip_4](elements/element_joint_plate_cr_c_ip_4.png)
![cr_c_ip_4, the lifted plate](elements/element_joint_plate_cr_c_ip_4_joint.png)

The milled half-lap of `cr_c_ip_2` and one vertical drill down the centre of the lap, along the cross's depth through both plates; the line keeps where 2024 declared it, where 2024 offset it too, reading past its ring and length arrays. The id 34.

\include{lineno} elements/element_joint_plate_cr_c_ip_4.cpp

## cr_c_ip_5: the Brussels half-lap

![cr_c_ip_5](elements/element_joint_plate_cr_c_ip_5.png)
![cr_c_ip_5, the lifted plate](elements/element_joint_plate_cr_c_ip_5_joint.png)

The milled half-lap with the asymmetry 2024 gave the Brussels sports tower, the bottom sides extended 0.27 on their first segment and shortened 0.075 on their third, a vertical 50 mm bit down the centre of the lap and a horizontal 10 mm bit below it, each bored through both plates, the bits named in `FabricationType` as `drill_50` and `drill_10`. The id 35, the Brussels dataset.

\include{lineno} elements/element_joint_plate_cr_c_ip_5.cpp

## cr_c_ip_custom: your own outlines

![cr_c_ip_custom](elements/element_joint_plate_cr_c_ip_custom.png)
![cr_c_ip_custom, the lifted plate](elements/element_joint_plate_cr_c_ip_custom_joint.png)

Your own outlines in the unit box, pairs (face 0, face 1) per side, the male's on the faces at y = -0.5 and 0.5, the first plate's on this pair, and the female's on the faces at x = -0.5 and 0.5, the second's, kept pair by pair as the 2024 library kept a custom design: the outlines carry the fabrication type nothing, and only a closed rectangle of five points, or a line of two, is merged into the plate's outline; here the slots split the cross's depth unequally, 60 down from the first plate's top edge and 140 up from the second's bottom edge, where `cr_c_ip_0` halves it. The id 39.

\include{lineno} elements/element_joint_plate_cr_c_ip_custom.cpp

## Angles

![cr_c_ip_2 crossing at 45 degrees](elements/element_joint_plate_cr_c_ip_angle_45.png)

Two plates crossing at 45 degrees: the milled half-laps of `cr_c_ip_2` follow the oblique crossing. The oracle runs every design of the family on its defaults crossing at 60 and 45 degrees (`cr@60`, `cr@45`); the slots are merged clipped on 2024's 0.01 mm grid in each plate's frame, so an oblique slot wall may stand half a grid step off, which the oracle allows over the walls of the crossing block and no more.

\include{lineno} elements/element_joint_plate_cr_c_ip_angle_45.cpp
