#pragma once

#include "pch.h"

#include "wood_interaction_contact.h"
#include "wood_interaction_feature.h"

namespace wood_session {

/// How forces pass between the edge's two elements; nothing computed yet, the record and its message are reserved.
struct InteractionStructure {

    /// The structure as JSON: type only.
    nlohmann::ordered_json jsondump() const { return nlohmann::ordered_json{{"type", "InteractionStructure"}}; }

    /// A structure from its JSON.
    static InteractionStructure jsonload(const nlohmann::json&) { return InteractionStructure{}; }

    /// The structure as wood_proto.InteractionStructure bytes: empty.
    std::string pb_dumps() const { return std::string(); }

    /// A structure from wood_proto.InteractionStructure bytes.
    static InteractionStructure pb_loads(const std::string&) { return InteractionStructure{}; }

    /// "InteractionStructure()".
    std::string str() const { return "InteractionStructure()"; }
};

/// Everything between the two elements of one graph edge: where they touch, what was cut, how forces pass. The pair itself lives on the edge, whose guid is this record's key in WoodSession::interactions.
struct Interaction {
    std::string guid; // The graph edge's guid.
    std::vector<InteractionContact> contacts; // Every place the pair touches.
    std::vector<InteractionFeature> features; // Every joint cut between the pair.
    std::optional<InteractionStructure> structure; // Empty until the structural pass exists.

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const Interaction& interaction);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// True when there is neither a contact nor a feature nor a structure.
    bool empty() const { return contacts.empty() && features.empty() && !structure.has_value(); }

    /// Stores a contact and returns its index; one that coincides with a stored contact is not added again. A contact without a guid gets one.
    int add_contact(InteractionContact contact);

    /// Stores a feature and returns its index. A feature without a guid gets one.
    int add_feature(InteractionFeature feature);

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The interaction as JSON: guid, contacts, features, structure.
    nlohmann::ordered_json jsondump() const;

    /// An interaction from its JSON.
    static Interaction jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The interaction as wood_proto.Interaction bytes.
    std::string pb_dumps() const;

    /// An interaction from wood_proto.Interaction bytes.
    static Interaction pb_loads(const std::string& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Interaction(guid, contacts, features)".
    std::string str() const;
};

} // namespace wood_session
