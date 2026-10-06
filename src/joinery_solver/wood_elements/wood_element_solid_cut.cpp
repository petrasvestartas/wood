#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "solid_cut.pb.h"

namespace wood_session {

using namespace session_cpp;

Mesh apply_solid_cuts(Mesh mesh, const std::vector<SolidCut>& cuts, bool drills) {

    std::vector<Mesh> pending;

    for (const SolidCut& cut : cuts) {
        if (cut.operation != SolidOperation::difference && !pending.empty()) {
            mesh = solid_difference(mesh, pending);
            pending.clear();
        }

        if (drills)
            for (const Line& drill : cut.drills)
                pending.push_back(drill_mesh(drill, cut.drill_radius, cut.drill_tolerance));

        if (!cut.mesh.number_of_faces())
            continue;

        if (std::optional<Mesh> result = compute_profile_cut(mesh, cut))
            mesh = std::move(*result);
        else if (cut.operation == SolidOperation::difference)
            pending.push_back(cut.mesh);
        else
            mesh = solid_boolean(mesh, cut.mesh, cut.operation, cut.tolerance);
    }

    if (!pending.empty())
        mesh = solid_difference(mesh, pending);

    return mesh;
}

BRep solid_cuts_brep(const Mesh& mesh, const std::vector<SolidCut>& cuts) {

    std::vector<Drill> drills;

    for (const SolidCut& cut : cuts)
        for (const Line& drill : cut.drills)
            drills.push_back({drill, cut.drill_radius});

    if (!drills.empty())
        if (std::optional<BRep> exact = drilled_brep(apply_solid_cuts(mesh, cuts, false), drills))
            return *exact;

    const Mesh cut = apply_solid_cuts(mesh, cuts);

    if (std::optional<BRep> planar = drilled_brep(cut, {}))
        return *planar;

    return mesh_brep(cut);
}

SolidCut SolidCut::difference(const Mesh& mesh) {

    SolidCut cut;
    cut.mesh = mesh;
    cut.operation = SolidOperation::difference;

    return cut;
}

SolidCut SolidCut::unite(const Mesh& mesh) {

    SolidCut cut;
    cut.mesh = mesh;
    cut.operation = SolidOperation::unite;

    return cut;
}

SolidCut SolidCut::transformed(const Xform& xform) const {
    SolidCut result = *this;
    result.mesh = mesh.transformed(xform);
    result.profile = transformed_list(profile, xform);
    result.extrusion = extrusion.transformed(xform);
    result.drills = transformed_list(drills, xform);
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

    for (const Line& drill : drills)
        if (!proto.add_drills()->ParseFromString(drill.pb_dumps()))
            throw std::runtime_error("Cannot serialize cutter drill");

    proto.set_drill_radius(drill_radius);
    proto.set_drill_tolerance(drill_tolerance);

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

    for (const session_proto::Line& drill : proto.drills())
        cut.drills.push_back(Line::pb_loads(drill.SerializeAsString()));

    cut.drill_radius = proto.drill_radius();
    cut.drill_tolerance = proto.drill_tolerance() > 0.0 ? proto.drill_tolerance() : 0.05;

    return cut;
}
}
