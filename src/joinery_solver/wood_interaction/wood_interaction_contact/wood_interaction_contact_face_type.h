#pragma once

#include "pch.h"

namespace wood_session {

/// Topology class of a face contact from the two face indices alone (index < 2 outer, >= 2 side); not InteractionFeaturePlate::joint_type, the solver's refined code.
enum class ContactType : int {
    unknown = -1, // No plate face convention: every Block contact.
    side_side = 0, // Both faces are sides; refines to 11, 12 or 13.
    side_top = 1, // One side face and one outer face; refines to 20.
    top_top = 2, // Both outer faces; refines to 40.
    end_top = 5, // Linear end against a plate outer face.
    end_end = 4, // Both linear end faces.
    end_side = 3 // End to side for linear elements.
};

inline std::string_view to_string(ContactType type) {
    switch (type) {
        case ContactType::unknown:   return "unknown";
        case ContactType::side_side: return "side_side";
        case ContactType::side_top:  return "side_top";
        case ContactType::top_top:   return "top_top";
        case ContactType::end_top:   return "end_top";
        case ContactType::end_end:   return "end_end";
        case ContactType::end_side:  return "end_side";
    }

    return "unknown";
}

} // namespace wood_session
