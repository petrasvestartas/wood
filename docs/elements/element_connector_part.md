# ConnectorPart {#elements_connector_part}

[TOC]

One solid of a connector (plate, wedge or key) with its cuts and pin bores; a child of the connector.

## Constructors

```cpp
ConnectorPart(const JointBeam& connector, size_t index, const std::string& name)
std::vector<std::shared_ptr<Joint>> JointBeam::children() const
```

`JointBeam::children()` gives a connector's parts and pins, nested under it when added (@ref elements_joint_beam).

## The parts of a wedge

![The parts of a wedge](elements/element_connector_part.png)

A wedge connector with its two beams hidden: the wedge part with the bores of its three pins, and the pins.

\include{lineno} elements/element_connector_part.cpp
