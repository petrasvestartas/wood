#pragma once

#include "wood_element_joint_beam.h"

using namespace session_cpp;

namespace wood_session {

/// One solid of a connector.
class JointConnectorPartElement : public JointBeamElement {
public:
    JointConnectorPartElement();

    /// Part index of the connector, with the connector's cuts and the bores of its pins through it; visible, named name.
    JointConnectorPartElement(const JointBeamElement& connector, size_t index, const std::string& name);

    /// The part with its cuts applied.
    const Mesh& element_geometry_mesh() const override;

    /// The part with its cuts applied and its bores exact.
    const BRep& element_geometry_brep() const override;

    std::string element_type_name() const override {
        return "JointConnectorPartElement";
    }

    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointConnectorPartElement>(*this);
    }
};

}
