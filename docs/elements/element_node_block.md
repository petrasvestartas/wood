# Node Block {#elements_node_block}

[TOC]

A solid block lofted between a bottom loop and a top loop, with optional paired hole loops, or made from a closed mesh.

## Constructors

```cpp
explicit NodeBlockElement(const std::vector<Polyline>& loops, const std::string& name = "block");
explicit NodeBlockElement(const Mesh& mesh, const std::string& name = "block");

static std::shared_ptr<NodeBlockElement> box(const Point& corner, double x_size, double y_size, double height, const std::string& name = "block");

// Standard pair for examples and tests
static std::array<std::shared_ptr<NodeBlockElement>, 2> pair_stacked(double lower_size = 400.0, double upper_size = 300.0, double height = 200.0);
```

## Lofted between two loops

A frustum between a 200 × 200 mm square and a 300 × 300 mm square 250 mm above it.

![](elements/element_node_block.png)

\include{lineno} elements/element_node_block.cpp

## A session of several elements

A block on a block.

![](elements/element_node_block_session.png)

\include{lineno} elements/element_node_block_session.cpp

## Holes

A hole is a drill, a joint; its interaction bores the block.

![](elements/element_joint_drill.png)

\include{lineno} elements/element_joint_drill.cpp

## Cut by a plane

A cutting plane is a joint; the block keeps the side its normal points to.

![](elements/element_node_block_cut_plane.png)

\include{lineno} elements/element_node_block_cut_plane.cpp

## Assign joints

Two stacked blocks, pins centred across their contact.

![](elements/element_joint_beam_centred_pins.png)

\include{lineno} elements/element_joint_beam_centred_pins.cpp
