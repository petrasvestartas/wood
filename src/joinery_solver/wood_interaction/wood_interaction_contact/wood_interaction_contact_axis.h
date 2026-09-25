#pragma once

#include "pch.h"

#include "wood_interaction_contact.h"

namespace wood_session {

/// Where two polylines come closest, beam axes or plate outlines: the segment between the closest points and where its ends sit on each side.
class InteractionContactAxis : public InteractionContact {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionContactAxis"; // The tag the kernel writes and the registry reads.

    session_cpp::Line segment; // From the closest point on the first element to the closest point on the second.
    double t_a = 0.0; // Parameter of the closest point along segment_a, 0..1.
    double t_b = 0.0; // Parameter of the closest point along segment_b, 0..1.
    int polyline_a = 0; // Which polyline of the first element: a beam has one axis, a plate one outline per face.
    int segment_a = 0; // Segment of that polyline.
    int polyline_b = 0; // Which polyline of the second element.
    int segment_b = 0; // Segment of that polyline.

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// An empty contact.
    InteractionContactAxis() = default;

    /// A contact from its closest segment and where the ends sit.
    InteractionContactAxis(session_cpp::Line segment, double t_a, double t_b, int polyline_a, int segment_a, int polyline_b, int segment_b);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "axis".
    std::string_view kind() const override;

    /// Sides swapped and the segment reversed: the contact read from the other end of the edge, same guid.
    std::shared_ptr<InteractionContact> flipped() const override;

    /// Same polylines and segments on both sides, the geometry aside.
    bool coincides(const InteractionContact& other) const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The fields as wood_proto.InteractionContactAxis bytes: what the kernel carries in interaction_data.
    std::string interaction_data_dumps() const override;

    /// A axis contact from wood_proto.InteractionContactAxis bytes; the kernel sets the guid and the name.
    static InteractionContactAxis interaction_data_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel, so a Session load rebuilds axis contacts.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionContactAxis(a=(polyline, segment, t), b=(polyline, segment, t), length)".
    std::string str() const override;
};

} // namespace wood_session
