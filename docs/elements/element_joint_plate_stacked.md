# JointPlate stacked {#elements_joint_plate_stacked}

[TOC]

The top-to-top family: two plates stacked face on face, pinned through their contact by drilled holes.

## Constructors

```cpp
// the family's defaults are division length 6 and shift 0.95, each design reading them its own way;
// the radius is the pin's
static std::shared_ptr<JointPlate> stacked_centre_pin(double radius = 1.0);                                                       // one hole at the centre of the contact
static std::shared_ptr<JointPlate> stacked_polylabel_pin(double radius = 1.0);                                                       // one hole at the polylabel, the centre of the largest inscribed circle
static std::shared_ptr<JointPlate> stacked_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0);               // `divisions` holes on the inscribed circle scaled by the shift
static std::shared_ptr<JointPlate> stacked_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);    // the contact offset inward by the shift (mm), a hole every division length along that ring
static std::shared_ptr<JointPlate> stacked_grid_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);    // a lattice of the division length filling the offset ring
static std::shared_ptr<JointPlate> stacked_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);    // the largest empty rectangle inset by 1 - shift, a hole every division length along its edges, a grid when negative
static std::shared_ptr<JointPlate> stacked_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);  // your own outlines, pairs (face 0, face 1) in the unit box

// placed on the plates' face contact, then passed to each plate in its target order
void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements)
std::shared_ptr<Interaction> interaction(size_t target) const
```

Every example shows two pairs, `Plate::pair_stacked(0.0)` and `Plate::pair_stacked(30.0)`: two stacked plates, square and turned 30 degrees, the upper one lifted.

## stacked_centre_pin: the centre

![stacked_centre_pin](elements/element_joint_plate_stacked_centre_pin.png)

![stacked_centre_pin, the unit box: male pink, female blue](joint_library/tt_stacked_centre_pin_8_unit.png)
![stacked_centre_pin, the plan](elements/element_joint_plate_stacked_centre_pin_plan.png)

One hole at the centre of the contact, drilled through both plates.

\include{lineno} elements/element_joint_plate_stacked_centre_pin.cpp

## stacked_polylabel_pin: the polylabel

![stacked_polylabel_pin](elements/element_joint_plate_stacked_polylabel_pin.png)

![stacked_polylabel_pin, the unit box: male pink, female blue](joint_library/tt_stacked_polylabel_pin_8_unit.png)
![stacked_polylabel_pin, the plan](elements/element_joint_plate_stacked_polylabel_pin_plan.png)

One hole at the polylabel of the contact, the centre of its largest inscribed circle.

\include{lineno} elements/element_joint_plate_stacked_polylabel_pin.cpp

## stacked_circle_pins: holes on the inscribed circle

![stacked_circle_pins](elements/element_joint_plate_stacked_circle_pins.png)

![stacked_circle_pins, the unit box: male pink, female blue](joint_library/tt_stacked_circle_pins_6_0.95_8_unit.png)
![stacked_circle_pins, the plan](elements/element_joint_plate_stacked_circle_pins_plan.png)

Six holes on the largest inscribed circle, its radius scaled by the shift 0.95.

\include{lineno} elements/element_joint_plate_stacked_circle_pins.cpp

## stacked_ring_pins: holes along the offset ring

![stacked_ring_pins](elements/element_joint_plate_stacked_ring_pins.png)

![stacked_ring_pins, the unit box: male pink, female blue](joint_library/tt_stacked_ring_pins_60_12_8_unit.png)
![stacked_ring_pins, the plan](elements/element_joint_plate_stacked_ring_pins_plan.png)

The contact offset inward by the shift, 20 mm, and a hole every 60 along that ring.

\include{lineno} elements/element_joint_plate_stacked_ring_pins.cpp

## stacked_grid_pins: a lattice in the offset ring

![stacked_grid_pins](elements/element_joint_plate_stacked_grid_pins.png)

![stacked_grid_pins, the unit box: male pink, female blue](joint_library/tt_stacked_grid_pins_60_12_8_unit.png)
![stacked_grid_pins, the plan](elements/element_joint_plate_stacked_grid_pins_plan.png)

The contact offset inward by 20 and filled with a lattice of holes 60 apart from the ring's first corner.

\include{lineno} elements/element_joint_plate_stacked_grid_pins.cpp

## stacked_rectangle_pins: holes along the largest rectangle

![stacked_rectangle_pins](elements/element_joint_plate_stacked_rectangle_pins.png)

![stacked_rectangle_pins, the unit box: male pink, female blue](joint_library/tt_stacked_rectangle_pins_60_0.95_8_unit.png)
![stacked_rectangle_pins, the plan](elements/element_joint_plate_stacked_rectangle_pins_plan.png)

The largest empty rectangle in the contact and a hole every 60 along its edges; a negative length fills a grid.

\include{lineno} elements/element_joint_plate_stacked_rectangle_pins.cpp

## stacked_custom: your own outlines

![stacked_custom](elements/element_joint_plate_stacked_custom.png)

![stacked_custom, the unit box: male pink, female blue](joint_library/tt_stacked_custom_unit.png)
![stacked_custom, the plan](elements/element_joint_plate_stacked_custom_plan.png)

Your own outlines in the unit box; here a hidden butterfly key in a bow-tie pocket milled into each plate.

\include{lineno} elements/element_joint_plate_stacked_custom.cpp

## Parameters

A pattern (`stacked_circle_pins` to `stacked_rectangle_pins`) is laid on the contact less a border of two drill radii.

![stacked_circle_pins sweep](joint_library/sweep_stacked_circle_pins.png)
![stacked_ring_pins, stacked_grid_pins and stacked_rectangle_pins sweep](joint_library/sweep_stacked_ring_pins_4_5.png)
