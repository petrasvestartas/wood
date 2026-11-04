#include "pch.h"
#include "wood_element_joint_connector_part.h"

namespace wood_session {

using namespace session_cpp;

JointConnectorPartElement::JointConnectorPartElement() {
    name = "part";
    is_visible = true;
}

JointConnectorPartElement::JointConnectorPartElement(const JointBeamElement& connector, size_t index, const std::string& name) {

    this->name = name;
    is_visible = true;
    parts = {connector.parts.at(index)};
    solid_features = connector.part_features(index);
    line_radius = connector.line_radius;
    chord_tolerance = connector.chord_tolerance;
}

const Mesh& JointConnectorPartElement::element_geometry_mesh() const {

    if (!mesh_)
        mesh_ = apply_solid_features(part_mesh(0), solid_features);

    return *mesh_;
}

const BRep& JointConnectorPartElement::element_geometry_brep() const {

    if (!brep_)
        brep_ = part_brep(0);

    return *brep_;
}

}
