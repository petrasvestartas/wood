# Plate Top-to-Side {#elements_joint_plate_top_side}

[TOC]

Top-to-side plate joints where tenons on the bottom edge of an upright plate (`target 0`, male) fit into mortises through a base plate (`target 1`, female). For the beam milled-wedge variant, see [JointBeam top_side](@ref elements_joint_beam_top_side).

## Constructors

```cpp
static std::shared_ptr<JointPlate> top_side_three_tenons();
static std::shared_ptr<JointPlate> top_side_annen_tenons(double tab_length = 80.0);
static std::shared_ptr<JointPlate> top_side_annen_shifted_tenons(int divisions = 2, double long_edge = 700.0);
static std::shared_ptr<JointPlate> top_side_dense_tenons(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> top_side_tenons(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> top_side_milled_wedge();
static std::shared_ptr<JointPlate> top_side_snap_fit(int divisions = 0);
static std::shared_ptr<JointPlate> top_side_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
std::shared_ptr<Interaction> interaction(size_t target) const;
```

Every example below shows two pairs (`Plate::pair_top_side(90.0)` and `Plate::pair_top_side(60.0)`) with the upright plate lifted above its mortises.

## Three Fixed Tenons
Three fixed tenons on the upright's bottom edge with three matching through-mortises in the base plate.

![](elements/element_joint_plate_top_side_three_tenons.png)

![](joint_library/ts_top_side_three_tenons_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_three_tenons();
```

## Annen Tenons
The through-tenons of Christopher Robeller's 2017 Annen plugin, built on the upright's own edge: its middle 80%, moved up to 200 mm towards the edge middle it shares with a plate across the base, in `4m + 1` equal segments with a tenon on every fourth from the third, each tenon pushed along the insertion vector through the base to its far face, and a slot per tenon in the base; `m` comes from the plate's `feature_divisions` when given, else from `round(0.8 edge / tab_length)`, Christopher's 80 mm tabs by default, 30 mm here for two tenons on the 250 mm edge.

![](elements/element_joint_plate_top_side_annen_tenons.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_annen_tenons(30.0);
```

## Annen Shifted Tenons
The through-tenons of the 2020 Annen shifted-pattern definition (`Annen200917.gh`), built on the line the upright shares with a plate across the base: its own mid-wall line, from the middle of its inner corner on one face and its outer corner on the other at each end, averaged with that of the plate across the base and shortened to its middle 90%, in `4·divisions + 1` equal steps when longer than `long_edge` and `2·divisions + 1` when shorter, a tenon half a step wide around every odd step counted from the edge's start, so the plate across the base, which runs the line the other way, lands its tenons in the even steps. Each tenon is cut by two planes through the insertion vector and the plate's `feature_axes` entry for the face, the plate's normal when none is given, from the base's near face to its far face, with a slot per tenon in the base; an edge shorter than the settings' `limit_min_joint_length` gets none, as the definition skipped it. Here the upright's 250 mm edge is shorter than `long_edge`, so five steps with a tenon at the first and the third.

![](elements/element_joint_plate_top_side_annen_shifted_tenons.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_annen_shifted_tenons();
```

## Every Point Visited
Parametric tenons visiting every interpolation division along the seam with a matching mortise per tenon.

![](elements/element_joint_plate_top_side_dense_tenons.png)

![](joint_library/ts_top_side_dense_tenons_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_dense_tenons(8, 0.5);
```

## Family Default Tenons
The family default placing one tenon every four divisions along the seam (`2` tenons at `divisions = 8`) with a matching mortise per tenon.

![](elements/element_joint_plate_top_side_tenons.png)

![](joint_library/ts_top_side_tenons_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(8, 0.5);
```

## Snap-Fit Tenons
Flexible cantilevered snap-fit tenons repeated once per division with matching through-mortises in the base plate.

![](elements/element_joint_plate_top_side_snap_fit.png)

![](joint_library/ts_top_side_snap_fit_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_snap_fit(0);
```

## Custom Outlines
A user-defined tapered through-tenon and matching tapered mortise defined in the `[-0.5, 0.5]^3` unit box.

![](elements/element_joint_plate_top_side_custom.png)

![](joint_library/ts_top_side_custom_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_custom(male, female);
```

## Parameters

Divisions and shift. `divisions` sets the number of tenons along the seam; `shift` adjusts their width and side lean.

![](joint_library/sweep_top_side_dense_tenons.png)

![](joint_library/sweep_top_side_tenons.png)

At other angles. The same tenons on orthogonal, skewed, and leaning uprights at `8`, `16`, and `24` divisions.

![](joint_library/sweep_top_side_tenons_angles.png)

Snap-fit detail. Each `top_side_snap_fit` tenon ends in a hook that locks underneath the base plate.

![](joint_library/ts_top_side_snap_fit.png)
