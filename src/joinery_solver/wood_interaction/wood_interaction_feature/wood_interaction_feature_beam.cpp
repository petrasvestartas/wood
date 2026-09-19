#include "pch.h"
#include "wood_interaction_feature_beam.h"
#include "interaction_feature_beam.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// FeatureBeam - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const FeatureBeam& feature) { return os << feature.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// FeatureBeam - JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json FeatureBeam::jsondump() const {

    nlohmann::ordered_json rects = nlohmann::ordered_json::array();
    for (const Polyline& volume : volumes)
        rects.push_back(volume.jsondump());

    return nlohmann::ordered_json{
        {"type", "FeatureBeam"},
        {"end_type", end_type},
        {"volumes", rects},
    };
}

FeatureBeam FeatureBeam::jsonload(const nlohmann::json& data) {

    FeatureBeam feature;
    feature.end_type = data.value("end_type", 0);
    if (data.contains("volumes"))
        for (size_t k = 0; k < 4 && k < data["volumes"].size(); ++k)
            feature.volumes[k] = Polyline::jsonload(data["volumes"][k]);

    return feature;
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
