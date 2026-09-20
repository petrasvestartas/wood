#pragma once

#include "pch.h"

#include "wood_element_plate.h"
#include "wood_settings.h"
#include "wood_interaction_feature_plate.h"

// ═══════════════════════════════════════════════════════════════════════════
// Feature detection
// ═══════════════════════════════════════════════════════════════════════════

/// Classifies one plate pair as a wood joint; true fills out_joint, and out_swap_planes_1 asks the caller to swap el1's faces 0 and 1. Tolerances, extensions and thresholds come from settings; `search_type` picks face-to-face, cross or both.
bool face_to_face_wood(
    wood_session::Plate& el0,
    wood_session::Plate& el1,
    std::pair<int, int> el_ids_in,
    const wood_session::Settings& settings,
    int search_type,
    wood_session::FeaturePlate& out_joint,
    bool& out_swap_planes_1);
