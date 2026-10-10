# JointBeam hilti {#elements_joint_hilti}

[TOC]

The Hilti connector: a connector of parts that joins two CLT slabs along a straight seam, flat or folded up to 50 degrees, with two identical birch plywood half-dovetails, one threaded rod and two steel discs, as tested on the series of 200 mm slabs folded 0 to 50 degrees.

![the reference: 200 mm CLT folded 0 to 50 degrees, and the connector's parts](hilti_reference_photo.jpg)

## Constructors

```cpp
// on the face contact of two slabs, their seam faces mitred so the contact is the seam
static std::shared_ptr<JointBeam> hilti(
    const Element& a,
    const Element& b,
    const InteractionContactFace& contact,
    double half_length = 140.0,     // a half along the rod, the seam to its outer end
    double neck_length = 50.0,      // the rectangular neck at the seam
    double wing_width = 120.0,      // the wing at the half's outer end
    double neck_width = 50.0,       // the neck, and the wing where it starts
    double height = 100.0,          // the half from its top down, the plywood's layers along the neck
    double rod_diameter = 16.0,     // the threaded rod
    double disc_diameter = 70.0,    // the round disc under each nut
    double disc_thickness = 8.0,
    double slot_width = 80.0,       // the obround access slot, wider than the disc it takes
    double cover = 20.0,            // the slabs' top at the seam down to the halves' top
    double rod_overhang = 15.0,     // the rod past each disc, the nut
    int sides = 32                  // the disc's and the slot ends' polygon, and the rod's chord tolerance
);

std::shared_ptr<Interaction> interaction(size_t target) const
std::vector<std::shared_ptr<Joint>> children() const
```

The connector is computed from the seam: x is the seam face's normal from the first slab into the second, y runs along the seam's longest edge and z points up. Its parts are made in that frame, so they keep their shape at every fold angle, as on the photo:
- **the halves:** each is a trapezoid wing widening away from the seam on a rectangular neck. The two form a straight bow-tie across the seam, its top `cover` under the seam's highest point and `height` deep.
- **the rod:** a `Pin` through both necks and both discs at the halves' middle, its nut past each disc.
- **the discs:** each takes the nut on the outer end of its half.

What follows the slabs is only what each target loses, `interaction(i)`:
- **the pocket of its half:** milled from the top face, so the half is set in from above. It starts 1 mm across the seam, so the cut shares no face with the seam face.
- **the seat of its disc.**
- **an obround access slot:** milled from the top face from the wing's end outwards, as long as the half, down to the lowest point of the half and the disc. It is wider than the disc, which passes through it.

Seen from above, each pair shows what the test series shows: a slot, the bow-tie across the seam, a slot. Because the bow-tie stays straight while the slabs fall away from the ridge, the more the pair folds, the more each wing stands out of its slab's top. The halves, the discs and the rod nest under the connector as `ConnectorPart` and `Pin` elements in `JointBeam::CONNECTOR_COLOR`, and the rod bores each part exactly.

`tests/joint_hilti.cpp` folds the pair 0, 10, 20, 30, 40 and 50 degrees and checks:
- every part identical in volume and edge lengths at every angle;
- each part inside its slab or standing out of its top, never below it;
- each part clear of the cut slab;
- each slab losing nothing but its parts' room, its pocket and its slot;
- the rod through all four parts, with an exact bore in each.

The 2024 dataset `inplane_hilti` keeps its plate joint (id 3, `ss_e_ip_2`): it is the reference the 2025 solver matches, and this connector is an element of its own rather than a design of the plate library.

## One pair at 30 degrees

![hilti](elements/element_joint_hilti.png)

Two 600 x 400 slabs 200 thick folded 30 degrees on a mitred seam with the connector's defaults, the right slab moved 300 off along the seam's normal so its pocket and its slot read.

\include{lineno} elements/element_joint_hilti.cpp

## Six fold angles

![hilti at six angles](elements/element_joint_hilti_angles.png)

The same pair folded 0 to 50 degrees, one row per angle as in the test series: the halves, discs and rod are the same solids in every row, while the pockets and the slots follow the slabs.

\include{lineno} elements/element_joint_hilti_angles.cpp
