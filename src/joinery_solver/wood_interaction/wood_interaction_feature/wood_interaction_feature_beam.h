#pragma once

#include "pch.h"

namespace wood_session {

/// A beam-to-beam joint: the four volume rectangles cut where two axes meet, as Beam::joint_volumes builds them.
struct FeatureBeam {
    int end_type = 0; // 0 crossing, 1 side to end, 2 end to end.
    std::array<session_cpp::Polyline, 4> volumes; // [0] and [1] on the first beam, [2] and [3] on the second.

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const FeatureBeam& feature);

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The joint as JSON: end_type, volumes.
    nlohmann::ordered_json jsondump() const;

    /// A joint from its JSON.
    static FeatureBeam jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The joint as wood_proto.FeatureBeam bytes.
    std::string pb_dumps() const;

    /// A joint from wood_proto.FeatureBeam bytes.
    static FeatureBeam pb_loads(const std::string& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "FeatureBeam(end_type)".
    std::string str() const;
};

} // namespace wood_session
