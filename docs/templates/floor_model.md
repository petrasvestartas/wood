# Part 2. Floor: the model {#templates_floor_model}

[TOC]

`wood_floor::Floor` (`src/templates/floor/floor.h`) builds the model from a @ref templates_floor_guide "FloorGuide": the face loops become elements lifted to `bay_height`, every two members that touch get a contact interaction, and the connectors and screws are made from those.

## Steps

Each step is one chapter, one picture per sub-step, in code order, numbered on from the guide's six:

7. @subpage templates_floor_07_elements (outlines into beams and plates, the lift to the floor, the scene tree, the columns and get_branch)
8. @subpage templates_floor_08_relationships (what every two members share, by the rules of the design, and the check against the kernel's contact search)
9. @subpage templates_floor_09_connectors (wedges, column plates, cross laps and dowels, and how each cuts and drills its members)
10. @subpage templates_floor_10_screws (the five screw kinds, their levels and aim, and the screw check)
11. @subpage templates_floor_11_checks (the guide's report, the BRep check and the eight examples)

## Overview

One picture per method of the floor, in the order `add_members`, `add_contacts`, `add_connectors` and `add_screws` run.

### add_quarters(), add_oculus(), add_columns()

![add_quarters(), add_oculus(), add_columns()](floor/915_elements.webp)

<span style="color:#2196EA">■ BeamVariable</span> <span style="color:#F2CC0C">■ Plate</span> <span style="color:#737373">■ Column, Support</span>

The loops become elements: ribs, inner beams and ring beams are `BeamVariable`, t-sections, beds, blocks and the oculus plates `Plate`, and each column a `Column` on its `Support`, named `outer_ribs_<i>_<q>`, `beds_<row>_<i>_<q>` ...

Code: [`add_quarters`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor.h#L173)

### add_contacts()

![add_contacts()](floor/916_contacts.webp)

<span style="color:#2196EA">■ contact polygons</span>

For every two members the design joins, the session's contact search (`compute_face_contact`) finds the face they share, and `add_interaction` stores it on the session's edge between them as an `InteractionContactFace` named by its kind and place (`seam_wedge_0`, `column_plate_0_1`, `block_dowels_0_1_0` ...).

Code: [`add_contacts`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor.h#L185)

### add_interaction(a, b, contact)

![add_interaction(a, b, contact)](floor/917_contact.webp)

<span style="color:#F2CC0C">■ member a</span> <span style="color:#E8478B">■ member b</span> <span style="color:#2196EA">■ the contact</span>

One of them: the two seam beams of seam 0 and the face they share.

Code: [`add_interaction`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor.h#L19)

### add_connectors()

![add_connectors()](floor/918_connectors.webp)

<span style="color:#2196EA">■ connector parts</span> <span style="color:#A3A3A3">■ members</span>

Each contact interaction gets the connector of its kind: seam and oculus wedges, column plates with their cross lap, dowels.

Code: [`add_connectors`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor.h#L188)

### add_screws()

![add_screws()](floor/919_screws.webp)

<span style="color:#2196EA">■ screw lines</span> <span style="color:#A3A3A3">■ member loops</span>

`rib_beam_screws`, `beam_mitre_screws` and `rib_corner_screws` find the 200 mm screw lines between members that butt; `JointBeam::screws` pre-drills them into both members.

Code: [`add_screws`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor.h#L191)

### Floor

![Floor](floor/921_floor.webp)

family colours, connectors in blue

The finished model: every member at `bay_height`, its contacts, connectors and screws.

Code: [`Floor`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor.h#L150)
