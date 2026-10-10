# JointPlate hilti {#elements_joint_plate_hilti}

[TOC]

The Hilti connector: a bow-tie key, two plywood halves, two steel discs and a bolt, in a bow-tie pocket across the straight seam of two CLT slabs, flat or folded up to 50 degrees.

![the reference: 200 mm CLT folded 0 to 50 degrees, and the connector's parts](hilti_reference_photo.jpg)

## Constructors

```cpp
// on the face contact of two slabs, their seam faces mitred so the contact is the seam
static std::shared_ptr<JointPlate> hilti(
    const Plate& a,
    const Plate& b,
    const InteractionContactFace& contact,
    double half_length = 120.0,     // the seam to the pocket's end, half the 240 mm cutout
    double neck_length = 27.7,      // the neck across the seam, half in each pocket
    double neck_width = 40.0,       // the neck
    double taper_end = 74.4,        // the seam to where the wing reaches its width
    double wing_width = 90.0,       // the wing, the cutout's width
    double depth = 93.0,            // the pocket, from the top down
    double height = 90.0,           // the key, its top depth - height under the top
    double rod_diameter = 12.0,     // the bolt
    double disc_diameter = 50.0,    // the steel disc under each nut, recessed in its wing's end
    double disc_thickness = 6.0,
    double router_radius = 20.0,    // the far corners of each pocket, a 40 mm router
    int sides = 32                  // the discs', the rounded corners' and the bolt's polygon
);

std::shared_ptr<Interaction> interaction(size_t target) const
```

The 2024 Hilti joint (`ss_e_r_2`, id 55) with its key, a plate joint like the rotated keys:
- **each slab, `interaction(i)`:** its half of the bow-tie pocket milled `depth` from the top, the neck across the seam widening into the wing, the far corners rounded by the router.

The key keeps its shape at every fold, so the more the pair folds, the more each wing stands out of its slab's top, as on the test series.

## One pair at 30 degrees

![hilti](elements/element_joint_plate_hilti.png)

Two 600 x 400 slabs 200 thick folded 30 degrees on a mitred seam with the connector's defaults, the right slab moved 300 off along the seam's normal so its pocket reads.

\include{lineno} elements/element_joint_plate_hilti.cpp

## Six fold angles

![hilti at six angles](elements/element_joint_plate_hilti_angles.png)

The same pair folded 0 to 50 degrees, one row per angle as in the test series: the halves, discs and bolt are the same solids in every row, while the pockets follow the slabs.

\include{lineno} elements/element_joint_plate_hilti_angles.cpp
