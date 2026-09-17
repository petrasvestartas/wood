#include "wood_pch.h"
#include "wood_assign.h"
#include "wood_session.h"
namespace wood_session {

namespace {

using session_cpp::Line;
using session_cpp::Point;
using session_cpp::Polyline;
using session_cpp::Vector;
using RTree3 = session_cpp::SpatialRTree<int, double, 3>;

/// The segment of poly nearest to point, and the squared distance to it.
std::pair<size_t, double> compute_closest_segment(const Polyline& poly, const Point& point) {
    size_t segment = 0;
    double distance = std::numeric_limits<double>::max();
    const std::vector<Line> lines = poly.get_lines();
    for (size_t i = 0; i < lines.size(); i++) {
        const Vector ab = lines[i].to_vector();
        const Vector ap = point - lines[i].start();
        const double len2 = ab.magnitude_squared();
        const double parameter = std::clamp(len2 > 0.0 ? ap.dot(ab) / len2 : 0.0, 0.0, 1.0);
        const double d2 = (ap - ab * parameter).magnitude_squared();
        if (d2 < distance) {
            distance = d2;
            segment = i;
        }
    }
    return {segment, distance};
}

void compute_element_aabb(const Plate& elem, double inflate, double out_min[3], double out_max[3]) {
    for (int k = 0; k < 3; k++) {
        out_min[k] = DBL_MAX;
        out_max[k] = -DBL_MAX;
    }
    for (const Polyline& poly : elem.polylines) {
        for (size_t i = 0; i < poly.point_count(); i++) {
            const Point pt = poly[i];
            for (int k = 0; k < 3; k++) {
                out_min[k] = std::min(out_min[k], pt[k] - inflate);
                out_max[k] = std::max(out_max[k], pt[k] + inflate);
            }
        }
    }
}

void compute_element_rtree(const std::vector<std::shared_ptr<Plate>>& elements, double inflate, RTree3& rtree) {
    for (int ei = 0; ei < static_cast<int>(elements.size()); ei++) {
        double mn[3];
        double mx[3];
        compute_element_aabb(*elements[ei], inflate, mn, mx);
        if (mn[0] > mx[0])
            continue;
        rtree.insert(mn, mx, ei);
    }
}

size_t get_side_slots(const Plate& elem) {
    const size_t n = elem.polylines.size() > 1 ? elem.polylines[1].point_count() : 0;
    return n > 0 ? n - 1 : 0;
}

}  // namespace

void assign_joint(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<session_cpp::Point>& points,
    const std::vector<int>& point_types,
    std::vector<std::vector<int>>& out_joint_types
) {
    const double threshold = globals::DISTANCE_SQUARED * 100.0;
    const double radius = std::max(globals::DISTANCE, std::sqrt(threshold));

    out_joint_types.clear();
    out_joint_types.resize(elements.size());
    for (size_t ei = 0; ei < elements.size(); ei++)
        out_joint_types[ei].assign(2 + get_side_slots(*elements[ei]), -1);

    if (points.empty() || point_types.size() < points.size())
        return;

    RTree3 rtree;
    compute_element_rtree(elements, radius, rtree);

    for (size_t pi = 0; pi < points.size(); pi++) {
        const Point& point = points[pi];
        const int type = point_types[pi];
        const double qmin[3] = {point[0] - radius, point[1] - radius, point[2] - radius};
        const double qmax[3] = {point[0] + radius, point[1] + radius, point[2] + radius};
        rtree.search(qmin, qmax, [&](const int ei) -> bool {
            const Plate& elem = *elements[ei];
            if (elem.polylines.size() < 2)
                return true;
            const auto [seg_top, d2_top] = compute_closest_segment(elem.polylines[1], point);
            const auto [seg_bot, d2_bot] = compute_closest_segment(elem.polylines[0], point);
            const bool is_top = d2_top <= d2_bot;
            const double d2_min = is_top ? d2_top : d2_bot;
            const size_t segment = is_top ? seg_top : seg_bot;
            if (d2_min >= threshold)
                return true;
            const int slot = type < 0 ? (is_top ? 1 : 0) : static_cast<int>(2 + segment);
            std::vector<int>& slots = out_joint_types[ei];
            if (slot >= 0 && slot < static_cast<int>(slots.size()))
                slots[slot] = std::abs(type);
            return true;
        });
    }
}

void assign_insertion(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<session_cpp::Line>& lines,
    std::vector<std::vector<session_cpp::Vector>>& out_insertion_vectors
) {
    const double threshold = globals::DISTANCE_SQUARED * 100.0;
    const double radius = std::max(globals::DISTANCE, std::sqrt(threshold));

    out_insertion_vectors.clear();
    out_insertion_vectors.resize(elements.size());
    for (size_t ei = 0; ei < elements.size(); ei++)
        out_insertion_vectors[ei].assign(2 + get_side_slots(*elements[ei]), Vector(0.0, 0.0, 0.0));

    if (lines.empty())
        return;

    RTree3 rtree;
    compute_element_rtree(elements, radius, rtree);

    for (const Line& line : lines) {
        const Point point = line.start();
        const Vector direction = line.to_vector();
        const double qmin[3] = {point[0] - radius, point[1] - radius, point[2] - radius};
        const double qmax[3] = {point[0] + radius, point[1] + radius, point[2] + radius};
        rtree.search(qmin, qmax, [&](const int ei) -> bool {
            const Plate& elem = *elements[ei];
            if (elem.polylines.size() < 2)
                return true;
            const auto [seg_top, d2_top] = compute_closest_segment(elem.polylines[1], point);
            const auto [seg_bot, d2_bot] = compute_closest_segment(elem.polylines[0], point);
            const double d2_min = std::min(d2_top, d2_bot);
            const size_t segment = d2_top <= d2_bot ? seg_top : seg_bot;
            if (d2_min >= threshold)
                return true;
            std::vector<Vector>& slots = out_insertion_vectors[ei];
            const int slot = static_cast<int>(segment + 2);
            if (slot < static_cast<int>(slots.size()))
                slots[slot] = direction;
            return true;
        });
    }
}

}  // namespace wood_session
