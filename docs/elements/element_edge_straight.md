# Edge Straight {#elements_edge_straight}

[TOC]

A straight timber member along an axis: a beam, or a column when it stands. Its section is a square, a profile, a world loop or a circle; holes, cuts and joints are interactions, not parts of the class.

## Constructors

```cpp
EdgeStraightElement(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions = {}, const std::string& name = "beam");

static std::shared_ptr<EdgeStraightElement> square(const Polyline& axis, double radius, const std::string& name = "beam");
static std::shared_ptr<EdgeStraightElement> corner_square(const Line& axis, const Plane& corner, double side, const std::string& name = "column");
static std::shared_ptr<EdgeStraightElement> from_section(const Line& axis, const Polyline& section, const std::string& name = "column");
static std::shared_ptr<EdgeStraightElement> round(const Line& axis, double radius, double chord_tolerance = 0.02, const std::string& name = "column");

// Standard pairs for examples and tests
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::array<std::shared_ptr<EdgeStraightElement>, 2> pair_end_to_end(double angle_degrees = 0.0, double radius = 75.0, double length = 1200.0, double extension = 150.0);
```

## A profile along an axis

A 120 × 200 mm rectangular profile swept along a straight axis.

![](elements/element_edge_straight.png)

\include{lineno} elements/element_edge_straight.cpp

## Sections

Every profile, each swept along a short edge: rectangle, round, W, hollow, slab band and T. A profile is loops in the section frame, loop 0 the outline, then holes.

![](elements/element_edge_straight_sections.png)

\include{lineno} elements/element_edge_straight_sections.cpp

## A column

A 200 × 300 mm section swept up a 3500 mm vertical axis. A column is an edge named for what it does; nothing in the class says column.

![](elements/element_edge_straight_column.png)

\include{lineno} elements/element_edge_straight_column.cpp

## A column with its head and seat

A floor corner column: glued head blocks, a steel support and pocket cuts, each an interaction on the same edge.

![](elements/element_edge_straight_column_session.png)

\include{lineno} elements/element_edge_straight_column_session.cpp

## A session of several elements

Two columns and the beam on them, each added to its group.

| assembly | still | |
|---|---|---|
| ![](joint_library/assembly_element_edge_straight_session.gif) | ![](elements/element_edge_straight_session.png) |  |

\include{lineno} elements/element_edge_straight_session.cpp

## Holes

A hole is a drill, a joint; its interaction bores the edge.

![](elements/element_edge_straight_holes.png)

\include{lineno} elements/element_edge_straight_holes.cpp

## Cut by a plane

A cutting plane is a joint; the edge keeps the side its normal points to.

![](elements/element_edge_straight_cut_plane.png)

\include{lineno} elements/element_edge_straight_cut_plane.cpp

## Assign joints

A joint computes itself from its two edges and goes on each with `add_interaction`.

| assembly | joint | tile |
|---|---|---|
| ![](joint_library/assembly_element_joint_beam_cross_half_lap.gif) | ![](joint_library/assembly_element_joint_beam_cross_half_lap.png) |  |

\include{lineno} elements/element_joint_beam_cross_half_lap.cpp

## Typical Dimensions

Sections in mm, lengths in m; a cell of the matrices is the area of the section b x h in cm², empty where the size is not stock.

### Products

| Product | Sections b x h | Section area cm² | Length | Source |
| --- | --- | --- | --- | --- |
| Solid timber, C24 | 60 x 80 to 240 x 300 | 48 to 720 | to 8, at times 12 | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 85 |
| Finger-jointed (KVH), C24 | 60 x 100 to 140 x 240 | 60 to 336 | 13 | [holzbau handbuch R4 T2 F1](https://www.habisreutinger.de/media/import/CSimages/1432/143225/R4_T2_F1_KVH_Duo_Triobalken.pdf) |
| Two or three laminations (Duo, Trio), C24 | 60 x 100 to 240 x 240, at most 240 x 280 | 60 to 672 | 13 | [holzbau handbuch R4 T2 F1](https://www.habisreutinger.de/media/import/CSimages/1432/143225/R4_T2_F1_KVH_Duo_Triobalken.pdf) |
| Glulam from stock, GL24 | 60 x 120 to 200 x 400 | 72 to 800 | 12 to 24 | [holzbau handbuch R4 T2 F2](https://informationsdienst-holz.de/fileadmin/Publikationen/2_Holzbau_Handbuch/R04_T02_F02_Herstellung-und-Eigenschaften-von-geklebten-Vollholzprodukten_2024-10.pdf) |
| Glulam to order | b 60 to 240 by 20, h 120 to 2000 by 40, h / b to 10 | to 4800 | to 65 | [holzbau handbuch R4 T2 F2](https://informationsdienst-holz.de/fileadmin/Publikationen/2_Holzbau_Handbuch/R04_T02_F02_Herstellung-und-Eigenschaften-von-geklebten-Vollholzprodukten_2024-10.pdf) |
| Glulam, the largest | 300 x 2500; Derix 280 x 2600; Hasslacher 280 x 4000 | 7500, 7280, 11200 | 65, 60, 42 | [holzbau handbuch R4 T2 F2](https://informationsdienst-holz.de/fileadmin/Publikationen/2_Holzbau_Handbuch/R04_T02_F02_Herstellung-und-Eigenschaften-von-geklebten-Vollholzprodukten_2024-10.pdf), [Derix](https://derix.de/produkte/brettschichtholz/), [Hasslacher](https://www.hasslacher.com/data/_dateimanager/broschuere/HNT-Brettschichtholz-DE.pdf) |
| Beech rod-laminated (SSH) | b 80 to 280, h 80 to 1240 | 64 to 3472 | 13.5 | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 89 |
| Beech veneer beam, GL75 | b 100 to 300, h 80 to 1360 by 40 | 80 to 4080 | 12 to 18 | [BauBuche](https://www.pollmeier.com/wp-content/uploads/2023/04/2025-11_DE_BauBuche_02_Produktuebersicht_web.pdf) |
| Spruce veneer beam (Kerto S) | 45, 51, 57, 63 x 200 to 600 | 90 to 378 | 2 to 25 | [Kerto S](https://www.metsagroup.com/metsawood/products-and-services/products/kerto-lvl/s-beam/) |
| I-joist (STEICO SJ) | flanges 45, 60, 90 x 39, h 160 to 500 | | 10 to 13 | [STEICO](https://www.steico.com/fileadmin/user_upload/Public_Docs/EUR_en/EUR_en_Technical_Data_Sheets/4._Product_information_construction_products/STEICO_product-sheet_building-system_EUR_en_i.pdf) |
| Round glulam column | diameter 120 to 240 | 113 to 452 | | [Mayr-Melnhof](https://www.mm-holz.com/wp-content/uploads/2026/04/101a_MMmasterline_Sonderbauteile_EN_2024_v1_digital.pdf) |
| Square columns from stock | 100, 120, 140, 160, 180, 200, 240 | 100, 144, 196, 256, 324, 400, 576 | 13 | [holzbau handbuch R4 T2 F1](https://www.habisreutinger.de/media/import/CSimages/1432/143225/R4_T2_F1_KVH_Duo_Triobalken.pdf) |
| Beech veneer column, GL75 | 200 x 200 to 600 x 600 | 400 to 3600 | 2 to 18 | [BauBuche](https://www.pollmeier.com/wp-content/uploads/2023/04/BauBuche-16Stuetzen-04-23-DE-150dpi.pdf) |

### Finger-jointed, preferred sections

| cm² | h 100 | h 120 | h 140 | h 160 | h 180 | h 200 | h 220 | h 240 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **b 60** | 60 | 72 | 84 | 96 | 108 | 120 | 132 | 144 |
| **b 80** |  | 96 |  | 128 | 144 | 160 | 176 | 192 |
| **b 100** | 100 |  |  | 160 |  | 200 |  | 240 |
| **b 120** |  | 144 |  | 192 |  | 240 |  | 288 |
| **b 140** |  |  | 196 |  |  |  |  |  |

Source: [holzbau handbuch R4 T2 F1](https://www.habisreutinger.de/media/import/CSimages/1432/143225/R4_T2_F1_KVH_Duo_Triobalken.pdf), table 4.1.

### Two or three laminations, preferred sections

| cm² | h 100 | h 120 | h 140 | h 160 | h 180 | h 200 | h 220 | h 240 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **b 60** | 60 | 72 | 84 | 96 | 108 | 120 | 132 | 144 |
| **b 80** | 80 | 96 | 112 | 128 | 144 | 160 | 176 | 192 |
| **b 100** | 100 | 120 | 140 | 160 | 180 | 200 | 220 | 240 |
| **b 120** |  | 144 |  | 192 | 216 | 240 | 264 | 288 |
| **b 140** |  |  | 196 | 224 | 252 | 280 | 308 | 336 |
| **b 160** |  |  |  | 256 |  | 320 | 352 | 384 |
| **b 180** |  |  |  |  | 324 | 360 | 396 | 432 |
| **b 200** |  |  |  |  |  | 400 | 440 | 480 |
| **b 240** |  |  |  |  |  |  |  | 576 |

Source: [holzbau handbuch R4 T2 F1](https://www.habisreutinger.de/media/import/CSimages/1432/143225/R4_T2_F1_KVH_Duo_Triobalken.pdf), table 4.2.

### Glulam, stock sections

| cm² | h 120 | h 140 | h 160 | h 200 | h 240 | h 280 | h 320 | h 360 | h 400 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **b 60** | 72 |  | 96 |  |  |  |  |  |  |
| **b 80** | 96 |  | 128 | 160 | 192 |  |  |  |  |
| **b 100** | 120 |  | 160 | 200 | 240 |  |  |  |  |
| **b 120** | 144 |  | 192 | 240 | 288 | 336 | 384 |  |  |
| **b 140** |  | 196 | 224 | 280 | 336 | 392 | 448 | 504 |  |
| **b 160** |  |  | 256 | 320 | 384 | 448 | 512 | 576 | 640 |
| **b 180** |  |  |  | 360 | 432 | 504 | 576 | 648 | 720 |
| **b 200** |  |  |  | 400 | 480 | 560 | 640 | 720 | 800 |

40 laminations, GL24c. Source: [holzbau handbuch R4 T2 F2](https://informationsdienst-holz.de/fileadmin/Publikationen/2_Holzbau_Handbuch/R04_T02_F02_Herstellung-und-Eigenschaften-von-geklebten-Vollholzprodukten_2024-10.pdf), table 3.4.

### Straight beams, depth for a span

| Beam | Span m | Depth | Width |
| --- | --- | --- | --- |
| On two supports | to 24 or 30 | L / 14 to L / 17 | at least h / 10 |
| Continuous | to 25 | L / 14 to L / 20 | at least h / 10 |
| Joist of solid timber | to 5 | L / 20 | h / 2 to 2 h / 3 |

Sources: [Glulam Handbook 1](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/glulamhandbook1-240508.pdf), table 6; [Glulam Handbook 2](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/glulamhandbook2-240508.pdf), table 3.1; [Hochschule Bochum](https://www.hochschule-bochum.de/fileadmin/public/Die-BO_Fachbereiche/fb_a/Maas/Konstruktives_Projekt/Skriptum_zur_Vordimensionierung.pdf). For a first size only: the structure is designed by an engineer.
