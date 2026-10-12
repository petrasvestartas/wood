# JointBeam connectors {#elements_joint_beam_connectors}

[TOC]

Connectors on the face contact of two members: a wedge, let-in plate or butterfly key, its pockets and its pins.

## Wedge

![Wedge](elements/element_joint_beam_wedge.png)

A wedge let into two beams side by side and pinned across; the right beam is moved off to show the pocket.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `length_margin` | | how far short of each end of the contact's top edge the wedge stops |
| `pocket_depth` | | how deep the pocket under each slanted face cuts into each member |
| `end` | none | a plane the wedge runs on to instead of stopping short |
| `pin_radius`, `pin_spacing`, `pin_sides` | 10, 320, 8 | the pins across the wedge: thickness, distance, facets |
| `overshoot` | 20 | how far the holes run past the pins |
| `profile` | `WEDGE_PROFILE` | the wedge's section: apex 197 below the top edge, 63.5 wide at the top |
| `pin_offset` | 80, -100 | where the first pin stands along and below the top edge |

\include{lineno} elements/element_joint_beam_wedge.cpp

## Let-in plate

![Let-in plate](elements/element_joint_beam_rectangle_plate.png)

A steel plate let into a column and a rib from the top, flush, with two pins through each member and the plate.

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `width` | 30 | the plate's thickness |
| `back`, `front` | 220, 265 | how far the plate runs into the column and into the rib |
| `height` | 250 | how far the plate runs down from the contact's top edge |
| `pin_length`, `pin_radius` | , 25 | the two pins in the column and the two in the rib, each through the plate too |
| `margin_x`, `margin_z` | 6.05, 3 | the pocket's clearance around the plate |
| `overshoot`, `pin_sides` | 25, 16 | how far the holes run past the pins, their facets |

\include{lineno} elements/element_joint_beam_rectangle_plate.cpp

## Butterfly key

![Butterfly key](elements/element_joint_beam_butterfly_key.png)

A key of two heads and a neck inlaid across the seam of two beams end to end, holding them together.

| Parameter | Default | What it changes |
| --- | --- | --- |
| `top` | 138.5 | how far below the beams' top the key starts; 0 is flush |
| `length` | 800 | the key's length across the seam |
| `head_length`, `head_width`, `neck_width` | 200, 40, 20 | the two heads and the neck between them |
| `depth`, `end_depth` | 58.5, 70.4 | the key's depth at the seam and at its ends |
| `pocket_depth` | 80 | the pockets' depth |

\include{lineno} elements/element_joint_beam_butterfly_key.cpp
