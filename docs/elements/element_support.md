# Support {#elements_support}

[TOC]

A steel column base assembly comprising a square base plate, anchor bolts, threaded adjustment rod, nuts, circular head plate, and inclined column pins. `Joint::support` recesses the head plate and pin bores into a column base.

## Constructors

```cpp
explicit Support(const Plane& plane, const std::string& name = "support");
Point column_foot() const;
static std::shared_ptr<Joint> Joint::support(const Support& support, const Column& column);
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `plane` | `xy` | Base plate underside centre and orientation (`z` up the column, `x` along a base plate edge) |
| `height` | `150` | Distance from base plate underside to head plate top (`150` to `200 mm` adjustment range) |
| `head_plate_diameter`, `head_plate_thickness`, `head_plate_recess` | `106`, `12`, `12` | Circular head plate dimensions and recess depth into the column end |
| `base_plate_size`, `base_plate_thickness` | `140`, `12` | Square base plate width and thickness |
| `base_plate_hole_diameter`, `base_plate_hole_spacing` | `15`, `104` | Four corner anchor bolt holes |
| `adjustment_nut_across_flats`, `adjustment_nut_top` | `36`, `64` | Lower hexagonal adjustment nut |
| `rod_diameter` | `30` | Threaded central rod diameter |
| `coupling_nut_across_flats`, `coupling_nut_height` | `55`, `30` | Upper hexagonal coupling nut below the head plate |
| `pin_count`, `pin_diameter`, `pin_length`, `pin_angle`, `pin_circle_diameter` | `3`, `8`, `180`, `25`, `50` | Inclined pins extending from the head plate into the column |
| `anchor_diameter`, `anchor_embedment` | `12`, `100` | Concrete slab anchor bolts |
| `chord_tolerance` | `0.05` | Maximum chord deviation for cylindrical facet approximations |

## On a plane

A steel column support placed on the world `xy` plane with its exact BRep geometry.

![](elements/element_support.png)

```cpp
scene.add(std::make_shared<Support>(Plane::xy_plane(), "support"));
```
