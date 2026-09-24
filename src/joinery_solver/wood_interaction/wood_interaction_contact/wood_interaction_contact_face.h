#pragma once

#include "pch.h"

#include "wood_interaction_contact_face_type.h"

namespace wood_session {

class WoodSession;

/// Two coplanar faces in contact and the region they share; the elements are the edge the interaction sits on.
struct ContactFace {
    WoodSession* _session = nullptr; // The scene this record was stored in; null until it is added, never written.
    int face_a = -1; // Face index on the edge's first element; -1 for a three-valence link that has no face.
    int face_b = -1; // Face index on the edge's second element.
    ContactType type = ContactType::unknown; // Topology class of the pair.
    session_cpp::Polyline polygon; // The boolean intersection of the two face outlines, closed, in face_a's plane; the largest region when Clipper returns several.

    ContactFace() = default;

    /// A contact from its faces, class and overlap.
    ContactFace(int face_a, int face_b, ContactType type, session_cpp::Polyline polygon);


    /// The scene this record belongs to; throws std::logic_error before the record is added to one.
    WoodSession& session() const;

    /// True once the record has been stored in a scene.
    bool has_session() const {
        return _session != nullptr;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const ContactFace& contact);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Faces swapped: the contact read from the other end of the edge.
    ContactFace flipped() const;

    /// Same faces and class, the polygon aside: what detection reuses instead of appending twice.
    bool coincides(const ContactFace& other) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as JSON: face_a, face_b, type, polygon.
    nlohmann::ordered_json jsondump() const;

    /// A contact from its JSON.
    static ContactFace jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as wood_proto.ContactFace bytes.
    std::string pb_dumps() const;

    /// A contact from wood_proto.ContactFace bytes.
    static ContactFace pb_loads(const std::string& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "ContactFace(face_a, face_b, type, points)".
    std::string str() const;
};

} // namespace wood_session
