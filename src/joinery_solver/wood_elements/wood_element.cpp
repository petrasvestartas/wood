#include "pch.h"
#include "wood_element.h"

namespace wood_session {

const Mesh& WoodElement::model_geometry_mesh() const {

    if (!_model_mesh_cache)
        _model_mesh_cache = apply_solid_cuts(trimmed_mesh(), solid_cuts);

    return *_model_mesh_cache;
}

const BRep& WoodElement::model_geometry_brep() const {

    if (!_model_brep_cache)
        _model_brep_cache = solid_cuts.empty() ? trimmed_brep() : solid_cuts_brep(trimmed_mesh(), solid_cuts);

    return *_model_brep_cache;
}

void WoodElement::invalidate_geometry() {

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    Element::invalidate_geometry();
    reset();
}

Mesh WoodElement::trimmed_mesh() const {
    return element_geometry_mesh();
}

BRep WoodElement::trimmed_brep() const {
    return element_geometry_brep();
}

}
