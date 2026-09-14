#pragma once
#include <vector>
#include "wood_element.h"
#include "../src/point.h"
#include "../src/line.h"
#include "../src/vector.h"

namespace wood_session {

/// Assign absolute joint types to bottom, top, or side slots; unassigned slots remain -1.
void assign_joint(
    const std::vector<WoodElement>&         elements,
    const std::vector<session_cpp::Point>&  points,
    const std::vector<int>&                 point_types,
    std::vector<std::vector<int>>&          out_joint_types
);

/// Assign line directions to side slots using the nearest segment to each line start.
void assign_insertion(
    const std::vector<WoodElement>&                elements,
    const std::vector<session_cpp::Line>&          lines,
    std::vector<std::vector<session_cpp::Vector>>& out_insertion_vectors
);

}
