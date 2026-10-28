# JointPlate top_side {#elements_joint_plate_top_side}

[TOC]

Top-to-side plate joints where tenons on the bottom edge of an upright plate (`target 0`, male) fit into mortises through a base plate (`target 1`, female). For the beam milled-wedge variant, see [JointBeam top_side](@ref elements_joint_beam_top_side).

## Constructors

```cpp
static std::shared_ptr<JointPlate> top_side_three_tenons();
static std::shared_ptr<JointPlate> top_side_annen_tenons();
static std::shared_ptr<JointPlate> top_side_dense_tenons(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> top_side_tenons(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> top_side_milled_wedge();
static std::shared_ptr<JointPlate> top_side_snap_fit(int divisions = 0);
static std::shared_ptr<JointPlate> top_side_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
std::shared_ptr<Interaction> interaction(size_t target) const;
```

Every example below shows two pairs (`Plate::pair_top_side(90.0)` and `Plate::pair_top_side(60.0)`) with the upright plate lifted above its mortises.

## top_side_three_tenons: three fixed tenons

Three fixed tenons on the upright's bottom edge with three matching through-mortises in the base plate.

![](elements/element_joint_plate_top_side_three_tenons.png)

![](joint_library/ts_top_side_three_tenons_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_three_tenons();
```

<details><summary>Example code: elements/element_joint_plate_top_side_three_tenons.cpp</summary>

\include{lineno} elements/element_joint_plate_top_side_three_tenons.cpp

</details>

## top_side_annen_tenons: Annen tenons

Two wide fixed tenons from the Annen box-girder system on the upright's bottom edge and two through-mortises in the base plate.

![](elements/element_joint_plate_top_side_annen_tenons.png)

![](joint_library/ts_top_side_annen_tenons_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_annen_tenons();
```

<details><summary>Example code: elements/element_joint_plate_top_side_annen_tenons.cpp</summary>

\include{lineno} elements/element_joint_plate_top_side_annen_tenons.cpp

</details>

## top_side_dense_tenons: every point visited

Parametric tenons visiting every interpolation division along the seam with a matching mortise per tenon.

![](elements/element_joint_plate_top_side_dense_tenons.png)

![](joint_library/ts_top_side_dense_tenons_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_dense_tenons(8, 0.5);
```

<details><summary>Example code: elements/element_joint_plate_top_side_dense_tenons.cpp</summary>

\include{lineno} elements/element_joint_plate_top_side_dense_tenons.cpp

</details>

## top_side_tenons: family default tenons

The family default placing one tenon every four divisions along the seam (`2` tenons at `divisions = 8`) with a matching mortise per tenon.

![](elements/element_joint_plate_top_side_tenons.png)

![](joint_library/ts_top_side_tenons_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(8, 0.5);
```

<details><summary>Example code: elements/element_joint_plate_top_side_tenons.cpp</summary>

\include{lineno} elements/element_joint_plate_top_side_tenons.cpp

</details>

## top_side_snap_fit: snap-fit tenons

Flexible cantilevered snap-fit tenons repeated once per division with matching through-mortises in the base plate.

![](elements/element_joint_plate_top_side_snap_fit.png)

![](joint_library/ts_top_side_snap_fit_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_snap_fit(0);
```

<details><summary>Example code: elements/element_joint_plate_top_side_snap_fit.cpp</summary>

\include{lineno} elements/element_joint_plate_top_side_snap_fit.cpp

</details>

## top_side_custom: custom outlines

A user-defined tapered through-tenon and matching tapered mortise defined in the `[-0.5, 0.5]^3` unit box.

![](elements/element_joint_plate_top_side_custom.png)

![](joint_library/ts_top_side_custom_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_side_custom(male, female);
```

<details><summary>Example code: elements/element_joint_plate_top_side_custom.cpp</summary>

\include{lineno} elements/element_joint_plate_top_side_custom.cpp

</details>

## Parameters

**Divisions and shift.** `divisions` sets the number of tenons along the seam; `shift` adjusts their width and side lean.

![](joint_library/sweep_top_side_dense_tenons.png)

![](joint_library/sweep_top_side_tenons.png)

**At other angles.** The same tenons on orthogonal, skewed, and leaning uprights at `8`, `16`, and `24` divisions.

![](joint_library/sweep_top_side_tenons_angles.png)

**Snap-fit detail.** Each `top_side_snap_fit` tenon ends in a hook that locks underneath the base plate.

![](joint_library/ts_top_side_snap_fit.png)
