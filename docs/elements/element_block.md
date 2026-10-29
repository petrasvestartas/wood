# Block {#elements_block}

[TOC]

A solid block lofted between a bottom loop and a top loop (with optional paired inner hole loops) or constructed directly from a closed mesh.

## Constructors

```cpp
explicit Block(const std::vector<Polyline>& loops, const std::string& name = "block");
explicit Block(const Mesh& mesh, const std::string& name = "block");
static std::shared_ptr<Block> box(const Point& corner, double x_size, double y_size, double height, const std::string& name = "block");
static std::array<std::shared_ptr<Block>, 2> pair_stacked(double lower_size = 400.0, double upper_size = 300.0, double height = 200.0);
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `loops` | — | Bottom outer loop, top outer loop, followed by pairs of bottom/top hole loops |
| `mesh` | — | Arbitrary closed mesh used directly as the block solid |

## Lofted between two loops

A tapered frustum block lofted between a 200 × 200 mm bottom square and a 300 × 300 mm top square 250 mm above it.

![](elements/element_block.png)

```cpp
const Polyline bottom = Polyline::rectangle({50.0, 50.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 200.0, 200.0);
const Polyline top = Polyline::rectangle({0.0, 0.0, 250.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 300.0);
scene.add(std::make_shared<Block>(std::vector<Polyline>{bottom, top}, "block"));
```
