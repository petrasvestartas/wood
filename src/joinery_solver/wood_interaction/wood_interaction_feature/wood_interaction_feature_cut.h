#pragma once

#include "pch.h"

#include "wood_interaction_feature.h"
#include "wood_element_solid_cut.h"

using namespace session_cpp;

namespace wood_session {

/// A solid feature: one element cuts another, the solid counterpart of a plate's merged outline feature. Added with WoodSession::add_interaction(source, target, cut): the edge keeps the cut in the source's frame, the target hosts it in its own frame and draws its model geometry with it.
class InteractionFeatureCut : public InteractionFeature {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionFeatureCut"; // The tag the kernel writes and the registry reads.
    SolidCut cut; // The cutter solid, profile and drills, in the source's frame.

    /// An empty cut.
    InteractionFeatureCut() = default;

    /// A feature carrying the cut.
    explicit InteractionFeatureCut(const SolidCut& cut);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "cut".
    std::string_view kind() const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The cut as wood_proto.SolidCut bytes.
    std::string interaction_data_dumps() const override;

    /// A cut from wood_proto.SolidCut bytes; the kernel sets the guid and the name.
    static InteractionFeatureCut interaction_data_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionFeatureCut(faces=..., drills=...)".
    std::string str() const override;
};

} // namespace wood_session
