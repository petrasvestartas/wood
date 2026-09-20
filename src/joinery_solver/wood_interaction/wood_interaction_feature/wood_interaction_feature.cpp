#include "pch.h"
#include "wood_serialization.h"
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

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeature - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// The protobuf message, printed.
nlohmann::ordered_json InteractionFeature::jsondump() const {

    wood_proto::InteractionFeature proto;
    proto.ParseFromString(pb_dumps());

    return json_of(proto);
}

/// The protobuf message, parsed.
InteractionFeature InteractionFeature::jsonload(const nlohmann::json& data) {

    wood_proto::InteractionFeature proto;
    message_from_json(data, proto);

    return pb_loads(proto.SerializeAsString());
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeature - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeature::pb_dumps() const {

    wood_proto::InteractionFeature proto;
    proto.set_guid(guid);
    proto.set_contact(contact);
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
