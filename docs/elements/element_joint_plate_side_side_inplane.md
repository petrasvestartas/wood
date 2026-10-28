# JointPlate side_side_inplane {#elements_joint_plate_side_side_inplane}

[TOC]

Side-to-side in-plane plate joints cutting finger outlines, key pockets, or Hilti connectors along the shared seam of two coplanar or shallowly folded plates. For end-to-end beam scarf joints, see [JointBeam end to end](@ref elements_joint_beam_end_to_end).

## Constructors

```cpp
static std::shared_ptr<JointPlate> side_side_inplane_three_fingers();
static std::shared_ptr<JointPlate> side_side_inplane_dovetails(int divisions = 0, double shift = 0.5);
static std::shared_ptr<JointPlate> side_side_inplane_butterfly_keys(int divisions = 0);
static std::shared_ptr<JointPlate> side_side_inplane_splayed_scarf();
static std::shared_ptr<JointPlate> side_side_inplane_cross_splayed_scarf();
static std::shared_ptr<JointPlate> side_side_inplane_tooth_keys(int divisions = 0);
static std::shared_ptr<JointPlate> side_side_inplane_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

static std::shared_ptr<JointPlate> side_side_inplane_hilti(
    const Plate& a,
    const Plate& b,
    const InteractionContactFace& contact,
    double half_length = 120.0,
    double neck_length = 27.7,
    double neck_width = 40.0,
    double taper_end = 74.4,
    double wing_width = 90.0,
    double depth = 93.0,
    double height = 90.0,
    double rod_diameter = 12.0,
    double disc_diameter = 50.0,
    double disc_thickness = 6.0,
    double router_radius = 20.0,
    int sides = 32
);

void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements);
std::shared_ptr<Interaction> interaction(size_t target) const;
```

Every standard example shows two pairs (`Plate::pair_in_plane(180.0)` coplanar and `Plate::pair_in_plane(165.0)` shallow V) separated to reveal the seam geometry.

## side_side_inplane_three_fingers: three fingers

Three fixed 20 mm deep rectangular fingers merged into both plate edges.

![](elements/element_joint_plate_side_side_inplane_three_fingers.png)

![](joint_library/ip_side_side_inplane_three_fingers_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_inplane_three_fingers();
```

<details><summary>Example code: elements/element_joint_plate_side_side_inplane_three_fingers.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_inplane_three_fingers.cpp

</details>

## side_side_inplane_dovetails: parametric fingers

Eight divisions at `shift = 0.5` producing four interlocking fingers per plate along the seam.

![](elements/element_joint_plate_side_side_inplane_dovetails.png)

![](joint_library/ip_side_side_inplane_dovetails_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_inplane_dovetails(8, 0.5);
```

<details><summary>Example code: elements/element_joint_plate_side_side_inplane_dovetails.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_inplane_dovetails.cpp

</details>

## side_side_inplane_butterfly_keys: butterfly keys

Four butterfly pockets cut across the seam along with the four butterfly key blocks that lock both plates together.

![](elements/element_joint_plate_side_side_inplane_butterfly_keys.png)

![](joint_library/ip_side_side_inplane_butterfly_keys_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_inplane_butterfly_keys(4);
```

<details><summary>Example code: elements/element_joint_plate_side_side_inplane_butterfly_keys.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_inplane_butterfly_keys.cpp

</details>

## side_side_inplane_tooth_keys: reversed-tooth keys

Four reversed eight-point tooth pockets cut across the seam along with their matching key blocks.

![](elements/element_joint_plate_side_side_inplane_tooth_keys.png)

![](joint_library/ip_side_side_inplane_tooth_keys_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_inplane_tooth_keys(4);
```

<details><summary>Example code: elements/element_joint_plate_side_side_inplane_tooth_keys.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_inplane_tooth_keys.cpp

</details>

## side_side_inplane_custom: custom outlines

A custom feather joint of eight alternating V-teeth defined in the `[-0.5, 0.5]^3` unit box and merged into both plate edges.

![](elements/element_joint_plate_side_side_inplane_custom.png)

![](joint_library/ip_side_side_inplane_custom_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_inplane_custom(male, female);
```

<details><summary>Example code: elements/element_joint_plate_side_side_inplane_custom.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_inplane_custom.cpp

</details>

## side_side_inplane_hilti: Hilti connector

A bow-tie pocket milled into two 200 mm CLT slabs folded at 30 degrees, housing plywood key halves, recessed steel washers, and a central tension bolt.

![](hilti_reference_photo.jpg)

![](elements/element_joint_plate_side_side_inplane_hilti.png)

```cpp
const std::shared_ptr<JointPlate> hilti = JointPlate::side_side_inplane_hilti(*left, *right, *contact);
scene.add(hilti);
scene.add_interaction(hilti, left, hilti->interaction(0));
scene.add_interaction(hilti, right, hilti->interaction(1));
```

<details><summary>Example code: elements/element_joint_plate_side_side_inplane_hilti.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_inplane_hilti.cpp

</details>

## side_side_inplane_hilti at six fold angles

Six slab pairs folded from 0 to 50 degrees in 10-degree steps, showing how the pocket depth tracks the fold angle while the connector key remains invariant.

![](elements/element_joint_plate_side_side_inplane_hilti_angles.png)

```cpp
for (int i = 0; i < 6; i++) {
    const std::shared_ptr<JointPlate> hilti = JointPlate::side_side_inplane_hilti(*a, *b, *contact);
}
```

<details><summary>Example code: elements/element_joint_plate_side_side_inplane_hilti_angles.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_inplane_hilti_angles.cpp

</details>

## Parameters

**Divisions and shift.** `divisions` sets the number of fingers; `shift` controls the dovetail lean (`0.5` rectangular, `0.0` and `1.0` opposite dovetail tapers).

![](joint_library/sweep_side_side_inplane_dovetails.png)

**On other seams.** The same dovetails mapped onto orthogonal, 75-degree slanted, and short seams.

![](joint_library/sweep_side_side_inplane_dovetails_seams.png)

**Key pocket sweeps.** `side_side_inplane_butterfly_keys` and `side_side_inplane_tooth_keys` across increasing division counts.

![](joint_library/sweep_side_side_inplane_keys.png)
