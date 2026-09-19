#pragma once

#include "pch.h"

#include "wood_interaction_feature_plate.h"
#include "wood_interaction_feature_beam.h"

namespace wood_session {

/// A plate-to-beam joint; nothing computed yet, the record and its message are reserved.
struct FeaturePlateBeam {

    /// The joint as JSON: type only.
    nlohmann::ordered_json jsondump() const { return nlohmann::ordered_json{{"type", "FeaturePlateBeam"}}; }

    /// A joint from its JSON.
    static FeaturePlateBeam jsonload(const nlohmann::json&) { return FeaturePlateBeam{}; }

    /// The joint as wood_proto.FeaturePlateBeam bytes: empty.
    std::string pb_dumps() const { return std::string(); }

    /// A joint from wood_proto.FeaturePlateBeam bytes.
    static FeaturePlateBeam pb_loads(const std::string&) { return FeaturePlateBeam{}; }

    /// "FeaturePlateBeam()".
    std::string str() const { return "FeaturePlateBeam()"; }
};

/// What the solver cut at one contact, exactly one kind: a plate joint, a beam joint or a plate-to-beam joint.
struct InteractionFeature {
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

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const InteractionFeature& feature);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The plate joint, or null when this is another kind.
    const FeaturePlate* plate() const { return std::get_if<FeaturePlate>(&data); }

    /// The plate joint, mutable, or null when this is another kind.
    FeaturePlate* plate() { return std::get_if<FeaturePlate>(&data); }

    /// The beam joint, or null when this is another kind.
    const FeatureBeam* beam() const { return std::get_if<FeatureBeam>(&data); }

    /// The plate-to-beam joint, or null when this is another kind.
    const FeaturePlateBeam* plate_beam() const { return std::get_if<FeaturePlateBeam>(&data); }

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
