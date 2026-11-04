#include "pch.h"
#include "wood_element_edge.h"

namespace wood_session {

std::vector<Plane> EdgeElement::compute_planes() const {
    return face_planes(model_geometry_mesh());
}

Mesh EdgeElement::trimmed_mesh() const {
    return cut_mesh(stock_mesh(), cuts);
}

BRep EdgeElement::trimmed_brep() const {
    return cut_brep(element_geometry_brep(), cuts);
}

}
