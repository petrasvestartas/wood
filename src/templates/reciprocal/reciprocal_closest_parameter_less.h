#pragma once
#include "line.h"

#include <vector>

namespace session_cpp {

/// Orders point indices by their closest-point parameter along line.
struct ClosestParameterLess {
    const Line&               line;    // the line the parameters are measured on
    const std::vector<Point>& points;  // the points the indices refer to

    bool operator()(int a, int b) const {
        return line.closest_point(points[a], false).first <
               line.closest_point(points[b], false).first;
    }
};

} // namespace session_cpp
