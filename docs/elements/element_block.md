# Block {#elements_block}

A block: a solid lofted between a bottom and a top loop, holes paired between them, or any closed mesh.

```cpp
explicit Block(const std::vector<Polyline>& loops, const std::string& name = "block")
explicit Block(const Mesh& mesh, const std::string& name = "block")
```

![Block](elements/element_block.png)

Left: lofted between a 300 square and a 200 square 250 above it. Right: from a closed mesh.

\include{lineno} elements/element_block.cpp
