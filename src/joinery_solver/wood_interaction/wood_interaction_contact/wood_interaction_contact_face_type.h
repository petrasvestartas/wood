#pragma once

#include "pch.h"

namespace wood_session {

/// Topology class of a face contact from the two face indices alone (index < 2 outer, >= 2 side).
enum class ContactType : int {
    unknown = -1,
    side_side = 0,
    side_top = 1,
    top_top = 2,
    end_top = 5,
    end_end = 4,
    end_side = 3,
    border = 6
};

inline std::string_view to_string(ContactType type) {
    switch (type) {
    case ContactType::unknown:
        return "unknown";
    case ContactType::side_side:
        return "side_side";
    case ContactType::side_top:
        return "side_top";
    case ContactType::top_top:
        return "top_top";
    case ContactType::end_top:
        return "end_top";
    case ContactType::end_end:
        return "end_end";
    case ContactType::end_side:
        return "end_side";
    case ContactType::border:
        return "border";
    }

    return "unknown";
}

}
