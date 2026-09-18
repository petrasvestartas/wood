#pragma once

#include "pch.h"

namespace wood_session {

/// What the solver knows about the joints before it looks at geometry: which elements are adjacent, how each face is entered, which joint each face gets, and which corners three elements share. Config::load_joint_data reads it from the dataset's sidecar txt files; a template fills it in memory. Every list is by element position, and an empty list means "not given".
struct JointData {
    std::vector<std::pair<int, int>> adjacency; // Adjacent element pairs; empty lets the solver search.
    std::vector<std::vector<session_cpp::Vector>> insertion_vectors; // Per element, one vector per face slot: bottom, top, then the sides.
    std::vector<std::vector<int>> joint_types; // Per element, one joint id per face slot; 0 no joint, -1 let the solver decide.
    std::vector<std::vector<int>> three_valence; // First row [instruction], 0 annen alignment, 1 vidy shadow joints; then [s0, s1, e20, e31] rows.

    /// True when nothing was given.
    bool empty() const { return adjacency.empty() && insertion_vectors.empty() && joint_types.empty() && three_valence.empty(); }
};

} // namespace wood_session
