#include "wood_pch.h"
#include "wood_merge.h"
#include "wood_session.h"
#include "wood_cut.h"
using namespace session_cpp;
using wood_session::WoodJoint;
using wood_session::Plate;

constexpr bool TRACE = false;

namespace {

// ═══════════════════════════════════════════════════════════════════════════
// Shared state
// ═══════════════════════════════════════════════════════════════════════════

/// Sort keys pack the edge id (scaled by 1e6) with the sub-edge fraction (scaled by 1e3).
constexpr double scale_0 = 1000000.0;
constexpr double scale_1 = 1000.0;

/// Joint point runs keyed by plate edge; a multimap, so joints sharing a key on one edge all survive.
using SortedRuns = std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>>;
using JointOutlines = std::array<std::vector<Polyline>, 2>;
using ElementJmf = std::vector<std::vector<std::pair<int, bool>>>;

/// Locals shared by the merge passes of one element.
struct MergeState {
    std::ofstream* log = nullptr;
    int element_id = -1;
    std::vector<Point> pline0;
    std::vector<Point> pline1;
    std::vector<Plane> joint_planes;
    /// First points before relocation: the closing duplicate is never relocated, so closure is tested against these.
    Point pline0_orig_front;
    Point pline1_orig_front;
    double distance_squared = 0.0;
    SortedRuns sorted0;
    SortedRuns sorted1;
    int last_id = -1;
    /// First and last joint lines, top and bottom: the closing corner moves to their intersection.
    std::array<Point, 2> last_segment0_start{{Point(0, 0, 0), Point(0, 0, 0)}};
    std::array<Point, 2> last_segment1_start{{Point(0, 0, 0), Point(0, 0, 0)}};
    std::array<Point, 2> last_segment0{{Point(0, 0, 0), Point(0, 0, 0)}};
    std::array<Point, 2> last_segment1{{Point(0, 0, 0), Point(0, 0, 0)}};
};

/// Relocated plate corners at a line joint's two ends, top and bottom.
struct Case2Corners {
    Point p0_int;
    Point p1_int;
    Point p2_int;
    Point p3_int;
    bool is_intersected_0 = false;
    bool is_intersected_1 = false;
    bool is_intersected_2 = false;
    bool is_intersected_3 = false;
};

/// Squared perpendicular distance from p to the infinite line through la and lb.
double perp_dist_sq(const Point& p, const Point& la, const Point& lb) {
    const Vector d = lb - la;
    const double l2 = d.magnitude_squared();
    if (l2 < 1e-20)
        return 0.0;
    const double t = (p - la).dot(d) / l2;
    const Point pr = la + d * t;
    return (p - pr).magnitude_squared();
}

// ═══════════════════════════════════════════════════════════════════════════
// Diagnostic log
// ═══════════════════════════════════════════════════════════════════════════

/// Opens data/output/merge.txt in append mode when tracing; null when off.
std::ofstream* merge_open_log(std::ofstream& owned) {
    if (TRACE) {
        owned.open((internal::output_dir() / "merge.txt").string(), std::ios::app);
        if (owned.is_open())
            return &owned;
    }
    return nullptr;
}

void merge_log_points(std::ofstream& log, const Polyline& pl) {
    for (size_t k = 0; k < pl.point_count(); k++) {
        const Point p = pl.get_point(k);
        log << " (" << p[0] << "," << p[1] << "," << p[2] << ")";
    }
}

/// Writes the element header (planes and both plate outlines) to the log.
void merge_log_element(const MergeState& st, const Plate& el) {
    if (!st.log || st.element_id < 0)
        return;
    std::ofstream& log = *st.log;
    log << "ELEMENT " << st.element_id
        << " planes0_o=(" << el.planes[0].origin()[0] << "," << el.planes[0].origin()[1] << "," << el.planes[0].origin()[2]
        << ") planes0_n=(" << el.planes[0].z_axis()[0] << "," << el.planes[0].z_axis()[1] << "," << el.planes[0].z_axis()[2]
        << ") planes1_o=(" << el.planes[1].origin()[0] << "," << el.planes[1].origin()[1] << "," << el.planes[1].origin()[2]
        << ") planes1_n=(" << el.planes[1].z_axis()[0] << "," << el.planes[1].z_axis()[1] << "," << el.planes[1].z_axis()[2]
        << ")\n";
    log << "  pline0 pts=" << el.polylines[0].point_count();
    merge_log_points(log, el.polylines[0]);
    log << "\n  pline1 pts=" << el.polylines[1].point_count();
    merge_log_points(log, el.polylines[1]);
    log << "\n";
}

/// Writes the merged top/bottom outlines to the log.
void merge_log_result(const MergeState& st, const Polyline& merged_top, const Polyline& merged_bot) {
    if (!st.log)
        return;
    std::ofstream& log = *st.log;
    log << "  MERGED el=" << st.element_id
        << " top.n=" << merged_top.point_count()
        << " bot.n=" << merged_bot.point_count()
        << (merged_top.point_count() != merged_bot.point_count() ? " COUNT_MISMATCH" : "")
        << "\n";
    log << "  merged_top:";
    merge_log_points(log, merged_top);
    log << "\n  merged_bot:";
    merge_log_points(log, merged_bot);
    log << "\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// Side joints
// ═══════════════════════════════════════════════════════════════════════════

/// Selects the joint's male/female outlines, checks the endpoint markers and swaps top/bottom when reversed; null means skip.
JointOutlines* merge_joint_outlines(
    const MergeState& st,
    const Plate& el,
    WoodJoint& jt,
    size_t i,
    int joint_id,
    bool male_or_female
) {
    JointOutlines& jm = male_or_female ? jt.m_outlines : jt.f_outlines;
    if (jm[0].size() < 2 || jm[1].size() < 2)
        return nullptr;
    if (jm[0][1].point_count() < 2 || jm[1][1].point_count() < 2)
        return nullptr;
    const Point ep_top0 = jm[0][1].get_point(0);
    const Point ep_bot0 = jm[1][1].get_point(0);
    const double d_top = Point::distance(ep_top0, el.planes[0].project(ep_top0));
    const double d_bot = Point::distance(ep_bot0, el.planes[0].project(ep_bot0));
    const bool is_geo_reversed = (d_top * d_top) > (d_bot * d_bot);
    if (st.log) {
        *st.log << "  J el=" << st.element_id << " i=" << i
                << " jid=" << joint_id << " mf=" << (male_or_female ? 'M' : 'F')
                << " jt=" << jt.joint_type
                << " ep_top0=(" << ep_top0[0] << "," << ep_top0[1] << "," << ep_top0[2]
                << ") ep_bot0=(" << ep_bot0[0] << "," << ep_bot0[1] << "," << ep_bot0[2]
                << ") d_top=" << d_top << " d_bot=" << d_bot
                << " reversed=" << (is_geo_reversed ? 1 : 0);
    }
    if (is_geo_reversed)
        std::swap(jm[0], jm[1]);
    return &jm;
}

/// Clips the rectangle joint against both plates and inserts the clipped runs.
void merge_case5_rectangle(MergeState& st, const Plate& el, const JointOutlines& jm) {
    Polyline joint_pline_0;
    std::pair<double, double> cp_pair_0;
    if (!Intersection::closed_and_open_paths_2d(el.polylines[0], jm[0][0], el.planes[0], joint_pline_0, cp_pair_0))
        return;
    Polyline joint_pline_1;
    std::pair<double, double> cp_pair_1;
    if (!Intersection::closed_and_open_paths_2d(el.polylines[1], jm[1][0], el.planes[1], joint_pline_1, cp_pair_1))
        return;
    const size_t key0 = (size_t)(scale_0 * std::floor(cp_pair_0.first)) + (size_t)(scale_1 * std::fmod(cp_pair_0.first, 1.0));
    const size_t key1 = (size_t)(scale_0 * std::floor(cp_pair_1.first)) + (size_t)(scale_1 * std::fmod(cp_pair_1.first, 1.0));
    st.sorted0.insert({key0, {cp_pair_0, joint_pline_0.get_points()}});
    st.sorted1.insert({key1, {cp_pair_1, joint_pline_1.get_points()}});
}

/// Intersects the joint plane with its neighbours and the top/bottom planes; a degenerate joint plane leaves the corners in place.
Case2Corners merge_case2_intersections(const MergeState& st, size_t i, int prev, int next, bool z_axis_valid) {
    const std::vector<Plane>& planes = st.joint_planes;
    Case2Corners c;
    c.is_intersected_0 = z_axis_valid && Intersection::plane_plane_plane(planes[2 + prev], planes[i], planes[0], c.p0_int);
    c.is_intersected_1 = z_axis_valid && Intersection::plane_plane_plane(planes[2 + next], planes[i], planes[0], c.p1_int);
    c.is_intersected_2 = z_axis_valid && Intersection::plane_plane_plane(planes[2 + prev], planes[i], planes[1], c.p2_int);
    c.is_intersected_3 = z_axis_valid && Intersection::plane_plane_plane(planes[2 + next], planes[i], planes[1], c.p3_int);
    return c;
}

/// Snaps the start corners to the previous joint's plane when that joint line is offset from the plate edge.
void merge_case2_back_relocate(
    const MergeState& st,
    const Plate& el,
    size_t i,
    const Point& j0_s,
    const Point& j1_s,
    Case2Corners& c
) {
    if (st.last_id != (int)i - 1)
        return;
    const Point e0a = el.polylines[0].get_point(i - 2);
    const Point e0b = el.polylines[0].get_point(i - 1);
    const Point e1a = el.polylines[1].get_point(i - 2);
    const Point e1b = el.polylines[1].get_point(i - 1);
    const bool gd0 = perp_dist_sq(j0_s, e0a, e0b) > st.distance_squared;
    const bool gd1 = perp_dist_sq(j1_s, e1a, e1b) > st.distance_squared;
    if (!gd0 && !gd1)
        return;
    const std::vector<Plane>& planes = st.joint_planes;
    Point p0;
    Point p1;
    const bool ji0 = Intersection::plane_plane_plane(planes[i], planes[i - 1], planes[0], p0);
    const bool ji1 = Intersection::plane_plane_plane(planes[i], planes[i - 1], planes[1], p1);
    if (ji0 && ji1) {
        c.p0_int = p0;
        c.p2_int = p1;
    }
}

/// Updates the joint plane, relocates the plate vertices at both joint ends and tracks the segment; false means skip.
bool merge_case2_relocate(MergeState& st, const Plate& el, JointOutlines& jm, size_t i) {
    const Point j0_s = jm[0][1].get_point(0);
    const Point j0_e = jm[0][1].get_point(1);
    const Point j1_s = jm[1][1].get_point(0);
    const Point j1_e = jm[1][1].get_point(1);
    const Vector x_axis = j0_e - j0_s;
    const Vector y_axis = j0_s - j1_s;
    const Vector z_axis = x_axis.cross(y_axis);
    const bool z_axis_valid = z_axis.magnitude() > 1e-12;
    if (z_axis_valid)
        st.joint_planes[i] = Plane::from_point_normal(j0_s, z_axis);
    if (st.pline0.size() < 4 || st.joint_planes.size() != st.pline0.size() + 1)
        return false;
    const size_t n = st.pline0.size() - 1;
    const int id = static_cast<int>(i) - 2;
    const int prev = ((int)n + id - 1) % (int)n;
    const int next = (id + 1) % (int)n;
    Case2Corners c = merge_case2_intersections(st, i, prev, next, z_axis_valid);
    merge_case2_back_relocate(st, el, i, j0_s, j1_s, c);
    if (c.is_intersected_0)
        st.pline0[id] = c.p0_int;
    if (c.is_intersected_1)
        st.pline0[next] = c.p1_int;
    if (c.is_intersected_2)
        st.pline1[id] = c.p2_int;
    if (c.is_intersected_3)
        st.pline1[next] = c.p3_int;
    st.last_segment0 = {{j0_s, j0_e}};
    st.last_segment1 = {{j1_s, j1_e}};
    if (i == 2) {
        st.last_segment0_start = st.last_segment0;
        st.last_segment1_start = st.last_segment1;
    }
    st.last_id = (int)i;
    return true;
}

/// Reverses the joint outlines when they run against the plate walk, then inserts them keyed by (id + 0.1, id + 0.9).
void merge_case2_flip_and_insert(
    MergeState& st,
    const WoodJoint& jt,
    JointOutlines& jm,
    size_t i,
    int joint_id,
    bool male_or_female
) {
    const int id = static_cast<int>(i) - 2;
    const Polyline& flip_ref = jm[0][0];
    if (flip_ref.point_count() >= 1) {
        const Point fr_front = flip_ref.get_point(0);
        const Point fr_back = flip_ref.get_point(flip_ref.point_count() - 1);
        const Point ref_pt = st.pline0[id + 1];
        const double d_front_sq = (fr_front - ref_pt).magnitude_squared();
        const double d_back_sq = (fr_back - ref_pt).magnitude_squared();
        const bool flipped = d_front_sq < d_back_sq;
        if (st.log) {
            *st.log << " fr_front=(" << fr_front[0] << "," << fr_front[1] << "," << fr_front[2]
                    << ") fr_back=(" << fr_back[0] << "," << fr_back[1] << "," << fr_back[2]
                    << ") ref=(" << ref_pt[0] << "," << ref_pt[1] << "," << ref_pt[2]
                    << ") d_f=" << d_front_sq << " d_b=" << d_back_sq
                    << " flipped=" << (flipped ? 1 : 0)
                    << " jm0[0].first=(" << jm[0][0].get_point(0)[0] << "," << jm[0][0].get_point(0)[1] << "," << jm[0][0].get_point(0)[2] << ")"
                    << " jm1[0].first=(" << jm[1][0].get_point(0)[0] << "," << jm[1][0].get_point(0)[1] << "," << jm[1][0].get_point(0)[2] << ")"
                    << "\n";
        }
        if (flipped) {
            jm[0][0].reverse();
            jm[1][0].reverse();
        }
    } else if (st.log) {
        *st.log << " (flip_ref empty)\n";
    }
    const std::pair<double, double> cp_pair(id + 0.1, id + 0.9);
    const size_t key = (size_t)(scale_0 * std::floor(cp_pair.first)) + (size_t)(scale_1 * std::fmod(cp_pair.first, 1.0));
    if (st.log) {
        *st.log << "  INSERT el=" << st.element_id << " i=" << i
                << " jid=" << joint_id << " jt=" << jt.joint_type
                << " mf=" << (male_or_female ? 'M' : 'F')
                << " jm0[0].n=" << jm[0][0].point_count()
                << " jm1[0].n=" << jm[1][0].point_count()
                << (jm[0][0].point_count() != jm[1][0].point_count() ? " COUNT_DIFF" : "")
                << "\n";
    }
    st.sorted0.insert({key, {cp_pair, jm[0][0].get_points()}});
    st.sorted1.insert({key, {cp_pair, jm[1][0].get_points()}});
}

/// Runs the side-joint passes for every joint on faces i = 2..N: 2-point markers are line joints, 5-point markers rectangles.
void merge_side_joints(MergeState& st, const Plate& el, const ElementJmf& el_jmf, std::vector<WoodJoint>& joints) {
    for (size_t i = 2; i < el_jmf.size() && i < el.planes.size(); i++) {
        for (size_t j = 0; j < el_jmf[i].size(); j++) {
            const int joint_id = el_jmf[i][j].first;
            const bool male_or_female = el_jmf[i][j].second;
            WoodJoint& jt = joints[joint_id];
            JointOutlines* jm = merge_joint_outlines(st, el, jt, i, joint_id, male_or_female);
            if (!jm)
                continue;
            const size_t marker = (*jm)[0][1].point_count();
            if (marker == 5) {
                merge_case5_rectangle(st, el, *jm);
                continue;
            }
            if (marker != 2)
                continue;
            if (!merge_case2_relocate(st, el, *jm, i))
                continue;
            merge_case2_flip_and_insert(st, jt, *jm, i, joint_id, male_or_female);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Merged outline
// ═══════════════════════════════════════════════════════════════════════════

/// Builds one merged plate outline from the relocated vertices and the sorted joint runs.
Polyline merge_build_outline(const std::vector<Point>& pline, SortedRuns& sorted, const Point& orig_front) {
    std::vector<bool> point_flags(pline.size(), true);
    for (const auto& kv : sorted) {
        const std::pair<double, double>& cp = kv.second.first;
        for (size_t k = (size_t)std::ceil(cp.first); k <= (size_t)std::floor(cp.second) && k < point_flags.size(); k++)
            point_flags[k] = false;
    }
    if (pline.size() > 1) {
        const Vector d = pline.back() - orig_front;
        if (std::abs(d[0]) < 1e-6 && std::abs(d[1]) < 1e-6 && std::abs(d[2]) < 1e-6)
            point_flags.back() = false;
    }
    if (!sorted.empty() && !point_flags.empty()) {
        const std::pair<double, double>& last_cp = sorted.rbegin()->second.first;
        if (last_cp.first > (double)pline.size() - 2.0 && last_cp.second < 1.0)
            point_flags[0] = false;
    }
    for (size_t k = 0; k < point_flags.size(); k++) {
        if (!point_flags[k])
            continue;
        const size_t kk = (size_t)(k * scale_0);
        sorted.insert({kk, {{(double)k, (double)k}, std::vector<Point>{pline[k]}}});
    }
    std::vector<Point> merged;
    for (const auto& kv : sorted) {
        const std::vector<Point>& run = kv.second.second;
        merged.insert(merged.end(), run.begin(), run.end());
    }
    if (!merged.empty())
        merged.push_back(merged.front());
    return Polyline(merged);
}

/// Moves the closing corner to the first/last joint-line intersection when both edges carry a joint.
void merge_close_corner(const MergeState& st, Polyline& merged_top, Polyline& merged_bot) {
    if (st.last_id != (int)st.pline0.size())
        return;
    if (!((st.last_segment0_start[0] - st.last_segment0_start[1]).magnitude_squared() > st.distance_squared))
        return;
    const Line ls0_start = Line::from_points(st.last_segment0_start[0], st.last_segment0_start[1]);
    const Line ls0 = Line::from_points(st.last_segment0[0], st.last_segment0[1]);
    const Line ls1_start = Line::from_points(st.last_segment1_start[0], st.last_segment1_start[1]);
    const Line ls1 = Line::from_points(st.last_segment1[0], st.last_segment1[1]);
    Point p0_close;
    Point p1_close;
    const bool ok0 = Intersection::line_line_3d(ls0_start, ls0, p0_close);
    const bool ok1 = Intersection::line_line_3d(ls1_start, ls1, p1_close);
    if (!ok0 || !ok1)
        return;
    std::vector<Point> pts_top = merged_top.get_points();
    std::vector<Point> pts_bot = merged_bot.get_points();
    if (!pts_top.empty())
        pts_top[0] = p0_close;
    if (!pts_bot.empty())
        pts_bot[0] = p1_close;
    if (pts_top.size() > 1)
        pts_top.back() = pts_top.front();
    if (pts_bot.size() > 1)
        pts_bot.back() = pts_bot.front();
    merged_top = Polyline(pts_top);
    merged_bot = Polyline(pts_bot);
}

// ═══════════════════════════════════════════════════════════════════════════
// Holes
// ═══════════════════════════════════════════════════════════════════════════

/// Appends the hole outlines of the top/bottom face joints (i = 0, 1) to result: every outline but the last, the bounding rectangle.
void merge_holes_top_bottom(
    const Plate& el,
    const ElementJmf& el_jmf,
    std::vector<WoodJoint>& joints,
    std::vector<Polyline>& result
) {
    for (size_t i = 0; i < 2 && i < el_jmf.size(); i++) {
        for (size_t k = 0; k < el_jmf[i].size(); k++) {
            const int joint_id = el_jmf[i][k].first;
            const bool male_or_female = el_jmf[i][k].second;
            WoodJoint& jt = joints[joint_id];
            JointOutlines& jm = male_or_female ? jt.m_outlines : jt.f_outlines;
            auto& jct = male_or_female ? jt.m_cut_types : jt.f_cut_types;
            if (jm[0].empty() || jm[1].empty())
                continue;
            const Point t_back0 = jm[0].back().get_point(0);
            const Point f_back0 = jm[1].back().get_point(0);
            const double dt = Point::distance(t_back0, el.planes[0].project(t_back0));
            const double df = Point::distance(f_back0, el.planes[0].project(f_back0));
            if ((dt * dt) > (df * df)) {
                std::swap(jm[0], jm[1]);
                std::swap(jct[0], jct[1]);
            }
            const size_t lim = jm[0].size() > 1 ? jm[0].size() - 1 : 0;
            for (size_t kk = 0; kk < lim && kk < jm[1].size(); kk++) {
                Polyline top = jm[0][kk];
                Polyline bot = jm[1][kk];
                if (!top.is_clockwise(el.planes[0])) {
                    top.reverse();
                    bot.reverse();
                }
                result.push_back(top);
                result.push_back(bot);
            }
        }
    }
}

/// Appends the hole outlines of the side joints (i = 2..N) to result: the outlines tagged wood_cut::hole, shadow joints included.
void merge_holes_side(
    const Plate& el,
    const ElementJmf& el_jmf,
    std::vector<WoodJoint>& joints,
    std::vector<Polyline>& result
) {
    for (size_t i = 2; i < el_jmf.size(); i++) {
        for (size_t k = 0; k < el_jmf[i].size(); k++) {
            const int joint_id = el_jmf[i][k].first;
            const bool male_or_female = el_jmf[i][k].second;
            WoodJoint& jt = joints[joint_id];
            JointOutlines& jm = male_or_female ? jt.m_outlines : jt.f_outlines;
            auto& jct = male_or_female ? jt.m_cut_types : jt.f_cut_types;
            if (jm[0].empty() || jm[1].empty())
                continue;
            if (jct[0].empty())
                continue;
            std::vector<int> id_of_holes;
            for (int ki = 0; ki < (int)jct[0].size(); ki += 2)
                if (jct[0][ki] == wood_cut::hole)
                    id_of_holes.push_back(ki);
            if (id_of_holes.empty())
                continue;
            const Point t_back = jm[0].back().get_point(0);
            const Point f_back = jm[1].back().get_point(0);
            const double dt = Point::distance(t_back, el.planes[0].project(t_back));
            const double df = Point::distance(f_back, el.planes[0].project(f_back));
            if ((dt * dt) > (df * df)) {
                std::swap(jm[0], jm[1]);
                std::swap(jct[0], jct[1]);
            }
            for (const int ki : id_of_holes) {
                if (ki >= (int)jm[0].size() || ki >= (int)jm[1].size())
                    continue;
                Polyline top = jm[0][ki];
                Polyline bot = jm[1][ki];
                if (!top.is_clockwise(el.planes[0])) {
                    top.reverse();
                    bot.reverse();
                }
                result.push_back(top);
                result.push_back(bot);
            }
        }
    }
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Entry point
// ═══════════════════════════════════════════════════════════════════════════

std::vector<session_cpp::Polyline> merge_joints_for_element(
    const Plate& el,
    const std::vector<std::vector<std::pair<int, bool>>>& el_jmf,
    std::vector<WoodJoint>& joints,
    int element_id = -1
) {
    std::ofstream owned;
    MergeState st;
    st.log = merge_open_log(owned);
    st.element_id = element_id;
    merge_log_element(st, el);

    st.pline0 = el.polylines[0].get_points();
    st.pline1 = el.polylines[1].get_points();
    st.joint_planes = el.planes;
    st.pline0_orig_front = st.pline0.empty() ? Point(0, 0, 0) : st.pline0.front();
    st.pline1_orig_front = st.pline1.empty() ? Point(0, 0, 0) : st.pline1.front();
    st.distance_squared = wood_session::globals::DISTANCE_SQUARED;

    merge_side_joints(st, el, el_jmf, joints);

    Polyline merged_top = merge_build_outline(st.pline0, st.sorted0, st.pline0_orig_front);
    Polyline merged_bot = merge_build_outline(st.pline1, st.sorted1, st.pline1_orig_front);
    merge_close_corner(st, merged_top, merged_bot);

    std::vector<Polyline> result;
    merge_holes_top_bottom(el, el_jmf, joints, result);
    merge_holes_side(el, el_jmf, joints, result);
    result.push_back(merged_top);
    result.push_back(merged_bot);

    merge_log_result(st, merged_top, merged_bot);
    return result;
}
