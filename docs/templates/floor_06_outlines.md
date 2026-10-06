# Floor 06: Beam, wedge, flange, bed, oculus and cutter outlines {#templates_floor_06_outlines}

[TOC]

This chapter builds the outlines of the inner beams, wedges, t-sections, beds, oculus and column cutters with `geometry::loft_planes` and the `Quarter` / `FloorGuide` member functions, each returning `Outline{top, bottom}` pairs at the datum z 0. Chapter 07 turns them into elements with `to_beam` and `to_plate`, and into the column's solid cuts.

Example: [templates_floor_4_quarters.cpp](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_4_quarters.cpp) builds the four quarters, every inner beam, wedge, t-section and bed made from these outlines, [templates_floor_5_oculus.cpp](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_5_oculus.cpp) builds the oculus ring, its bottom wedges and the central plate, and [templates_floor_2_column_model.cpp](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_2_column_model.cpp) carves column 0's head with quarter 0's six column cutters.

![](floor/film_06_outlines.webp)

<span style="color:#2196EA">■ built</span> what each step builds   <span style="color:#E8478B">■ variable</span> the variable or value it introduces   <span style="color:#F2CC0C">■ result</span> a second result, set apart   <span style="color:#737373">■ input</span> what it reads from earlier steps, dashed for helpers   <span style="color:#A3A3A3">■ context</span> everything else

## 82. loft_planes helper

![](floor/082_loft_planes.webp)

<span style="color:#2196EA">■ built</span> `Outline{top, bottom}` loops and their corners   <span style="color:#E8478B">■ variable</span> `planes[0..3]`, the ring   <span style="color:#737373">■ input</span> `bottom = inner_beams[0][0]`, `top = inner_beams[0][1]`   <span style="color:#A3A3A3">■ context</span> lines joining facing corners

`loft_planes(planes, bottom, top, flip)` meets each pair of neighbouring ring planes with `bottom` and with `top` and returns `Outline{top loop, bottom loop}`, corner `i` facing corner `i`; every inner beam, wedge block and oculus piece is built with it. A corner whose solve returns `nullopt` (two planes parallel, or all three on one line) is skipped without error, so a degenerate ring gives a loop with fewer corners; `flip = true` swaps the two loops.

Code: `geometry::loft_planes`, [floor_geometry.cpp:273-296](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_geometry.cpp#L273-L296)

## 83. inner_beams: levels and face choice

![](floor/083_beam_levels.webp)

<span style="color:#2196EA">■ built</span> `side0 = level(0.0)`, `side1 = level(guide.soffit)`   <span style="color:#E8478B">■ variable</span> `face = 0`: `outer_ribs[0][0]`   <span style="color:#737373">■ input</span> `outer_ribs[0][1]`, the tied variant (dashed)   <span style="color:#A3A3A3">■ context</span> `inner_beams[1][0]`, the oculus end

`Quarter::inner_beams` places the beams between `side0 = level(0.0)` and `side1 = level(guide.soffit)`, and `face = seam_through_ribs ? 0 : 1` runs the seam beams to the bay edge `outer_ribs[k][0]` by default, or stops them on the rib's inner face `outer_ribs[k][1]` in the tied variant.

Code: `Quarter::inner_beams`, [floor_members.cpp:93-98](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L93-L98)

## 84. Seam beam 0

![](floor/084_seam_beam_0.webp)

<span style="color:#2196EA">■ built</span> `inner_beams()[0].bottom` on x = 0   <span style="color:#F2CC0C">■ result</span> `inner_beams()[0].top` on x = -60   <span style="color:#E8478B">■ variable</span> the corners `bottom[0..3]`

Seam beam 0 lofts the ring bay edge, datum, the oculus edge's tilted bearing plane and soffit between the seam plane x = 0 and its far face x = -60, so its oculus end is cut on the 5 degree plane: 2000 long at the datum, 2024.6 at the soffit.

Code: `Quarter::inner_beams`, [floor_members.cpp:101](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L101)

## 85. Oculus beam

![](floor/085_oculus_beam.webp)

<span style="color:#2196EA">■ built</span> the oculus beam, its bottom loop on `oculus_edges[0].tilted` and its top loop on `oculus_edges[0].back`   <span style="color:#E8478B">■ variable</span> lean 17.4 = `-soffit tan(oculus_plane_angle)`   <span style="color:#737373">■ input</span> `inner_beams_0_0`, `inner_beams_2_0` and the drop from the edge centre (dashed)   <span style="color:#A3A3A3">■ context</span> lines joining facing corners

The oculus beam fits between the far faces of seam beams 0 and 2, with its bottom loop on `oculus_edges[0].tilted`, shared with ring beam 0, and its top loop on the vertical back face `oculus_edges[0].back`, 60 mm into the quarter where the inner ribs end. `tilted` lies on the edge at z 0 and leans toward the centre by `-soffit * tan(5 deg)` = 17.4 mm at the soffit.

Code: `Quarter::inner_beams`, [floor_members.cpp:102](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L102); planes `oculus_edge`, [floor.cpp:92-103](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L92-L103)

## 86. Seam beam 2

![](floor/086_seam_beam_2.webp)

<span style="color:#2196EA">■ built</span> `inner_beams_0_0`, `inner_beams_1_0`, `inner_beams_2_0`   <span style="color:#A3A3A3">■ context</span> the outer ribs of quarter 0

Seam beam 2 mirrors seam beam 0 on seam 3, with its loops on y = 0 and y = -60; with `seam_through_ribs` the two outer ribs end on the beams' far faces x = -60 and y = -60.

Code: `Quarter::inner_beams`, [floor_members.cpp:103](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L103)

## 87. Wedges: bounding rib faces and bed planes

![](floor/087_wedge_bounds.webp)

<span style="color:#2196EA">■ built</span> `ribs[0..2]`, the two faces of each block   <span style="color:#737373">■ input</span> `beds[i] = bed_top_planes[i]`   <span style="color:#A3A3A3">■ context</span> the outer and inner ribs

`Quarter::wedges` bounds block `i` by its two rib faces `ribs[i]`, the datum `top = level(0.0)` and the bed plane `beds[i] = geometry().bed_top_planes[i]`, so each block stands on the top of the bed row in its panel.

Code: `Quarter::wedges`, [floor_members.cpp:107-116](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L107-L116); `panel_top_plane` and `bed_top_planes`, [floor.cpp:48-62](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L48-L62), [343-356](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L343-L356)

## 88. Wedges: loft between fan and far face

![](floor/088_wedge_blocks.webp)

<span style="color:#2196EA">■ built</span> `wedges_0_0`, `wedges_1_0`, `wedges_2_0`   <span style="color:#A3A3A3">■ context</span> the outer and inner ribs

Each block is `loft_planes({ribs[i][0], beds[i], ribs[i][1], top}, cp.wedges[i][0], cp.wedges[i][1])`, from the fan plane against the column head to the far face `block_planes` sets at `run_in[0]`, `middle_wedge_factor * mean(run_in)` and `run_in[1]`: 240, 300 and 240 on the square bay.

Code: `Quarter::wedges`, [floor_members.cpp:118-123](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L118-L123); `block_planes`, [floor.cpp:313-322](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L313-L322)

## 89. tsection: trim four traces

![](floor/089_tsection_trim.webp)

<span style="color:#2196EA">■ built</span> `cut00`, the soffit trimmed   <span style="color:#F2CC0C">■ result</span> `cut10`, the `+t` trimmed   <span style="color:#E8478B">■ variable</span> `tsections = 27`   <span style="color:#737373">■ input</span> the untrimmed traces (dashed), `cut_plane0`, `cut_plane1`   <span style="color:#A3A3A3">■ context</span> outer rib 0's bottom loop

`tsection` trims the soffit and `+t` traces on both flange faces between the beam face `cut_plane0` and the fan plane `cut_plane1`, projecting before trimming so every end lies exactly on both cut planes. `trim` first pushes both end segments out by `EXTENSION` = 1000, so the soffit reaches the fan plane along its extended first chord at z -694.8.

Code: `tsection`, [floor_members.cpp:130-136](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L130-L136); `trim`, [floor_geometry.cpp:69-80](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_geometry.cpp#L69-L80)

## 90. tsection: the 15-point loop

![](floor/090_tsection_loop.webp)

<span style="color:#2196EA">■ built</span> `tsections()[0].top`, 15 points   <span style="color:#E8478B">■ variable</span> the point order 0 .. 14 and its direction   <span style="color:#A3A3A3">■ context</span> outer rib 0's bottom loop

The top loop is `cut00`, then `cut10` reversed, then `cut00.front()` again, a closed 15-point strip; the bottom loop is built the same way from `cut01` and `cut11`, giving a 27 mm plate flat against the rib face.

Code: `tsection`, [floor_members.cpp:138-146](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L138-L146)

## 91. outer_tsection projections

![](floor/091_outer_tsection.webp)

<span style="color:#2196EA">■ built</span> the soffit on `ts[0][0]` and on `ts[1][0]`   <span style="color:#E8478B">■ variable</span> `outer = outer_ribs[0][0].z_axis()`   <span style="color:#737373">■ input</span> `parabolas[0][0]` in plan   <span style="color:#A3A3A3">■ context</span> the two flange face traces

`outer_tsection` slides the outer parabola's soffit and `+t` along the outer rib normal onto face 0, then hands `tsection` the projections onto face 1 along `sweep` (soffit) and `outer` (`+t`); every direction is horizontal, so z does not change.

Code: `outer_tsection`, [floor_members.cpp:149-159](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L149-L159)

## 92. Flanges 0 and 5

![](floor/092_flanges_0_5.webp)

<span style="color:#2196EA">■ built</span> `tsections_0_0`, `tsections_5_0`   <span style="color:#A3A3A3">■ context</span> the outer ribs

T-sections 0 and 5 project along the outer normal both ways, so each is a straight 27 mm extrusion of the trimmed strip, running from the fan plane to the seam beam's far face.

Code: `Quarter::tsections`, [floor_members.cpp:161-171](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L161-L171), [184](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L184); planes `construction_planes`, [floor.cpp:204-211](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L204-L211)

## 93. Flanges 1 and 4

![](floor/093_flanges_1_4.webp)

<span style="color:#2196EA">■ built</span> `tsections()[1].bottom` on `ts[1][1]`   <span style="color:#F2CC0C">■ result</span> `tsections()[1].top` on `ts[1][0]`   <span style="color:#E8478B">■ variable</span> `panel.rib_sweep`, the soffit's direction   <span style="color:#737373">■ input</span> `outer0`, the `+t`'s direction, and faces `ts[1][0]`, `ts[1][1]` (dashed)   <span style="color:#A3A3A3">■ context</span> inner rib 0

T-section 1 lies against inner rib 0's outer face; its soffit reaches face 1 along `rib_sweep` and its `+t` along `outer0`, so the bottom loop is not a parallel copy of the top loop (fan end at z -700.9 instead of -694.8). T-section 4 mirrors it.

Code: `Quarter::tsections`, [floor_members.cpp:172](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L172), [183](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L183)

## 94. Flanges 2 and 3

![](floor/094_flanges_2_3.webp)

<span style="color:#2196EA">■ built</span> `tsections_2_0`, `tsections_3_0`   <span style="color:#E8478B">■ variable</span> `panel.ruling` u, the `+t`'s direction   <span style="color:#737373">■ input</span> `panel.rib_sweep` r, the soffit's direction   <span style="color:#A3A3A3">■ context</span> the inner ribs

T-sections 2 and 3 call `tsection` directly on the central panel traces, carrying the soffit to face 1 along `rib_sweep` and the `+t` along the ruling `u`, cut between the oculus beam back face and the middle fan plane.

Code: `Quarter::tsections`, [floor_members.cpp:173-182](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L173-L182)

## 95. bed_row: trim layers on both side faces

![](floor/095_bed_layers.webp)

<span style="color:#2196EA">■ built</span> `layers[0]`, `layers[1]`: `+t` trimmed   <span style="color:#F2CC0C">■ result</span> `layers[2]`, `layers[3]`: `+2t` trimmed   <span style="color:#737373">■ input</span> the untrimmed layers (dashed)   <span style="color:#A3A3A3">■ context</span> outer rib 0 and inner rib 0

`bed_row` trims the `+t` and `+2t` layers on both side faces in one `trim_alike` call, which cuts every layer on the segment the first layer crosses, so all four keep the same vertex count and vertex `i` stays the same parabola point. On a skewed bay, trimming each layer alone could cross a cut plane on different segments and break the quads; on the square bay the result equals `trim`.

Code: `bed_row`, [floor_members.cpp:189-193](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L189-L193); `trim_alike`, [floor_geometry.cpp:82-125](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_geometry.cpp#L82-L125)

## 96. bed_row: one plate per facet

![](floor/096_bed_plates.webp)

<span style="color:#2196EA">■ built</span> `beds_0_0_0` .. `beds_0_5_0`   <span style="color:#A3A3A3">■ context</span> the ribs, `tsections_0_0` and `tsections_1_0`

Each facet `i` becomes one 27 mm plank from side face to side face, its bottom quad on the `+t` layer (the flange top) and its top quad on `+2t`, so seven trimmed points give six plates per row.

Code: `bed_row`, [floor_members.cpp:195-203](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L195-L203)

## 97. outer_bed_row projections

![](floor/097_outer_bed_row.webp)

<span style="color:#2196EA">■ built</span> the `+t` on `side0` and on `side1`   <span style="color:#E8478B">■ variable</span> `outer` = (0,1,0)   <span style="color:#737373">■ input</span> `parabolas[0][1]` (`+t`) and `parabolas[0][2]` (`+2t`)   <span style="color:#A3A3A3">■ context</span> the two side face traces

`outer_bed_row` projects the outer parabola's `+t` and `+2t` onto both side faces along the outer rib normal and passes them to `bed_row`.

Code: `outer_bed_row`, [floor_members.cpp:206-213](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L206-L213)

## 98. Three bed rows

![](floor/098_bed_rows.webp)

<span style="color:#2196EA">■ built</span> `beds()[0]`, `beds()[1]`, `beds()[2]`   <span style="color:#A3A3A3">■ context</span> the outer and inner ribs

`Quarter::beds` returns three rows, the two outer ones from `outer_bed_row` and the central one straight from the central panel traces: 18 plates per quarter on the square bay.

Code: `Quarter::beds`, [floor_members.cpp:215-226](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L215-L226)

## 99. Oculus levels side0..side3

![](floor/099_oculus_levels.webp)

<span style="color:#2196EA">■ built</span> `side0`, `side1`, `side2`, `side3`   <span style="color:#A3A3A3">■ context</span> the ring beams

`FloorGuide::oculus` builds the ring once for the whole floor, on four levels.

| Variable | Value | Meaning |
|---|---|---|
| `side0` | 0 | Datum, ring beam top |
| `side1` | -171.78 | `soffit + tsections` |
| `side2` | -198.78 | `soffit`, ring beam and bottom wedge bottom |
| `side3` | -144.78 | `soffit + 2 * tsections`, central plate top |

Code: `FloorGuide::oculus`, [floor_members.cpp:232-237](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L232-L237)

## 100. Oculus planes tilted[q], inner[q]

![](floor/100_oculus_planes.webp)

<span style="color:#2196EA">■ built</span> `tilted[q]` at `side0`, on the edge   <span style="color:#E8478B">■ variable</span> `tilted[q]` at `side2`, 17.4 inward   <span style="color:#F2CC0C">■ result</span> `inner[q] = ring_inner`, 60 inside

For each oculus edge `q`, `tilted[q]` is the bearing plane leaning 5 degrees (on the edge at z 0, 17.4 mm inward at the soffit) and `inner[q] = ring_inner` stands vertically 60 mm inside the edge.

Code: `FloorGuide::oculus`, [floor_members.cpp:239-245](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L239-L245); `oculus_edge`, [floor.cpp:92-103](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L92-L103)

## 101. Ring beams: pinwheel

![](floor/101_ring_beams.webp)

<span style="color:#2196EA">■ built</span> `oculus_0` .. `oculus_3`   <span style="color:#E8478B">■ variable</span> each beam's run on to `tilted[(i + 1) % 4]`   <span style="color:#A3A3A3">■ context</span> the oculus edges

Ring beam `i` lofts between `tilted[i]` and `inner[i]` from soffit to datum and runs through its corner to the next edge's `tilted[(i + 1) % 4]`, so the four close as a pinwheel with no mitres; `flip = true` makes `top` the face shared with the quarter's oculus beam.

Code: `FloorGuide::oculus`, [floor_members.cpp:247-250](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L247-L250)

## 102. Bottom wedges: ledge strips

![](floor/102_bottom_wedges.webp)

<span style="color:#2196EA">■ built</span> `oculus_4` .. `oculus_7`   <span style="color:#A3A3A3">■ context</span> the ring beams

Each bottom wedge is a 27 x 27 mm ledge strip along ring beam `i`'s inner face, from the soffit to soffit + t, laid as a pinwheel inside the ring to carry the central plate.

Code: `FloorGuide::oculus`, [floor_members.cpp:252-255](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L252-L255)

## 103. Central plate

![](floor/103_central_plate.webp)

<span style="color:#2196EA">■ built</span> `oculus_8`   <span style="color:#E8478B">■ variable</span> half-diagonal 915.1   <span style="color:#A3A3A3">■ context</span> the ring beams and bottom wedges

`loft_planes(inner, side1, side3)` makes the square central plate `oculus_8`, which fills the ring on the bottom wedges and belongs to no quarter. Its top at -144.78 and the beds' `+2t` end at -144.76 differ by about 0.02 mm; nothing in the construction ties them.

Code: `FloorGuide::oculus`, [floor_members.cpp:257-259](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L257-L259)

## 104. column_cutters: fan_top

![](floor/104_fan_top.webp)

<span style="color:#2196EA">■ built</span> `fan_top[0..4]`   <span style="color:#737373">■ input</span> `edge(head, k)`, the head edges

`Quarter::column_cutters` carves the column head along `fan_top = {side0, wedges[0][0], wedges[1][0], wedges[2][0], side1}`, five planes each through one head edge at the datum, between the levels `xy0`, `xy1` and `xy2`. `levels[1]` is a placeholder 0 in `column_corner` until the `FloorGuide` constructor sets it to `rib_bottom_level`.

Code: `Quarter::column_face`, [floor_members.cpp:289-303](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L289-L303); `Quarter::column_cutters`, [305-313](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L305-L313); `column_corner`, [floor.cpp:134-137](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L134-L137); levels[1] [floor.cpp:375-384](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L375-L384), [458-459](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L458-L459)

## 105. column_cutters: fan_bottom

![](floor/105_fan_bottom.webp)

<span style="color:#2196EA">■ built</span> `fan_bottom[0..3]`   <span style="color:#E8478B">■ variable</span> the shaft square, `column_head` = 220, at `levels[2]`   <span style="color:#737373">■ input</span> chamfer `edge(column, 2)` and the drops to `levels[2]` (dashed)   <span style="color:#A3A3A3">■ context</span> the head edges

`fan_bottom = {side0, x = -2780, y = -2780, side1}` holds the two shaft faces but no plane for the chamfer edge 2, so below the rib-bottom level the carve returns to the square shaft corner (-2780, -2780).

Code: `Quarter::column_cutters`, [floor_members.cpp:311](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L311)

## 106. Cutter corners p0, p1, p2

![](floor/106_cutter_corners.webp)

<span style="color:#2196EA">■ built</span> `p0`, `p1`: the top fan creases   <span style="color:#F2CC0C">■ result</span> `p2`: the bottom fan creases   <span style="color:#737373">■ input</span> the crease lines `fan_top[i]` x `fan_top[i + 1]` (dashed)   <span style="color:#A3A3A3">■ context</span> the head and the shaft square

`p0` and `p1` are the top fan creases at the datum and at the rib-bottom level, `p2` the bottom fan creases at -730, with `p2[1]` the shaft corner. Every solve is unwrapped with `.value()`, so a parallel pair throws `std::bad_optional_access`.

Code: `Quarter::column_face`, [floor_members.cpp:289-303](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L289-L303); `Quarter::column_cutters`, [312-318](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L312-L318)

## 107. Six cutter quads

![](floor/107_cutter_quads.webp)

<span style="color:#2196EA">■ built</span> `quads[0..2]` on the fan   <span style="color:#F2CC0C">■ result</span> `quads[3..5]` down to `levels[2]`   <span style="color:#E8478B">■ variable</span> `p2[1] +- quarter`, 155.6 long   <span style="color:#A3A3A3">■ context</span> the head

Quads 0 to 2 are the top fan faces; quads 3 and 5 slope from fan faces 0 and 2 down to the shaft faces, and quad 4 slopes from the chamfer face down to the 155.6 mm segment `p2[1] +- quarter` centred on the shaft corner.

Code: `Quarter::column_cutters`, [floor_members.cpp:320-328](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L320-L328), `faces[i]` = `{p0[i], p0[i + 1], p1[i + 1], p1[i]}`

## 108. stretch: edges lengthened

![](floor/108_stretch_edges.webp)

<span style="color:#2196EA">■ built</span> quad 0 lengthened   <span style="color:#E8478B">■ variable</span> `d0`, `d1`: `CUTTER_MARGIN` = 100   <span style="color:#737373">■ input</span> `quads[0]` before stretch

`stretch` first lengthens edges 0-1 and 2-3 by `CUTTER_MARGIN` = 100 at both ends, so the cutter overshoots the side planes and neighbouring fan faces and leaves no sliver of head between two cutters.

Code: `stretch`, [floor_members.cpp:266-274](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L266-L274)

## 109. stretch: edges pushed apart

![](floor/109_stretch_apart.webp)

<span style="color:#2196EA">■ built</span> `quads[0]` stretched, a top quad   <span style="color:#F2CC0C">■ result</span> `quads[3]` stretched, a bottom quad   <span style="color:#E8478B">■ variable</span> the moves `-d2`, `-d3`   <span style="color:#737373">■ input</span> the two quads lengthened, `quads[3]` dashed

It then moves edge 0-1 outward by 100, and edge 2-3 only for top quads (`i < 3`), so a bottom quad keeps its edge on -730 and the carve never goes deeper than `column_head_depth`. The doc comment of `stretch` says "inwards", but every edge that moves goes outward.

Code: `stretch`, [floor_members.cpp:276-287](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L276-L287)

## 110. Cutter plates thickened

![](floor/110_cutter_plates.webp)

<span style="color:#2196EA">■ built</span> `column_cutters()[0..2]`, the top cutters   <span style="color:#F2CC0C">■ result</span> `column_cutters()[3..5]`, the bottom cutters   <span style="color:#E8478B">■ variable</span> `normal`, `CUTTER_MARGIN` = 100   <span style="color:#A3A3A3">■ context</span> the capitel box 340 x 340 x 730

Each stretched quad is thickened 100 mm along its normal toward the bay, and `column_cuts` turns the six outlines into difference `SolidCut`s lifted by `bay_height` on the column.

Code: `Quarter::column_cutters`, [floor_members.cpp:330-339](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_members.cpp#L330-L339); `column_cuts`, [floor_elements.cpp:92-105](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_elements.cpp#L92-L105); [floor_models.cpp:197-204](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_models.cpp#L197-L204)
