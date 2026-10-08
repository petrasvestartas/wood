# Floor {#templates_floor}

[TOC]

The vaulted timber floor bay: a bay on four columns, cut by four seams into four quarters of parabolic ribs, beams, column blocks, t-sections and beds around a central oculus. Two `WoodSession` classes build it: `wood_floor::FloorGuide` (`src/templates/floor/floor_guide.h`) computes the geometry, and `wood_floor::Floor` (`src/templates/floor/floor.h`) builds the elements, their contacts, the connectors and the screws from it.

![The floor in its key steps](floor/floor_film.webp)

The floor is two classes, read in this order:

## 1. FloorGuide: the geometry

Read @subpage templates_floor_guide first. From four corners and the parameters it computes every quarter's planes, plan quads and parabolas, and every member as two face loops: geometry only, no elements; steps 1 to 6.

## 2. Floor: the model

Then @subpage templates_floor_model builds on it. The guide's loops become elements, their contacts become interactions, and the connectors and screws are made from them; steps 7 to 11.

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

## Reading the pictures

Each colour marks one role, the same in picture, key and text:

| Colour | Role |
|---|---|
| <span style="color:#2196EA">■ blue</span> `#2196EA` | what the step builds |
| <span style="color:#E8478B">■ pink</span> `#E8478B` | the variable or value the step introduces |
| <span style="color:#F2CC0C">■ yellow</span> `#F2CC0C` | a second result, set apart from the first |
| <span style="color:#737373">■ grey</span> `#737373` | what the step reads from earlier steps; dashed, a construction helper |
| <span style="color:#A3A3A3">■ light grey</span> `#DADADA` | context, solid, with `#B8B8B8` edges |

Member families use `FAMILY_COLORS`: <span style="color:#E8478B">outer ribs</span>, <span style="color:#F2CC0C">inner ribs</span>, <span style="color:#7C7C7C">inner beams</span>, <span style="color:#A8A8A8">wedges</span>, <span style="color:#D9B860">t-sections</span>, <span style="color:#6FA9D8">beds</span>, oculus ring <span style="color:#E06CA0">light pink</span>, column <span style="color:#6E6E6E">dark grey</span>, connectors <span style="color:#2196EA">BRG blue</span>. Black plates are code names; quarter 0 stands for all four, since every quarter runs the same code at its own corner.

## Data structures

FloorGuide holds the corners, the parameters as its own fields and the geometry they make, every member as two face loops; Floor builds the elements, their contact interactions, the connectors and the screws from it.

```mermaid
classDiagram
    direction TB
    class FloorGuide {
        <<WoodSession>>
        corners[4], size_ parameters
        quarter_polygon(q), quarter_column_polygon(q)
        construction_planes(q), construction_quads(q)
        boundary_parabolas(q), central_panel(q)
        outer_ribs(q), inner_ribs(q), inner_beams(q)
        wedges(q), tsections(q), beds(q)
        oculus(), column_cutters(q)
    }
    class Floor {
        <<WoodSession>>
        guide
        quarters, ring, columns
        add_members()
        add_contacts() by contact search
        add_connectors()
        add_screws()
        rib_beam_screws, beam_mitre_screws, rib_corner_screws
    }
    FloorGuide --> Floor : loops become elements
```

Each member has one name everywhere, from `MemberRef::name()`.

| Family | Count per quarter | Element | Name |
|---|---|---|---|
| `outer_ribs` | 2 | `BeamVariable` | `outer_ribs_<i>_<q>` |
| `inner_ribs` | 2 | `BeamVariable` | `inner_ribs_<i>_<q>` |
| `inner_beams` | 3: seam, oculus edge, seam | `BeamVariable` | `inner_beams_<i>_<q>` |
| `wedges` | 3 at the column head | `Plate` | `wedges_<i>_<q>` |
| `tsections` | 6 beside the ribs | `Plate` | `tsections_<i>_<q>` |
| `beds` | 3 rows | `Plate` | `beds_<row>_<i>_<q>` |
| ring | 1 beam and 1 bottom wedge | `BeamVariable`, `Plate` | `oculus_<q>`, `oculus_<q + 4>` |
| column | 1 support, 1 column | `Support`, `Column` | `support_<q>`, `column_<q>` |
| central plate | 1 for the floor | `Plate` | `oculus_8` |

## Examples

| Example | What it builds |
|---|---|
| [templates_floor_1_floorguide](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_1_floorguide.cpp) | the guide: quarter 0's plan and every member's quads, faces and parabolas under its name, chapters 1 to 5 |
| [templates_floor_2_column_model](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_2_column_model.cpp) | one column on its support, carved by its six head cutters |
| [templates_floor_3_columns_model](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_3_columns_model.cpp) | the four columns at the bay corners |
| [templates_floor_4_quarters](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_4_quarters.cpp) | the four quarters in place |
| [templates_floor_5_oculus](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_5_oculus.cpp) | the oculus ring, its bottom wedges and plate |
| [templates_floor_6_contacts_floor](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_6_contacts_floor.cpp) | the quarters, the ring and the eight wedge connectors |
| [templates_floor_7_contacts_cantilevers](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_7_contacts_cantilevers.cpp) | the whole square bay with columns, every connector and screw, BReps with exact bores |
| [templates_floor_8_rectangle](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_8_rectangle.cpp) | the floor on a 6000 x 4800 bay with every connector and screw, BReps |
