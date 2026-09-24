#pragma once

#include "pch.h"

namespace wood_session {

class WoodSession;

/// One crossing of two plates: where their side faces pass through each other, as plane_to_face computes it.
struct ContactCross {
    WoodSession* _session = nullptr; // The scene this record was stored in; null until it is added, never written.
    std::array<int, 2> faces_a{-1, -1}; // The two side faces of the first element the crossing involves.
    std::array<int, 2> faces_b{-1, -1}; // The two side faces of the second element the crossing involves.
    session_cpp::Polyline polygon; // Closed quad on the mid-plane, 5 points.
    std::array<session_cpp::Polyline, 2> lines; // The two perpendicular centrelines of polygon, 2 points each.
    std::array<session_cpp::Polyline, 2> volumes; // The two parallel quads bounding the joint volume.


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
    friend std::ostream& operator<<(std::ostream& os, const ContactCross& contact);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Sides swapped: the contact read from the other end of the edge.
    ContactCross flipped() const;

    /// Same faces on both sides, the geometry aside: what detection reuses instead of appending twice.
    bool coincides(const ContactCross& other) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as JSON: faces_a, faces_b, polygon, lines, volumes.
    nlohmann::ordered_json jsondump() const;

    /// A contact from its JSON.
    static ContactCross jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as wood_proto.ContactCross bytes.
    std::string pb_dumps() const;

    /// A contact from wood_proto.ContactCross bytes.
    static ContactCross pb_loads(const std::string& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "ContactCross(faces_a, faces_b, points)".
    std::string str() const;
};

} // namespace wood_session
