#pragma once

#include "pch.h"

namespace wood_session {

/// How forces pass between the edge's two elements; nothing computed yet, the record and its message are reserved.
struct InteractionStructure {

    /// The structure as JSON: type only.
    nlohmann::ordered_json jsondump() const { return nlohmann::ordered_json{{"type", "InteractionStructure"}}; }

    /// A structure from its JSON.
    static InteractionStructure jsonload(const nlohmann::json&) { return InteractionStructure{}; }

    /// The structure as wood_proto.InteractionStructure bytes: empty.
    std::string pb_dumps() const {
        return std::string();
    }

    /// A structure from wood_proto.InteractionStructure bytes.
    static InteractionStructure pb_loads(const std::string&) { return InteractionStructure{}; }

    /// "InteractionStructure()".
    std::string str() const {
        return "InteractionStructure()";
    }
};

} // namespace wood_session
