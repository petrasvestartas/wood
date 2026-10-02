#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

/// The unit vector from a to b.
static Vector unit(const Point& a, const Point& b) {
    return (b - a).normalized();
}

// ═══════════════════════════════════════════════════════════════════════════
// Sizes
// ═══════════════════════════════════════════════════════════════════════════

double FloorSizes::static_h() const {
    return height - rise;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plan
// ═══════════════════════════════════════════════════════════════════════════

FloorPlan FloorPlan::rectangle(double half_x, double half_y, double oculus, OculusRule rule) {
    return quadrilateral({Point(-half_x, -half_y, 0.0), Point(half_x, -half_y, 0.0), Point(half_x, half_y, 0.0), Point(-half_x, half_y, 0.0)}, oculus, rule);
}

FloorPlan FloorPlan::quadrilateral(const std::array<Point, 4>& corners, double oculus, OculusRule rule) {

    FloorPlan plan;
    plan.corners = corners;
    plan.oculus = oculus;
    plan.rule = rule;

    return plan;
}

Point FloorPlan::centre() const {
    return Point::centroid({corners[0], corners[1], corners[2], corners[3]});
}

Point FloorPlan::midpoint(size_t k) const {
    return Line::from_points(corners[k % 4], corners[(k + 1) % 4]).center();
}

double FloorPlan::corner_angle(size_t k) const {

    const Vector after = unit(corners[k % 4], corners[(k + 1) % 4]);
    const Vector before = unit(corners[k % 4], corners[(k + 3) % 4]);

    return std::acos(std::clamp(after.dot(before), -1.0, 1.0)) * 180.0 / M_PI;
}

std::array<Point, 4> FloorPlan::oculus_corners() const {

    const Point c = centre();
    std::array<Point, 4> result;

    for (size_t q = 0; q < 4; q++) {
        const Point m = midpoint(q);
        double distance = oculus;

        if (rule == OculusRule::compas)
            distance = oculus * (m - c).magnitude() / std::sqrt((midpoint(q + 3) - c).magnitude() * (midpoint(q + 1) - c).magnitude());
        else if (rule == OculusRule::explicit_distances)
            distance = oculus_distances[q];

        result[q] = c + unit(c, m) * distance;
    }

    return result;
}

/// The angle in degrees between two directions.
static double angle_between(const Vector& a, const Vector& b) {
    return std::acos(std::clamp(a.normalized().dot(b.normalized()), -1.0, 1.0)) * 180.0 / M_PI;
}

double FloorPlan::oculus_corner_angle(size_t k) const {

    const std::array<Point, 4> o = oculus_corners();

    return angle_between(o[(k + 3) % 4] - o[k % 4], o[(k + 1) % 4] - o[k % 4]);
}

double FloorPlan::oculus_seam_angle(size_t k) const {

    const std::array<Point, 4> o = oculus_corners();

    return angle_between(midpoint(k) - centre(), o[(k + 1) % 4] - o[k % 4]);
}

bool FloorPlan::valid(std::string& why) const {

    const Point c = centre();
    const std::array<Point, 4> oculus_points = oculus_corners();

    for (size_t k = 0; k < 4; k++) {
        const Vector after = corners[(k + 1) % 4] - corners[k];
        const Vector before = corners[(k + 3) % 4] - corners[k];
        const double turn = after.cross(corners[(k + 2) % 4] - corners[(k + 1) % 4])[2];

        if (std::abs(corners[k][2]) > 0.0) {
            why = fmt::format("corner {} is not at z 0", k);
            return false;
        }

        if (turn <= 0.0) {
            why = fmt::format("the corners are not counter-clockwise and convex at corner {}", (k + 1) % 4);
            return false;
        }

        if (after.magnitude() <= 0.0 || before.magnitude() <= 0.0) {
            why = fmt::format("corner {} repeats its neighbour", k);
            return false;
        }

        const double along = (oculus_points[k] - c).dot(unit(c, midpoint(k)));

        if (along <= 0.0 || along >= (midpoint(k) - c).magnitude()) {
            why = fmt::format("oculus corner {} is not between the centre and the midpoint of edge {}", k, k);
            return false;
        }
    }

    for (size_t k = 0; k < 4; k++)
        if (std::sin(oculus_corner_angle(k) * M_PI / 180.0) < std::sin(oculus_seam_angle(k) * M_PI / 180.0)) {
            why = fmt::format("the ring beam leaves quarter {}'s oculus beam face uncovered at oculus corner {}: corner angle {:.3f}, seam angle {:.3f} degrees", (k + 1) % 4, k, oculus_corner_angle(k), oculus_seam_angle(k));
            return false;
        }

    why.clear();

    return true;
}

}
