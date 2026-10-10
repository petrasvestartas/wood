#include "pch.h"
#include "wood_interaction_feature_beam.h"
#include "interaction_feature_beam.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureBeam - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionFeatureBeam::kind() const {
    return "beam";
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureBeam - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeatureBeam::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionFeatureBeam::interaction_data_dumps() const {

    wood_proto::InteractionFeatureBeam proto;
    proto.set_end_type(end_type);

    for (const Polyline& volume : volumes)
        if (!proto.add_volumes()->ParseFromString(volume.pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");

    if (!proto.mutable_normal()->ParseFromString(normal.pb_dumps()))
        throw std::runtime_error("Failed to parse Vector protobuf data");
    for (const Vector& axis : axes)
        if (!proto.add_axes()->ParseFromString(axis.pb_dumps()))
            throw std::runtime_error("Failed to parse Vector protobuf data");

    return proto.SerializeAsString();
}

InteractionFeatureBeam InteractionFeatureBeam::interaction_data_loads(const std::string& data) {

    wood_proto::InteractionFeatureBeam proto;
    if (!proto.ParseFromString(data))
        throw std::runtime_error("Failed to parse InteractionFeatureBeam protobuf data");

    InteractionFeatureBeam feature;
    feature.end_type = proto.end_type();

    for (int k = 0; k < 4 && k < proto.volumes_size(); ++k)
        feature.volumes[k] = Polyline::pb_loads(proto.volumes(k).SerializeAsString());

    if (proto.has_normal())
        feature.normal = Vector::pb_loads(proto.normal().SerializeAsString());
    for (int k = 0; k < 2 && k < proto.axes_size(); ++k)
        feature.axes[k] = Vector::pb_loads(proto.axes(k).SerializeAsString());

    return feature;
}

std::shared_ptr<Interaction> InteractionFeatureBeam::clone() const {
    return std::make_shared<InteractionFeatureBeam>(*this);
}

/// The registered factory: interaction_data bytes to a beam joint.
static std::shared_ptr<Interaction> feature_beam_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionFeatureBeam>(InteractionFeatureBeam::interaction_data_loads(data));
}

void InteractionFeatureBeam::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), feature_beam_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureBeam - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeatureBeam::str() const {
    return fmt::format("InteractionFeatureBeam(end_type={})", end_type);
}

} // namespace wood_session
