#pragma once

#include "pch.h"

#include "wood_interaction_feature_plate.h"
#include "wood_interaction_feature_beam.h"
#include "wood_interaction_feature_plate_beam.h"

namespace wood_session {

/// What the solver cut at one contact, exactly one kind: a plate joint, a beam joint or a plate-to-beam joint.
struct InteractionFeature {
    std::string guid; // Key of this feature; minted when the interaction stores it.
    int contact = -1; // Index in Interaction::contacts of the contact this was solved from.
    bool reversed = false; // True when the male element, the host of element_features[0], is the edge's second element.
    std::array<session_cpp::ElementFeature, 2> element_features; // [0] what the male host carries, [1] what the female host carries; an ElementFeature copy drops its guid, so their identities live in feature_guids.
    std::array<std::string, 2> feature_guids; // The guids of the two element features, stamped back onto them by to_element_features().
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

    /// Stores the two element features and records their guids.
    void set_element_features(const std::array<session_cpp::ElementFeature, 2>& features);

    /// The two element features with their guids stamped, [0] male, [1] female.
    std::array<session_cpp::ElementFeature, 2> to_element_features() const;

    /// The element feature of one end of the edge with its guid stamped: 0 the first element, 1 the second; reversed picks the other side.
    session_cpp::ElementFeature element_feature_at(int end) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The feature as JSON: guid, contact, reversed, element_features, kind, data.
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
