#pragma once

#include "pch.h"

namespace wood_session {

/// Merged cut outlines of a plate, per face: [0] the outer boundary, [1..] holes.
struct Features {
    std::vector<session_cpp::Polyline> top; // Top face: the outer outline first, then one outline per hole.
    std::vector<session_cpp::Polyline> bottom; // Bottom face: the outer outline first, then one outline per hole.
};

} // namespace wood_session
