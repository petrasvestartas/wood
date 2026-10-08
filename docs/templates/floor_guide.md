# Part 1. FloorGuide: the geometry {#templates_floor_guide}

[TOC]

`wood_floor::FloorGuide` (`src/templates/floor/floor_guide.h`) computes geometry only, and no elements: from four corners and the parameters it builds the planes, plan quads and parabolas of every quarter and ends with every member as two face loops at the datum z 0. Part 2, @ref templates_floor_model, turns those loops into the model.

## Steps

Each step is one chapter, one picture per sub-step, in code order:

1. @subpage templates_floor_01_bay (the corners, the centre, the seams and the oculus, the bay edges with their rib bands, and the four column corners)
2. @subpage templates_floor_02_quarter_planes (every member's two faces in quarter 0: outer ribs, seam and oculus beams, inner ribs, the wedge fan and the t-sections)
3. @subpage templates_floor_03_parabolas (the plan quads, the run-in solve that levels both outer ribs at the column, the final wedge faces and the rib parabolas with their layers)
4. @subpage templates_floor_04_central_panel (the sweep that makes the central bed panel buildable, its traces, and the bed tops the wedges stand on)
5. @subpage templates_floor_05_rib_outlines (the rib outlines, the middle cutter level, the soffit, and how the guide draws itself)
6. @subpage templates_floor_06_outlines (every other member's outline, the oculus ring and the six column cutters)

## Overview

One picture per method of the guide, in the order `compute()` runs them.

### FloorGuide

![FloorGuide](floor/901_floor_guide.webp)

<span style="color:#2196EA">■ corners, oculus points</span> <span style="color:#737373">■ seams, dashed</span>

The guide is made from four corners and the parameters (`size_outer_ribs`, `size_wedge`, `height`, `rise` ...); `compute()` runs every method below for each quarter and draws the result.

Code: [`FloorGuide`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L67)

### quarter_polygon

![quarter_polygon](floor/902_quarter_polygon.webp)

<span style="color:#2196EA">■ quarter 0</span> <span style="color:#A3A3A3">■ the other quarters</span>

The bay is split into four quarters; quarter q is the one at `corners[q]`. Every FloorGuide method takes q and computes that quarter alone, so a rectangular bay gets four different quarters; the pictures below show q = 0. The quarter's five lines carry all its planes: the bay edges, the two seams, the oculus edge.

Code: [`quarter_polygon`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L117)

### quarter_column_polygon

![quarter_column_polygon](floor/903_quarter_column_polygon.webp)

<span style="color:#2196EA">■ the column head</span> <span style="color:#737373">■ column_frame</span>

The column head at the quarter's corner, where the ribs start; `column_frame` gives its axes.

Code: [`quarter_column_polygon`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L120)

### construction_planes

![construction_planes](floor/904_construction_planes.webp)

<span style="color:#2196EA">■ base face</span> <span style="color:#F2CC0C">■ offset face</span> <span style="color:#A3A3A3">■ the member's footprint</span>

A plane pair for every member: its base face on one of the polygon's lines, and the face offset by the member's size. Every member is cut from these planes.

Code: [`construction_planes`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L133)

### construction_quads

![construction_quads](floor/905_construction_quads.webp)

family colours

Where each member's four planes meet the floor datum: its footprint in plan.

Code: [`construction_quads`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L136)

### boundary_parabolas

![boundary_parabolas](floor/906_boundary_parabolas.webp)

<span style="color:#2196EA">■ the parabolas</span> <span style="color:#F2CC0C">■ their +t and +2t layers</span> <span style="color:#737373">■ rib quads</span>

A parabola under each rib axis, from `-height` at the column to `-static_h` at the seam. The inner ones are the outer ones projected onto the inner ribs.

Code: [`boundary_parabolas`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L146)

### central_panel

![central_panel](floor/907_central_panel.webp)

<span style="color:#2196EA">■ soffit traces</span> <span style="color:#F2CC0C">■ layers</span> <span style="color:#E8478B">■ ruling</span>

Between the two inner ribs, one ruling crosses the panel and one sweep serves both ribs (rule A), so the central beds stay flat quads.

Code: [`central_panel`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L149)

### Two face loops

![Two face loops](floor/908_loops.webp)

<span style="color:#2196EA">■ [0]: top</span> <span style="color:#F2CC0C">■ [1]: bottom</span>

Every member method below returns each member as `std::array<Polyline, 2>`: its two face loops, lofted by `loft`. The guide stops here; the Floor turns loops into elements.

Code: [`loft`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L198)

### outer_ribs, inner_ribs

![outer_ribs, inner_ribs](floor/909_ribs.webp)

<span style="color:#2196EA">■ the ribs</span> <span style="color:#A3A3A3">■ the rest of the quarter</span>

Each rib's parabola trimmed by the planes it ends on, on both of its faces.

Code: [`outer_ribs`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L171)

### tsections

![tsections](floor/910_tsections.webp)

<span style="color:#2196EA">■ the t-sections</span> <span style="color:#A3A3A3">■ ribs and beams</span>

Flange strips beside the rib faces; the beds rest on them.

Code: [`tsections`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L168)

### beds

![beds](floor/911_beds.webp)

<span style="color:#2196EA">■ the beds</span>

Three rows of bed plates between the ribs, each row trimmed alike so every plate stays a quad.

Code: [`beds`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L165)

### wedges, inner_beams

![wedges, inner_beams](floor/912_wedges_and_beams.webp)

<span style="color:#2196EA">■ column blocks and inner beams</span>

The three column blocks at the head, and the three beams on the seams and the oculus edge.

Code: [`wedges`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L177)

### oculus()

![oculus()](floor/913_oculus.webp)

<span style="color:#2196EA">■ ring beams</span> <span style="color:#F2CC0C">■ the quarters' oculus beams</span> <span style="color:#A3A3A3">■ bottom wedges and central plate</span>

Four ring beams around the hole, one per oculus edge, each meeting its quarter's oculus beam on the tilted plane.

Code: [`oculus`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L183)

### column_cutters

![column_cutters](floor/914_column_cutters.webp)

<span style="color:#2196EA">■ the cutters</span> <span style="color:#A3A3A3">■ the column</span>

Six plates that carve the column head so the ribs and the column blocks sit on it.

Code: [`column_cutters`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor_guide.h#L189)
