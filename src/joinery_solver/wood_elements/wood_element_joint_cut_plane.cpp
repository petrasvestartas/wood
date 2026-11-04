#include "pch.h"
#include "wood_element_joint_cut_plane.h"
#include "wood_interaction_feature_plane.h"
#include "element_joint.pb.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════
JointCutPlaneElement::JointCutPlaneElement() {
    name = "cut_plane";
    is_visible = true;
}

JointCutPlaneElement::JointCutPlaneElement(const Plane& plane, double size, const std::string& name)
    : size(size) {
    this->name = name;
    is_visible = true;
    cuts = {plane};
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
std::shared_ptr<Interaction> JointCutPlaneElement::interaction(size_t) const {
    return std::make_shared<InteractionFeaturePlane>(plane());
}

const Mesh& JointCutPlaneElement::element_geometry_mesh() const {

    if (!mesh_) {
        const Vector x = plane().x_axis() * (0.5 * size);
        const Vector y = plane().y_axis() * (0.5 * size);
        const Point o = plane().origin();

        mesh_ = Mesh::from_vertices_and_faces(
            {o - x - y, o + x - y, o + x + y, o - x + y}, {{0, 1, 2, 3}}
        );
    }

    return *mesh_;
}

const BRep& JointCutPlaneElement::element_geometry_brep() const {

    if (!brep_)
        brep_ = mesh_brep(element_geometry_mesh());

    return *brep_;
}

const Mesh& JointCutPlaneElement::model_geometry_mesh() const {
    return element_geometry_mesh();
}

const BRep& JointCutPlaneElement::model_geometry_brep() const {
    return element_geometry_brep();
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════
void JointCutPlaneElement::write_proto(wood_proto::JointElement& proto) const {
    JointElement::write_proto(proto);
    proto.set_cut_size(size);
}

void JointCutPlaneElement::read_proto(const wood_proto::JointElement& proto) {
    JointElement::read_proto(proto);
    size = proto.cut_size();
}

}
