#pragma once

#include "pch.h"

#include "wood_joint.h"

namespace wood_session {

/// Everything the relation between two elements is made of: where they touch, and what the solver made of it. The scene keeps one per pair as a typed object; the graph edge string is only its saved form.
struct WoodInteraction {
    std::vector<FaceContact> contacts; // Every overlap region between the pair, face_a on the element the pair was read from.
    std::vector<WoodJoint> joints; // What get_connection_zones made of them; a joint names its own two elements.
    static constexpr std::string_view TYPE = "WoodInteraction"; // Value of "type" the attribute is written under, and the grammar's whole guard.

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const WoodInteraction& interaction);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// True when there is neither a contact nor a joint.
    bool empty() const { return contacts.empty() && joints.empty(); }

    /// face_a and face_b, element_a and element_b swapped in every contact: the pair read from the other end.
    WoodInteraction flipped() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The interaction as JSON: contacts, joints, type.
    nlohmann::ordered_json jsondump() const;

    /// An interaction from its JSON.
    static WoodInteraction jsonload(const nlohmann::json& data);

    /// jsondump() as the string pb_dump writes on the graph edge.
    std::string to_attribute() const;

    /// Total: an attribute this grammar does not describe ("bvh_collision", "default", "") or cannot read, such as a pb written before the kernel JSON, comes back empty; pb_load reads every edge through it once.
    static WoodInteraction from_attribute(const std::string& attribute);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "WoodInteraction(contacts, joints)".
    std::string str() const;
};

} // namespace wood_session
