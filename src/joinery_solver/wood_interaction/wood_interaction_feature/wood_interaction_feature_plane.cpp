#include "pch.h"
#include "wood_interaction_feature_plane.h"
#include "interaction_feature_plane.pb.h"

using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlane - Constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionFeaturePlane::InteractionFeaturePlane(const Plane& plane) : plane(plane) {
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlane - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionFeaturePlane::kind() const {
    return "plane";
}

InteractionFeaturePlane InteractionFeaturePlane::transformed(const Xform& xform) const {

    InteractionFeaturePlane result = *this;
    result.plane = plane.transformed(xform);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlane - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeaturePlane::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionFeaturePlane::interaction_data_dumps() const {
    return pb_dumps();
}

std::string InteractionFeaturePlane::pb_dumps() const {

    wood_proto::InteractionFeaturePlane proto;
    proto.set_source(source);

    if (!proto.mutable_plane()->ParseFromString(plane.pb_dumps()))
        throw std::runtime_error("Cannot serialize the plane feature");

    return proto.SerializeAsString();
}

InteractionFeaturePlane InteractionFeaturePlane::pb_loads(const std::string& data) {

    wood_proto::InteractionFeaturePlane proto;

    if (!proto.ParseFromString(data))
        throw std::runtime_error("Invalid plane feature data");

    InteractionFeaturePlane feature(Plane::pb_loads(proto.plane().SerializeAsString()));
    feature.source = proto.source();

    return feature;
}

std::shared_ptr<Interaction> InteractionFeaturePlane::clone() const {
    return std::make_shared<InteractionFeaturePlane>(*this);
}

/// The registered factory: interaction_data bytes to a plane feature.
static std::shared_ptr<Interaction> feature_plane_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionFeaturePlane>(InteractionFeaturePlane::pb_loads(data));
}

void InteractionFeaturePlane::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), feature_plane_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlane - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeaturePlane::str() const {
    return fmt::format("InteractionFeaturePlane(origin={}, normal={})", plane.origin().str(), plane.z_axis().str());
}

} // namespace wood_session
