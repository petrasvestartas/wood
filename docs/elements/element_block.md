# Block {#elements_block}

[TOC]

A block: a solid lofted between a bottom and a top loop, holes paired between them, or any closed mesh.

## Constructors

```cpp
explicit Block(const std::vector<Polyline>& loops, const std::string& name = "block")
explicit Block(const Mesh& mesh, const std::string& name = "block")
```

## Parameters

| Parameter | Default | What it changes in 3D |
| --- | --- | --- |
| `loops` | | the bottom loop, the top loop, then pairs of hole loops; a wider top makes a tapered block |
| `mesh` | | any closed mesh taken as the solid |

## Lofted between two loops

![Lofted between two loops](elements/element_block.png)

A 200 square and a 300 square 250 above it, wider at the top.

\include{lineno} elements/element_block.cpp
