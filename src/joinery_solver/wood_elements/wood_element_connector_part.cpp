#include "pch.h"
#include "wood_element_connector_part.h"

namespace wood_session {

using namespace session_cpp;

ConnectorPart::ConnectorPart() {
    name = "part";
    is_visible = true;
}

ConnectorPart::ConnectorPart(const JointBeam& connector, size_t index, const std::string& name) : index(index) {

    this->name = name;
    is_visible = true;
    parts = {connector.parts.at(index)};
    solid_cuts = connector.part_cuts(index);
    line_radius = connector.line_radius;
    chord_tolerance = connector.chord_tolerance;
}

}
