#include "pch.h"
#include "wood_view.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Names
// ═══════════════════════════════════════════════════════════════════════════

std::string_view contact_type_name(ContactType type) {

    switch (type) {
        case ContactType::side_side: return "side_side";
        case ContactType::side_top: return "side_top";
        case ContactType::top_top: return "top_top";
        case ContactType::unknown: break;
    }

    return "unknown";
}

std::string joint_type_name(int joint_type) {

    switch (joint_type) {
        case 11: return "ss_op_11";
        case 12: return "ss_ip_12";
        case 13: return "ss_rot_13";
        case 20: return "ts_20";
        case 30: return "cross_30";
        case 40: return "tt_40";
        default: return fmt::format("type_{}", joint_type);
    }
}

} // namespace wood_session
