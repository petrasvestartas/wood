#pragma once

#include "wood_element_joint_beam.h"

namespace wood_session {

/// One solid of a connector, its plate, wedge or key, with the connector's cuts into it and the exact bores of the dowels passing through it; a child of the connector in the tree, carrying no relation and no dowel of its own.
class ConnectorPart : public JointBeam {
public:
    size_t index = 0; // Which part of the connector this is, so a cut stored on the connector later reaches it; not written, the parts are cut once.

    ConnectorPart();

    /// Part index of the connector, with the connector's cuts and the bores of its dowels through it; visible, named name.
    ConnectorPart(const JointBeam& connector, size_t index, const std::string& name);

    std::string element_type_name() const override {
        return "ConnectorPart";
    }
    std::shared_ptr<session_cpp::Element> clone() const override {
        return std::make_shared<ConnectorPart>(*this);
    }
};

}
