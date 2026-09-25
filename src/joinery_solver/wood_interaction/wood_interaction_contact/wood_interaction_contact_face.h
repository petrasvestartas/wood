#pragma once

#include "pch.h"

#include "wood_interaction_contact.h"
#include "wood_interaction_contact_face_type.h"

namespace wood_session {

/// Two coplanar faces in contact and the region they share; the elements are the edge the interaction sits on.
class InteractionContactFace : public InteractionContact {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionContactFace"; // The tag the kernel writes and the registry reads.

    int face_a = -1; // Face index on the edge's first element; -1 for a three-valence link that has no face.
    int face_b = -1; // Face index on the edge's second element.
    ContactType type = ContactType::unknown; // Topology class of the pair.
    session_cpp::Polyline polygon; // The boolean intersection of the two face outlines, closed, in face_a's plane; the largest region when Clipper returns several.

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// An empty contact.
    InteractionContactFace() = default;

    /// A contact from its faces, class and overlap.
    InteractionContactFace(int face_a, int face_b, ContactType type, session_cpp::Polyline polygon);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "face".
    std::string_view kind() const override;

    /// Faces swapped: the contact read from the other end of the edge, same guid.
    std::shared_ptr<InteractionContact> flipped() const override;

    /// Same faces and class, the polygon aside.
    bool coincides(const InteractionContact& other) const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The fields as wood_proto.InteractionContactFace bytes: what the kernel carries in interaction_data.
    std::string interaction_data_dumps() const override;

    /// A face contact from wood_proto.InteractionContactFace bytes; the kernel sets the guid and the name.
    static InteractionContactFace interaction_data_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel, so a Session load rebuilds face contacts.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionContactFace(face_a, face_b, type, points)".
    std::string str() const override;
};

} // namespace wood_session
