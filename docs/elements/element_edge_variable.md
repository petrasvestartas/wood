# Edge Variable {#elements_edge_variable}

[TOC]

A member whose section changes along its axis, lofted through closed station sections.

## Constructors

```cpp
EdgeVariableElement(const Line& axis, const std::vector<Polyline>& sections, const std::string& name = "edge_variable");

static std::shared_ptr<EdgeVariableElement> between(const Polyline& first, const Polyline& last, const std::string& name = "edge_variable");
static std::shared_ptr<EdgeVariableElement> box(const Point& start, const Vector& direction, double length, double width, double height, const std::string& name = "beam");

// Standard pairs for examples and tests
static std::array<std::shared_ptr<EdgeVariableElement>, 2> pair_side_by_side(double length = 1600.0, double width = 200.0, double height = 300.0);
static std::array<std::shared_ptr<EdgeVariableElement>, 2> pair_end_to_end(double length = 1000.0, double width = 200.0, double height = 300.0);
static std::array<std::shared_ptr<EdgeVariableElement>, 2> pair_tee(double length = 600.0, double width = 120.0, double joist_width = 160.0, double height = 300.0);
```

## Parabolic rib

Seven sections along a 3000 mm axis, the depth falling parabolically from 730 mm to 300 mm.

![](elements/element_edge_variable.png)

\include{lineno} elements/element_edge_variable.cpp

## A session of several elements

The joist and the beam of a tee.

| assembly | still | |
|---|---|---|
| ![](joint_library/assembly_element_edge_variable_session.gif) | ![](elements/element_edge_variable_session.png) |  |

\include{lineno} elements/element_edge_variable_session.cpp

## Holes

A hole is a drill, a joint; its interaction bores the rib.

![](elements/element_edge_variable_holes.png)

\include{lineno} elements/element_edge_variable_holes.cpp

## Cut by a plane

The rib trimmed at both ends by two cutting planes, each a joint.

![](elements/element_edge_variable_cut_plane.png)

\include{lineno} elements/element_edge_variable_cut_plane.cpp

## Assign joints

Two ribs side by side, a wedge let into both.

| assembly | joint | tile |
|---|---|---|
| ![](joint_library/assembly_element_joint_beam_wedge.gif) | ![](joint_library/assembly_element_joint_beam_wedge.png) |  |

\include{lineno} elements/element_joint_beam_wedge.cpp

## Typical Dimensions

Depths against the span L; h at the support, H at the ridge; the beams 5 to 7 m apart.

| Glulam beam | Slope | Span m | h | H |
| --- | --- | --- | --- | --- |
| Tapered to one or both sides | 1.4° to 10°, usually 3° to 6° | 10 to 30 | L / 30 to L / 20 | L / 16 to L / 14 |
| Pitched, its underside curved | 3° to 15° | 10 to 20 | L / 30 | L / 16 to L / 13, R over 10 m |
| Continuous with haunches | from 3° | to 25 | L / 18 in the field | L / 14 over the support |
| Fish belly | | 10 to 70 | L / 25 to L / 20 | L / 16 |
| Portal with finger-jointed haunches | from 14° | 15 to 25 | | (leg + rafter) / 13 |
| Portal on three hinges | 10° to 40° | 15 to 50 | L / 50 to L / 40 at the foot, L / 60 to L / 36 at the ridge | L / 22 to L / 15 at the haunch |

The width is 100 to 260 by 20, at least h / 10; a taper is cut into the compression side, into the tension side 2° to 3° at most. The sections are those of [straight glulam](element_edge_straight.md), to 300 x 2500.

Sources: [Glulam Handbook 1](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/glulamhandbook1-240508.pdf), table 6; [Glulam Handbook 2](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/glulamhandbook2-240508.pdf), table 3.1 and chapter 7; [Derix](https://derix.de/media/freie-dokumente/DERIX_BSH_Brosch_DE_2025_RZ_Web.pdf). For a first size only.
