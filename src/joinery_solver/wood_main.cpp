#include "wood_pch.h"
#include "wood_element_plate.h"
#include "wood_face_to_face.h"
#include "wood_joint.h"
#include "wood_merge.h"
#include "wood_session.h"
using namespace session_cpp;

constexpr bool TRACE = false;

/// In-memory adjacency and three-valence overrides for the ChevronJoineryData overload.
static thread_local std::vector<std::pair<int, int>> tl_adjacency_override;
static thread_local std::vector<std::vector<int>> tl_three_valence_override;

namespace {

using wood_session::WoodJoint;
using wood_session::Plate;
using wood_session::joint_orient_to_connection_area;
using wood_session::merge_linked_joints;
using wood_session::joint_get_divisions;
using wood_session::side_removal_ss_e_r_1_port;
using wood_session::side_removal;
using wood_session::tt_e_p_0;
using wood_session::tt_e_p_1;
using wood_session::tt_e_p_2;
using wood_session::tt_e_p_3;
using wood_session::tt_e_p_4;
using wood_session::tt_e_p_5;

/// The joint library is header-only and static; it lands in this TU's anonymous namespace.
#include "wood_joint_lib.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint geometry dispatch
// ═══════════════════════════════════════════════════════════════════════════

/// Whether a per-face joint id falls in the id range of the detected joint type; -1 always matches.
static bool id_matches(const int type, const int id) {
    if (id == -1)
        return true;
    switch (type) {
        case 11: return id >= 10 && id <= 19;
        case 12: return id >= 1 && id <= 9;
        case 13: return id >= 50 && id <= 59;
        case 20: return id >= 20 && id <= 29;
        case 30: return id >= 30 && id <= 39;
        case 40: return id >= 40 && id <= 49;
        case 60: return id >= 60 && id <= 69;
    }
    return false;
}

/// Unit joinery geometry for `id` (family by tens, variant by id); id -1 falls back to the type's default variant.
static void joint_create_geometry(
    WoodJoint& joint,
    double division_distance,
    double shift_param,
    int id,
    std::vector<WoodJoint>* all_joints = nullptr,
    const std::vector<std::shared_ptr<Plate>>* elements = nullptr) {
    joint_get_divisions(joint, division_distance);
    joint.shift = shift_param;

    if (id == 0)
        return;
    if (!id_matches(joint.joint_type, id))
        return;

    int group = -1;
    if (id >= 1 && id <= 9) {
        group = 0;
    } else if (id >= 10 && id <= 19) {
        group = 1;
    } else if (id >= 20 && id <= 29) {
        group = 2;
    } else if (id >= 30 && id <= 39) {
        group = 3;
    } else if (id >= 40 && id <= 49) {
        group = 4;
    } else if (id >= 50 && id <= 59) {
        group = 5;
    } else if (id >= 60 && id <= 69) {
        group = 6;
    } else {
        switch (joint.joint_type) {
            case 11: group = 1; break;
            case 12: group = 0; break;
            case 13: group = 5; break;
            case 20: group = 2; break;
            case 30: group = 3; break;
            case 40: group = 4; break;
            default: group = -1;
        }
    }

    static thread_local std::set<int> warned_ids;
    auto warn_unimpl = [&](const char* family) {
        if (warned_ids.insert(id).second)
            fmt::print(stderr, "joint_create_geometry: id={} ({}) not ported, using family default\n", id, family);
    };

    switch (group) {
        case 0:
            switch (id) {
                case 1: ss_e_ip_1(joint); break;
                case 2: ss_e_ip_0(joint); break;
                case 3: ss_e_ip_2(joint); break;
                case 4: ss_e_ip_3(joint); break;
                case 5: ss_e_ip_4(joint); break;
                case 6: if (elements) ss_e_ip_5(joint, *elements); break;
                case 8: if (elements) side_removal(joint, *elements); break;
                case 9: ss_e_ip_custom(joint); break;
                default: warn_unimpl("ss_e_ip"); ss_e_ip_1(joint); break;
            }
            break;

        case 1:
            switch (id) {
                case 10: ss_e_op_1(joint); break;
                case 11: ss_e_op_2(joint); break;
                case 12: ss_e_op_0(joint); break;
                case 13: ss_e_op_3(joint); break;
                case 14: ss_e_op_4(joint, 0.0, true); break;
                case 15:
                    if (all_joints)
                        ss_e_op_5(joint, *all_joints, false);
                    else
                        ss_e_op_4(joint);
                    break;
                case 16:
                    if (all_joints)
                        ss_e_op_5(joint, *all_joints, true);
                    else
                        ss_e_op_4(joint);
                    break;
                case 17: ss_e_op_17(joint); break;
                case 18: ss_e_op_tutorial(joint); break;
                case 19: ss_e_op_custom(joint); break;
                default:
                    if (id < 0) {
                        if (all_joints)
                            ss_e_op_5(joint, *all_joints, false);
                        else
                            ss_e_op_4(joint);
                    } else {
                        warn_unimpl("ss_e_op");
                        ss_e_op_1(joint);
                    }
                    break;
            }
            break;

        case 2:
            switch (id) {
                case 20: ts_e_p_3(joint); break;
                case 21: ts_e_p_2(joint); break;
                case 22: ts_e_p_3(joint); break;
                case 23: ts_e_p_0(joint); break;
                case 25: ts_e_p_5(joint); break;
                case 28: if (elements) side_removal(joint, *elements); break;
                case 29: ts_e_p_custom(joint); break;
                default: warn_unimpl("ts_e_p"); ts_e_p_3(joint); break;
            }
            break;

        case 3:
            switch (id) {
                case 30: cr_c_ip_0(joint); break;
                case 31: cr_c_ip_1(joint); break;
                case 32: cr_c_ip_2(joint); break;
                case 33: cr_c_ip_3(joint); break;
                case 34: cr_c_ip_4(joint); break;
                case 35: cr_c_ip_5(joint); break;
                case 38: if (elements) side_removal(joint, *elements); break;
                case 39: cr_c_ip_custom(joint); break;
                default: warn_unimpl("cr_c_ip"); cr_c_ip_0(joint); break;
            }
            break;

        case 4:
            switch (id) {
                case 40: if (elements) tt_e_p_0(joint, *elements); break;
                case 41: if (elements) tt_e_p_1(joint, *elements); break;
                case 42: if (elements) tt_e_p_2(joint, *elements); break;
                case 43: if (elements) tt_e_p_3(joint, *elements); break;
                case 44: if (elements) tt_e_p_4(joint, *elements); break;
                case 45: if (elements) tt_e_p_5(joint, *elements); break;
                default: warn_unimpl("tt_e_p"); break;
            }
            break;

        case 5:
            switch (id) {
                case 54: ss_e_r_3(joint); break;
                case 55: ss_e_r_2(joint); break;
                case 56: ss_e_r_0(joint); break;
                case 57: if (elements) side_removal(joint, *elements); break;
                case 58:
                    if (elements)
                        side_removal_ss_e_r_1_port(joint, *elements);
                    else
                        ss_e_r_0(joint);
                    break;
                case 59: ss_e_r_custom(joint); break;
                default: warn_unimpl("ss_e_r"); ss_e_r_0(joint); break;
            }
            break;

        case 6:
            switch (id) {
                case 60: b_0(joint); break;
                case 69: b_custom(joint); break;
                default: warn_unimpl("b"); b_0(joint); break;
            }
            break;

        default:
            warn_unimpl("unwired-group");
            switch (joint.joint_type) {
                case 11: case 12: ss_e_op_1(joint); break;
                case 20: ts_e_p_3(joint); break;
                default: break;
            }
            break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Three-valence
// ═══════════════════════════════════════════════════════════════════════════

/// Order-independent key for an element pair.
static uint64_t gcz_pair_key(int a, int b) {
    if (a > b)
        std::swap(a, b);
    return ((uint64_t)a << 32) | (uint64_t)b;
}

/// Element pair -> joint index (last joint wins); rebuilt wherever the joint list may have changed.
static std::unordered_map<uint64_t, int> gcz_joints_map(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<WoodJoint>& joints) {
    std::unordered_map<uint64_t, int> joints_map;
    for (size_t ji = 0; ji < joints.size(); ji++) {
        const int e0 = index_of(elements, joints[ji].element_a);
        const int e1 = index_of(elements, joints[ji].element_b);
        joints_map[gcz_pair_key(e0, e1)] = (int)ji;
    }
    return joints_map;
}

/// Vidy method: shadow joints (link = true) between each side plate and the plate it is glued to, translated to that plate's far face.
static void three_valence_joint_addition_vidy(
    const std::vector<std::vector<int>>& tv_groups,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& joints,
    std::unordered_map<uint64_t, int>& joints_map) {
    if (tv_groups.size() < 2)
        return;

    joints.reserve(joints.size() + (tv_groups.size() - 1) * 2);

    for (size_t gi = 1; gi < tv_groups.size(); gi++) {
        const std::vector<int>& g = tv_groups[gi];
        if (g.size() != 4)
            continue;
        int s0 = g[0];
        int s1 = g[1];
        int e20 = g[2];
        int e31 = g[3];
        const int n_elems = (int)elements.size();
        if (s0 < 0 || s1 < 0 || e20 < 0 || e31 < 0)
            continue;
        if (s0 >= n_elems || s1 >= n_elems)
            continue;
        if (e20 >= n_elems || e31 >= n_elems)
            return;

        if (e20 != e31) {
            auto is_parallel_wood = [](const Vector& a, const Vector& b) -> bool {
                const double ll = a.magnitude() * b.magnitude();
                if (ll <= 0.0)
                    return false;
                return std::abs(a.dot(b) / ll) >= std::cos(wood_session::globals::ANGLE);
            };
            const Vector n_s0 = elements[s0]->planes[0].z_axis();
            const Vector n_e31 = elements[e31]->planes[0].z_axis();
            const Vector n_s1 = elements[s1]->planes[0].z_axis();
            const Vector n_e20 = elements[e20]->planes[0].z_axis();
            if (!is_parallel_wood(n_s0, n_e31) || !is_parallel_wood(n_s1, n_e20))
                continue;
        }

        const auto it = joints_map.find(gcz_pair_key(s0, s1));
        if (it == joints_map.end())
            continue;
        const int id = it->second;
        if (!joints[id].joint_volumes_pair_a_pair_b[0].has_value())
            continue;

        double d00 = elements[e31]->planes[0].squared_distance(elements[s0]->planes[0].origin());
        double d01 = elements[e31]->planes[1].squared_distance(elements[s0]->planes[0].origin());
        const Plane plane00_far = d00 < d01 ? elements[e31]->planes[0] : elements[e31]->planes[1];

        d00 = elements[s0]->planes[0].squared_distance(plane00_far.origin());
        d01 = elements[s0]->planes[1].squared_distance(plane00_far.origin());
        const Plane plane01_near = d00 < d01 ? elements[s0]->planes[1] : elements[s0]->planes[0];

        double d10 = elements[e20]->planes[0].squared_distance(elements[s1]->planes[0].origin());
        double d11 = elements[e20]->planes[1].squared_distance(elements[s1]->planes[0].origin());
        const Plane plane10_far = d10 < d11 ? elements[e20]->planes[0] : elements[e20]->planes[1];

        d10 = elements[s1]->planes[0].squared_distance(plane10_far.origin());
        d11 = elements[s1]->planes[1].squared_distance(plane10_far.origin());
        const Plane plane11_near = d10 < d11 ? elements[s1]->planes[1] : elements[s1]->planes[0];

        const Polyline& jvol = *joints[id].joint_volumes_pair_a_pair_b[0];
        const Line l0 = Line::from_points(jvol.get_point(0), jvol.get_point(1));
        const Line l1 = Line::from_points(jvol.get_point(1), jvol.get_point(2));

        const Point proj_p1 = plane01_near.project(jvol.get_point(1));
        const Point proj_p2 = plane01_near.project(jvol.get_point(2));
        const Vector proj_l1_dir = proj_p1 - proj_p2;
        const bool is_parallel_01 = (proj_l1_dir.is_parallel_to(l1.to_vector()) == 0);
        const std::array<Line, 2> ll = is_parallel_01 ? std::array<Line, 2>{l1, l0} : std::array<Line, 2>{l0, l1};

        Point p00;
        Point p01;
        Point p10;
        Point p11;
        if (!Intersection::line_plane(ll[0], plane00_far, p00, false))
            continue;
        if (!Intersection::line_plane(ll[0], plane01_near, p01, false))
            continue;
        if (e20 == e31) {
            p10 = p00;
            p11 = p01;
        } else {
            if (!Intersection::line_plane(ll[1], plane10_far, p10, false))
                continue;
            if (!Intersection::line_plane(ll[1], plane11_near, p11, false))
                continue;
        }

        const Vector trans0 = p00 - p01;
        const Vector trans1 = p10 - p11;

        const double vsum = trans0[0] + trans0[1] + trans0[2] + trans1[0] + trans1[1] + trans1[2];
        if (vsum < -1e8 || vsum > 1e8)
            continue;

        auto copy_vols = [&]() -> std::array<Polyline, 4> {
            std::array<Polyline, 4> vols;
            for (int k = 0; k < 4; k++)
                if (joints[id].joint_volumes_pair_a_pair_b[k].has_value())
                    vols[k] = *joints[id].joint_volumes_pair_a_pair_b[k];
            return vols;
        };
        std::array<Polyline, 4> jv0_copy = copy_vols();
        std::array<Polyline, 4> jv1_copy = copy_vols();

        int shift_amt = 0;
        for (int j = 0; j < 4; j++) {
            const Vector v = jv1_copy[0].get_point(j) - jv1_copy[0].get_point(j + 1);
            if (v.is_parallel_to(trans1) == 1) {
                shift_amt = j;
                break;
            }
        }
        for (size_t k = 0; k < 4; k++)
            if (jv1_copy[k].point_count() == 5)
                jv1_copy[k].shift(shift_amt);

        std::array<Line, 2> jlines0 = joints[id].joint_lines;
        std::array<Line, 2> jlines1 = joints[id].joint_lines;

        for (int k = 0; k < 2; k++) {
            jv0_copy[k].translate(trans0);
            jv1_copy[k].translate(trans1);
            jlines0[k] += trans0;
            jlines1[k] += trans1;
        }

        if (index_of(elements, joints[id].element_a) == s1) {
            std::swap(e20, e31);
            std::swap(s0, s1);
        }

        WoodJoint shadow0;
        shadow0.element_a = elements[s0]->guid();
        shadow0.element_b = elements[e20]->guid();
        shadow0.contact.face_a = -1;
        shadow0.contact.face_b = -1;
        shadow0.cross_faces = {-1, -1};
        shadow0.joint_type = joints[id].joint_type;
        shadow0.contact.area = joints[id].contact.area;
        shadow0.joint_lines = jlines0;
        shadow0.joint_volumes_pair_a_pair_b = {jv0_copy[0], jv0_copy[1], std::nullopt, std::nullopt};
        shadow0.link = true;
        const int shadow0_idx = (int)joints.size();
        joints.push_back(std::move(shadow0));
        joints_map[gcz_pair_key(s0, e20)] = shadow0_idx;

        int shadow1_idx = -1;
        if (e20 != e31) {
            WoodJoint shadow1;
            shadow1.element_a = elements[s1]->guid();
            shadow1.element_b = elements[e31]->guid();
            shadow1.contact.face_a = -1;
            shadow1.contact.face_b = -1;
            shadow1.cross_faces = {-1, -1};
            shadow1.joint_type = joints[id].joint_type;
            shadow1.contact.area = joints[id].contact.area;
            shadow1.joint_lines = jlines1;
            shadow1.joint_volumes_pair_a_pair_b = {jv1_copy[0], jv1_copy[1], std::nullopt, std::nullopt};
            shadow1.link = true;
            shadow1_idx = (int)joints.size();
            joints.push_back(std::move(shadow1));
            joints_map[gcz_pair_key(s1, e31)] = shadow1_idx;
        }

        if (e20 != e31)
            joints[id].linked_joints = {shadow0_idx, shadow1_idx};
        else
            joints[id].linked_joints = {shadow0_idx};
    }
}

/// Clip a joint's volume pairs between the planes through `a` and `b` that are normal to the first volume.
static void gcz_clip_volumes(WoodJoint& j, const Point& a, const Point& b) {
    if (!j.joint_volumes_pair_a_pair_b[0].has_value())
        return;
    const Polyline& vol = *j.joint_volumes_pair_a_pair_b[0];
    Vector normal(0, 0, 1);
    if (vol.point_count() >= 3) {
        const Point p0 = vol.get_point(0);
        const Point p1 = vol.get_point(1);
        const Point p2 = vol.get_point(2);
        normal = (p2 - p1).cross(p0 - p1);
    }
    normal.normalize_self();
    const Plane pl_a = Plane::from_point_normal(a, normal);
    const Plane pl_b = Plane::from_point_normal(b, normal);
    for (int vp = 0; vp < 4; vp += 2) {
        if (!j.joint_volumes_pair_a_pair_b[vp].has_value() || !j.joint_volumes_pair_a_pair_b[vp + 1].has_value())
            continue;
        Polyline& v0 = *j.joint_volumes_pair_a_pair_b[vp];
        Polyline& v1 = *j.joint_volumes_pair_a_pair_b[vp + 1];
        const Line s0l = Line::from_points(v0.get_point(0), v1.get_point(0));
        const Line s1l = Line::from_points(v0.get_point(1), v1.get_point(1));
        const Line s2l = Line::from_points(v0.get_point(2), v1.get_point(2));
        const Line s3l = Line::from_points(v0.get_point(3), v1.get_point(3));
        Intersection::plane_4lines(pl_a, s0l, s1l, s2l, s3l, v0);
        Intersection::plane_4lines(pl_b, s0l, s1l, s2l, s3l, v1);
    }
}

/// Annen method: shorten the two overlapping joint lines at a 3-plate corner by the plate thickness and clip the volumes to match.
static void three_valence_joint_alignment_annen(
    const std::vector<std::vector<int>>& tv_groups,
    const std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& joints) {
    const std::unordered_map<uint64_t, int> joints_map = gcz_joints_map(elements, joints);

    for (size_t gi = 1; gi < tv_groups.size(); gi++) {
        const std::vector<int>& g = tv_groups[gi];
        if (g.size() != 4)
            continue;
        const int s0 = g[0];
        const int s1 = g[1];
        const int e20 = g[2];
        const int e31 = g[3];

        const auto it0 = joints_map.find(gcz_pair_key(s0, s1));
        const auto it1 = joints_map.find(gcz_pair_key(e20, e31));
        if (it0 == joints_map.end() || it1 == joints_map.end())
            continue;

        WoodJoint& j0 = joints[it0->second];
        WoodJoint& j1 = joints[it1->second];

        const Line l0 = j0.joint_lines[0];
        const double d_s = Point::distance(l0.start(), j1.joint_lines[0].start());
        const double d_e = Point::distance(l0.start(), j1.joint_lines[0].end());
        const Line l1 = (d_s <= d_e) ? j1.joint_lines[0] : -j1.joint_lines[0];

        // A degenerate overlap (parallel but disjoint lines) would plant end caps at garbage positions.
        Line overlap;
        if (!l0.overlap_average(l1, overlap))
            continue;

        double thickness = 0;
        const int e0_idx = index_of(elements, j0.element_a);
        if (e0_idx >= 0 && e0_idx < (int)elements.size()) {
            const Plate& el = *elements[e0_idx];
            if (el.polylines.size() >= 2 && el.polylines[0].point_count() > 0 && el.polylines[1].point_count() > 0) {
                const Point p0 = el.polylines[0].get_point(0);
                const Point p1_proj = el.planes[1].project(p0);
                thickness = Point::distance(p0, p1_proj);
            }
        }
        // Line::extend has no clamp: shrinking by more than half the length inverts the segment.
        thickness = std::min(thickness, overlap.length() * 0.5 - 1e-9);
        if (thickness < 0.0)
            thickness = 0.0;
        overlap.extend(-thickness, -thickness);

        j0.joint_lines[0] = overlap;
        j1.joint_lines[0] = overlap;

        const Point ol_s = overlap.start();
        const Point ol_e = overlap.end();
        gcz_clip_volumes(j0, ol_s, ol_e);
        gcz_clip_volumes(j1, ol_e, ol_s);
    }
}

/// j_mf[element][face] = [(joint index, is_male)].
using JMF = std::vector<std::vector<std::vector<std::pair<int, bool>>>>;

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════
// Pipeline stages
// ═══════════════════════════════════════════════════════════════════════════

namespace {

using GczClock = std::chrono::high_resolution_clock;

/// Detection parameters handed to face_to_face_wood for every adjacent pair.
struct GczDetectParams {
    std::vector<double> joint_volume_extension;
    double limit_min_joint_length;
    double distance_squared;
    double coplanar_tolerance;
    double dihedral_angle_threshold;
    bool all_treated_as_rotated;
    bool rotated_joint_as_average;
};

/// Detection counters: successes, failures and successes per joint type [11, 12, 13, 20, 30, 40].
struct GczDetectStats {
    int counts[6] = {0, 0, 0, 0, 0, 0};
    int n_failed = 0;
    int n_success = 0;
};

/// Family lookup for one joint: representing id, division length and shift.
struct GczFamilyParams {
    int id;
    double div_dist;
    double shift;
};

/// Pre-orient unit-cube geometry shared by joints with an equal cache key.
struct GczCachedJointGeom {
    std::string name;
    std::array<std::vector<Polyline>, 2> m_outlines;
    std::array<std::vector<Polyline>, 2> f_outlines;
    std::array<std::vector<int>, 2> m_cut_types;
    std::array<std::vector<int>, 2> f_cut_types;
    bool unit_scale;
    double unit_scale_distance;
};

using GczJointCache = std::map<std::string, GczCachedJointGeom>;

/// Stage boundaries for the trace timing report.
struct GczTimes {
    GczClock::time_point t0;
    GczClock::time_point t1;
    GczClock::time_point t2;
    GczClock::time_point t3;
    GczClock::time_point t3a;
    GczClock::time_point t3b;
    GczClock::time_point t3c;
    GczClock::time_point t4;
};

/// Stage 1: candidate pairs from the adjacency sidecar, the thread-local override or the OBB+BVH search.
static std::vector<std::pair<int, int>> gcz_adjacency(
    const std::string& adj_name,
    const std::vector<std::shared_ptr<Plate>>& wood_elems) {
    std::vector<std::pair<int, int>> adjacency_pairs;
    if (!adj_name.empty()) {
        std::ifstream adj_in(adj_name);
        int a;
        int b;
        while (adj_in >> a >> b)
            adjacency_pairs.emplace_back(a, b);
        if (TRACE)
            fmt::print("adjacency: {} pairs from {}\n", adjacency_pairs.size(), adj_name);
    }
    if (adjacency_pairs.empty() && !tl_adjacency_override.empty())
        adjacency_pairs = tl_adjacency_override;

    if (adjacency_pairs.empty()) {
        const double distance = wood_session::globals::DISTANCE;
        if (TRACE)
            fmt::print(stderr, "[GCZ] adjacency_search start  DISTANCE={}\n", distance);
        std::vector<wood_session::ContactElement> view;
        view.reserve(wood_elems.size());
        for (const std::shared_ptr<Plate>& plate : wood_elems)
            view.emplace_back(*plate);
        adjacency_pairs = wood_session::adjacency_search(view, distance);
        if (TRACE)
            fmt::print(stderr, "[GCZ] adjacency pairs={}\n", adjacency_pairs.size());
        if (TRACE)
            fmt::print("adjacency: {} pairs from OBB+BVH\n", adjacency_pairs.size());
    }
    return adjacency_pairs;
}

/// Stage 2: per-element insertion vectors from the sidecar (one element per line, `x y z ...`) into each Plate, reversed plates flipped.
static void gcz_load_insertion_vectors(
    const std::string& iv_name,
    std::vector<std::shared_ptr<Plate>>& wood_elems) {
    std::vector<std::vector<Vector>> per_element(wood_elems.size(), std::vector<Vector>{});
    if (!iv_name.empty()) {
        std::ifstream iv_in(iv_name);
        std::string iv_line;
        size_t ei = 0;
        size_t total_loaded = 0;
        while (std::getline(iv_in, iv_line) && ei < per_element.size()) {
            std::istringstream iss(iv_line);
            std::vector<Vector>& vecs = per_element[ei];
            double x;
            double y;
            double z;
            while (iss >> x >> y >> z) {
                vecs.emplace_back(x, y, z);
                total_loaded++;
            }
            ei++;
        }
        if (TRACE)
            fmt::print("insertion_vectors: {} vectors across {} elements from {}\n", total_loaded, ei, iv_name);
    }
    for (size_t ei = 0; ei < wood_elems.size(); ei++) {
        if (wood_elems[ei]->insertion_vectors().empty())
            wood_elems[ei]->insertion_vectors() = per_element[ei];
        if (wood_elems[ei]->reversed) {
            std::vector<Vector>& vecs = wood_elems[ei]->insertion_vectors();
            if (vecs.size() > 2)
                std::reverse(vecs.begin() + 2, vecs.end());
        }
    }
}

/// Stage 3: run face_to_face_wood on every adjacent pair; joints stay in adjacency-pair order.
static std::vector<WoodJoint> gcz_detect(
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    const std::vector<std::pair<int, int>>& adjacency_pairs,
    const GczDetectParams& params,
    const SearchType search_type,
    GczDetectStats& stats) {
    std::vector<WoodJoint> all_joints;
    all_joints.reserve(adjacency_pairs.size());
    if (TRACE)
        fmt::print(stderr, "[GCZ] joint detection loop  pairs={}\n", adjacency_pairs.size());
    const int n_wood_elems = static_cast<int>(wood_elems.size());
    for (size_t k = 0; k < adjacency_pairs.size(); ++k) {
        const int ia = adjacency_pairs[k].first;
        const int ib = adjacency_pairs[k].second;
        if (TRACE)
            fmt::print(stderr, "[GCZ]   pair k={}  ia={} ib={}\n", k, ia, ib);

        if (ia < 0 || ib < 0 || ia >= n_wood_elems || ib >= n_wood_elems) {
            fmt::print(stderr, "  WARNING: adjacency pair {} references elements ({}, {}) but only {} were loaded - skipping.\n", k, ia, ib, n_wood_elems);
            continue;
        }

        WoodJoint joint;
        bool swap_planes_b = false;
        const bool ok = face_to_face_wood(
            *wood_elems[ia],
            *wood_elems[ib],
            {ia, ib},
            params.joint_volume_extension,
            params.limit_min_joint_length,
            params.distance_squared,
            params.coplanar_tolerance,
            params.dihedral_angle_threshold,
            params.all_treated_as_rotated,
            params.rotated_joint_as_average,
            search_type,
            joint,
            swap_planes_b);
        if (TRACE)
            fmt::print(stderr, "[GCZ]   face_to_face_wood done  ok={}  type={}\n", (int)ok, ok ? joint.joint_type : -1);
        if (swap_planes_b) {
            std::swap(wood_elems[ib]->planes[0], wood_elems[ib]->planes[1]);
            std::swap(wood_elems[ib]->polylines[0], wood_elems[ib]->polylines[1]);
        }
        if (!ok) {
            if (TRACE && !joint.dbg_fail_reason.empty())
                fmt::print("  FAIL pair ({},{}) coplanar={} boolean={} reason={}\n", ia, ib, joint.dbg_coplanar, joint.dbg_boolean, joint.dbg_fail_reason);
            ++stats.n_failed;
            continue;
        }
        ++stats.n_success;
        switch (joint.joint_type) {
            case 11: ++stats.counts[0]; break;
            case 12: ++stats.counts[1]; break;
            case 13: ++stats.counts[2]; break;
            case 20: ++stats.counts[3]; break;
            case 30: ++stats.counts[4]; break;
            case 40: ++stats.counts[5]; break;
            default: break;
        }
        all_joints.push_back(std::move(joint));
    }
    return all_joints;
}

/// One row of ints per non-empty line of the file.
static std::vector<std::vector<int>> gcz_read_int_rows(const std::string& path) {
    std::vector<std::vector<int>> rows;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::vector<int> row;
        int v;
        while (iss >> v)
            row.push_back(v);
        if (!row.empty())
            rows.push_back(row);
    }
    return rows;
}

/// Stage 4: three-valence groups from the sidecar or the thread-local override; first row's first value 0 = annen alignment, 1 = vidy addition.
static void gcz_three_valence(
    const std::string& tv_name,
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    std::vector<WoodJoint>& all_joints) {
    const std::vector<std::vector<int>> tv_groups = !tv_name.empty() ? gcz_read_int_rows(tv_name) : tl_three_valence_override;
    if (tv_groups.size() > 1) {
        std::unordered_map<uint64_t, int> joints_map = gcz_joints_map(wood_elems, all_joints);
        const int instruction = tv_groups[0].empty() ? 0 : tv_groups[0][0];
        if (instruction == 1) {
            const size_t before_vidy = all_joints.size();
            three_valence_joint_addition_vidy(tv_groups, wood_elems, all_joints, joints_map);
            if (TRACE)
                fmt::print("vidy_addition: {} shadow joints created (total {})\n", all_joints.size() - before_vidy, all_joints.size());
        } else {
            three_valence_joint_alignment_annen(tv_groups, wood_elems, all_joints);
        }
    }
    if (TRACE && !tv_name.empty())
        fmt::print("three_valence: {} groups applied\n", tv_groups.size());
}

/// Stage 5: per-element per-face joint type ids (the wood JOINTS_TYPES filter); 0 = no joint, tens digit = family.
static std::vector<std::vector<int>> gcz_joint_types(
    const std::string& jt_name,
    const std::vector<std::shared_ptr<Plate>>& wood_elems) {
    std::vector<std::vector<int>> per_element(wood_elems.size());
    if (!jt_name.empty()) {
        std::ifstream jt_in(jt_name);
        std::string jt_line;
        size_t ei = 0;
        size_t total_loaded = 0;
        while (std::getline(jt_in, jt_line) && ei < per_element.size()) {
            std::istringstream iss(jt_line);
            int v;
            while (iss >> v) {
                per_element[ei].push_back(v);
                total_loaded++;
            }
            ei++;
        }
        if (TRACE)
            fmt::print("joints_types: {} ids across {} elements from {}\n", total_loaded, ei, jt_name);
    }
    for (size_t ei = 0; ei < wood_elems.size(); ++ei)
        if (per_element[ei].empty() && !wood_elems[ei]->joint_types.empty())
            per_element[ei] = wood_elems[ei]->joint_types;
    return per_element;
}

/// Wood's id_representing_joint_name: max of the two face ids in the JOINTS_TYPES table, -1 when the table says nothing.
static int gcz_representing_id(
    const WoodJoint& j,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const std::vector<std::shared_ptr<Plate>>& wood_elems) {
    int id_representing_joint_name = -1;
    if (!per_element_joints_types.empty()) {
        const int e0 = index_of(wood_elems, j.element_a);
        const int e1 = index_of(wood_elems, j.element_b);
        const int f0 = j.contact.face_a;
        const int f1 = j.contact.face_b;
        // The table uses pre-reversal face indices; a reversed winding reorders the side planes.
        auto orig_face = [&](int ei, int fi) -> int {
            if (ei < 0 || ei >= (int)wood_elems.size())
                return fi;
            if (!wood_elems[ei]->reversed)
                return fi;
            if (fi < 2)
                return 1 - fi;
            const int n_sides = (int)wood_elems[ei]->planes.size() - 2;
            return 2 + (n_sides - 1 - (fi - 2));
        };
        const int of0 = orig_face(e0, f0);
        const int of1 = orig_face(e1, f1);
        const int id0 = (e0 >= 0 && e0 < (int)per_element_joints_types.size() && of0 >= 0 && of0 < (int)per_element_joints_types[e0].size())
            ? std::abs(per_element_joints_types[e0][of0]) : 0;
        const int id1 = (e1 >= 0 && e1 < (int)per_element_joints_types.size() && of1 >= 0 && of1 < (int)per_element_joints_types[e1].size())
            ? std::abs(per_element_joints_types[e1][of1]) : 0;
        if (e0 >= 0 && e0 < (int)per_element_joints_types.size() &&
            e1 >= 0 && e1 < (int)per_element_joints_types.size() &&
            (per_element_joints_types[e0].size() > 0 || per_element_joints_types[e1].size() > 0)) {
            id_representing_joint_name = std::max(id0, id1);
            if (id_representing_joint_name == 0)
                id_representing_joint_name = -1;
        }
    }
    return id_representing_joint_name;
}

/// Row of JOINTS_PARAMETERS_AND_TYPES for a joint type: 11->1 12->0 13->5 20->2 30->3 40->4 60->6.
static int gcz_row(const int joint_type) {
    switch (joint_type) {
        case 11: return 1;
        case 12: return 0;
        case 13: return 5;
        case 20: return 2;
        case 30: return 3;
        case 40: return 4;
        case 60: return 6;
        default: return 1;
    }
}

/// Per-family row of JOINTS_PARAMETERS_AND_TYPES: division length, shift and the default id when none was given.
static GczFamilyParams gcz_family_params(const int joint_type, const int id_representing_joint_name) {
    static constexpr double JPT_DEFAULTS[21] = {
        300, 0.5,  3,
        450, 0.64, 15,
        450, 0.5,  20,
        300, 0.5,  30,
          6, 0.95, 40,
        300, 0.5,  58,
        300, 1.0,  60,
    };
    const std::vector<double>& JPT_global = wood_session::globals::JOINTS_PARAMETERS_AND_TYPES;
    const bool jpt_ok = JPT_global.size() >= 21;
    static thread_local bool jpt_warned = false;
    if (!jpt_ok && !jpt_warned) {
        fmt::print(stderr, "  WARNING: JOINTS_PARAMETERS_AND_TYPES has {} entries, expected 21 - using built-in defaults.\n", JPT_global.size());
        jpt_warned = true;
    }
    auto JPT = [&](size_t idx) -> double {
        return jpt_ok ? JPT_global[idx] : JPT_DEFAULTS[idx];
    };
    const int row = gcz_row(joint_type);

    GczFamilyParams fam;
    fam.id = id_representing_joint_name;
    if (fam.id == -1)
        fam.id = (int)JPT(row * 3 + 2);
    fam.div_dist = JPT(row * 3 + 0);
    fam.shift = JPT(row * 3 + 1);
    return fam;
}

/// Wood's get_key number format: std::to_string truncated at two decimals.
static std::string gcz_key_num(double v) {
    v += 1e-9;
    const std::string s = std::to_string(v);
    const auto dot = s.find('.');
    if (dot != std::string::npos && dot + 3 <= s.size())
        return s.substr(0, dot + 3);
    return s;
}

/// Unit-geometry cache key: the id stands in for `name` (id->constructor is deterministic).
static std::string gcz_cache_key(const int id_representing_joint_name, const WoodJoint& j) {
    return std::to_string(id_representing_joint_name) + ";" + gcz_key_num(j.shift) + ";" + gcz_key_num((double)j.divisions);
}

/// Stage 6, one joint: unit geometry (cached for butterflies), orient to the connection area, merge linked shadows.
static void gcz_build_one_joint(
    WoodJoint& j,
    const GczFamilyParams& fam,
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    std::vector<WoodJoint>& all_joints,
    GczJointCache& unique_joints_cache) {
    j.scale = {
        wood_session::globals::JOINT_SCALE[0],
        wood_session::globals::JOINT_SCALE[1],
        wood_session::globals::JOINT_SCALE[2]};

    // ss_e_r_2/3 and ss_e_ip_2 divide by the element thickness, not the 40 mm default.
    if (j.joint_type == 13 || j.joint_type == 12) {
        const int ei = index_of(wood_elems, j.element_a);
        if (ei >= 0 && ei < (int)wood_elems.size())
            j.unit_scale_distance = wood_elems[ei]->thickness;
    }

    joint_get_divisions(j, fam.div_dist);
    j.shift = fam.shift;
    const std::string cache_key = gcz_cache_key(fam.id, j);

    // Only type 12 (butterflies) is cached; caching the other types regressed top_to_side_box and vda_floor_0.
    const bool use_cache = (j.joint_type == 12) && j.linked_joints.empty();

    const auto cache_it = use_cache ? unique_joints_cache.find(cache_key) : unique_joints_cache.end();
    if (!use_cache) {
        joint_create_geometry(j, fam.div_dist, fam.shift, fam.id, &all_joints, &wood_elems);
    } else if (cache_it != unique_joints_cache.end()) {
        const GczCachedJointGeom& u = cache_it->second;
        j.name = u.name;
        j.m_outlines = u.m_outlines;
        j.f_outlines = u.f_outlines;
        j.m_cut_types = u.m_cut_types;
        j.f_cut_types = u.f_cut_types;
        j.unit_scale = u.unit_scale;
        j.unit_scale_distance = u.unit_scale_distance;
    } else {
        joint_create_geometry(j, fam.div_dist, fam.shift, fam.id, &all_joints, &wood_elems);
        GczCachedJointGeom u;
        u.name = j.name;
        u.m_outlines = j.m_outlines;
        u.f_outlines = j.f_outlines;
        u.m_cut_types = j.m_cut_types;
        u.f_cut_types = j.f_cut_types;
        u.unit_scale = j.unit_scale;
        u.unit_scale_distance = j.unit_scale_distance;
        unique_joints_cache.emplace(cache_key, std::move(u));
    }
    if (TRACE)
        fmt::print(stderr, "[GCZ]   after joint_create_geometry  no_orient={}\n", (int)j.no_orient);
    if (!j.no_orient) {
        if (TRACE)
            fmt::print(stderr, "[GCZ]   calling joint_orient_to_connection_area\n");
        joint_orient_to_connection_area(j);
        if (TRACE)
            fmt::print(stderr, "[GCZ]   joint_orient done\n");
    }
    if (!j.linked_joints.empty() && (fam.id == 15 || fam.id == 16)) {
        for (int sid : j.linked_joints)
            if (!all_joints[sid].no_orient)
                joint_orient_to_connection_area(all_joints[sid]);
        merge_linked_joints(j, all_joints);
    }
}

/// Stages 6-7: unit joinery geometry and orientation for every detected joint, in detection order (the cache is order-dependent).
static void gcz_geometry(
    std::vector<WoodJoint>& all_joints,
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    const std::vector<std::vector<int>>& per_element_joints_types) {
    GczJointCache unique_joints_cache;
    if (TRACE)
        fmt::print(stderr, "[GCZ] geometry loop start  all_joints={}\n", all_joints.size());
    for (WoodJoint& j : all_joints) {
        if (TRACE)
            fmt::print(stderr, "[GCZ]   geom joint type={}  e0={} e1={}\n", j.joint_type, j.element_a, j.element_b);
        const int id_representing_joint_name = gcz_representing_id(j, per_element_joints_types, wood_elems);
        const GczFamilyParams fam = gcz_family_params(j.joint_type, id_representing_joint_name);
        if (j.link)
            continue;
        gcz_build_one_joint(j, fam, wood_elems, all_joints, unique_joints_cache);
    }
}

/// Stage 8: j_mf[element][face] = [(joint index, is_male)]; shadow joints go to the extra last slot.
static JMF gcz_build_jmf(
    const std::vector<std::shared_ptr<Plate>>& wood_elems,
    const std::vector<WoodJoint>& all_joints) {
    const size_t n_elems = wood_elems.size();
    JMF j_mf(n_elems);
    for (size_t ei = 0; ei < n_elems; ei++)
        j_mf[ei].resize(wood_elems[ei]->planes.size() + 1);
    for (size_t ji = 0; ji < all_joints.size(); ji++) {
        const WoodJoint& j = all_joints[ji];
        const int e0 = index_of(wood_elems, j.element_a);
        const int e1 = index_of(wood_elems, j.element_b);
        if (j.link) {
            if (e0 >= 0 && e0 < (int)n_elems)
                j_mf[e0].back().push_back({(int)ji, true});
            if (e1 >= 0 && e1 < (int)n_elems)
                j_mf[e1].back().push_back({(int)ji, false});
        } else {
            const int f0 = j.contact.face_a;
            const int f1 = j.contact.face_b;
            if (e0 >= 0 && e0 < (int)n_elems && f0 >= 0 && f0 < (int)j_mf[e0].size())
                j_mf[e0][f0].push_back({(int)ji, true});
            if (e1 >= 0 && e1 < (int)n_elems && f1 >= 0 && f1 < (int)j_mf[e1].size())
                j_mf[e1][f1].push_back({(int)ji, false});
        }
    }
    return j_mf;
}

/// Stage 9: merge joint cuts into each plate's polylines and de-interleave [holes..., outer_top, outer_bot] into `features`.
static void gcz_merge(
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    const JMF& j_mf,
    std::vector<WoodJoint>& all_joints) {
    const size_t n_elems = wood_elems.size();
    for (size_t ei = 0; ei < n_elems; ei++) {
        std::vector<Polyline> merged = merge_joints_for_element(*wood_elems[ei], j_mf[ei], all_joints, (int)ei);
        auto& feat = wood_elems[ei]->features;
        feat.top.clear();
        feat.bottom.clear();
        if (merged.size() >= 2) {
            const size_t n_holes = merged.size() - 2;
            feat.top.reserve(1 + n_holes / 2);
            feat.bottom.reserve(1 + n_holes / 2);
            feat.top.push_back(std::move(merged[merged.size() - 2]));
            feat.bottom.push_back(std::move(merged[merged.size() - 1]));
            for (size_t hi = 0; hi + 2 <= n_holes; hi += 2) {
                feat.top.push_back(std::move(merged[hi]));
                feat.bottom.push_back(std::move(merged[hi + 1]));
            }
        }
    }
}

/// Trace summary: counts per type and per-stage timings.
static void gcz_report(
    const std::vector<std::shared_ptr<Plate>>& wood_elems,
    const std::vector<std::pair<int, int>>& adjacency_pairs,
    const GczDetectStats& stats,
    const GczTimes& t) {
    auto ms = [](auto a, auto b) {
        return std::chrono::duration<double, std::milli>(b - a).count();
    };
    fmt::print("{} elements -> {} adjacency pairs\n", wood_elems.size(), adjacency_pairs.size());
    fmt::print("  joints: {} success / {} failed\n", stats.n_success, stats.n_failed);
    fmt::print(
        "  by type: 11={} 12={} 13={} 20={} 30={} 40={}\n",
        stats.counts[0], stats.counts[1], stats.counts[2], stats.counts[3], stats.counts[4], stats.counts[5]);
    fmt::print("  time: {:.0f}ms\n", ms(t.t0, t.t4));
    fmt::print(
        "  stages(ms): setup={:.1f} adjacency={:.1f} detect={:.1f} tv={:.1f} geom={:.1f} jmf={:.1f} merge={:.1f}\n",
        ms(t.t0, t.t2) - ms(t.t1, t.t2), ms(t.t1, t.t2), ms(t.t2, t.t3), ms(t.t3, t.t3a), ms(t.t3a, t.t3b), ms(t.t3b, t.t3c), ms(t.t3c, t.t4));
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════
// get_connection_zones
// ═══════════════════════════════════════════════════════════════════════════

std::vector<WoodJoint> get_connection_zones(
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    SearchType search_type) {

    if (TRACE)
        fmt::print(stderr, "[GCZ] enter  n_elems={}  search_type={}\n", wood_elems.size(), (int)search_type);

    using namespace wood_session::globals;
    const std::string short_name = DATA_SET_INPUT_NAME;
    const double dihedral_threshold = FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE;

    GczTimes times;
    times.t0 = GczClock::now();

    const std::string adj_name = DATA_SET_ADJACENCY;
    const std::string tv_name = DATA_SET_THREE_VALENCE;
    const std::string iv_name = DATA_SET_INSERTION_VECTORS;
    const std::string jt_name = DATA_SET_JOINTS_TYPES;
    const std::vector<double> ext_vec = JOINT_VOLUME_EXTENSION;

    if (TRACE)
        fmt::print("\n=== {}.obj ===\n", short_name);

    times.t1 = GczClock::now();

    const std::vector<std::pair<int, int>> adjacency_pairs = gcz_adjacency(adj_name, wood_elems);
    times.t2 = GczClock::now();

    const GczDetectParams params{
        ext_vec,
        LIMIT_MIN_JOINT_LENGTH,
        1e-6,
        DISTANCE_SQUARED,
        dihedral_threshold,
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED,
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE};

    wood_session::set_cross_joint_distance_squared(DISTANCE_SQUARED);

    gcz_load_insertion_vectors(iv_name, wood_elems);

    GczDetectStats stats;
    std::vector<WoodJoint> all_joints = gcz_detect(wood_elems, adjacency_pairs, params, search_type, stats);
    times.t3 = GczClock::now();

    gcz_three_valence(tv_name, wood_elems, all_joints);

    const std::vector<std::vector<int>> per_element_joints_types = gcz_joint_types(jt_name, wood_elems);

    times.t3a = GczClock::now();
    gcz_geometry(all_joints, wood_elems, per_element_joints_types);
    times.t3b = GczClock::now();
    if (TRACE)
        fmt::print(stderr, "[GCZ] geometry dispatch done  all_joints={}\n", all_joints.size());

    const JMF j_mf = gcz_build_jmf(wood_elems, all_joints);
    times.t3c = GczClock::now();
    if (TRACE)
        fmt::print(stderr, "[GCZ] j_mf built  starting merge\n");

    gcz_merge(wood_elems, j_mf, all_joints);
    times.t4 = GczClock::now();

    if (TRACE)
        gcz_report(wood_elems, adjacency_pairs, stats, times);
    for (WoodJoint& j : all_joints)
        j.sync_features();
    return all_joints;
}

/// In-memory overload: insertion vectors and joint types land on the plates, adjacency and three-valence in thread-locals.
std::vector<wood_session::WoodJoint> get_connection_zones(
    std::vector<std::shared_ptr<wood_session::Plate>>& elements,
    SearchType search_type,
    const wood_session::ChevronJoineryData& joinery_data) {
    for (size_t ei = 0; ei < elements.size(); ++ei) {
        if (elements[ei]->insertion_vectors().empty() && ei < joinery_data.insertion_vectors.size()) {
            const auto& iv18 = joinery_data.insertion_vectors[ei];
            std::vector<Vector>& ivec = elements[ei]->insertion_vectors();
            for (int s = 0; s < 6; ++s)
                ivec.emplace_back(iv18[s * 3 + 0], iv18[s * 3 + 1], iv18[s * 3 + 2]);
        }
    }

    for (size_t ei = 0; ei < elements.size(); ++ei) {
        if (elements[ei]->joint_types.empty() && ei < joinery_data.joints_per_face.size()) {
            const auto& jt6 = joinery_data.joints_per_face[ei];
            elements[ei]->joint_types.assign(jt6.begin(), jt6.end());
        }
    }

    // RAII clear: a throw inside the pipeline must not leave this model's adjacency for the next solve on this thread.
    struct TlOverrideGuard {
        ~TlOverrideGuard() {
            std::vector<std::pair<int, int>>().swap(tl_adjacency_override);
            std::vector<std::vector<int>>().swap(tl_three_valence_override);
        }
    } tl_guard;

    tl_adjacency_override = joinery_data.adjacency;

    tl_three_valence_override.clear();
    tl_three_valence_override.push_back({0});
    for (const auto& tv : joinery_data.three_valence)
        tl_three_valence_override.push_back({tv[0], tv[1], tv[2], tv[3]});

    return get_connection_zones(elements, search_type);
}
