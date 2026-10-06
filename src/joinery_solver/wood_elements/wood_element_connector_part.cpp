#include "pch.h"
#include "wood_element_connector_part.h"

namespace wood_session {

using namespace session_cpp;

ConnectorPart::ConnectorPart() {
    name = "part";
    is_visible = true;
}

ConnectorPart::ConnectorPart(const JointBeam& connector, size_t index, const std::string& name) {

    this->name = name;
    is_visible = true;
    parts = {connector.parts.at(index)};
    solid_cuts = connector.part_cuts(index);
    line_radius = connector.line_radius;
    chord_tolerance = connector.chord_tolerance;
}

const Mesh& ConnectorPart::element_geometry_mesh() const {

    if (!mesh_)
        mesh_ = apply_solid_features(part_mesh(0), solid_cuts);

    return *mesh_;
}

const BRep& ConnectorPart::element_geometry_brep() const {

    if (!brep_)
        brep_ = part_brep(0);

    return *brep_;
}

}
