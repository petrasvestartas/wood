#pragma once

#include "pch.h"

#include "wood_feature_construction.h"

namespace wood_session {

/// Order-independent key for an element pair.
uint64_t pair_key(int a, int b);

/// Element pair -> joint index (last joint wins); rebuilt wherever the joint list may have changed.
std::unordered_map<uint64_t, int> joints_by_element_pair(
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::vector<InteractionFeaturePlate>& joints
);

/// Element pair -> the indices of all its joints in detection order, as a pair listed twice in the adjacency has two.
std::unordered_map<uint64_t, std::vector<int>> joints_of_element_pairs(
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::vector<InteractionFeaturePlate>& joints
);

/// The two joints a three-valence row {a, b} means when either pair has several: the two whose joint lines overlap the most, the first of each when none do; false when either pair has none.
bool row_joints(
    const std::unordered_map<uint64_t, std::vector<int>>& pairs,
    uint64_t a,
    uint64_t b,
    const std::vector<InteractionFeaturePlate>& joints,
    int& joint_a,
    int& joint_b
);

/// Vidy method.
void add_vidy_shadow_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    std::vector<InteractionFeaturePlate>& joints,
    std::unordered_map<uint64_t, int>& joints_map,
    double angle
);

/// Annen method: shorten the two overlapping joint lines at a 3-plate corner by the plate thickness and clip the volumes to match.
void align_annen_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    std::vector<InteractionFeaturePlate>& joints
);

/// Applies the three-valence groups.
void link_three_valence_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    std::vector<InteractionFeaturePlate>& all_joints,
    double angle
);

}
