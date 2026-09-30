#include "wood_element_geometry.h"
#include "solid_cut.pb.h"

namespace wood_session {

using namespace session_cpp;

Mesh apply_solid_cuts(Mesh mesh, const std::vector<SolidCut>& cuts) {
    for (const SolidCut& cut : cuts) {
        if (std::optional<Mesh> result = compute_profile_cut(mesh, cut))
            mesh = std::move(*result);
        else
            mesh = solid_boolean(mesh, cut.mesh, cut.operation, cut.tolerance);
    }
    return mesh;
}

SolidCut SolidCut::transformed(const Xform& xform) const {
    SolidCut result = *this;
    result.mesh = mesh.transformed(xform);
    result.profile = transformed_list(profile, xform);
    result.extrusion = extrusion.transformed(xform);
    return result;
}
std::string SolidCut::pb_dumps() const {
    wood_proto::SolidCut proto;
    proto.set_joint_guid(joint_guid);
    proto.set_operation(static_cast<int>(operation));
    proto.set_tolerance(tolerance);
    if (!proto.mutable_mesh()->ParseFromString(mesh.pb_dumps()) || !proto.mutable_extrusion()->ParseFromString(extrusion.pb_dumps()))
        throw std::runtime_error("Cannot serialize solid cutter");
    for (const Polyline& ring : profile)
        if (!proto.add_profile()->ParseFromString(ring.pb_dumps()))
            throw std::runtime_error("Cannot serialize cutter profile");
    return proto.SerializeAsString();
}
SolidCut SolidCut::pb_loads(const std::string& data) {
    wood_proto::SolidCut proto;
    if (!proto.ParseFromString(data) || proto.operation() < 0 || proto.operation() > 2 || !std::isfinite(proto.tolerance()) || proto.tolerance() <= 0)
        throw std::runtime_error("Invalid solid cutter data");
    SolidCut cut;
    cut.joint_guid = proto.joint_guid();
    cut.operation = static_cast<SolidOperation>(proto.operation());
    cut.tolerance = proto.tolerance();
    cut.mesh = Mesh::pb_loads(proto.mesh().SerializeAsString());
    cut.extrusion = Vector::pb_loads(proto.extrusion().SerializeAsString());
    for (const session_proto::Polyline& ring : proto.profile())
        cut.profile.push_back(Polyline::pb_loads(ring.SerializeAsString()));
    return cut;
}
}
