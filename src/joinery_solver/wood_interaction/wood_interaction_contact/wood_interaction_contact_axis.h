#pragma once

#include "pch.h"

namespace wood_session {

class WoodSession;

/// Where two polylines come closest, beam axes or plate outlines: the segment between the closest points and where its ends sit on each side.
struct ContactAxis {
    WoodSession* _session = nullptr; // The scene this record was stored in; null until it is added, never written.
    session_cpp::Line segment; // From the closest point on the first element to the closest point on the second.
    double t_a = 0.0; // Parameter of the closest point along segment_a, 0..1.
    double t_b = 0.0; // Parameter of the closest point along segment_b, 0..1.
    int polyline_a = 0; // Which polyline of the first element: a beam has one axis, a plate one outline per face.
    int segment_a = 0; // Segment of that polyline.
    int polyline_b = 0; // Which polyline of the second element.
    int segment_b = 0; // Segment of that polyline.

    ContactAxis() = default;

    /// A contact from its closest segment and where the ends sit.
    ContactAxis(session_cpp::Line segment, double t_a, double t_b, int polyline_a, int segment_a, int polyline_b, int segment_b);


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
    friend std::ostream& operator<<(std::ostream& os, const ContactAxis& contact);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Sides swapped and the segment reversed: the contact read from the other end of the edge.
    ContactAxis flipped() const;

    /// Same polylines and segments on both sides, the geometry aside: what detection reuses instead of appending twice.
    bool coincides(const ContactAxis& other) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as JSON: segment, t_a, t_b, polyline_a, segment_a, polyline_b, segment_b.
    nlohmann::ordered_json jsondump() const;

    /// A contact from its JSON.
    static ContactAxis jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as wood_proto.ContactAxis bytes.
    std::string pb_dumps() const;

    /// A contact from wood_proto.ContactAxis bytes.
    static ContactAxis pb_loads(const std::string& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "ContactAxis(a=(polyline, segment, t), b=(polyline, segment, t), length)".
    std::string str() const;
};

} // namespace wood_session
