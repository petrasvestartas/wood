# Floor 10: Pins {#templates_floor_10_pins}

[TOC]

<em>Step 10 of @ref templates_floor_model · previous: @ref templates_floor_09_connectors · next: @ref templates_floor_11_examples</em>

Where one member butts into another, `Floor::compute_connectors` lays two pins across their contact with `JointBeam::headed_pins` and `add_connectors` pre-drills them into both members: per side of a quarter the outer rib on its seam beam, the seam beam on the oculus beam and the oculus beam on the inner rib, and at each of the four ring corners ring beam q on the next. Every pin is a `Pin`, a 200 mm cylinder from its head, level along the axis of the member that ends on the contact, and both members read the same drill lines.

Example: [templates_floor_7_contacts_cantilevers.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_7_contacts_cantilevers.cpp) builds the floor with its pins.

## 301. Pins

![](floor/301_pins.webp)

Per quarter two `outer_rib_seam_beam`, two `seam_beam_oculus_beam` and two `oculus_beam_inner_rib` connectors, two pins each.

## 302. The contact

![](floor/302_pin_contact.webp)

`add_contacts` stores `pins_outer_rib_0_0` with the seam beam first, the member the pins pass through, then the outer rib; the normal points into the rib.

## 303. The axes

![](floor/303_pin_axes.webp)

x runs level along the contact (z cross the normal) and y up it, so vertical means up however narrow the face.

## 304. The stations

![](floor/304_pin_stations.webp)

`PinLayout::vertical` with two pins: the contact inset by `PIN_INSET` = 20, one station at the ring's top and one at its bottom, moved `PIN_SHIFT` = 15 along x.

## 305. The head

![](floor/305_pin_head.webp)

`pin_head` goes back from the station along the pin's direction, the level axis of the member that ends on the contact, and puts the head where the line leaves the other member's stock; the pin runs 200 from there into the member that ends.

## 306. Two quarters at a seam

![](floor/306_pins_at_a_seam.webp)

The two quarters' pins at a seam take shift -15 (k 0) and +15 (k 1), so their heads stay apart.

## 307. The layouts

![](floor/307_pin_layouts.webp)

`PinLayout` on the same contact: `corners` at the inset ring's extreme corners, `vertical` a column up it, `horizontal` a row along it.

## 308. The Pin children

![](floor/308_pin_children.webp)

A pin connector nests one `Pin` per line, named `connector_pins_outer_rib_0_0_pin_i`, and pre-drills a 4 mm hole into both members.

## 309. Every pin

![](floor/309_every_pin.webp)

28 pin connectors `connector_pins_<joint>_<place>` with 56 pins, the butt joints in `connectors_q` of their quarter, the four ring corners in the oculus.
