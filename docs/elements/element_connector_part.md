# ConnectorPart {#elements_connector_part}

[TOC]

One solid of a connector, its plate, wedge or key, with the connector's cuts and the exact bores of its pins; a child of the connector in the tree.

## Constructors

```cpp
ConnectorPart(const JointBeam& connector, size_t index, const std::string& name)
std::vector<std::shared_ptr<Joint>> JointBeam::children() const
```

A part is not made by hand: `JointBeam::children()` gives a connector's parts and pins, and the session nests them under the connector when it is added (@ref elements_joint_beam).

## The parts of a wedge

![The parts of a wedge](elements/element_connector_part.png)

A wedge connector with its two beams hidden: the wedge part with the bores of its three pins, and the pins.

\include{lineno} elements/element_connector_part.cpp
