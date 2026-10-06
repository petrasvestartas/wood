#include "pch.h"
#include "wood_element.h"

namespace wood_session {

/// The mesh in the frame to_local maps into, every coordinate on a 1e-6 mm grid there.
static Mesh snapped(const Mesh& mesh, const Xform& to_local) {

    std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.transformed(to_local).to_vertices_and_faces();

    for (Point& point : data.first)
        point = Point(std::round(point[0] * 1e6) / 1e6, std::round(point[1] * 1e6) / 1e6, std::round(point[2] * 1e6) / 1e6);

    return Mesh::from_vertices_and_faces(data.first, data.second);
}

/// The features of one kind: the adds, or every other one.
static std::vector<InteractionFeatureSolid> features_of(const std::vector<InteractionFeatureSolid>& features, bool adds) {

    std::vector<InteractionFeatureSolid> found;

    for (const InteractionFeatureSolid& feature : features)
        if ((feature.operation == SolidOperation::add) == adds)
            found.push_back(feature);

    return found;
}

Mesh WoodElement::stock_mesh() const {

    std::vector<InteractionFeatureSolid> adds = features_of(solid_features, true);
    const Plane plane = frame();
    const Xform to_world = Xform::frame_to_world(plane.origin(), plane.x_axis(), plane.y_axis(), plane.z_axis());
    const std::optional<Xform> to_local = to_world.inverse();

    if (adds.empty() || !to_local)
        return element_geometry_mesh();

    for (InteractionFeatureSolid& feature : adds)
        feature.mesh = snapped(feature.mesh, *to_local);

    return apply_solid_features(snapped(element_geometry_mesh(), *to_local), adds).transformed(to_world);
}

const Mesh& WoodElement::model_geometry_mesh() const {

    if (!_model_mesh_cache)
        _model_mesh_cache = apply_solid_features(trimmed_mesh(), features_of(solid_features, false));

    return *_model_mesh_cache;
}

const BRep& WoodElement::model_geometry_brep() const {

    if (!_model_brep_cache)
        _model_brep_cache = solid_features.empty() ? trimmed_brep() : solid_features_brep(trimmed_mesh(), features_of(solid_features, false));

    return *_model_brep_cache;
}

void WoodElement::invalidate_geometry() {

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    Element::invalidate_geometry();
    reset();
}

Plane WoodElement::frame() const {
    return Plane::xy_plane();
}

Mesh WoodElement::trimmed_mesh() const {
    return stock_mesh();
}

BRep WoodElement::trimmed_brep() const {
    return element_geometry_brep();
}

}
