# FloorGuide and Floor {#templates_floor_00_vocabulary}

The floor is two classes. **`FloorGuide`** (`floor_guide.h`) computes geometry only: planes, quads, parabolas and every member as two face loops. **`Floor`** (`floor.h`) builds the model from it: elements, the contact interactions between them, connectors and screws.

```mermaid
flowchart TD
    subgraph G["FloorGuide: geometry"]
        direction TB
        A["corners + parameters"] --> P["quarter_polygon, quarter_column_polygon"]
        P --> CP["construction_planes"] --> CQ["construction_quads"] --> BP["boundary_parabolas, central_panel"]
        BP --> M["outer_ribs, inner_ribs, inner_beams, wedges, tsections, beds, oculus, column_cutters: two face loops each"]
    end
    subgraph F["Floor: model"]
        direction TB
        E["add_quarters, add_oculus, add_columns: elements"] --> I["add_contacts: interactions"] --> J["add_connectors"] --> S["add_screws"]
    end
    M --> E
```

## FloorGuide: the geometry

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

## Floor: the model

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

`ScrewLines` finds the 200 mm screw lines between members that butt; `JointBeam::screws` pre-drills them into both members.

Code: [`add_screws`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor.h#L191)

### Floor

![Floor](floor/921_floor.webp)

family colours, connectors in blue

The finished model: every member at `bay_height`, its contacts, connectors and screws.

Code: [`Floor`](https://github.com/petrasvestartas/wood/blob/fb0e0986bd4dfdde98bb038926f4f202aac7dadb/src/templates/floor/floor.h#L150)
