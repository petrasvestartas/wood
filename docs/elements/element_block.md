# Block {#elements_block}

[TOC]

A block: a solid lofted between a bottom and a top loop, holes paired between them, or any closed mesh.

## Constructors

```cpp
explicit Block(const std::vector<Polyline>& loops, const std::string& name = "block")
explicit Block(const Mesh& mesh, const std::string& name = "block")
```

## Lofted between two loops

![Lofted between two loops](elements/element_block.png)

A 300 square and a 200 square 250 above it.

\include{lineno} elements/element_block.cpp
