# Edge Curved {#elements_edge_curved}

[TOC]

A single-curved or double-curved glulam member swept along a `NurbsCurve` centreline as a stack of lamellae.

## Constructors

```cpp
EdgeCurvedElement(const NurbsCurve& centreline, double width, double height, const std::string& name = "beam_curved");

static std::shared_ptr<EdgeCurvedElement> single_curved(const NurbsCurve& centreline, double width, double height, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
static std::shared_ptr<EdgeCurvedElement> double_curved(const NurbsCurve& centreline, double width, double height, const std::vector<Vector>& directions = {}, const std::vector<double>& stations = {}, int num_width = 2, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
static std::shared_ptr<EdgeCurvedElement> from_surface(const NurbsCurve& centreline, const NurbsSurface& surface, double width, double height, int num_width = 2, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
```

## Single-curved and double-curved glulam

A planar arch of 6 lamellae trimmed by plumb cuts, beside a spatial S-curved member with a twisting frame.

![](elements/element_edge_curved.png)

\include{lineno} elements/element_edge_curved.cpp

## A session of several elements

Three arches side by side.

![](elements/element_edge_curved_session.png)

\include{lineno} elements/element_edge_curved_session.cpp

## Holes

A hole is a drill, a joint; its interaction bores the arch at its crown.

![](elements/element_edge_curved_holes.png)

\include{lineno} elements/element_edge_curved_holes.cpp

## Cut by a plane

A cutting plane is a joint; the arch keeps the side its normal points to.

![](elements/element_edge_curved_cut_plane.png)

\include{lineno} elements/element_edge_curved_cut_plane.cpp

## Assign joints

Two half arches cut flat at the crown, pins centred across their contact.

| assembly | still | |
|---|---|---|
| ![](joint_library/assembly_element_edge_curved_joints.gif) | ![](elements/element_edge_curved_joints.png) |  |

\include{lineno} elements/element_edge_curved_joints.cpp

## Typical Dimensions

Sections in mm, lengths and radii in m.

| Glulam, curved | Section b x h | Section area cm² | Length | Radius | Source |
| --- | --- | --- | --- | --- | --- |
| Mayr-Melnhof | b 60 to 280, h 100 to 2200 | 60 to 6160 | to 56.3, rise to 4.0 | from 1.0 | [MM special components](https://www.mm-holz.com/wp-content/uploads/2026/04/101a_MMmasterline_Sonderbauteile_EN_2024_v1_digital.pdf) |
| Hasslacher | b 80 to 280, h to 4000 | to 11200 | to 42 | | [Hasslacher](https://www.hasslacher.com/data/_dateimanager/broschuere/HNT-Brettschichtholz-DE.pdf) |
| Usual laminations | 30 to 40, thinner under a radius of 7 | | | from 7 | [holzbau handbuch R4 T2 F2](https://informationsdienst-holz.de/fileadmin/Publikationen/2_Holzbau_Handbuch/R04_T02_F02_Herstellung-und-Eigenschaften-von-geklebten-Vollholzprodukten_2024-10.pdf), [Derix](https://derix.de/media/freie-dokumente/DERIX_BSH_Brosch_DE_2025_RZ_Web.pdf) |
| Curved at all | rise over L / 100, less is a straight beam with camber | | | | [holzbau handbuch R4 T2 F2](https://informationsdienst-holz.de/fileadmin/Publikationen/2_Holzbau_Handbuch/R04_T02_F02_Herstellung-und-Eigenschaften-von-geklebten-Vollholzprodukten_2024-10.pdf) |
| Transport | rise of a piece to 3.5 | | | | [Derix](https://derix.de/media/freie-dokumente/DERIX_BSH_Brosch_DE_2025_RZ_Web.pdf) |
| Veneer boards bent along the grain | any t | | | from 450 t | [Kerto](https://www.metsagroup.com/globalassets/metsa-wood/attachments/kerto-lvl-manual/en/kerto-manual-lvl-curved-structures.pdf) |

### Lamination and radius

EN 14080: a lamination is at most t = R / 250 · (1 + f / 150), and 45 (35 outdoors), f the finger joints' bending strength in N/mm². Bending costs no strength from R = 240 t; in practice R is never under about 170 t.

| t | R at f = 30 | R at f = 40 | R = 240 t, full strength | published |
| --- | --- | --- | --- | --- |
| 6 | 1.25 | 1.18 | 1.44 |  |
| 10 | 2.08 | 1.97 | 2.40 | 2 (UK, 10 laminations) |
| 15 | 3.12 | 2.96 | 3.60 |  |
| 20 | 4.17 | 3.95 | 4.80 |  |
| 25 | 5.21 | 4.93 | 6.00 |  |
| 30 | 6.25 | 5.92 | 7.20 |  |
| 33 | 6.88 | 6.51 | 7.92 | 7; 8 in haunches (Nordic) |
| 35 | 7.29 | 6.91 | 8.40 |  |
| 40 | 8.33 | 7.89 | 9.60 |  |
| 45 | 9.38 | 8.88 | 10.80 | 9 (UK) |

The first three radii are computed from the rules. Sources: [holzbau handbuch R4 T2 F2](https://informationsdienst-holz.de/fileadmin/Publikationen/2_Holzbau_Handbuch/R04_T02_F02_Herstellung-und-Eigenschaften-von-geklebten-Vollholzprodukten_2024-10.pdf), table 3.3; [Glulam Handbook 2](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/glulamhandbook2-240508.pdf), table 7.2; [Buckland](https://cdn.asp.events/CLIENT_Media_10_B9D470DF_5056_B739_5443B4BBAA6F8ED4/sites/Grand-Designs-Live/media/libraries/exhibitor-news/96096-Buckland-Timber-Technical-Guide.pdf).

### Curved structures, depth for a span

| Structure | Span m | Depth |
| --- | --- | --- |
| Arch on three hinges | 20 to 100 | L / 50, rise over L / 7 |
| Arch on two hinges | 30 to 100 | L / 50 to L / 30, rise over L / 10 |
| Curved beam of one depth | to 20 | L / 17 at the top, b L / 140 to L / 120, R over 8 to 10 |
| Portal with curved haunches | 15 to 50 | (leg + rafter) / 15, R about 8 in 33 laminations |

Sources: [Glulam Handbook 1](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/glulamhandbook1-240508.pdf), table 6; [Glulam Handbook 2](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/glulamhandbook2-240508.pdf), tables 3.1 and 7.3, fig. 11.6. For a first size only.
