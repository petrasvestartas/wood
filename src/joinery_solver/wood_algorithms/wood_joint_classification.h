#pragma once

#include "pch.h"
#include "wood_element_edge_straight.h"
#include "wood_joint_category.h"

using namespace laika_cpp;

namespace wood_session {

/// How two straight members meet: read from the closest points of their axes and how near each lies to an end of its axis.
struct JointClassification {
    JointCategory category = JointCategory::none; // none when the members do not touch.
    JointFamily family = JointFamily::none; // The compendium's family of the category.
    Point point; // Where they meet, on the first member's axis.
    std::array<bool, 2> ends = {false, false}; // Whether each member meets there with an end.
};

/// The joint two beams need, read from their axes: an end is a closest point within the other's half section of an axis end;
/// axes within 5 degrees of parallel splice when in line at both ends, else lie side by side; otherwise two ends make a corner,
/// one end on a side a t (within 5 degrees of square) or an angled joint, two sides a cross.
JointClassification classify_joint(const EdgeStraightElement& a, const EdgeStraightElement& b, double tolerance = 1.0);

/// The category of a plate joint the solver detected, by its joint_type (11 side_side_outplane, 12 side_side_inplane, 13 side_side_rotated, 20 side_top, 30 cross, 40 top_top, 60 border).
JointCategory plate_category(int joint_type);

/// The compendium family of a category; a side_end at an angle is angled, not t.
JointFamily joint_family(JointCategory category, bool square = true);

}
