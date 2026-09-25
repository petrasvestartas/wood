#pragma once

#include "pch.h"

#include "wood_interaction_feature.h"

namespace wood_session {

/// A plate-to-beam joint; nothing computed yet, the class and its message are reserved.
class InteractionFeaturePlateBeam : public InteractionFeature {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionFeaturePlateBeam"; // The tag the kernel writes and the registry reads.

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "plate_beam".
    std::string_view kind() const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The contact guid as wood_proto.InteractionFeaturePlateBeam bytes.
    std::string interaction_data_dumps() const override;

    /// A plate-to-beam joint from wood_proto.InteractionFeaturePlateBeam bytes; the kernel sets the guid and the name.
    static InteractionFeaturePlateBeam interaction_data_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionFeaturePlateBeam()".
    std::string str() const override;
};

} // namespace wood_session
