#pragma once

#include "pch.h"

#include "wood_element_plate.h"
#include "wood_interaction_feature_plate.h"

// ═══════════════════════════════════════════════════════════════════════════
// Feature detection
// ═══════════════════════════════════════════════════════════════════════════

/// Classifies one element pair as a wood joint with every tunable explicit; true fills out_joint, and out_swap_planes_1 asks the caller to swap el1's faces 0 and 1. A joint line no longer than sqrt(zero_length_squared) is degenerate; coplanar_tolerance is the squared distance within which two faces are coplanar.
bool face_to_face_wood(
    wood_session::Plate& el0,
    wood_session::Plate& el1,
    std::pair<int, int> el_ids_in,
    const std::vector<double>& joint_volume_extension,
    double limit_min_joint_length,
    double zero_length_squared,
    double coplanar_tolerance,
    double dihedral_angle_threshold,
    bool all_treated_as_rotated,
    bool rotated_joint_as_average,
    int  search_type,
    wood_session::FeaturePlate& out_joint,
    bool& out_swap_planes_1);
