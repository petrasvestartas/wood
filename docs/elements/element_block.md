# Block {#elements_block}

[TOC]

A solid block lofted between a bottom loop and a top loop (with optional paired inner hole loops) or constructed directly from a closed mesh.

## Constructors

```cpp
explicit NodeBlockElement(const std::vector<Polyline>& loops, const std::string& name = "block");
explicit NodeBlockElement(const Mesh& mesh, const std::string& name = "block");
static std::shared_ptr<NodeBlockElement> box(const Point& corner, double x_size, double y_size, double height, const std::string& name = "block");
static std::array<std::shared_ptr<NodeBlockElement>, 2> pair_stacked(double lower_size = 400.0, double upper_size = 300.0, double height = 200.0);
```

## Lofted between two loops

A tapered frustum block lofted between a 200 × 200 mm bottom square and a 300 × 300 mm top square 250 mm above it.

![](elements/element_block.png)

\include{lineno} elements/element_block.cpp
