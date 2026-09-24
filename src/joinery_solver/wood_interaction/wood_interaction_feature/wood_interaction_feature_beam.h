#pragma once

#include "pch.h"

namespace wood_session {

class WoodSession;

/// A beam-to-beam joint: the four volume rectangles cut where two axes meet, as Beam::joint_volumes builds them.
struct FeatureBeam {
    WoodSession* _session = nullptr; // The scene this record was stored in; null until it is added, never written.
    int end_type = 0; // 0 crossing, 1 side to end, 2 end to end.
    std::array<session_cpp::Polyline, 4> volumes; // [0] and [1] on the first beam, [2] and [3] on the second.


    /// The scene this record belongs to; throws std::logic_error before the record is added to one.
    WoodSession& session() const;

    /// True once the record has been stored in a scene.
    bool has_session() const {
        return _session != nullptr;
    }

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
