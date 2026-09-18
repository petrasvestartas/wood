#pragma once

#include "pch.h"

#include "wood_face_to_face_contact_type.h"

namespace wood_session {

/// One face pair in contact: the two faces, the class, the overlap region (closed, in the first face's plane).
struct FaceContact {
    int face_a = 0; // Face index on the first element.
    int face_b = 0; // Face index on the second element.
    ContactType type = ContactType::unknown; // Topology class of the pair.
    session_cpp::Polyline area; // The overlap region, closed, in face_a's plane.
    std::string guid; // Its key in WoodSession::get_contact; minted when the scene stores it.
    std::string element_a; // Guid of the element face_a is on; set when the scene stores it.
    std::string element_b; // Guid of the element face_b is on.

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as JSON: guid, face_a, face_b, type, area; the elements are the edge it sits on.
    nlohmann::ordered_json jsondump() const;

    /// A contact from its JSON.
    static FaceContact jsonload(const nlohmann::json& data);
};

} // namespace wood_session
