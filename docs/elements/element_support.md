# Support {#elements_support}

[TOC]

A steel column base with the manufacturer's dimensions on a plane: base plate, tube, head plate, pins and anchors. `Joint::support` lets its head plate into a column end and drills the column pins, a subtract feature from the joint.

## Constructors

```cpp
explicit Support(const Plane& plane, const std::string& name = "support")
Point column_foot() const
static std::shared_ptr<Joint> Joint::support(const Support& support, const Column& column)
```

## Parameters

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `plane` | xy | the base plate underside centre, z up the column, x along a base plate side |
| `height` | 150 | base plate underside to head plate top, 150 to 200 by the adjustment |
| `head_plate_diameter`, `head_plate_thickness`, `head_plate_recess` | 106, 12, 12 | the head plate disc and how deep it is let into the column |
| `base_plate_size`, `base_plate_thickness` | 140, 12 | the square base plate |
| `base_plate_hole_diameter`, `base_plate_hole_spacing` | 15, 104 | the four anchor drillings |
| `adjustment_nut_across_flats`, `adjustment_nut_top` | 36, 64 | the hexagon on the base plate |
| `rod_diameter` | 30 | the threaded rod between the nuts |
| `coupling_nut_across_flats`, `coupling_nut_height` | 55, 30 | the hexagon under the head plate |
| `pin_count`, `pin_diameter`, `pin_length`, `pin_angle`, `pin_circle_diameter` | 3, 8, 180, 25, 50 | the pins from the head plate up into the column |
| `anchor_diameter`, `anchor_embedment` | 12, 100 | the anchors into the slab |
| `chord_tolerance` | 0.05 | how far a round part's facets may stray |

## On a plane

![On a plane](elements/element_support.png)

A support on the xy plane, written as its exact BRep: the round parts and the four anchor holes of the base plate are exact cylinders.

\include{lineno} elements/element_support.cpp
