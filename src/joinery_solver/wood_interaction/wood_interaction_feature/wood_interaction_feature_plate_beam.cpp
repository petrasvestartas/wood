#include "pch.h"
#include "wood_interaction_feature_plate_beam.h"
#include "interaction_feature_plate_beam.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlateBeam - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionFeaturePlateBeam::kind() const {
    return "plate_beam";
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlateBeam - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeaturePlateBeam::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionFeaturePlateBeam::interaction_data_dumps() const {

    wood_proto::InteractionFeaturePlateBeam proto;
    proto.set_contact_guid(contact_guid);

    return proto.SerializeAsString();
}

InteractionFeaturePlateBeam InteractionFeaturePlateBeam::interaction_data_loads(const std::string& data) {

    wood_proto::InteractionFeaturePlateBeam proto;
    proto.ParseFromString(data);

    InteractionFeaturePlateBeam feature;
    feature.contact_guid = proto.contact_guid();

    return feature;
}

std::shared_ptr<Interaction> InteractionFeaturePlateBeam::clone() const {
    return std::make_shared<InteractionFeaturePlateBeam>(*this);
}

/// The registered factory: interaction_data bytes to a plate-to-beam joint.
static std::shared_ptr<Interaction> feature_plate_beam_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionFeaturePlateBeam>(InteractionFeaturePlateBeam::interaction_data_loads(data));
}

void InteractionFeaturePlateBeam::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), feature_plate_beam_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlateBeam - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeaturePlateBeam::str() const {
    return "InteractionFeaturePlateBeam()";
}

} // namespace wood_session
