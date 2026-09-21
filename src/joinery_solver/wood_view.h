#pragma once

#include "pch.h"

#include "wood_interaction_contact_face_type.h"

namespace wood_session {

class WoodSession;

/// Arranges a scene for the viewer, one group per element: the element itself, then `attributes`, `contacts` and `joints` child groups, each flag adding or leaving out that part; pb_dump writes it.
void add_to_tree(WoodSession& scene, bool with_geometry = true, bool with_attributes = true, bool with_contacts = true, bool with_joints = true);

/// The geometry features of every grouped element (plate outlines, beam and column axis and sections, the centroid of each) drawn under an `attributes` child group, or every such group and its objects removed; the toggle for what is a lookup aid, not the element. Runs sync_geometry first, since the features come from compute_geometry().
void show_attributes(WoodSession& scene, bool on);

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
