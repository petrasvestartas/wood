#include "pch.h"
#include "wood_interaction_feature_solid.h"
#include "interaction_feature_solid.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureSolid - Static constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionFeatureSolid InteractionFeatureSolid::subtract(const Mesh& mesh) {

    InteractionFeatureSolid feature;
    feature.mesh = mesh;
    feature.operation = SolidOperation::subtract;

    return feature;
}

InteractionFeatureSolid InteractionFeatureSolid::add(const Mesh& mesh) {

    InteractionFeatureSolid feature;
    feature.mesh = mesh;
    feature.operation = SolidOperation::add;

    return feature;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureSolid - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionFeatureSolid::kind() const {
    return "solid";
}

InteractionFeatureSolid InteractionFeatureSolid::transformed(const Xform& xform) const {

    InteractionFeatureSolid result = *this;
    result.mesh = mesh.transformed(xform);
    result.profile.clear();

    for (const Polyline& ring : profile)
        result.profile.push_back(ring.transformed(xform));

    result.extrusion = extrusion.transformed(xform);
    result.drills.clear();

    for (const Line& drill : drills)
        result.drills.push_back(drill.transformed(xform));

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureSolid - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeatureSolid::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionFeatureSolid::interaction_data_dumps() const {
    return pb_dumps();
}

std::string InteractionFeatureSolid::pb_dumps() const {

    wood_proto::InteractionFeatureSolid proto;
    proto.set_source(source);
    proto.set_operation(static_cast<int>(operation));
    proto.set_tolerance(tolerance);

    if (!proto.mutable_mesh()->ParseFromString(mesh.pb_dumps()) || !proto.mutable_extrusion()->ParseFromString(extrusion.pb_dumps()))
        throw std::runtime_error("Cannot serialize the solid feature");

    for (const Polyline& ring : profile)
        if (!proto.add_profile()->ParseFromString(ring.pb_dumps()))
            throw std::runtime_error("Cannot serialize the solid feature profile");

    for (const Line& drill : drills)
        if (!proto.add_drills()->ParseFromString(drill.pb_dumps()))
            throw std::runtime_error("Cannot serialize the solid feature drill");

    proto.set_drill_radius(drill_radius);
    proto.set_drill_tolerance(drill_tolerance);

    return proto.SerializeAsString();
}

InteractionFeatureSolid InteractionFeatureSolid::pb_loads(const std::string& data) {

    wood_proto::InteractionFeatureSolid proto;

    if (!proto.ParseFromString(data) || proto.operation() < 0 || proto.operation() > 2 || !std::isfinite(proto.tolerance()) || proto.tolerance() <= 0)
        throw std::runtime_error("Invalid solid feature data");

    InteractionFeatureSolid feature;
    feature.source = proto.source();
    feature.operation = static_cast<SolidOperation>(proto.operation());
    feature.tolerance = proto.tolerance();
    feature.mesh = Mesh::pb_loads(proto.mesh().SerializeAsString());
    feature.extrusion = Vector::pb_loads(proto.extrusion().SerializeAsString());

    for (const session_proto::Polyline& ring : proto.profile())
        feature.profile.push_back(Polyline::pb_loads(ring.SerializeAsString()));

    for (const session_proto::Line& drill : proto.drills())
        feature.drills.push_back(Line::pb_loads(drill.SerializeAsString()));

    feature.drill_radius = proto.drill_radius();
    feature.drill_tolerance = proto.drill_tolerance() > 0.0 ? proto.drill_tolerance() : 0.05;

    return feature;
}

std::shared_ptr<Interaction> InteractionFeatureSolid::clone() const {
    return std::make_shared<InteractionFeatureSolid>(*this);
}

/// The registered factory: interaction_data bytes to a solid feature.
static std::shared_ptr<Interaction> feature_solid_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionFeatureSolid>(InteractionFeatureSolid::pb_loads(data));
}

void InteractionFeatureSolid::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), feature_solid_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureSolid - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeatureSolid::str() const {
    return fmt::format("InteractionFeatureSolid(operation={}, faces={}, drills={})", operation == SolidOperation::add ? "add" : operation == SolidOperation::subtract ? "subtract" : "intersect", mesh.number_of_faces(), drills.size());
}

} // namespace wood_session
