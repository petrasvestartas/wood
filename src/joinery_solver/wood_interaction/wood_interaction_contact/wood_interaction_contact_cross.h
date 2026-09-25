#pragma once

#include "pch.h"

#include "wood_interaction_contact.h"

namespace wood_session {

/// One crossing of two plates: where their side faces pass through each other, as plane_to_face computes it.
class InteractionContactCross : public InteractionContact {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionContactCross"; // The tag the kernel writes and the registry reads.

    std::array<int, 2> faces_a{-1, -1}; // The two side faces of the first element the crossing involves.
    std::array<int, 2> faces_b{-1, -1}; // The two side faces of the second element the crossing involves.
    session_cpp::Polyline polygon; // Closed quad on the mid-plane, 5 points.
    std::array<session_cpp::Polyline, 2> lines; // The two perpendicular centrelines of polygon, 2 points each.
    std::array<session_cpp::Polyline, 2> volumes; // The two parallel quads bounding the joint volume.

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "cross".
    std::string_view kind() const override;

    /// Sides swapped: the contact read from the other end of the edge, same guid.
    std::shared_ptr<InteractionContact> flipped() const override;

    /// Same faces on both sides, the geometry aside.
    bool coincides(const InteractionContact& other) const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The fields as wood_proto.InteractionContactCross bytes: what the kernel carries in interaction_data.
    std::string interaction_data_dumps() const override;

    /// A cross contact from wood_proto.InteractionContactCross bytes; the kernel sets the guid and the name.
    static InteractionContactCross interaction_data_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel, so a Session load rebuilds cross contacts.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionContactCross(faces_a, faces_b, points)".
    std::string str() const override;
};

} // namespace wood_session
