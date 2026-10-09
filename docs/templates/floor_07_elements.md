# Floor 7: Members and columns {#templates_floor_07_elements}

[TOC]

<em>Step 7 of @ref templates_floor_model · previous: @ref templates_floor_guide · next: @ref templates_floor_08_contacts</em>

`Floor::add_members` turns the guide's face loops into elements: `add_quarters` makes the six member families of every quarter, `add_oculus` the ring around the hole, `add_columns` a column on its support at every corner, then `add_contacts` (chapter 8) finds where they touch. Every member is built at the guide's datum and lifted to `bay_height`. The pictures show quarter 0 of the default 6000 x 6000 bay.

Example: [templates_floor_2_column_model.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_2_column_model.cpp) builds one column on its support; [templates_floor_7_contacts_cantilevers.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_7_contacts_cantilevers.cpp) the whole floor.

## 201. Floor(guide)

![](floor/201_floor.webp)

<span style="color:#737373">■ input</span> `guide.corners`   <span style="color:#A3A3A3">■ context</span> the bay edges, seams and oculus of the guide

`Floor(guide, name)` is an empty `WoodSession`, named `floor` by default, that keeps its own copy of the guide; nothing is built until `add_members` is called.

Code: `Floor::Floor`, [floor.cpp:43-46](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L43-L46); [floor.h:42-64](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.h#L42-L64).

## 202. add_members

![](floor/202_add_members.webp)

<span style="color:#2196EA">■ add_quarters</span> the quarters' members   <span style="color:#F2CC0C">■ add_oculus</span> the ring and the oculus plates   <span style="color:#E8478B">■ add_columns</span> the columns and their supports

`add_members` calls `add_quarters`, `add_oculus`, `add_columns` and last `add_contacts`, in that order.

Code: `Floor::add_members`, [floor.cpp:52-58](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L52-L58).

## 203. quarter_group

![](floor/203_quarter_group.webp)

<span style="color:#2196EA">■ built</span> `quarter_0`   <span style="color:#737373">■ input</span> the other quarters' polygons

`quarter_group(q)` returns the session's group `quarter_q`, found by its name or made the first time, and every member of quarter q goes in a group under it.

Code: `Floor::quarter_group`, [floor.cpp:159-161](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L159-L161); `Session::group_named`, [session_cpp session.cpp:1145-1155](https://github.com/petrasvestartas/session_cpp/blob/22aba8a5262db321692f89d27f73c334772c9c5d/src/session.cpp#L1145-L1155).

## 204. The lift to the floor

![](floor/204_add_placed.webp)

<span style="color:#2196EA">■ built</span> `outer_ribs_0_0` at the floor   <span style="color:#737373">■ input</span> its loops at the datum, dashed

Each new element is made with its name, moved up by `bay_height` (3500) with `place(lift)` and added under its group; the guide's loops all lie at the datum z 0.

Code: `Floor::add_quarters`.

## 205. A bed row

![](floor/205_bed_row.webp)

<span style="color:#2196EA">■ built</span> the plates of bed row 0   <span style="color:#737373">■ input</span> `bottom`, the two bottom rails   <span style="color:#E8478B">■ variable</span> `top`, the two top rails   <span style="color:#A3A3A3">■ context</span> the rest of quarter 0

`add_quarters` starts with the beds: `guide.bed_rails(q)` gives each row's two bottom and two top rails, and `Plate::row_between(bottom, top)` lofts one plate per rail segment, its bottom quad and its top quad.

Code: `Floor::add_quarters`, [floor.cpp:68-77](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L68-L77); `Plate::row_between`, [wood_element_plate.cpp:91-103](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_elements/wood_element_plate.cpp#L91-L103); `FloorGuide::bed_rails`, [floor_guide.cpp:597-625](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.cpp#L597-L625).

## 206. The three bed rows

![](floor/206_bed_rows.webp)

<span style="color:#2196EA">■ row 0</span>   <span style="color:#F2CC0C">■ row 1</span>   <span style="color:#E8478B">■ row 2</span>   <span style="color:#A3A3A3">■ context</span> the rest of quarter 0

Row r goes in the group `beds_r_q` under `beds_q`, its plates named `beds_r_i_q`.

Code: `Floor::add_quarters`, [floor.cpp:68-77](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L68-L77).

## 207. tsections

![](floor/207_tsections.webp)

<span style="color:#2196EA">■ built</span> the six t-section plates   <span style="color:#A3A3A3">■ context</span> quarter 0 without its beds

Each `tsections` loop pair becomes `Plate(loops[1], loops[0])`, a flange strip beside a rib face named `tsections_i_q`; the beds rest on them.

Code: `Floor::add_quarters`, [floor.cpp:79-85](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L79-L85).

## 208. rib(): the two loops

![](floor/208_rib_loops.webp)

<span style="color:#E8478B">■ near</span> `loops[0]`, its soffit points dotted   <span style="color:#F2CC0C">■ far</span> `loops[1]`

A rib's two face loops each list the two top corners `near[0]` and `near[1]`, then the soffit points from `near[2]`; `rib()` makes one section per soffit point.

Code: `Floor::rib`, [floor.cpp:170-175](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L170-L175).

## 209. rib(): the sections

![](floor/209_rib_sections.webp)

<span style="color:#2196EA">■ built</span> `sections`   <span style="color:#737373">■ input</span> the two loops

At each soffit point the section is the closed quad low, high, far_high, far_low, high straight above it at the datum, the first and last sections up to the top corners so the rib ends in its end planes.

Code: `Floor::rib`, [floor.cpp:177-192](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L177-L192).

## 210. rib(): the axis

![](floor/210_rib_axis.webp)

<span style="color:#E8478B">■ variable</span> `axis`   <span style="color:#A3A3A3">■ context</span> the rib it makes

The axis runs from the middle of the two loops' corners `[1]` to the middle of their corners `[0]`, and `BeamVariable(axis, sections)` is the rib.

Code: `Floor::rib`, [floor.cpp:194-196](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L194-L196).

## 211. outer_ribs

![](floor/211_outer_ribs.webp)

<span style="color:#2196EA">■ built</span> the two outer ribs   <span style="color:#A3A3A3">■ context</span> the rest of quarter 0

The two `outer_ribs` loop pairs become `rib()` beams named `outer_ribs_i_q`.

Code: `Floor::add_quarters`, [floor.cpp:87-93](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L87-L93).

## 212. inner_ribs

![](floor/212_inner_ribs.webp)

<span style="color:#2196EA">■ built</span> the two inner ribs   <span style="color:#A3A3A3">■ context</span> the rest of quarter 0

The two `inner_ribs` loop pairs become `rib()` beams named `inner_ribs_i_q`.

Code: `Floor::add_quarters`, [floor.cpp:95-101](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L95-L101).

## 213. wedges

![](floor/213_wedges.webp)

<span style="color:#2196EA">■ built</span> the three column blocks   <span style="color:#A3A3A3">■ context</span> the rest of quarter 0

The three `wedges` loop pairs, the blocks on the column head, become `Plate(loops[1], loops[0])` named `wedges_i_q`.

Code: `Floor::add_quarters`, [floor.cpp:103-109](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L103-L109).

## 214. beam(): the end sections

![](floor/214_beam_caps.webp)

<span style="color:#E8478B">■ first</span> from loop corners 0 and 3   <span style="color:#F2CC0C">■ last</span> from loop corners 1 and 2   <span style="color:#A3A3A3">■ context</span> the beam

`beam(loops, {0, 3}, {1, 2})` takes the end sections from corners 0 and 3 and from corners 1 and 2 of both loops, and `BeamVariable::between(first, last)` lofts the beam between them.

Code: `Floor::beam`, [floor.cpp:199-207](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L199-L207); `BeamVariable::between`, [wood_element_beam_variable.cpp:23-28](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_elements/wood_element_beam_variable.cpp#L23-L28).

## 215. inner_beams

![](floor/215_inner_beams.webp)

<span style="color:#2196EA">■ built</span> the two seam beams and the oculus beam   <span style="color:#A3A3A3">■ context</span> the rest of quarter 0

The three `inner_beams` loop pairs, seam 0, the oculus edge and seam 1, become `beam()` beams named `inner_beams_i_q`.

Code: `Floor::add_quarters`, [floor.cpp:111-117](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L111-L117).

## 216. The four quarters

![](floor/216_add_quarters.webp)

<span style="color:#E8478B">■ outer_ribs</span>   <span style="color:#F2CC0C">■ inner_ribs</span>   <span style="color:#7C7C7C">■ inner_beams</span>   <span style="color:#A8A8A8">■ wedges</span>   <span style="color:#F5D890">■ tsections</span>   <span style="color:#A6D3F6">■ beds</span>

`add_quarters` runs the same code for every quarter at its own corner, the families in the order beds, tsections, outer_ribs, inner_ribs, wedges, inner_beams, each in its group under `quarter_q`; `quarters[q]` keeps them by family.

Code: `Floor::add_quarters`, [floor.cpp:60-119](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L60-L119).

## 217. add_oculus: the ring beams

![](floor/217_ring_beams.webp)

<span style="color:#2196EA">■ built</span> `oculus_0` to `oculus_3`   <span style="color:#A3A3A3">■ context</span> the quarters' inner beams and ribs

`guide.oculus()` gives nine loop pairs; the first four become ring beams by `beam(loops, {1, 0}, {2, 3})`, each named `oculus_q` in the group `oculus_q` under `oculus`, and kept in `ring`.

Code: `Floor::add_oculus`, [floor.cpp:121-132](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L121-L132); `FloorGuide::oculus`, [floor_guide.cpp:751-779](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.cpp#L751-L779).

## 218. add_oculus: the bottom wedges

![](floor/218_bottom_wedges.webp)

<span style="color:#2196EA">■ built</span> `oculus_4` to `oculus_7`   <span style="color:#A3A3A3">■ context</span> the ring and the quarters' members, seen from below

Loops 4 to 7 are the bottom wedges the ring beams sit on, plates in the same quarter groups, kept in `oculus_plates`.

Code: `Floor::add_oculus`, [floor.cpp:133-136](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L133-L136).

## 219. add_oculus: the central plate

![](floor/219_central_plate.webp)

<span style="color:#2196EA">■ built</span> `oculus_8`   <span style="color:#A3A3A3">■ context</span> the ring and the quarters' members

Loop 8 is the central plate `oculus_8`, the only member in the group `oculus` at the top of the tree.

Code: `Floor::add_oculus`, [floor.cpp:128-136](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L128-L136).

## 220. add_column

![](floor/220_add_column.webp)

<span style="color:#2196EA">■ built</span> `column_0` and its support   <span style="color:#A3A3A3">■ context</span> the quarters

`add_column(corner)` builds the column as a `WoodSession` of its own with `column(guide, k)`, grafts it into the group `column_k` of `quarter_k` and keeps the grafted column in `columns[k]`.

Code: `Floor::add_column`, [floor.cpp:146-157](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L146-L157); `column`, [floor.cpp:15-35](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L15-L35).

## 221. The support

![](floor/221_support.webp)

<span style="color:#2196EA">■ built</span> `support_0`   <span style="color:#E8478B">■ variable</span> `support_plane(0)`

`column()` stands a `Support` on `guide.support_plane(k)`, its base plate on the slab under the column corner's centre, and names it `support_k`.

Code: `column`, [floor.cpp:20-21](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L20-L21); `FloorGuide::support_plane`, [floor_guide.cpp:197-202](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.cpp#L197-L202).

## 222. The column axis

![](floor/222_column_axis.webp)

<span style="color:#E8478B">■ variable</span> `column_axis(bay_height)`   <span style="color:#A3A3A3">■ context</span> the support

`Support::column_axis(bay_height)` runs from `column_foot()`, the head plate top less its recess, straight up to the floor top.

Code: `Support::column_axis`, [wood_element_support.cpp:147-152](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_elements/wood_element_support.cpp#L147-L152); `Support::column_foot`, [wood_element_support.cpp:143-145](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_elements/wood_element_support.cpp#L143-L145).

## 223. Column::square

![](floor/223_column_square.webp)

<span style="color:#2196EA">■ built</span> `column_0`, the shaft   <span style="color:#E8478B">■ variable</span> `column_frame(0)`

`Column::square(axis, column_frame(k), size_column_head)` sweeps the 220 square from the corner frame's origin along its x and y axes up the axis.

Code: `column`, [floor.cpp:22](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L22); `Column::square`, [wood_element_column.cpp:92-94](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_elements/wood_element_column.cpp#L92-L94); `FloorGuide::column_frame`, [floor_guide.cpp:183-195](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.cpp#L183-L195).

## 224. head_blocks

![](floor/224_head_blocks.webp)

<span style="color:#F2CC0C">■ result</span> `column_0_head_0`, `column_0_head_1`   <span style="color:#A3A3A3">■ context</span> the shaft

`Column::head_blocks(head_side, head_height)` makes two blocks over the top `column_head_depth` (730), one beyond each far side of the section, that widen the head to a `size_column_head + size_column_head_chamfer` (340) square.

Code: `Column::head_blocks`, [wood_element_column.cpp:96-125](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_elements/wood_element_column.cpp#L96-L125); `WoodSession::add_column`, [wood_session.cpp:1508-1512](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_session.cpp#L1508-L1512).

## 225. The head glued on

![](floor/225_glued.webp)

<span style="color:#2196EA">■ built</span> `column_0` with its head

`WoodSession::add_column` adds each head block hidden and glues it on with `add_interaction(block, column, InteractionFeatureSolid(block, SolidOperation::add))`, so the column's stock is the shaft and both blocks.

Code: `WoodSession::add_column`, [wood_session.cpp:1498-1512](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_session.cpp#L1498-L1512); `column`, [floor.cpp:32-33](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L32-L33).

## 226. Joint::support

![](floor/226_support_joint.webp)

<span style="color:#2196EA">■ built</span> the head plate disc of the joint   <span style="color:#E8478B">■ variable</span> `drill_lines`, the three column screws   <span style="color:#A3A3A3">■ context</span> the support

`add_column` adds the support and `Joint::support(support, column)`: the head plate disc let up into the column end and three screws from its underside, which `add_joint` cuts and drills out of the column.

Code: `WoodSession::add_column`, [wood_session.cpp:1514-1519](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_session.cpp#L1514-L1519); `Joint::support`, [wood_element_joint.cpp:78-93](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_elements/wood_element_joint.cpp#L78-L93).

## 227. The cutters

![](floor/227_cutters.webp)

<span style="color:#2196EA">■ built</span> the six cutter plates   <span style="color:#A3A3A3">■ context</span> the glued column

`column()` turns the six `guide.column_cutters(k)` loop pairs into plates `column_cutters_i_k`, placed at `bay_height` around the head.

Code: `column`, [floor.cpp:24-30](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L24-L30); `FloorGuide::column_cutters`, [floor_guide.cpp:789-842](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.cpp#L789-L842).

## 228. The carved head

![](floor/228_carved.webp)

<span style="color:#2196EA">■ built</span> `column_0`

`add_column` adds each cutter hidden and takes it away with `InteractionFeatureSolid(cutter, SolidOperation::subtract)`, leaving the inclined faces the ribs and the column blocks bear on.

Code: `WoodSession::add_column`, [wood_session.cpp:1521-1525](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_session.cpp#L1521-L1525).

## 229. add_columns

![](floor/229_add_columns.webp)

<span style="color:#2196EA">■ built</span> the four columns and their supports   <span style="color:#A3A3A3">■ context</span> the bay edges on the slab

`add_columns` runs `add_column` at every corner, `column_k` on `support_k` in `quarter_k`.

Code: `Floor::add_columns`, [floor.cpp:140-144](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor.cpp#L140-L144).
