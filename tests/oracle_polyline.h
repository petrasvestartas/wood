#pragma once

#include "wood_session.h"
#include "wood_element_geometry.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Loop validity - what the oracle tests ask of a merged outline and the kernel does not offer
// (Polyline::is_planar, Polyline::is_simple and a point-set distance are kernel gaps; written once here)
// ═══════════════════════════════════════════════════════════════════════════

namespace oracle {

/// The gap between the first and the last point of a loop; zero when it is closed.
inline double closing_gap(const Polyline& loop) {

    return loop.get_point(0).distance(loop.get_point(loop.point_count() - 1));
}

/// The largest distance of a point of the loop from the plane through its first point with the loop's Newell normal.
inline double planarity(const Polyline& loop) {

    const std::vector<Point> points = loop.get_points();
    const Plane plane = Plane::from_point_normal(points[0], compute_newell(points));
    double worst = 0.0;

    for (const Point& point : points)
        worst = std::max(worst, std::abs(plane.signed_distance(point)));

    return worst;
}

/// The number of consecutive point pairs closer than tolerance, the closing pair included.
inline size_t consecutive_duplicates(const Polyline& loop, double tolerance) {

    const std::vector<Point> points = loop.get_points();
    size_t count = 0;

    for (size_t i = 1; i < points.size(); i++)
        if (points[i].distance(points[i - 1]) < tolerance)
            count++;

    return count;
}

/// True when no two non-adjacent segments of the loop come within tolerance of each other; the first and the last segment of a closed loop count as adjacent.
inline bool is_simple(const Polyline& loop, double tolerance) {

    const std::vector<Line> segments = loop.get_lines();
    const size_t n = segments.size();
    const bool closed = closing_gap(loop) < tolerance;

    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 2; j < n; j++) {
            if (closed && i == 0 && j == n - 1)
                continue;
            if (segments[i].length() < tolerance || segments[j].length() < tolerance)
                continue;
            double t0 = 0.0;
            double t1 = 0.0;
            if (!Intersection::line_line_parameters(segments[i], segments[j], t0, t1, 0.0, true, true))
                continue;
            if (segments[i].point_at(t0).distance(segments[j].point_at(t1)) < tolerance)
                return false;
        }
    }

    return true;
}

/// The largest distance from a point of one loop to the nearest point of the other, either way: zero when the two hold the same points in any order.
inline double point_set_distance(const Polyline& a, const Polyline& b) {

    const std::vector<Point> pa = a.get_points();
    const std::vector<Point> pb = b.get_points();
    double worst = 0.0;

    for (const Point& p : pa) {
        double nearest = std::numeric_limits<double>::infinity();
        for (const Point& q : pb)
            nearest = std::min(nearest, p.distance(q));
        worst = std::max(worst, nearest);
    }
    for (const Point& q : pb) {
        double nearest = std::numeric_limits<double>::infinity();
        for (const Point& p : pa)
            nearest = std::min(nearest, q.distance(p));
        worst = std::max(worst, nearest);
    }

    return worst;
}

/// The points of a loop without the closing repeat: what two loops are compared by.
inline size_t open_point_count(const Polyline& loop) {

    return loop.point_count() - (loop.point_count() > 1 && closing_gap(loop) < 1e-9 ? 1 : 0);
}

/// The merged outlines of a plate after joinery, top then bottom, outer first then holes; the plate's own two outlines before any joint.
inline std::vector<Polyline> merged_loops(const Plate& plate) {

    std::vector<Polyline> loops;

    if (plate.features.top.empty()) {
        loops.push_back(plate.polylines[1]);
        loops.push_back(plate.polylines[0]);
        return loops;
    }

    loops.insert(loops.end(), plate.features.top.begin(), plate.features.top.end());
    loops.insert(loops.end(), plate.features.bottom.begin(), plate.features.bottom.end());

    return loops;
}

} // namespace oracle
