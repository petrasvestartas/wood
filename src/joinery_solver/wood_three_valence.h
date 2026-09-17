#pragma once

#include "wood_pch.h"

#include "wood_joint.h"

namespace wood_session {

/// Order-independent key for an element pair.
uint64_t pair_key(int a, int b);

/// Element pair -> joint index (last joint wins); rebuilt wherever the joint list may have changed.
std::unordered_map<uint64_t, int> joints_by_element_pair(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<WoodJoint>& joints
);

/// Vidy method: shadow joints (link = true) between each side plate and the plate it is glued to, translated to that plate's far face.
void add_vidy_shadow_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& joints,
    std::unordered_map<uint64_t, int>& joints_map
);

/// Annen method: shorten the two overlapping joint lines at a 3-plate corner by the plate thickness and clip the volumes to match.
void align_annen_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    const std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& joints
);

/// In-memory three-valence groups for the next link_three_valence_joints on this thread: first row [instruction], then [s0, s1, e20, e31] rows.
void set_three_valence_override(std::vector<std::vector<int>> groups);

/// Drops the in-memory groups; the next link_three_valence_joints reads the sidecar again.
void clear_three_valence_override();

/// Three-valence groups from the sidecar or the thread-local override; first row's first value 0 = annen alignment, 1 = vidy addition.
void link_three_valence_joints(
    const std::string& three_valence_name,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& all_joints
);

} // namespace wood_session
