#pragma once

#include "pch.h"

#include "wood_feature_construction.h"

namespace wood_session {

/// Order-independent key for an element pair.
uint64_t pair_key(int a, int b);

/// Element pair -> joint index (last joint wins); rebuilt wherever the joint list may have changed.
std::unordered_map<uint64_t, int> joints_by_element_pair(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<InteractionFeaturePlate>& joints
);

/// Vidy method: shadow joints (link = true) between each side plate and the plate it is glued to, translated to that plate's far face.
void add_vidy_shadow_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<InteractionFeaturePlate>& joints,
    std::unordered_map<uint64_t, int>& joints_map,
    double angle
);

/// Annen method: shorten the two overlapping joint lines at a 3-plate corner by the plate thickness and clip the volumes to match.
void align_annen_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    const std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<InteractionFeaturePlate>& joints
);

/// Applies the three-valence groups, the first row [instruction], 0 = annen alignment, 1 = vidy addition, then [s0, s1, e20, e31] rows; nothing when there are none. `angle` is the parallel-normal tolerance in radians.
void link_three_valence_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<InteractionFeaturePlate>& all_joints,
    double angle
);

} // namespace wood_session
