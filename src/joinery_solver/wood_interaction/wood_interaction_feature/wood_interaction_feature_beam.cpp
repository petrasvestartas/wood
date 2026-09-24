#include "pch.h"
#include "wood_serialization.h"
#include "wood_interaction_feature_beam.h"
#include "interaction_feature_beam.pb.h"
using namespace session_cpp;

namespace wood_session {

WoodSession& FeatureBeam::session() const {

    if (!_session)
        throw std::logic_error("FeatureBeam::session: the record is not in a scene");

    return *_session;
}

// ═══════════════════════════════════════════════════════════════════════════
// FeatureBeam - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const FeatureBeam& feature) {
    return os << feature.str();
}

// ═══════════════════════════════════════════════════════════════════════════
// FeatureBeam - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// The protobuf message, printed.
nlohmann::ordered_json FeatureBeam::jsondump() const {

    wood_proto::FeatureBeam proto;
    proto.ParseFromString(pb_dumps());

    return json_of(proto);
}

/// The protobuf message, parsed.
FeatureBeam FeatureBeam::jsonload(const nlohmann::json& data) {

    wood_proto::FeatureBeam proto;
    message_from_json(data, proto);

    return pb_loads(proto.SerializeAsString());
}

// ═══════════════════════════════════════════════════════════════════════════
// FeatureBeam - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string FeatureBeam::pb_dumps() const {

    wood_proto::FeatureBeam proto;
    proto.set_end_type(end_type);
    for (const Polyline& volume : volumes)
        proto.add_volumes()->ParseFromString(volume.pb_dumps());

    return proto.SerializeAsString();
}

FeatureBeam FeatureBeam::pb_loads(const std::string& data) {

    wood_proto::FeatureBeam proto;
    proto.ParseFromString(data);

    FeatureBeam feature;
    feature.end_type = proto.end_type();
    for (int k = 0; k < 4 && k < proto.volumes_size(); ++k)
        feature.volumes[k] = Polyline::pb_loads(proto.volumes(k).SerializeAsString());

    return feature;
}

// ═══════════════════════════════════════════════════════════════════════════
// FeatureBeam - String
// ═══════════════════════════════════════════════════════════════════════════

std::string FeatureBeam::str() const {
    return fmt::format("FeatureBeam(end_type={})", end_type);
}

} // namespace wood_session
