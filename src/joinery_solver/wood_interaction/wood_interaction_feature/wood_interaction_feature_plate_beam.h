#pragma once

#include "pch.h"

namespace wood_session {

/// A plate-to-beam joint; nothing computed yet, the record and its message are reserved.
struct FeaturePlateBeam {

    /// The joint as JSON: type only.
    nlohmann::ordered_json jsondump() const { return nlohmann::ordered_json{{"type", "FeaturePlateBeam"}}; }

    /// A joint from its JSON.
    static FeaturePlateBeam jsonload(const nlohmann::json&) { return FeaturePlateBeam{}; }

    /// The joint as wood_proto.FeaturePlateBeam bytes: empty.
    std::string pb_dumps() const {
        return std::string();
    }

    /// A joint from wood_proto.FeaturePlateBeam bytes.
    static FeaturePlateBeam pb_loads(const std::string&) { return FeaturePlateBeam{}; }

    /// "FeaturePlateBeam()".
    std::string str() const {
        return "FeaturePlateBeam()";
    }
};

} // namespace wood_session
