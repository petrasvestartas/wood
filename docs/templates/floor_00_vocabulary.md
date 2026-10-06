# 0. Vocabulary {#templates_floor_00_vocabulary}

Every class of `floor.h`, one picture each, in the order the floor is built. The chapters after this page use these names.

```mermaid
flowchart TD
    FloorGuide --> BayEdge & Seam & OculusEdge & ColumnCorner
    FloorGuide --> QuarterGeometry
    QuarterGeometry --> ConstructionPlanes & ConstructionQuads & CentralPanel
    FloorGuide --> Quarter
    Quarter --> Rib & TSection & BedRow & ColumnCutters
    Rib & TSection & BedRow & ColumnCutters --> Outline
    FloorGuide --> Contacts & Screws
    Screws --> OculusScrew
    Contacts & Screws --> Relationship
    Outline & Relationship --> Floor
```

## FloorGuide

![FloorGuide](floor/901_floor_guide.webp)

<span style="color:#2196EA">■ the class</span> <span style="color:#737373">■ the seams, dashed</span>

The guide is the first thing you make: four corners and the parameters as its fields; `compute()` turns them into every part on this page and draws them.

Code: [`FloorGuide`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L76)

## BayEdge

![BayEdge](floor/902_bay_edge.webp)

<span style="color:#2196EA">■ the class</span> <span style="color:#F2CC0C">■ its band</span>

One side of the bay. Its band is the strip of outer rib that the two quarters on either side of the edge midpoint share.

Code: [`BayEdge`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L163)

## Seam

![Seam](floor/903_seam.webp)

<span style="color:#2196EA">■ the class</span> <span style="color:#F2CC0C">■ faces_into(0)</span> <span style="color:#E8478B">■ faces_into(1)</span>

The line from an edge midpoint to the centre, where two quarters meet. Each quarter reads its own seam beam face from it.

Code: [`Seam`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L174)

## OculusEdge

![OculusEdge](floor/904_oculus_edge.webp)

<span style="color:#2196EA">■ the class</span> and its tilted plane <span style="color:#F2CC0C">■ back</span> <span style="color:#E8478B">■ ring_inner</span>

One side of the central hole. Its tilted plane is where a quarter's oculus beam meets a ring beam.

Code: [`OculusEdge`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L193)

## ColumnCorner

![ColumnCorner](floor/905_column_corner.webp)

<span style="color:#2196EA">■ the class</span>: head and frame <span style="color:#F2CC0C">■ wedge_fan</span>

The column head at one bay corner, with its frame and the fan of planes the column blocks stand on.

Code: [`ColumnCorner`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L205)

## ConstructionPlanes

![ConstructionPlanes](floor/906_construction_planes.webp)

<span style="color:#F2CC0C">■ base face</span> <span style="color:#2196EA">■ the class</span>: the face offset by the thickness <span style="color:#A3A3A3">■ the member footprint</span>

Two planes per member of a quarter: the face it starts from and the face it is offset to. Every member is cut from these planes.

Code: [`ConstructionPlanes`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L234)

## ConstructionQuads

![ConstructionQuads](floor/907_construction_quads.webp)

family colours

Where each member's four planes meet the floor datum: its footprint in plan.

Code: [`ConstructionQuads`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L252)

## CentralPanel

![CentralPanel](floor/908_central_panel.webp)

<span style="color:#2196EA">■ the class</span>: soffit traces <span style="color:#F2CC0C">■ +t and +2t layers</span> <span style="color:#E8478B">■ the ruling</span>

The panel between the two inner ribs. One ruling crosses it and one sweep serves both ribs, so its bed plates stay buildable.

Code: [`CentralPanel`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L271)

## QuarterGeometry

![QuarterGeometry](floor/909_quarter_geometry.webp)

<span style="color:#2196EA">■ the class</span>: the quarter polygon <span style="color:#F2CC0C">■ the rib parabolas</span> <span style="color:#737373">■ rib quads</span>

Everything one quarter is built from: polygon, planes, quads, run-ins, parabolas, central panel and bed planes. The guide holds four of them.

Code: [`QuarterGeometry`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L306)

## Outline

![Outline](floor/910_outline.webp)

<span style="color:#2196EA">■ the class</span>: top <span style="color:#F2CC0C">■ bottom</span>

A member as two closed loops. The element is lofted between them.

Code: [`Outline`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L347)

## Quarter

![Quarter](floor/911_quarter.webp)

family colours

A view of one quarter of the guide. It builds that quarter's member outlines: outer ribs, inner ribs, inner beams, column blocks, t-sections and beds.

Code: [`Quarter`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L375)

## Rib

![Rib](floor/912_rib.webp)

<span style="color:#2196EA">■ the class</span> <span style="color:#A3A3A3">■ the rest of the quarter</span>

A rib's outline: its parabola trace, trimmed by the planes it ends on, on both faces of the rib.

Code: [`Rib`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L415)

## TSection

![TSection](floor/913_tsection.webp)

<span style="color:#2196EA">■ the class</span> <span style="color:#A3A3A3">■ the ribs and beams</span>

A flange strip beside a rib face. The beds rest on it.

Code: [`TSection`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L435)

## BedRow

![BedRow](floor/914_bed_row.webp)

<span style="color:#2196EA">■ the class</span>

A row of bed plates between two ribs, trimmed alike so every plate stays a quad.

Code: [`BedRow`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L447)

## ColumnCutters

![ColumnCutters](floor/915_column_cutters.webp)

<span style="color:#2196EA">■ the class</span> <span style="color:#A3A3A3">■ the column</span>

Six plates that carve the column head so the ribs and the column blocks sit on it.

Code: [`ColumnCutters`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L459)

## Ring

![Ring](floor/921_ring.webp)

<span style="color:#2196EA">■ the class</span>: ring beams <span style="color:#F2CC0C">■ the quarters' oculus beams</span> <span style="color:#A3A3A3">■ bottom wedges and central plate</span>

The ring is not a class: it is the family `ring`, the four ring beams `FloorGuide::oculus()` makes around the hole, one per oculus edge. They are neither t-sections nor the quarters' inner beams: each quarter's oculus beam (`inner_beams[1]`) sits outside its ring beam and meets it on the tilted plane, where the oculus wedge goes.

## Relationship

![Relationship](floor/916_relationship.webp)

<span style="color:#F2CC0C">■ member a</span> <span style="color:#E8478B">■ member b, its loops</span> <span style="color:#2196EA">■ the class</span>: the contact

Two members, named by `MemberRef`, the plane they meet on, and the contact the connector stands on.

Code: [`Relationship`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L497)

## Contacts

![Contacts](floor/917_contacts.webp)

<span style="color:#2196EA">■ the class</span>: the contacts <span style="color:#A3A3A3">■ the members</span>

Finds every surface two members share: where the wedges, column plates, ties and dowels go.

Code: [`Contacts`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L521)

## Screws

![Screws](floor/918_screws.webp)

<span style="color:#2196EA">■ the class</span>: the screws <span style="color:#A3A3A3">■ the members' loops</span>

Finds where the assembly screws go between members that butt, each a 200 mm line.

Code: [`Screws`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L556)

## OculusScrew

![OculusScrew](floor/919_oculus_screw.webp)

<span style="color:#2196EA">■ the class</span>: the screws <span style="color:#A3A3A3">■ ring beam and oculus beam</span>

Aims a screw from a ring beam into a quarter's oculus beam, past the wedge between them.

Code: [`OculusScrew`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L601)

## Floor

![Floor](floor/920_floor.webp)

family colours, the connectors in <span style="color:#2196EA">■ blue</span>

The model: every member placed at `bay_height`, then the connectors and the screws added.

Code: [`Floor`](https://github.com/petrasvestartas/wood/blob/75225ff780b6cf052999a265701fd37a14e116e4/src/templates/floor/floor.h#L671)
