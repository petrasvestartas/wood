#pragma once

#include "wood_pch.h"

#include "wood_joint.h"

using JointMembership = std::vector<std::vector<std::vector<std::pair<int, bool>>>>;

/// Stitches the oriented joint cut outlines into the element's top and bottom outlines: [hole0_top, hole0_bot, ..., merged_top, merged_bot].
std::vector<session_cpp::Polyline> merge_joints_for_element(
    const wood_session::Plate& element,
    const std::vector<std::vector<std::pair<int, bool>>>& membership,
    std::vector<wood_session::WoodJoint>& joints,
    int element_id
);
