#pragma once

#include "pch.h"

namespace wood_session {

/// Topology class of a face contact from the two face indices alone (index < 2 outer, >= 2 side).
/// Not WoodJoint::joint_type: that is the solver's refined code (11/12/13/20/30/40) and needs geometry.
enum class ContactType : int {
    unknown = -1, // No plate face convention: every Block contact.
    side_side = 0, // Both faces are sides; refines to 11, 12 or 13.
    side_top = 1, // One side face and one outer face; refines to 20.
    top_top = 2, // Both outer faces; refines to 40.
    cross = 3, // The elements pass through each other: plane_to_face, CrossJoint.
    line = 4, // The two boundary polylines cross within tolerance.
};

} // namespace wood_session
