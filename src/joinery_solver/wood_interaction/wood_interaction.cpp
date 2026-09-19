#include "pch.h"
#include "wood_interaction.h"
#include "interaction.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Interaction - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const Interaction& interaction) { return os << interaction.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// Interaction - Geometry
// ═══════════════════════════════════════════════════════════════════════════

int Interaction::add_contact(InteractionContact contact) {

    for (size_t i = 0; i < contacts.size(); ++i)
        if (contacts[i].coincides(contact))
            return static_cast<int>(i);

    if (contact.guid.empty())
        contact.guid = ::guid();
    contacts.push_back(std::move(contact));

    return static_cast<int>(contacts.size()) - 1;
}

int Interaction::add_feature(InteractionFeature feature) {

    if (feature.guid.empty())
        feature.guid = ::guid();
    features.push_back(std::move(feature));

    return static_cast<int>(features.size()) - 1;
}

// ═══════════════════════════════════════════════════════════════════════════
// Interaction - JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json Interaction::jsondump() const {

    nlohmann::ordered_json touches = nlohmann::ordered_json::array();
    for (const InteractionContact& contact : contacts)
        touches.push_back(contact.jsondump());

    nlohmann::ordered_json cuts = nlohmann::ordered_json::array();
    for (const InteractionFeature& feature : features)
        cuts.push_back(feature.jsondump());

    return nlohmann::ordered_json{
        {"type", "Interaction"},
        {"guid", guid},
        {"contacts", touches},
        {"features", cuts},
        {"structure", structure.has_value() ? structure->jsondump() : nlohmann::ordered_json(nullptr)},
    };
}

Interaction Interaction::jsonload(const nlohmann::json& data) {

    Interaction interaction;
    interaction.guid = data.value("guid", std::string());
    if (data.contains("contacts"))
        for (const nlohmann::json& contact : data["contacts"])
            interaction.contacts.push_back(InteractionContact::jsonload(contact));
    if (data.contains("features"))
        for (const nlohmann::json& feature : data["features"])
            interaction.features.push_back(InteractionFeature::jsonload(feature));
    if (data.contains("structure") && !data["structure"].is_null())
        interaction.structure = InteractionStructure::jsonload(data["structure"]);

    return interaction;
}

// ═══════════════════════════════════════════════════════════════════════════
// Interaction - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Interaction::pb_dumps() const {

    wood_proto::Interaction proto;
    proto.set_guid(guid);
    for (const InteractionContact& contact : contacts)
        proto.add_contacts()->ParseFromString(contact.pb_dumps());
    for (const InteractionFeature& feature : features)
        proto.add_features()->ParseFromString(feature.pb_dumps());
    if (structure.has_value())
        proto.mutable_structure()->ParseFromString(structure->pb_dumps());

    return proto.SerializeAsString();
}

Interaction Interaction::pb_loads(const std::string& data) {

    wood_proto::Interaction proto;
    proto.ParseFromString(data);

    Interaction interaction;
    interaction.guid = proto.guid();
    for (const wood_proto::InteractionContact& contact : proto.contacts())
        interaction.contacts.push_back(InteractionContact::pb_loads(contact.SerializeAsString()));
    for (const wood_proto::InteractionFeature& feature : proto.features())
        interaction.features.push_back(InteractionFeature::pb_loads(feature.SerializeAsString()));
    if (proto.has_structure())
        interaction.structure = InteractionStructure::pb_loads(proto.structure().SerializeAsString());

    return interaction;
}

// ═══════════════════════════════════════════════════════════════════════════
// Interaction - String
// ═══════════════════════════════════════════════════════════════════════════

std::string Interaction::str() const {
    return fmt::format("Interaction(guid={}, contacts={}, features={})", guid, contacts.size(), features.size());
}

} // namespace wood_session
