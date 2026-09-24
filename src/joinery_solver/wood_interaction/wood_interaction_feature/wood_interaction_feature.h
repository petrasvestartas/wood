#pragma once

#include "pch.h"

#include "wood_interaction_feature_plate.h"
#include "wood_interaction_feature_beam.h"
#include "wood_interaction_feature_plate_beam.h"

namespace wood_session {

class WoodSession;

/// What the solver cut at one contact, exactly one kind: a plate joint, a beam joint or a plate-to-beam joint.
struct InteractionFeature {
    WoodSession* _session = nullptr; // The scene this record was stored in; null until it is added, never written.
    std::string guid; // Key of this feature; minted when the interaction stores it.
    int contact = -1; // Index in Interaction::contacts of the contact this was solved from.
    std::variant<FeaturePlate, FeatureBeam, FeaturePlateBeam> data; // The kind, one at a time.

    InteractionFeature() = default;

    /// A plate joint.
    explicit InteractionFeature(FeaturePlate plate);

    /// A beam joint.
    explicit InteractionFeature(FeatureBeam beam);

    /// A plate-to-beam joint.
    explicit InteractionFeature(FeaturePlateBeam plate_beam);


    /// The scene this record belongs to; throws std::logic_error before the record is added to one.
    WoodSession& session() const;

    /// True once the record has been stored in a scene.
    bool has_session() const {
        return _session != nullptr;
    }

    /// Stores the scene on this record and on the kind it holds.
    void set_session(WoodSession* scene);

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const InteractionFeature& feature);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The plate joint, or null when this is another kind.
    const FeaturePlate* plate() const {
        return std::get_if<FeaturePlate>(&data);
    }

    /// The plate joint, mutable, or null when this is another kind.
    FeaturePlate* plate() {
        return std::get_if<FeaturePlate>(&data);
    }

    /// The beam joint, or null when this is another kind.
    const FeatureBeam* beam() const {
        return std::get_if<FeatureBeam>(&data);
    }

    /// The plate-to-beam joint, or null when this is another kind.
    const FeaturePlateBeam* plate_beam() const {
        return std::get_if<FeaturePlateBeam>(&data);
    }

    /// "plate", "beam" or "plate_beam".
    std::string_view kind() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The feature as JSON: guid, contact, kind, data.
    nlohmann::ordered_json jsondump() const;

    /// A feature from its JSON.
    static InteractionFeature jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The feature as wood_proto.InteractionFeature bytes.
    std::string pb_dumps() const;

    /// A feature from wood_proto.InteractionFeature bytes.
    static InteractionFeature pb_loads(const std::string& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionFeature(guid, contact, kind)".
    std::string str() const;
};

} // namespace wood_session
