# Floor 02: Quarter planes {#templates_floor_02_quarter_planes}

[TOC]

`compute_quarter` calls `construction_planes` and `column_seats` to build the member planes `ConstructionPlanes cp` of one quarter and the column seats in `guide.columns[q]`. Every plane except the oculus beam's is a pair `{plane, plane.translate_by_normal(distance)}` (`[0]` base face, `[1]` offset face); values are for quarter 0 of `FloorGuide::rectangle(3000, 3000)`.

Example: [templates_floor_1_floorguide.cpp](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_1_floorguide.cpp) draws every member's two face planes `face_0` and `face_1` of quarter 0.

![](floor/film_02_quarter_planes.webp)

<span style="color:#2196EA">■ built</span> what the step builds   <span style="color:#F2CC0C">■ result</span> a second result   <span style="color:#E8478B">■ variable</span> the variable it introduces   <span style="color:#737373">■ input</span> what it reads, dashed for a helper   <span style="color:#A3A3A3">■ context</span> context

## 23. Outer rib planes, re-origined

![](floor/023_outer_rib_planes.webp)

<span style="color:#2196EA">■ built</span> `cp.outer_ribs[0]`, `cp.outer_ribs[1]`, their new origins and normals   <span style="color:#E8478B">■ variable</span> `outer_ribs = 100`   <span style="color:#737373">■ input</span> `edges[0].band[0].origin()`, `edges[3].band[0].origin()` and the re-origin move   <span style="color:#A3A3A3">■ context</span> quarter 0

`construction_planes` moves the origin of `band[0]` of edges `q` and `q - 1` (edges 0 and 3 for quarter 0) to the centre of the quarter's own half edge, keeping the normal, and `pair` adds the inner face `outer_ribs` = 100 mm inside the bay.

Code: `construction_planes`, [floor.cpp:185-187](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L185-L187); `reoriginated` [floor.cpp:20-22](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L20-L22); `pair` [floor.cpp:15-17](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L15-L17); `bay_edge` [floor.cpp:69-77](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L69-L77).

## 24. Seam beam planes into the quarter

![](floor/024_seam_beam_planes.webp)

<span style="color:#2196EA">■ built</span> `cp.inner_beams[0]`, `cp.inner_beams[2]`: seam plane and normal solid, far face dashed   <span style="color:#E8478B">■ variable</span> `inner_beams = 60`   <span style="color:#A3A3A3">■ context</span> quarter 0, `cp.outer_ribs`

`Seam::faces_into(q)` pairs the vertical plane over the half seam, its normal always pointing into the asking quarter, with a far face `inner_beams` = 60 mm behind; quarter 0 reads `seams[0]` as beam 0 and `seams[3]` as beam 2.

Code: `construction_planes`, [floor.cpp:190](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L190); `Seam::plane_into` [floor.cpp:411-416](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L411-L416); `Seam::faces_into` [floor.cpp:418-420](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L418-L420); `seam` [floor.cpp:80-90](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L80-L90); `edge_plane` [floor_geometry.cpp:23-25](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_geometry.cpp#L23-L25).

## 25. Oculus beam planes

![](floor/025_oculus_beam_planes.webp)

<span style="color:#2196EA">■ built</span> `cp.inner_beams[1][0]` = `oculus.tilted`   <span style="color:#F2CC0C">■ result</span> `cp.inner_beams[1][1]` = `oculus.back`   <span style="color:#E8478B">■ variable</span> `oculus_plane_angle = 5`, `inner_beams = 60`   <span style="color:#737373">■ input</span> `edge_plane(line, -z)` dashed   <span style="color:#A3A3A3">■ context</span> `oculus.ring_inner`, the datum

`cp.inner_beams[1]` is {`oculus.tilted`, `oculus.back`}: the oculus edge plane turned by `-oculus_plane_angle` (5 deg) about the edge, and the vertical edge plane moved `inner_beams` = 60 mm toward the column corner. Unlike every other pair, `[1]` is not `[0]` offset: it is vertical while `[0]` leans.

Code: `construction_planes`, [floor.cpp:189-190](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L189-L190); `oculus_edge` [floor.cpp:93-103](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L93-L103); `rotate` [floor_geometry.cpp:15-17](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_geometry.cpp#L15-L17).

## 26. p0 and p1

![](floor/026_p0_p1.webp)

<span style="color:#2196EA">■ built</span> `p0`, `p1`   <span style="color:#737373">■ input</span> `cp.inner_beams[0][1]`, `cp.inner_beams[1][1]`, `cp.inner_beams[2][1]`   <span style="color:#A3A3A3">■ context</span> `oculus_edges[0].line`, `cp.inner_beams[0][0]`, `cp.inner_beams[2][0]`

`p0` and `p1` are where the oculus back face `cp.inner_beams[1][1]` meets the far faces (not the base planes) of seam beams 0 and 2 on the datum `xy = level(0.0)`; they are the far ends of the two inner ribs.

Code: `construction_planes`, [floor.cpp:192-194](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L192-L194); `plane_plane_plane` [floor_geometry.cpp:51-59](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_geometry.cpp#L51-L59).

## 27. p2 = head[2], p3 = head[3]

![](floor/027_head_chamfer.webp)

<span style="color:#2196EA">■ built</span> `p2 = head[2]`, `p3 = head[3]` and the chamfer between them   <span style="color:#737373">■ input</span> `column.head`, `column.x_axis`, `column.y_axis`   <span style="color:#A3A3A3">■ context</span> bay edges 0 and 3

`p2 = column.head[2]` and `p3 = column.head[3]`, the two chamfer vertices of the head polygon from chapter 1, are the column ends of the inner ribs, and `chamfer_direction` = `unit(head[3] - head[2])`.

Code: `construction_planes`, [floor.cpp:195-196](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L195-L196); `column_corner` [floor.cpp:122-143](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L122-L143); `corner_frame` [floor.cpp:106-119](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L106-L119).

## 28. Inner rib planes and their normals

![](floor/028_inner_rib_planes.webp)

<span style="color:#2196EA">■ built</span> `cp.inner_ribs[0]`, `cp.inner_ribs[1]`: outer face solid, central face dashed, `normals[k]`   <span style="color:#737373">■ input</span> `p0`, `p1`   <span style="color:#A3A3A3">■ context</span> quarter 0, `column.head`, the inner beams' far faces

`cp.inner_ribs[k]` pairs the vertical plane through p2 and p0 (rib 0) or p3 and p1 (rib 1), its normal toward the quarter diagonal, with a central face `inner_ribs` = 60 mm toward the diagonal, which `central_panel` later reads as `faces[k]`.

Code: `construction_planes`, [floor.cpp:197-199](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L197-L199); `central_panel` [floor_panel.cpp:143-146](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_panel.cpp#L143-L146).

## 29. Wedge fan: tilted chamfer plane

![](floor/029_tilted_chamfer_plane.webp)

<span style="color:#2196EA">■ built</span> `tilted` = `cp.wedges[1][0]`   <span style="color:#E8478B">■ variable</span> `wedge_plane_angle = -10`   <span style="color:#737373">■ input</span> `side1 = edge(head, 2)`, `edge_plane(side1, +z)` dashed   <span style="color:#A3A3A3">■ context</span> `column.head`

`wedge_fan` turns the vertical plane on the chamfer `side1 = edge(head, 2)` by `wedge_plane_angle` (-10 deg) about the chamfer, so `tilted` moves away from the column below the datum; it becomes `cp.wedges[1][0]`, the face of the middle wedge block.

Code: `wedge_fan`, [floor.cpp:146-152](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L146-L152).

## 30. Wedge fan: crease lines

![](floor/030_crease_lines.webp)

<span style="color:#2196EA">■ built</span> `line0`, `line1`   <span style="color:#737373">■ input</span> `tilted`, `cp.inner_ribs[0][1]`, `cp.inner_ribs[1][1]`   <span style="color:#A3A3A3">■ context</span> `column.head`

`line0` and `line1` are the creases of `tilted` with the central faces `cp.inner_ribs[0][1]` and `cp.inner_ribs[1][1]` (not the outer faces); only their directions are used next.

Code: `wedge_fan`, [floor.cpp:153-154](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L153-L154); `plane_plane` [floor_geometry.cpp:31-39](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_geometry.cpp#L31-L39).

## 31. Side fan planes and provisional far faces

![](floor/031_fan_planes.webp)

<span style="color:#2196EA">■ built</span> `wedge0`, `tilted`, `wedge2` = `cp.wedges[i][0]` and their normals   <span style="color:#F2CC0C">■ result</span> `cp.wedges[i][1]`, the provisional far faces   <span style="color:#A3A3A3">■ context</span> the shaft faces of `column.head`, `cp.inner_ribs`

`wedge0` contains head edge `side0` parallel to `line0`, `wedge2` contains `side2` parallel to `line1`, and `wedge_fan` pairs them and `tilted` with far faces `wedge`, `wedge * middle_wedge_factor` and `wedge` into `cp.wedges` (also kept as `column.wedge_fan`). These far faces are provisional: after `run_ins`, `block_planes` offsets them by `run_in[0]`, `1.25 * mean(run_in)` and `run_in[1]`. On the square bay `run_in = {240, 240}` so they match; on `FloorGuide::rectangle(3000, 2400)` they end at 240 / 267.292 / 187.667.

Code: `wedge_fan`, [floor.cpp:155-158](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L155-L158); stored at [floor.cpp:201-202](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L201-L202); replaced by `block_planes` [floor.cpp:314-322](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L314-L322), [368](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L368).

## 32. T-section planes

![](floor/032_tsection_planes.webp)

<span style="color:#2196EA">■ built</span> `cp.tsections[0]` to `cp.tsections[5]`   <span style="color:#737373">■ input</span> `cp.outer_ribs`, `cp.inner_ribs`   <span style="color:#A3A3A3">■ context</span> quarter 0

`cp.tsections` holds six flange pairs, one on each rib face that faces a bed panel, each offset `tsections` = 27 mm into that panel. The sign is `+` where the face normal already points into the panel and `-` where it points into the rib.

Code: `construction_planes`, [floor.cpp:204-211](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L204-L211).

## 33. column_seats: column_offset

![](floor/033_column_offset.webp)

<span style="color:#E8478B">■ variable</span> `band = 100`   <span style="color:#737373">■ input</span> the `cp.outer_ribs[0]` and `cp.outer_ribs[1]` bands over the head   <span style="color:#A3A3A3">■ context</span> bay edges 0 and 3, `column.head`

`column.column_offset = {offset, offset}` with `offset = column_head * sin(phi)`, `phi = (corner_angle(k) - 90) / 2`, is the signed distance of the column's outer face from the bay edge: positive where the outer rib band overhangs the column face, negative where the column stands outside the bay edge. At a right corner it is 0 and the local `band = (outer_ribs - offset) / cos(phi)` is exactly `outer_ribs`; only `FloorReport` reads the offset.

Code: `column_seats`, [floor.cpp:162-167](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L162-L167) (called at 367); `report.column_offset_mm` [floor_report.cpp:157](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_report.cpp#L157).

## 34. column_seats: wedge_seat

![](floor/034_wedge_seat.webp)

<span style="color:#2196EA">■ built</span> `column.wedge_seat[0]`, `[1]`, `[2]`   <span style="color:#737373">■ input</span> the outer rib bands, `cp.inner_ribs[0]`, `cp.inner_ribs[1]`   <span style="color:#A3A3A3">■ context</span> `column.head`

`column.wedge_seat` = `{column_head_chamfer - band, chamfer_length - inner_ribs/sin0 - inner_ribs/sin1, column_head_chamfer - band}`, {20.000, 19.296, 20.000} here, is what is left of the head sides and chamfer for the three wedge blocks; like `column_offset` it is informational, copied only to `wedge_seat_mm`.

Code: `column_seats`, [floor.cpp:169-172](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor.cpp#L169-L172) (called at 367); `report.wedge_seat_mm` [floor_report.cpp:156](https://github.com/petrasvestartas/wood/blob/79d4d5c74fdbb4c7b474b2df14efe7f6c7b72e2f/src/templates/floor/floor_report.cpp#L156).
