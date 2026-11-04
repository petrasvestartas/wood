# Column {#elements_column}

[TOC]

A column swept from a closed cross-section along a straight axis and modified by incoming `InteractionFeatureSolid` operations (glued head blocks, support seats, and rib pockets).

## Constructors

```cpp
Column(const Line& axis, const Polyline& section, const std::string& name = "column");
static std::shared_ptr<Column> square(const Line& axis, const Plane& corner, double side, const std::string& name = "column");
std::vector<std::shared_ptr<Block>> head_blocks(double head_side, double head_height) const;
```

## A rectangle along an axis

A 200 × 300 mm rectangular section swept along a 3500 mm vertical axis.

![](elements/element_column.png)

\include{lineno} elements/element_column.cpp

## Floor column assembly

A floor corner column assembly combining the 220 mm shaft, two glued head blocks (`SolidOperation::add`), a steel `Support`, and six pocket/bore cutters (`SolidOperation::subtract`).

![](elements/element_column_session.png)

\include{lineno} elements/element_column_session.cpp
