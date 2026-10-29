# Plate Top-to-Top {#elements_joint_plate_top_top}

[TOC]

Top-to-top plate joints connecting two face-stacked plates (`Plate::pair_stacked(0.0)` and `Plate::pair_stacked(30.0)`) with drilled pin patterns or inlaid surface pockets across their shared `InteractionContactFace`.

## Constructors

```cpp
static std::shared_ptr<JointPlate> top_top_centre_pin(double radius = 1.0);
static std::shared_ptr<JointPlate> top_top_polylabel_pin(double radius = 1.0);
static std::shared_ptr<JointPlate> top_top_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0);
static std::shared_ptr<JointPlate> top_top_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);
static std::shared_ptr<JointPlate> top_top_grid_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);
static std::shared_ptr<JointPlate> top_top_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);
static std::shared_ptr<JointPlate> top_top_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements);
std::shared_ptr<Interaction> interaction(size_t target) const;
```

## Centroid Pin
Drills a single hole at the centroid of the contact polygon through both stacked plates.

![](elements/element_joint_plate_top_top_centre_pin.png)

![](joint_library/tt_top_top_centre_pin_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_top_centre_pin(8.0);
```

## Pole of Inaccessibility Pin
Drills a single hole at the polylabel (centre of the largest inscribed circle) of the contact polygon.

![](elements/element_joint_plate_top_top_polylabel_pin.png)

![](joint_library/tt_top_top_polylabel_pin_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_top_polylabel_pin(8.0);
```

## Inscribed Circle Pins
Drills `divisions` equally spaced holes around the contact's largest inscribed circle scaled by `shift`.

![](elements/element_joint_plate_top_top_circle_pins.png)

![](joint_library/tt_top_top_circle_pins_6_0.95_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_top_circle_pins(6, 0.95, 8.0);
```

## Offset Perimeter Ring Pins
Offsets the contact perimeter inward by `shift` and drills a hole every `division_length` along the inset ring.

![](elements/element_joint_plate_top_top_ring_pins.png)

![](joint_library/tt_top_top_ring_pins_60_12_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_top_ring_pins(60.0, 20.0, 8.0);
```

## Interior Lattice Pins
Fills the inward-offset contact polygon with a 2D lattice of holes spaced by `division_length`.

![](elements/element_joint_plate_top_top_grid_pins.png)

![](joint_library/tt_top_top_grid_pins_60_12_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_top_grid_pins(60.0, 20.0, 8.0);
```

## Inscribed Rectangle Pins
Finds the largest axis-aligned rectangle inside the contact and places holes every `division_length` along its edges (or fills a grid when `division_length` is negative).

![](elements/element_joint_plate_top_top_rectangle_pins.png)

![](joint_library/tt_top_top_rectangle_pins_60_0.95_8_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_top_rectangle_pins(60.0, 0.95, 8.0);
```

## Custom Face Outlines
Mills a custom hidden bow-tie pocket into the mating faces of both stacked plates to house an internal butterfly key.

![](elements/element_joint_plate_top_top_custom.png)

![](joint_library/tt_top_top_custom_unit.png)

```cpp
const std::shared_ptr<JointPlate> joint = JointPlate::top_top_custom(male, female);
```

## Parameters

All multi-pin patterns (`top_top_circle_pins` through `top_top_rectangle_pins`) keep a minimum edge clearance of two pin radii from the contact boundary.

![](joint_library/sweep_top_top_circle_pins.png)

![](joint_library/sweep_top_top_pins.png)
