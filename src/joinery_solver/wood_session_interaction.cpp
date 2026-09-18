#include "pch.h"
#include "wood_session_interaction.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const WoodInteraction& interaction) { return os << interaction.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction - Geometry
// ═══════════════════════════════════════════════════════════════════════════

WoodInteraction WoodInteraction::flipped() const {

    WoodInteraction out = *this;
    for (FaceContact& contact : out.contacts) {
        std::swap(contact.face_a, contact.face_b);
        std::swap(contact.element_a, contact.element_b);
    }

    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction - JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json WoodInteraction::jsondump() const {

    nlohmann::ordered_json faces = nlohmann::ordered_json::array();
    for (const FaceContact& contact : contacts)
        faces.push_back(contact.jsondump());

    nlohmann::ordered_json cuts = nlohmann::ordered_json::array();
    for (const WoodJoint& joint : joints)
        cuts.push_back(joint.jsondump());

    return nlohmann::ordered_json{
        {"type", std::string(TYPE)},
        {"contacts", faces},
        {"joints", cuts},
    };
}

WoodInteraction WoodInteraction::jsonload(const nlohmann::json& data) {

    WoodInteraction interaction;

    if (data.contains("contacts"))
        for (const nlohmann::json& contact : data["contacts"])
            interaction.contacts.push_back(FaceContact::jsonload(contact));

    if (data.contains("joints"))
        for (const nlohmann::json& joint : data["joints"])
            interaction.joints.push_back(WoodJoint::jsonload(joint));

    return interaction;
}

std::string WoodInteraction::to_attribute() const { return jsondump().dump(); }

/// The "type" key is the whole grammar: any other attribute on an edge comes back empty.
WoodInteraction WoodInteraction::from_attribute(const std::string& attribute) {

    if (attribute.empty() || attribute.front() != '{')
        return WoodInteraction{};

    nlohmann::json data;
    try {
        data = nlohmann::json::parse(attribute);
    } catch (const std::exception&) {
        return WoodInteraction{};
    }

    if (!data.is_object() || data.value("type", std::string()) != TYPE)
        return WoodInteraction{};

    try {
        return jsonload(data);
    } catch (const std::exception&) {
        return WoodInteraction{};
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction - String
// ═══════════════════════════════════════════════════════════════════════════

std::string WoodInteraction::str() const {
    std::ostringstream os;
    os << "WoodInteraction(contacts=" << contacts.size() << ", joints=" << joints.size() << ")";
    return os.str();
}

} // namespace wood_session
