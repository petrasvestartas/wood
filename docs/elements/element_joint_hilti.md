# JointBeam hilti {#elements_joint_hilti}

[TOC]

The Hilti connector: a connector of parts that joins two CLT slabs along a straight seam, flat or folded up to 50 degrees, with two identical plywood halves of a bow-tie, one bolt and two steel discs, in the bow-tie pocket of the 2024 joint, as tested on the series of 200 mm slabs folded 0 to 50 degrees.

![the reference: 200 mm CLT folded 0 to 50 degrees, and the connector's parts](hilti_reference_photo.jpg)

## Constructors

```cpp
// on the face contact of two slabs, their seam faces mitred so the contact is the seam
static std::shared_ptr<JointBeam> hilti(
    const Element& a,
    const Element& b,
    const InteractionContactFace& contact,
    double half_length = 120.0,     // the seam to the pocket's end, half the 240 mm cutout
    double neck_length = 27.7,      // the neck across the seam, half in each pocket
    double neck_width = 40.0,       // the neck
    double taper_end = 74.4,        // the seam to where the wing reaches its width
    double wing_width = 90.0,       // the wing, the cutout's width
    double depth = 93.0,            // the pocket, from the top down
    double height = 90.0,           // the parts, their top depth - height under the top
    double rod_diameter = 12.0,     // the bolt
    double disc_diameter = 50.0,    // the steel disc under each nut, recessed in its wing's end
    double disc_thickness = 6.0,
    double router_radius = 20.0,    // the far corners of each pocket, a 40 mm router
    int sides = 32                  // the discs', the rounded corners' and the bolt's polygon
);

std::shared_ptr<Interaction> interaction(size_t target) const
std::vector<std::shared_ptr<Joint>> children() const
```

The connector is the 2024 Hilti joint (`ss_e_r_2`, id 55, and its variant `ss_e_r_3`, id 54) with its parts. Its defaults are the product's: a 240 x 90 x 93 mm cutout, a 27.7 x 40 neck and a 40 mm router, on CLT of at least 120 mm.

It is computed from the seam: x is the seam face's normal from the first slab into the second, y runs along the seam's longest edge, z points up, and the origin is the middle of the seam's top edge. The parts are made in that frame, so they keep their shape at every fold angle:
- **the halves:** each is a neck crossing the seam that widens over `taper_end` into a wing `wing_width` wide, straight to its end, its far corners rounded like its pocket's. The two form a straight bow-tie across the seam, `height` deep, its top `depth - height` under the seam's top edge.
- **the discs:** each is a steel disc recessed in its wing's end, taking the nut.
- **the bolt:** a `Pin` from disc to disc through both halves at their middle.

What follows the slabs is only what each target loses, `interaction(i)`:
- **the pocket of its half:** the 2024 half bow-tie milled from the top face, `depth` under the seam's top edge, its far corners rounded by the 40 mm router. It starts 1 mm across the seam, so the cut shares no face with the seam face.

Seen from above, each pair shows the 2024 joint: a bow-tie pocket across the seam, the parts set in from above. Because the bow-tie stays straight while the slabs fall away from the ridge, the more the pair folds, the more each wing stands out of its slab's top, as on the test series. The halves, the discs and the bolt nest under the connector as `ConnectorPart` and `Pin` elements in `JointBeam::CONNECTOR_COLOR`, and the bolt bores each part exactly.

`tests/joint_hilti.cpp` folds the pair 0, 10, 20, 30, 40 and 50 degrees and checks:
- every part identical in volume and edge lengths at every angle;
- each part inside its slab or standing out of its top, never below it;
- each part clear of the cut slab;
- each slab losing nothing but its pocket;
- the bolt through all four parts, with an exact bore in each.

The 2024 dataset `inplane_hilti` keeps its plate joint (id 3, `ss_e_ip_2`): it is the reference the 2025 solver matches, and this connector is an element of its own rather than a design of the plate library.

## One pair at 30 degrees

![hilti](elements/element_joint_hilti.png)

Two 600 x 400 slabs 200 thick folded 30 degrees on a mitred seam with the connector's defaults, the right slab moved 300 off along the seam's normal so its pocket reads.

\include{lineno} elements/element_joint_hilti.cpp

## Six fold angles

![hilti at six angles](elements/element_joint_hilti_angles.png)

The same pair folded 0 to 50 degrees, one row per angle as in the test series: the halves, discs and bolt are the same solids in every row, while the pockets follow the slabs.

\include{lineno} elements/element_joint_hilti_angles.cpp
