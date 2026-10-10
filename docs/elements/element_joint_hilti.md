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
    double thickness = 50.0,        // the plywood across the slab
    double rod_diameter = 16.0,     // the threaded rod
    double disc_diameter = 70.0,    // the round disc under each nut
    double disc_thickness = 8.0,
    double slot_width = 60.0,       // the obround access slot
    double lift = 0.0,              // the rod above the middle of the seam face, 0 at mid-thickness
    double rod_overhang = 15.0,     // the rod past each disc, the nut
    int sides = 32                  // the disc's and the slot ends' polygon, and the rod's chord tolerance
);

std::shared_ptr<Interaction> interaction(size_t target) const
std::vector<std::shared_ptr<Joint>> children() const
```

The connector is computed from the seam: x is the seam face's normal from the first slab into the second, y runs along the seam's longest edge and z points up. Its parts are made in that frame, so they keep their shape at every fold angle: each half is a trapezoid wing widening away from the seam on a rectangular neck, outlined in the plane of the rod and the seam and as thick as the plywood across it, sunk square to the seam face one in each slab; the rod is a `Pin` through both necks and both discs, its nut past each disc; each disc takes the nut on the outer end of its half. What follows the slabs is only what each target loses, `interaction(i)`: the pocket of its half (started 1 mm across the seam so the cut shares no face with the seam face), the seat of its disc, and an obround access slot as long as the half, over its outer end, milled along the top face's normal from the top down to the lowest point of the half and the disc. The halves, the discs and the rod nest under the connector as `ConnectorPart` and `Pin` elements in `JointBeam::CONNECTOR_COLOR`; the rod bores each part exactly. `tests/joint_hilti.cpp` folds the pair 0, 10, 20, 30, 40 and 50 degrees and checks every part identical in volume and edge lengths at every angle, inside its slab, clear of the cut slab, the slab's stock covered by its cut model, its parts and its slot, and the rod through all four parts with an exact bore in each.

The 2024 dataset `inplane_hilti` keeps its plate joint (id 3, `ss_e_ip_2`): it is the reference the 2025 solver matches, and this connector is an element of its own rather than a design of the plate library.

## One pair at 30 degrees

![hilti](elements/element_joint_hilti.png)

Two 600 x 400 slabs 200 thick folded 30 degrees on a mitred seam with the connector's defaults, the right slab moved 300 off along the seam's normal so its pocket and its slot read.

\include{lineno} elements/element_joint_hilti.cpp

## Six fold angles

![hilti at six angles](elements/element_joint_hilti_angles.png)

The same pair folded 0 to 50 degrees, one row per angle as in the test series: the halves, discs and rod are the same solids in every row, while the pockets and the slots follow the slabs.

\include{lineno} elements/element_joint_hilti_angles.cpp
