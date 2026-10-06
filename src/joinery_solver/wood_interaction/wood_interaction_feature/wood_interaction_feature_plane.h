#pragma once

#include "pch.h"

#include "wood_interaction_feature.h"

using namespace session_cpp;

namespace wood_session {

/// A plane feature: a plane one element cuts another by, the target keeping the side the normal points to. Added with WoodSession::add_interaction(source, target, feature): the edge keeps the plane in the source's frame, the target a copy in its own frame, applied before its solid features are taken away.
class InteractionFeaturePlane : public InteractionFeature {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionFeaturePlane"; // The tag the kernel writes and the registry reads.
    std::string source; // The guid of the element that put it there, so its next feature replaces it.
    Plane plane; // The cutting plane; the target keeps the side its normal points to.

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// An empty feature.
    InteractionFeaturePlane() = default;

    /// A feature cutting by plane.
    explicit InteractionFeaturePlane(const Plane& plane);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "plane".
    std::string_view kind() const override;

    /// A copy with the plane moved by xform.
    InteractionFeaturePlane transformed(const Xform& xform) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The feature as wood_proto.InteractionFeaturePlane bytes.
    std::string interaction_data_dumps() const override;

    /// The feature as wood_proto.InteractionFeaturePlane bytes, the same as interaction_data_dumps.
    std::string pb_dumps() const;

    /// A feature from wood_proto.InteractionFeaturePlane bytes.
    static InteractionFeaturePlane pb_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionFeaturePlane(origin=..., normal=...)".
    std::string str() const override;
};

} // namespace wood_session
