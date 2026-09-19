#include "pch.h"
#include "wood_interaction_feature.h"
#include "interaction_feature.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeature - Constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionFeature::InteractionFeature(FeaturePlate plate) : data(std::move(plate)) {}

InteractionFeature::InteractionFeature(FeatureBeam beam) : data(std::move(beam)) {}

InteractionFeature::InteractionFeature(FeaturePlateBeam plate_beam) : data(std::move(plate_beam)) {}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeature - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const InteractionFeature& feature) { return os << feature.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeature - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionFeature::kind() const {

    if (plate())
        return "plate";
    if (beam())
        return "beam";

    return "plate_beam";
}

void InteractionFeature::set_element_features(const std::array<ElementFeature, 2>& features) {
    for (int k = 0; k < 2; ++k) {
        element_features[k] = features[k];
        feature_guids[k] = features[k].guid();
    }
}

std::array<ElementFeature, 2> InteractionFeature::to_element_features() const {

    std::array<ElementFeature, 2> out{element_features[0], element_features[1]};
    for (int k = 0; k < 2; ++k)
        if (!feature_guids[k].empty())
            out[k].guid() = feature_guids[k];

    return out;
}

/// Moved out of the stamped pair: a copy would mint a fresh guid.
ElementFeature InteractionFeature::element_feature_at(int end) const {

    std::array<ElementFeature, 2> both = to_element_features();
    return std::move(both[(end == 1) != reversed ? 1 : 0]);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeature - JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json InteractionFeature::jsondump() const {
    return nlohmann::ordered_json{
        {"type", "InteractionFeature"},
        {"guid", guid},
        {"contact", contact},
        {"reversed", reversed},
        {"element_features", {to_element_features()[0].jsondump(), to_element_features()[1].jsondump()}},
        {"kind", std::string(kind())},
        {"data", std::visit([](const auto& kind) { return kind.jsondump(); }, data)},
    };
}

InteractionFeature InteractionFeature::jsonload(const nlohmann::json& data) {

    InteractionFeature feature;
    feature.guid = data.value("guid", std::string());
    feature.contact = data.value("contact", -1);
    feature.reversed = data.value("reversed", false);
    if (data.contains("element_features"))
        for (size_t k = 0; k < 2 && k < data["element_features"].size(); ++k) {
            feature.element_features[k] = ElementFeature::jsonload(data["element_features"][k]);
            feature.feature_guids[k] = data["element_features"][k].value("guid", std::string());
        }

    const std::string kind = data.value("kind", std::string("plate"));
    const nlohmann::json& body = data.contains("data") ? data["data"] : nlohmann::json::object();
    if (kind == "beam")
        feature.data = FeatureBeam::jsonload(body);
    else if (kind == "plate_beam")
        feature.data = FeaturePlateBeam::jsonload(body);
    else
        feature.data = FeaturePlate::jsonload(body);

    return feature;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeature - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeature::pb_dumps() const {

    wood_proto::InteractionFeature proto;
    proto.set_guid(guid);
    proto.set_contact(contact);
    proto.set_reversed(reversed);
    for (const ElementFeature& element_feature : to_element_features())
        proto.add_element_features()->ParseFromString(element_feature.pb_dumps());
    if (const FeaturePlate* kind = plate())
        proto.mutable_plate()->ParseFromString(kind->pb_dumps());
    if (const FeatureBeam* kind = beam())
        proto.mutable_beam()->ParseFromString(kind->pb_dumps());
    if (const FeaturePlateBeam* kind = plate_beam())
        proto.mutable_plate_beam()->ParseFromString(kind->pb_dumps());

    return proto.SerializeAsString();
}

InteractionFeature InteractionFeature::pb_loads(const std::string& data) {

    wood_proto::InteractionFeature proto;
    proto.ParseFromString(data);

    InteractionFeature feature;
    feature.guid = proto.guid();
    feature.contact = proto.contact();
    feature.reversed = proto.reversed();
    for (int k = 0; k < 2 && k < proto.element_features_size(); ++k) {
        feature.element_features[k] = ElementFeature::pb_loads(proto.element_features(k).SerializeAsString());
        feature.feature_guids[k] = proto.element_features(k).guid();
    }
    if (proto.has_beam())
        feature.data = FeatureBeam::pb_loads(proto.beam().SerializeAsString());
    else if (proto.has_plate_beam())
        feature.data = FeaturePlateBeam::pb_loads(proto.plate_beam().SerializeAsString());
    else
        feature.data = FeaturePlate::pb_loads(proto.plate().SerializeAsString());

    return feature;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeature - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeature::str() const {
    return fmt::format("InteractionFeature(guid={}, contact={}, kind={})", guid, contact, kind());
}

} // namespace wood_session
