# Floor {#templates_floor}

[TOC]

Vaulted timber floor bay supported on four carved columns with a central interlocking oculus ring (`src/templates/floor/`).

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_floor_guide.html">
    <img src="962_section_construction_quads.webp" alt="Floor Guide">
    <span class="wood-card-title">Floor Guide</span>
  </a>
  <a class="wood-card" href="templates_floor_model.html">
    <img src="floor_viewer.webp" alt="Floor Model">
    <span class="wood-card-title">Floor Model</span>
  </a>
</div>

## Examples {#floor_examples}

### 1 Floor Guide {#floor_example_1}

Step 1 of the timber floor: the guide alone, a FloorGuide on the corners of the 6000 x 6000 square, (-3000, -3000) first and counter-clockwise, with the default parameters, a session that draws its own construction, and quarter 0 of it taken out with get_branch("quarter_0") as a WoodSession. Every quarter_q holds plan_q (the quarter polygon, the column head, the oculus corner) and one group per member family, outer_ribs_q, inner_ribs_q, inner_beams_q, wedges_q and tsections_q, each in its own colour; under it one group per member named as the Floor names that member's element, e.g. outer_ribs_0_0, holding the member's plan quad at the floor datum, its two face planes face_0 and face_1, and for a rib its three parabolas: the soffit, the t-sections' top and the beds' top. No member is built here. Floor(guide) builds them under the same names as elements; templates_floor_3_contacts and templates_floor_7_contacts_cantilevers show them.

\include{lineno} templates_floor_1_floorguide.cpp

### 2 Column Model {#floor_example_2}

Step 2 of the timber floor: the support of corner 0, a Sherpa Power Base L 140 C from its datasheet, and the 220 square column standing on it from the support's head plate to the floor at 3500, with the head 120 wider on the two bay sides over the top 730. The support joint lets the head plate into the column end and drills the three column pins; the six column cutters of quarter 0 carve the head down to the outer rib bottoms.

\include{lineno} templates_floor_2_column_model.cpp

### 3 Contacts {#floor_example_3}

Step 3 of the timber floor: Floor(guide) on the 6000 x 6000 square, viewed for its contacts. Floor::add_contacts stores, for every two members the design joins, the face they share as a named contact interaction. The viewer draws every contact as a red fill on its faces, so View Xray shows them through the members. Per quarter q there are 17: seam_wedge_q, seam beam 0 beside the next quarter's seam beam; oculus_wedge_q, the oculus beam on its ring beam; column_plate_q_0 and column_plate_q_1, the column on each outer rib; six block_pins_q_b_side, each column block on the rib either side; two pins_outer_rib_q_k, the seam beam and the outer rib ending on it; two pins_seam_beam_q_k, the seam beam and the oculus beam; two pins_inner_rib_q_k, the oculus beam and the inner rib; and pins_ring_corner_q, ring beam q against ring beam q + 1. 68 contacts in all.

\include{lineno} templates_floor_3_contacts.cpp

### 7 Contacts Cantilevers {#floor_example_7}

Step 7 of the timber floor: the whole square bay, the four quarters, the oculus and the four columns on their supports, with every connector: the wedges of step 6, cut flush with the floor top; a column plate on every column-to-outer-rib contact (a Plate 30 thick, 220 into the column, 265 into the rib, two pins in each), the two plates of a column head half-lapped by a cross plate joint; four Ø8 x 30 pins on every wedge block to rib contact; and last the 56 assembly pins (200 x d4, pre-drilled, level along the member that ends on the contact, no member cut). The seam beams and their wedge run on through the outer rib band to the bay's outer face, the outer ribs ending on the beams, pinned along the rib from the beam's seam face; the four ring beams pinned at their corners. Every member carries a drill feature per hole, every column its head cuts as cut features. pb_dump writes every element as its exact BRep, round bores and pins. Connectors are blue, each in connectors_q of its quarter_q, the oculus wedges in connectors of oculus.

\include{lineno} templates_floor_7_contacts_cantilevers.cpp

### 8 Rectangle {#floor_example_8}

Step 8 of the timber floor: the same model on a 6000 x 4800 bay, a FloorGuide on the corners (-HALF_X, -HALF_Y) to (-HALF_X, HALF_Y) counter-clockwise. The quarters are built in place at their own corners and mirror each other; the oculus is a square diamond of half-diagonal 1000; the central panel follows rule A, its inner ribs swept along one direction so the central bed is one planar-faced cylinder with every layer 27 thick; both outer ribs of a corner end at one level (the short ribs' rib start solved to 187.667), which is the middle cutter level, so every rib meets its column head within 0.307 mm. The connectors and pins are those of step 7.

\include{lineno} templates_floor_8_rectangle.cpp

### 9 Orientation {#floor_example_9}

The fabrication orientation of one column of the square bay: the floor built from the guide, column_0 read back as its own branch with its glued blocks, support seat and head cuts, and turned by the transform from its upright section plane to the ground plane, so the column lies flat on the ground as it is machined; that branch alone is written.

\include{lineno} templates_floor_9_orientation.cpp

<div style="display:none">

- @subpage templates_floor_guide
- @subpage templates_floor_model
</div>
