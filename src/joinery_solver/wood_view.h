#pragma once

#include "pch.h"

#include "wood_interaction_contact_face_type.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Colours
// ═══════════════════════════════════════════════════════════════════════════

/// "side_side" / "side_top" / "top_top" / "unknown" - the group name a face contact of that class is filed under.
std::string_view contact_type_name(ContactType type);

/// "ss_ip_12" / "ss_op_11" / "ss_rot_13" / "ts_20" / "cross_30" / "tt_40", or "type_<n>" for a code the table does not name.
std::string joint_type_name(int joint_type);

/// The colour of a face contact class: the colour of the joint type it refines to, grey when unknown.
session_cpp::Color contact_color(ContactType type);

/// The colour of a joint type: 12 navy, 11 orange, 13 deep pink, 20 pink, 40 green, 30 yellow, grey otherwise.
session_cpp::Color joint_color(int joint_type);

} // namespace wood_session
