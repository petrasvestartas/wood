# Face Planar {#elements_face_planar}

[TOC]

A flat timber plate bounded by a bottom and a top outline, one side face per edge.

## Constructors

```cpp
FacePlanarElement(const Polyline& bottom, const Polyline& top, const std::string& name = "plate");

static std::shared_ptr<FacePlanarElement> from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, double thickness, const std::string& name = "plate");
static std::shared_ptr<FacePlanarElement> from_line(const Line& base, const Vector& up, double height, double thickness, const std::string& name = "plate");
static std::vector<std::shared_ptr<FacePlanarElement>> row_between(const std::array<Polyline, 2>& bottom, const std::array<Polyline, 2>& top, const std::string& name = "plate");

// Standard pairs for examples and tests
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_in_plane(double angle_degrees = 180.0, double length = 150.0, double width = 400.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_folded(double angle_degrees = 90.0, double length = 150.0, double width = 400.0, double height = 150.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_rotated(double twist_degrees = 10.0, double length = 150.0, double width = 400.0, double thickness = 100.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_top_side(double angle_degrees = 90.0, double base_size = 300.0, double upright_size = 250.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_crossing(double angle_degrees = 90.0, double length = 250.0, double height = 200.0, double thickness = 40.0);
static std::array<std::shared_ptr<FacePlanarElement>, 2> pair_stacked(double twist_degrees = 0.0, double length = 400.0, double width = 300.0, double thickness = 40.0);
```

## From two polylines

A four-sided plate lofted between a bottom polygon and the same polygon lifted 40 mm.

![](elements/element_face_planar.png)

\include{lineno} elements/element_face_planar.cpp

## A session of several elements

A row of bed plates made between a near and a far parabolic rail.

| assembly | still | |
|---|---|---|
| ![](joint_library/assembly_element_face_planar_session.gif) | ![](elements/element_face_planar_session.png) |  |

\include{lineno} elements/element_face_planar_session.cpp

## Holes

Four through-holes, each a drill, a joint, its interaction bores the plate.

![](elements/element_face_planar_holes.png)

\include{lineno} elements/element_face_planar_holes.cpp

## Cut by a plane

A cutting plane is a joint; the plate keeps the side its normal points to.

![](elements/element_face_planar_cut_plane.png)

\include{lineno} elements/element_face_planar_cut_plane.cpp

## Assign joints

Eight box plates whose joint types and insertion directions are set by points and lines.

![](elements/element_face_planar_joints.png)

\include{lineno} elements/element_face_planar_joints.cpp

## Typical Dimensions

Thickness in mm, formats in m, areas in m².

### Panels

| Panel | Thickness | Width x length | Area | Source |
| --- | --- | --- | --- | --- |
| Cross-laminated, common | 60 to 340 | 3.0 (3.4) x 14 | 42 (47.6) | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 92 |
| Cross-laminated, KLH | 60 to 320 | 3.50 x 16.50 | 57.8 | [KLH](https://www.klh.at/en/panel-types-surfaces-technical-details/) |
| Cross-laminated, binderholz | 51 to 315 | 3.50 x 22 | 77 | [ETA-06/0009](https://www.dibt.de/pdf_storage/2022/ETA-06!0009%288.03.04-17!21%29e.pdf) |
| Cross-laminated, Derix | to 400 | 3.50 x 17.80 | 62.3 | [Derix](https://derix.de/media/freie-dokumente/DERIX_X_Lam_Brosch_DE_2025_01_RZ_Web.pdf) |
| Cross-laminated, Schilliger | 27 to 400 | 3.40 x 14 | 47.6 | [Schilliger](https://www.schilliger.ch/wp-content/uploads/2025/04/SchilligerHolz-Themenfolder-CLT-2024-A4-DE-V1klein.pdf) |
| Veneer lumber, spruce (Kerto Q) | 21, 24, 27, 30, 33, 39, 45, 51, 57, 63, 69, 75 | 1.83 x 25, 2.50 x 20 | 45.8, 50 | [Kerto Q](https://www.metsagroup.com/contentassets/f8ff384a208d4786959efa4bca9fab2c/kerto-lvl-q-panel-product-datasheet-en.pdf) |
| Veneer lumber, beech | 40, 60 on request | 1.82 x 18 | 32.8 | [BauBuche](https://www.pollmeier.com/wp-content/uploads/2023/04/2025-11_DE_BauBuche_02_Produktuebersicht_web.pdf) |
| Solid wood, three or five layers | 13, 15, 16, 19, 21, 22, 26, 27, 32, 35, 40, 42, 50, 52, 55, 60 | 1.00, 1.25, 2.05, 2.50 x 5, 6 | 5 to 15 | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 94 |
| Solid wood, one layer | 14, 18, 21, 22, 24, 27, 32, 34, 42, 50, 52 | 1.25, 2.05 x 5 | 6.3, 10.3 | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 94 |
| Plywood | 4, 5, 6, 8, 9, 12, 15, 18, 21, 24, 27, 30 | 1.25 x 2.50 | 3.1 | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 95 |
| Oriented strand board | 12, 15, 18, 22, 25 | 1.25, 2.50 x 2.50, 5.00 | 3.1 to 12.5 | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 95 |
| Particleboard | 12, 16, 19, 22, 25, 28, 38 | 2.10 x 2.80, 5.60 | 5.9, 11.8 | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 96 |
| Fibreboard (MDF) | 3, 4, 6, 8, 10, 12, 16, 19, 22, 24, 25, 30, 40 | 2.07 x 2.80, 4.10 | 5.8, 8.5 | Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 96 |

### Cross-laminated floor panels

| Thickness | Layers | Laminations along (and **across**) | m³ per m² |
| --- | --- | --- | --- |
| 60 | 3 | 20 **20** 20 | 0.06 |
| 80 | 3 | 30 **20** 30 | 0.08 |
| 90 | 3 | 30 **30** 30 | 0.09 |
| 100 | 3 | 30 **40** 30 (KLH 40 **20** 40) | 0.10 |
| 120 | 3 | 40 **40** 40 | 0.12 |
| 100 | 5 | 20 **20** 20 **20** 20 | 0.10 |
| 120 | 5 | 30 **20** 20 **20** 30 | 0.12 |
| 140 | 5 | 40 **20** 20 **20** 40 | 0.14 |
| 160 | 5 | 40 **20** 40 **20** 40 | 0.16 |
| 180 | 5 | 40 **30** 40 **30** 40 | 0.18 |
| 200 | 5 | 40 **40** 40 **40** 40 | 0.20 |
| 200 | 7 | 20 **40** 20 **40** 20 **40** 20 | 0.20 |
| 220 | 7 | 40 **20** 40 **20** 40 **20** 40 (KLH 30 **40** 30 **20** 30 **40** 30) | 0.22 |
| 240 | 7 | 30 **40** 30 **40** 30 **40** 30 | 0.24 |

Sources: [KLH](https://www.klh.at/en/panel-types-surfaces-technical-details/), [Theurl](https://www.theurl-holz.at/wp-content/uploads/2025/07/standardaufbauten-cltplus-2.pdf), [Schilliger](https://www.schilliger.ch/wp-content/uploads/2025/04/SchilligerHolz-Themenfolder-CLT-2024-A4-DE-V1klein.pdf), [Hasslacher](https://www.hasslacher.com/data/_dateimanager/broschuere/HNT-Brettsperrholz-DE.pdf); a wall has its outer layers upright. A piece wider than 3.00 m needs a transport permit.
