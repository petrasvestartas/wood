#include "pch.h"
#include "wood_element_cut_plane.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

CutPlane::CutPlane(const Plane& plane, double size, const std::string& name) : WoodElement(name), plane(plane), size(size) {
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<InteractionFeaturePlane> CutPlane::feature() const {
    return std::make_shared<InteractionFeaturePlane>(plane);
}

const Mesh& CutPlane::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        const Vector x = plane.x_axis() * (0.5 * size);
        const Vector y = plane.y_axis() * (0.5 * size);
        const Point o = plane.origin();

        _element_geometry_mesh = Mesh::from_vertices_and_faces(
            {o - x - y, o + x - y, o + x + y, o - x + y},
            {{0, 1, 2, 3}}
        );
    }

    return *_element_geometry_mesh;
}

const BRep& CutPlane::element_geometry_brep() const {

    if (!_element_geometry_brep)
        _element_geometry_brep = mesh_brep(element_geometry_mesh());

    return *_element_geometry_brep;
}

const Mesh& CutPlane::model_geometry_mesh() const {
    return element_geometry_mesh();
}

const BRep& CutPlane::model_geometry_brep() const {
    return element_geometry_brep();
}

void CutPlane::compute_geometry_mesh_impl() {
    set_geometry(model_geometry_mesh());
    set_features(session_features(*this));
}

void CutPlane::compute_geometry_brep_impl() {
    set_geometry(model_geometry_brep());
    set_features(session_features(*this));
}

} // namespace wood_session
