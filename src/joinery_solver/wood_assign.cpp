#include "wood_assign.h"
#include "wood_session.h"
#include "../src/spatial_rtree.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace wood_session {

namespace {

using RTree3 = session_cpp::SpatialRTree<int, double, 3>;

std::pair<size_t, double> compute_closest_segment(const session_cpp::Polyline& poly, const session_cpp::Point& point) {
    size_t segment = 0;
    double distance = std::numeric_limits<double>::max();
    const size_t count = poly.point_count();
    const double qx = point[0];
    const double qy = point[1];
    const double qz = point[2];
    for (size_t i = 0; i + 1 < count; i++) {
        const session_cpp::Point a = poly.get_point(i);
        const session_cpp::Point b = poly.get_point(i + 1);
        const double ax = a[0];
        const double ay = a[1];
        const double az = a[2];
        const double bx = b[0];
        const double by = b[1];
        const double bz = b[2];
        const double abx = bx - ax;
        const double aby = by - ay;
        const double abz = bz - az;
        const double apx = qx - ax;
        const double apy = qy - ay;
        const double apz = qz - az;
        const double len2 = abx*abx + aby*aby + abz*abz;
        double parameter = len2 > 0.0 ? (apx*abx + apy*aby + apz*abz) / len2 : 0.0;
        parameter = std::clamp(parameter, 0.0, 1.0);
        const double dx = apx - parameter * abx;
        const double dy = apy - parameter * aby;
        const double dz = apz - parameter * abz;
        const double d2 = dx*dx + dy*dy + dz*dz;
        if (d2 < distance) {
            distance = d2;
            segment = i;
        }
    }
    return {segment, distance};
}

void compute_element_aabb(const Plate& elem, double inflate, double out_min[3], double out_max[3]) {
    for (int k = 0; k < 3; k++) {
        out_min[k] =  DBL_MAX;
        out_max[k] = -DBL_MAX;
    }
    for (const auto& poly : elem.polylines) {
        for (size_t i = 0; i < poly.point_count(); i++) {
            const auto& pt = poly[i];
            out_min[0] = std::min(out_min[0], pt[0] - inflate);
            out_min[1] = std::min(out_min[1], pt[1] - inflate);
            out_min[2] = std::min(out_min[2], pt[2] - inflate);
            out_max[0] = std::max(out_max[0], pt[0] + inflate);
            out_max[1] = std::max(out_max[1], pt[1] + inflate);
            out_max[2] = std::max(out_max[2], pt[2] + inflate);
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

}

void assign_joint(
    const std::vector<std::shared_ptr<Plate>>&        elements,
    const std::vector<session_cpp::Point>& points,
    const std::vector<int>&                point_types,
    std::vector<std::vector<int>>&         out_joint_types)
{
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
        const session_cpp::Point& point = points[pi];
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

            const int slot = (type < 0)
                             ? (is_top ? 1 : 0)
                             : static_cast<int>(2 + segment);

            auto& slots = out_joint_types[ei];
            if (slot >= 0 && slot < static_cast<int>(slots.size()))
                slots[slot] = std::abs(type);
            return true;
        });
    }
}

void assign_insertion(
    const std::vector<std::shared_ptr<Plate>>&                elements,
    const std::vector<session_cpp::Line>&          lines,
    std::vector<std::vector<session_cpp::Vector>>& out_insertion_vectors)
{
    const double threshold = globals::DISTANCE_SQUARED * 100.0;
    const double radius = std::max(globals::DISTANCE, std::sqrt(threshold));

    out_insertion_vectors.clear();
    out_insertion_vectors.resize(elements.size());
    for (size_t ei = 0; ei < elements.size(); ei++)
        out_insertion_vectors[ei].assign(2 + get_side_slots(*elements[ei]), session_cpp::Vector(0.0, 0.0, 0.0));

    if (lines.empty())
        return;

    RTree3 rtree;
    compute_element_rtree(elements, radius, rtree);

    for (const auto& line : lines) {
        const session_cpp::Point point = line.start();
        const session_cpp::Vector direction = line.to_vector();

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

            auto& slots = out_insertion_vectors[ei];
            const int slot = static_cast<int>(segment + 2);
            if (slot < static_cast<int>(slots.size()))
                slots[slot] = direction;
            return true;
        });
    }
}

}
