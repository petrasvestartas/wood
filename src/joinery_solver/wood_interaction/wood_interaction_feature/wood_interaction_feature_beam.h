#pragma once

#include "pch.h"

#include "wood_interaction_feature.h"

namespace wood_session {

/// A beam-to-beam joint: the four volume rectangles cut where two axes meet, as Beam::joint_volumes builds them.
class InteractionFeatureBeam : public InteractionFeature {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionFeatureBeam"; // The tag the kernel writes and the registry reads.

    int end_type = 0; // 0 crossing, 1 side to end, 2 end to end.
    std::array<session_cpp::Polyline, 4> volumes; // [0] and [1] on the edge's first beam, [2] and [3] on the second.

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "beam".
    std::string_view kind() const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The fields as wood_proto.InteractionFeatureBeam bytes: what the kernel carries in interaction_data.
    std::string interaction_data_dumps() const override;

    /// A beam joint from wood_proto.InteractionFeatureBeam bytes; the kernel sets the guid and the name.
    static InteractionFeatureBeam interaction_data_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel, so a Session load rebuilds beam joints.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionFeatureBeam(end_type)".
    std::string str() const override;
};

} // namespace wood_session
