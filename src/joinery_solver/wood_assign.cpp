// ═══════════════════════════════════════════════════════════════════════════
// wood/wood_assign.cpp — implementations of assign_joint / assign_insertion.
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_assign.h"
#include "wood_session.h"          // wood_session::globals::DISTANCE / DISTANCE_SQUARED
#include "../src/closest.h"
#include "../src/spatial_rtree.h"

#include <cfloat>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace wood_session {

// ─── File-local helpers ────────────────────────────────────────────────────
namespace {

using RTree3 = session_cpp::SpatialRTree<int, double, 3>;

// Returns {segment_index, squared_distance} of the closest segment in `poly`
// to point `p`.  Equivalent to the internal get_closest_distance() helper in
// the main-branch rtree_util.cpp (lines 56–82).
std::pair<size_t, double>
closest_segment_to_point(const session_cpp::Polyline& poly,
                         const session_cpp::Point&    p)
{
    size_t best_seg = 0;
    double best_d2  = std::numeric_limits<double>::max();
    const size_t n  = poly.point_count();
    for (size_t i = 0; i + 1 < n; i++) {
        auto seg            = session_cpp::Line::from_points(poly[i], poly[i + 1]);
        auto [cp, t, dist]  = session_cpp::Closest::line_point(seg, p);
        const double d2     = dist * dist;
        if (d2 < best_d2) { best_d2 = d2; best_seg = i; }
    }
    return {best_seg, best_d2};
}

// Writes the axis-aligned bounding box of all polyline points in `elem`,
// expanded by `inflate` on every side, into out_min[3] / out_max[3].
void element_aabb(const WoodElement& elem, double inflate,
                  double out_min[3], double out_max[3])
{
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

// Build an R-tree over element AABBs, storing the element index as payload.
RTree3 build_element_rtree(const std::vector<WoodElement>& elements, double inflate)
{
    RTree3 rtree;
    for (int ei = 0; ei < static_cast<int>(elements.size()); ei++) {
        double mn[3], mx[3];
        element_aabb(elements[ei], inflate, mn, mx);
        rtree.insert(mn, mx, ei);
    }
    return rtree;
}

// Number of side-edge slots for element ei (= segments in the top face polyline).
// Returns 0 when the element has fewer than two polylines.
size_t n_side_slots(const WoodElement& elem)
{
    return elem.polylines.size() > 1
           ? elem.polylines[1].point_count() - 1
           : 0;
}

} // anonymous namespace

// ─── assign_joint ─────────────────────────────────────────────────────────

void assign_joint(
    const std::vector<WoodElement>&        elements,
    const std::vector<session_cpp::Point>& points,
    const std::vector<int>&                point_types,
    std::vector<std::vector<int>>&         out_joint_types)
{
    const double dist   = globals::DISTANCE;
    // Same threshold as main branch: DISTANCE_SQUARED * 100 (rtree_util.cpp:227,245)
    const double d2_thr = globals::DISTANCE_SQUARED * 100.0;

    // Initialize output.  Slot layout: 0=bot, 1=top, 2..N=side edges.
    out_joint_types.clear();
    out_joint_types.resize(elements.size());
    for (size_t ei = 0; ei < elements.size(); ei++) {
        out_joint_types[ei].assign(2 + n_side_slots(elements[ei]), -1);
    }

    if (points.empty() || point_types.size() < points.size()) {
        return;
    }

    const RTree3 rtree = build_element_rtree(elements, dist);

    for (size_t pi = 0; pi < points.size(); pi++) {
        const session_cpp::Point& p = points[pi];
        const int t = point_types[pi];

        double qmin[3] = {p[0] - dist, p[1] - dist, p[2] - dist};
        double qmax[3] = {p[0] + dist, p[1] + dist, p[2] + dist};

        rtree.search(qmin, qmax, [&](const int ei) -> bool {
            const auto& elem = elements[ei];
            if (elem.polylines.size() < 2) {
                return true;
            }

            // Test top (index 1) and bottom (index 0) face polylines.
            auto [seg_top, d2_top] = closest_segment_to_point(elem.polylines[1], p);
            auto [seg_bot, d2_bot] = closest_segment_to_point(elem.polylines[0], p);

            // Pick the closer face.
            const bool   is_top  = d2_top <= d2_bot;
            const double d2_min  = is_top ? d2_top : d2_bot;
            const size_t best_seg = is_top ? seg_top : seg_bot;

            if (d2_min >= d2_thr) {
                return true;
            }

            // t < 0 → face-plane slot (top=1, bot=0)
            // t > 0 → side-edge slot  (2 + segment index)
            const int slot = (t < 0)
                             ? (is_top ? 1 : 0)
                             : static_cast<int>(2 + best_seg);

            auto& slots = out_joint_types[ei];
            if (slot >= 0 && slot < static_cast<int>(slots.size())) {
                slots[slot] = std::abs(t);
            }
            return true;
        });
    }
}

// ─── assign_insertion ─────────────────────────────────────────────────────

void assign_insertion(
    const std::vector<WoodElement>&                elements,
    const std::vector<session_cpp::Line>&          lines,
    std::vector<std::vector<session_cpp::Vector>>& out_insertion_vectors)
{
    const double dist   = globals::DISTANCE;
    // Same threshold as assign_joint / main branch.
    const double d2_thr = globals::DISTANCE_SQUARED * 100.0;

    // Initialize output.  Same slot layout as assign_joint.
    out_insertion_vectors.clear();
    out_insertion_vectors.resize(elements.size());
    for (size_t ei = 0; ei < elements.size(); ei++) {
        out_insertion_vectors[ei].assign(
    }
            2 + n_side_slots(elements[ei]),
            session_cpp::Vector(0.0, 0.0, 0.0));

    if (lines.empty()) {
        return;
    }

    const RTree3 rtree = build_element_rtree(elements, dist);

    for (const auto& line : lines) {
        // Only the start point is used for the closest-point search;
        // the full line direction is what gets stored (main branch rtree_util.cpp:133–150).
        const session_cpp::Point  p0  = line.start();
        const session_cpp::Vector dir = line.to_vector();

        double qmin[3] = {p0[0] - dist, p0[1] - dist, p0[2] - dist};
        double qmax[3] = {p0[0] + dist, p0[1] + dist, p0[2] + dist};

        rtree.search(qmin, qmax, [&](const int ei) -> bool {
            const auto& elem = elements[ei];
            if (elem.polylines.size() < 2) {
                return true;
            }

            auto [seg_top, d2_top] = closest_segment_to_point(elem.polylines[1], p0);
            auto [seg_bot, d2_bot] = closest_segment_to_point(elem.polylines[0], p0);

            const double d2_min  = std::min(d2_top, d2_bot);
            const size_t best_seg = d2_top <= d2_bot ? seg_top : seg_bot;

            if (d2_min >= d2_thr) {
                return true;
            }

            auto& slots = out_insertion_vectors[ei];
            const int slot = static_cast<int>(best_seg + 2);
            if (slot < static_cast<int>(slots.size())) {
                slots[slot] = dir;
            }
            return true;
        });
    }
}

} // namespace wood_session
