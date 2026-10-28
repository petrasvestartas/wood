# JointPlate side_side_outplane {#elements_joint_plate_side_side_outplane}

[TOC]

Side-to-side out-of-plane plate joints cutting mitered fingers or tenon-and-mortise pairs along folded plate corners (`Plate::pair_folded(90.0)` and `Plate::pair_folded(60.0)`).

## Constructors

```cpp
static std::shared_ptr<JointPlate> side_side_outplane_three_fingers();
static std::shared_ptr<JointPlate> side_side_outplane_fingers(int divisions = 0, double shift = 0.64);
static std::shared_ptr<JointPlate> side_side_outplane_graded_fingers(int divisions = 0, double shift = 0.64);
static std::shared_ptr<JointPlate> side_side_outplane_miter_tenon();
static std::shared_ptr<JointPlate> side_side_outplane_tenons(
    int divisions = 0,
    double taper = 0.0,
    bool chamfer = true,
    bool modify_outline = true,
    const std::array<double, 2>& x = {-0.5, 0.5},
    const std::array<double, 2>& y = {-0.5, 0.5},
    const std::array<double, 2>& z = {-0.5, 0.5}
);
static std::shared_ptr<JointPlate> side_side_outplane_linked_tenons(int divisions = 0, bool disable_divisions = false);
static std::shared_ptr<JointPlate> side_side_outplane_linked_tenons_wall(int divisions = 0);
static std::shared_ptr<JointPlate> side_side_outplane_capped_fingers(int divisions = 4);
static std::shared_ptr<JointPlate> side_side_outplane_notch();
static std::shared_ptr<JointPlate> side_side_outplane_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements);
std::shared_ptr<Interaction> interaction(size_t target) const;
```

## side_side_outplane_three_fingers: three fingers

Three fixed mitered fingers merged into the floor and wall edges at 90 and 60 degrees.

![](elements/element_joint_plate_side_side_outplane_three_fingers.png)

![](joint_library/op_side_side_outplane_three_fingers_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_three_fingers();
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_three_fingers.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_three_fingers.cpp

</details>

## side_side_outplane_fingers: parametric fingers

Eight divisions producing four mitered fingers on the floor interlocking with four notches in the wall.

![](elements/element_joint_plate_side_side_outplane_fingers.png)

![](joint_library/op_side_side_outplane_fingers_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_fingers(8, 0.5);
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_fingers.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_fingers.cpp

</details>

## side_side_outplane_graded_fingers: graded splay fingers

Eight divisions with a non-uniform shift that splays the central finger pairs twice as far as the outer pairs.

![](elements/element_joint_plate_side_side_outplane_graded_fingers.png)

![](joint_library/op_side_side_outplane_graded_fingers_8_0.5_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_graded_fingers(8, 0.5);
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_graded_fingers.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_graded_fingers.cpp

</details>

## side_side_outplane_miter_tenon: miter tenon and mortise

A single central tenon on the mitered wall edge passing through a matching mortise in the floor lip.

![](elements/element_joint_plate_side_side_outplane_miter_tenon.png)

![](joint_library/op_side_side_outplane_miter_tenon_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_miter_tenon();
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_miter_tenon.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_miter_tenon.cpp

</details>

## side_side_outplane_tenons: chamfered tenons and through mortises

Parametric tenons along the mitered wall edge paired with through-mortises in the floor plate.

![](elements/element_joint_plate_side_side_outplane_tenons.png)

![](joint_library/op_side_side_outplane_tenons_8_0_0_1_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_tenons(8, 0.0, false);
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_tenons.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_tenons.cpp

</details>

## side_side_outplane_linked_tenons: linked tenons

Extended chamfered tenons designed to interlock with adjacent linked joints at multi-plate folded nodes.

![](elements/element_joint_plate_side_side_outplane_linked_tenons.png)

![](joint_library/op_side_side_outplane_linked_tenons_8_0_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_linked_tenons(8, false);
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_linked_tenons.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_linked_tenons.cpp

</details>

## side_side_outplane_linked_tenons_wall: Vidy wall tenons

Linked tenon variant for the Vidy theatre wall seams where the secondary linked joint divisions are disabled.

![](elements/element_joint_plate_side_side_outplane_linked_tenons_wall.png)

![](joint_library/op_side_side_outplane_linked_tenons_wall_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_linked_tenons_wall(8);
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_linked_tenons_wall.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_linked_tenons_wall.cpp

</details>

## Vidy node: four folded plates

A four-plate folded Vidy corner node combining `side_side_outplane_linked_tenons` on the roof seams and `side_side_outplane_linked_tenons_wall` on the wall seams.

![](elements/element_joint_plate_vidy_node.png)

```cpp
const std::shared_ptr<JointPlate> roof_joint = JointPlate::side_side_outplane_linked_tenons(8, false);
const std::shared_ptr<JointPlate> wall_joint = JointPlate::side_side_outplane_linked_tenons_wall(8);
```

<details><summary>Example code: elements/element_joint_plate_vidy_node.cpp</summary>

\include{lineno} elements/element_joint_plate_vidy_node.cpp

</details>

## side_side_outplane_capped_fingers: fingers with flat miter caps

Four divisions producing two interior fingers flanked by flat miter caps at both ends of the seam.

![](elements/element_joint_plate_side_side_outplane_capped_fingers.png)

![](joint_library/op_side_side_outplane_capped_fingers_4_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_capped_fingers(4);
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_capped_fingers.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_capped_fingers.cpp

</details>

## side_side_outplane_notch: single central notch

A single rectangular tongue-and-notch over the middle half of the mitered seam.

![](elements/element_joint_plate_side_side_outplane_notch.png)

![](joint_library/op_side_side_outplane_notch_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_notch();
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_notch.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_notch.cpp

</details>

## side_side_outplane_custom: custom outlines

Custom wide-narrow-wide fingers on the floor plate and matching sockets in the wall plate defined in the `[-0.5, 0.5]^3` unit box.

![](elements/element_joint_plate_side_side_outplane_custom.png)

![](joint_library/op_side_side_outplane_custom_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_custom(male, female);
```

<details><summary>Example code: elements/element_joint_plate_side_side_outplane_custom.cpp</summary>

\include{lineno} elements/element_joint_plate_side_side_outplane_custom.cpp

</details>

## Parameters

**Divisions and shift.** `divisions` sets the finger count; `shift` above `0.5` skews the finger sides into a dovetail.

![](joint_library/sweep_side_side_outplane_fingers.png)

**At other angles.** The same fingers at 90, 120, and 150 degrees leaning with the miter plane.

![](joint_library/sweep_side_side_outplane_fingers_angles.png)

**Chevron pair sweep.** `side_side_outplane_fingers` on a pair of folded plates from the Chevron template across `shift` rows and `divisions` columns.

![](joint_library/sweep_side_side_outplane_fingers_chevron.png)

**Graded finger sweep.** `side_side_outplane_graded_fingers` across increasing `shift` and `divisions`.

![](joint_library/sweep_side_side_outplane_graded_fingers.png)

**Tenon parameters.** `taper` narrows the tenon sides and `chamfer` bevels the tenon tips inside the floor plate.

![](joint_library/sweep_side_side_outplane_tenons.png)

![](joint_library/sweep_side_side_outplane_linked_tenons_wall_capped.png)
