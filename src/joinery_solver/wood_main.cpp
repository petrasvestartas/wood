// ═══════════════════════════════════════════════════════════════════════════
// wood/wood_main.cpp — wood joint detection pipeline orchestration.
//
// Public entries:
//   get_connection_zones(vector<Plate>&, SearchType) -> vector<WoodJoint>
//     Mutates each element's `features` (top/bottom merged outlines).
//     Returns every detected joint with full geometry.
//   fill_session(Session&, elements, joints, include_loft = true)
//     Splats the result into a Session for .pb persistence / visualization.
//
// Pipeline stages:
//   1. BVH broad-phase to find candidate adjacent element pairs
//      (adjacency_search, wood_face_to_face.cpp).
//   2. For every adjacent pair, call face_to_face_wood to classify the
//      joint (type 11/12/13/20/30/40) and compute area / lines / volumes.
//   3. Three-valence alignment or shadow-joint insertion (if tv file exists).
//   4. joint_create_geometry dispatch → unit-cube outlines from joint library.
//   5. joint_orient_to_connection_area → world-frame outlines.
//   6. merge_joints_for_element → cut outlines into plate polylines, then
//      de-interleaved into each element's `features`.
//
// Globals (wood_globals.cpp / wood_session.h): tuning parameters read by the
// pipeline. Tests call reset_defaults() then override specific entries before
// calling get_connection_zones. Auxiliary per-dataset txt files (adjacency,
// three_valence, insertion_vectors, joints_types) are resolved from
// globals::DATA_SET_INPUT_NAME which load_plates() sets.
// ═══════════════════════════════════════════════════════════════════════════

#include "../src/session.h"
#include "../src/element.h"
#include "../src/intersection.h"
#include "../src/plane.h"
#include "../src/polyline.h"
#include "../src/line.h"
#include "../src/vector.h"
#include "../src/point.h"
#include "../src/xform.h"
#include "../src/tolerance.h"
#include "../src/mesh.h"
#include "wood_element_plate.h"
#include "wood_face_to_face.h"
#include "wood_joint.h"
#include "wood_merge.h"
#include "wood_session.h"
#include <fmt/core.h>
#include <chrono>
#include <filesystem>
#include <vector>
#include <array>
#include <optional>
#include <utility>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <fstream>
#include <sstream>
#include <string>

using namespace session_cpp;

// In-memory overrides for ChevronJoineryData path — avoids writing/reading
// __FILE__-relative temp files that don't exist on PyPI wheel installs.
static thread_local std::vector<std::pair<int,int>>  tl_adjacency_override;
static thread_local std::vector<std::vector<int>>     tl_three_valence_override;

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


// ───────────────────────────────────────────────────────────────────────────
// Joint library: all joint geometry constructors live in `wood_joint_lib.h`
// (the session equivalent of wood's `wood_joint_lib.cpp`). Adding a new
// joint variant means editing that header AND wiring it into the dispatcher
// in `joint_create_geometry` further down this file.
// ───────────────────────────────────────────────────────────────────────────
#include "wood_joint_lib.h"


// Wood `joint::orient_to_connection_area` (`wood_joint.cpp:270-355`) has an
// optional unit_scale block at lines 276-319. When `joint.unit_scale` is
// true, both joint volume rectangles are moved toward each other along the
// Z axis (joint-line direction) so their separation equals
// `unit_scale_distance` instead of the natural joint length. This prevents
// `change_basis` from stretching the unit-cube joint outline along Z when
// the joint length differs from a fixed-size template. Annen joints don't
// enable it, so this is dormant for the current test datasets — but it's
// here for any future joint variant that does (`ss_e_op_2..6`, `ts_e_p_4/5`,
// `ss_e_r_3`, `ss_e_ip_5`).

// Create unit joinery geometry based on `id_representing_joint_name`.
//
// Wood's variant dispatcher lives at `wood_joint_lib.cpp:6075-6448`. Given a
// per-face joint id (from the `JOINTS_TYPES` file), it picks the family by
// numeric range and then the variant by exact id within the family. The
// `JOINT_NAMES` comment table in `wood_globals.cpp:38-47` is **misleading**:
// it claims `JOINT_NAMES[10] = ss_e_op_0`, but the actual switch at
// `wood_joint_lib.cpp:6337-6340` calls `ss_e_op_1` for case 10. Annen uses
// id=10 → `ss_e_op_1` → 8-point outline → byte-exact wood reference match.
//
// Pass `id = -1` when no `JOINTS_TYPES` file is loaded — the function then
// falls back to a topology-based default that mirrors wood's `default:`
// branches. This keeps behavior unchanged for datasets without a per-face
// id (annen_box_pair, hexbox).
static void joint_create_geometry(WoodJoint& joint, double division_distance,
                                  double shift_param, int id,
                                  std::vector<WoodJoint>* all_joints = nullptr,
                                  const std::vector<std::shared_ptr<Plate>>* elements = nullptr) {
    joint_get_divisions(joint, division_distance);
    joint.shift = shift_param;

    if (id == 0) { return; } // already filtered upstream; defensive only

    // Type-id compatibility check (wood_joint_lib.cpp:6109-6140). Wood's
    // group assignment requires id to fall in the type's range OR be -1.
    // If id is present but incompatible with the joint's detected type,
    // wood falls through to empty-joint detection and skips emission.
    // Mirror by returning early without setting any outlines.
    auto id_matches_type = [](int t, int jid) -> bool {
        if (jid == -1) { return true; }
        switch (t) {
            case 11: return jid >= 10 && jid <= 19;
            case 12: return jid >= 1  && jid <= 9;
            case 13: return jid >= 50 && jid <= 59;
            case 20: return jid >= 20 && jid <= 29;
            case 30: return jid >= 30 && jid <= 39;
            case 40: return jid >= 40 && jid <= 49;
            case 60: return jid >= 60 && jid <= 69;
        }
        return false;
    };
    if (!id_matches_type(joint.joint_type, id)) { return; }

    // Wood's id-family ranges (`wood_joint_lib.cpp:6113-6140`):
    //   1-9   → group 0 (ss_e_ip)
    //   10-19 → group 1 (ss_e_op)
    //   20-29 → group 2 (ts_e_p)
    //   30-39 → group 3 (cr_c_ip)
    //   40-49 → group 4 (tt_e_p)
    //   50-59 → group 5 (ss_e_r)
    //   60-69 → group 6 (b)
    int group = -1;
    if (id >= 1  && id <= 9 ) {
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
    }
    else {
        // No JOINTS_TYPES file (id < 0). Infer the group from the joint's
        // detected topology so existing datasets without a per-face id keep
        // working unchanged.
        switch (joint.joint_type) {
            case 11: group = 1; break; // ss_e_op
            case 12: group = 0; break; // ss_e_ip
            case 13: group = 5; break; // ss_e_r
            case 20: group = 2; break; // ts_e_p
            case 30: group = 3; break; // cr_c_ip
            case 40: group = 4; break; // tt_e_p
            default: group = -1;
        }
    }

    // Warn-once per unique id when an unimplemented case is reached. The
    // constructor is still called (falls through to the group's default)
    // so behaviour is unchanged — but the first encounter is now visible
    // in the log instead of silently producing wrong geometry.
    // thread_local: this file supports concurrent get_connection_zones
    // calls (see the thread_local overrides below); a shared static set
    // mutated without synchronization was a data race.
    static thread_local std::set<int> warned_ids;
    auto warn_unimpl = [&](const char* family) {
        if (warned_ids.insert(id).second) {
            fmt::print("joint_create_geometry: id={} ({}) not ported, using family default\n",
                       id, family);
        }
    };

    switch (group) {
        // ── ss_e_ip (side-side in-plane, type 12) ─ wood_joint_lib.cpp:6290-6332
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

        // ── ss_e_op (side-side out-of-plane, type 11) ─ wood_joint_lib.cpp:6334-6392
        case 1:
            switch (id) {
                case 10: ss_e_op_1(joint); break;
                case 11: ss_e_op_2(joint); break;
                case 12: ss_e_op_0(joint); break;
                case 13: ss_e_op_3(joint); break;
                case 14: ss_e_op_4(joint, 0.0, true); break;
                case 15:
                    if (all_joints) { ss_e_op_5(joint, *all_joints, false); }
                    else { ss_e_op_4(joint); }
                    break;
                case 16:
                    if (all_joints) { ss_e_op_5(joint, *all_joints, true); }
                    else { ss_e_op_4(joint); }
                    break;
                case 17: ss_e_op_17(joint); break;
                case 18: ss_e_op_tutorial(joint); break;
                case 19: ss_e_op_custom(joint); break;
                default:
                    // Wood falls to ss_e_op_1 by default (wood_joint_lib.cpp:6387).
                    // Session's legacy sentinel `id=-1` (no JOINTS_TYPES file,
                    // vidy-style) keeps the shadow-joint-linking path alive.
                    if (id < 0) {
                        if (all_joints) { ss_e_op_5(joint, *all_joints, false); }
                        else { ss_e_op_4(joint); }
                    } else {
                        warn_unimpl("ss_e_op");
                        ss_e_op_1(joint);
                    }
                    break;
            }
            break;

        // ── ts_e_p (top-side, type 20) ─ wood_joint_lib.cpp:6395-6448
        case 2:
            switch (id) {
                case 20: ts_e_p_3(joint); break;   // wood default
                case 21: ts_e_p_2(joint); break;
                case 22: ts_e_p_3(joint); break;   // wood also routes 22 → ts_e_p_3
                case 23: ts_e_p_0(joint); break;
                case 25: ts_e_p_5(joint); break;
                case 28: if (elements) side_removal(joint, *elements); break;
                case 29: ts_e_p_custom(joint); break;
                // Not ported: 24 (ts_e_p_4, 458-line chamfer/extension).
                default: warn_unimpl("ts_e_p"); ts_e_p_3(joint); break;
            }
            break;

        // ── cr_c_ip (cross, type 30) ─ wood_joint_lib.cpp:6450-6502
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

        // ── tt_e_p (top-top, type 40) ─ wood_joint_lib.cpp:6490-6523
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

        // ── ss_e_r (side-side rotated, type 13) ─ wood_joint_lib.cpp:6525-6553
        case 5:
            switch (id) {
                case 54: ss_e_r_3(joint); break;
                case 55: ss_e_r_2(joint); break;
                case 56: ss_e_r_0(joint); break;
                case 57: if (elements) side_removal(joint, *elements); break;
                case 58:
                    if (elements) { side_removal_ss_e_r_1_port(joint, *elements); }
                    else { ss_e_r_0(joint); }
                    break;
                case 59: ss_e_r_custom(joint); break;
                default: warn_unimpl("ss_e_r"); ss_e_r_0(joint); break;
            }
            break;

        // ── b (beam, type 60) ─ wood_joint_lib.cpp:6557-6583
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
                case 20:          ts_e_p_3(joint);  break;
                default: break;
            }
            break;
    }
}

/// Order-independent key for an element pair.
static uint64_t gcz_pair_key(int a, int b) {
    if (a > b) { std::swap(a, b); }
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

// side_removal_ss_e_r_1 — simplified port of wood_joint_lib.cpp:2723-3100.
// Wood's full function swaps v0/v1 + joint_lines/volumes, extends side-face
// corners by convex-corner check, offsets by plate-plane normals, and builds
// a pline0_moved/pline1_moved set as outlines. We skip the optional
// `shift>0 && merge_with_joint` branch (clipper offset + conic cut; would
// require porting `clipper_util::offset_in_3d`, `get_intersection_between_two_polylines`
// boolean, and `ss_e_r_1` unit geometry — ~1000 lines total). That branch
// only adds extra conic/mill cuts; the base mill_project cuts we emit here
// should match wood's primary plate-side removal.
//
// joint.el_ids: (v0, v1) with f0_0 and f1_0 the side-face indices.
// wood_elems[v0]->polylines[f0_0] is the 5-pt side rectangle on plate v0.
static void three_valence_joint_addition_vidy(
    const std::vector<std::vector<int>>& tv_groups,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& joints,
    std::unordered_map<uint64_t, int>& joints_map,
    const std::vector<std::pair<int,int>>& /*adjacency_pairs*/)
{
    if (tv_groups.size() < 2) { return; }

    // Pre-reserve to prevent reallocation during push_back (which would
    // invalidate joints[id] references). Each group can add up to 2 joints.
    joints.reserve(joints.size() + (tv_groups.size() - 1) * 2);

    // FIX #2: Proper CGAL-compatible squared distance from point to plane.
    // CGAL::squared_distance(point, plane) = (a*px+b*py+c*pz+d)^2 / (a^2+b^2+c^2)
    auto sq_dist_pt_plane = [](const Point& p, const Plane& pl) -> double {
        double v = pl.a()*p[0] + pl.b()*p[1] + pl.c()*p[2] + pl.d();
        double n2 = pl.a()*pl.a() + pl.b()*pl.b() + pl.c()*pl.c();
        return (n2 > 1e-20) ? (v * v) / n2 : (v * v);
    };

    for (size_t gi = 1; gi < tv_groups.size(); gi++) {
        auto& g = tv_groups[gi];
        if (g.size() != 4) { continue; }
        int s0 = g[0], s1 = g[1], e20 = g[2], e31 = g[3];
        int n_elems = (int)elements.size();
        if (s0 < 0 || s1 < 0 || e20 < 0 || e31 < 0) { continue; }
        if (s0 >= n_elems || s1 >= n_elems) { continue; }
        // Match wood's behavior: out-of-range e20/e31 causes UB → huge vsum → return
        if (e20 >= n_elems || e31 >= n_elems) { return; }

        // Parallel check matching wood's is_same_direction(can_be_flipped=true)
        // wood: is_parallel_to != 0 → |cos_angle| >= cos(0.11) ≈ 0.994
        if (e20 != e31) {
            auto is_parallel_wood = [](const Vector& a, const Vector& b) -> bool {
                double ll = a.magnitude() * b.magnitude();
                if (ll <= 0.0) { return false; }
                return std::abs(a.dot(b) / ll) >= std::cos(wood_session::globals::ANGLE);
            };
            Vector n_s0 = elements[s0]->planes[0].z_axis();
            Vector n_e31 = elements[e31]->planes[0].z_axis();
            Vector n_s1 = elements[s1]->planes[0].z_axis();
            Vector n_e20 = elements[e20]->planes[0].z_axis();
            if (!is_parallel_wood(n_s0, n_e31) || !is_parallel_wood(n_s1, n_e20)) { continue; }
        }

        // Find primary joint between s0-s1
        auto it = joints_map.find(gcz_pair_key(s0, s1));
        if (it == joints_map.end()) { continue; }
        int id = it->second;
        if (!joints[id].joint_volumes_pair_a_pair_b[0].has_value()) { continue; }

        // Find nearest/farthest planes between element pairs
        double d00 = sq_dist_pt_plane(elements[s0]->planes[0].origin(), elements[e31]->planes[0]);
        double d01 = sq_dist_pt_plane(elements[s0]->planes[0].origin(), elements[e31]->planes[1]);
        Plane plane00_far = d00 < d01 ? elements[e31]->planes[0] : elements[e31]->planes[1];

        d00 = sq_dist_pt_plane(plane00_far.origin(), elements[s0]->planes[0]);
        d01 = sq_dist_pt_plane(plane00_far.origin(), elements[s0]->planes[1]);
        Plane plane01_near = d00 < d01 ? elements[s0]->planes[1] : elements[s0]->planes[0];

        double d10 = sq_dist_pt_plane(elements[s1]->planes[0].origin(), elements[e20]->planes[0]);
        double d11 = sq_dist_pt_plane(elements[s1]->planes[0].origin(), elements[e20]->planes[1]);
        Plane plane10_far = d10 < d11 ? elements[e20]->planes[0] : elements[e20]->planes[1];

        d10 = sq_dist_pt_plane(plane10_far.origin(), elements[s1]->planes[0]);
        d11 = sq_dist_pt_plane(plane10_far.origin(), elements[s1]->planes[1]);
        Plane plane11_near = d10 < d11 ? elements[s1]->planes[1] : elements[s1]->planes[0];

        // Joint volume edge lines for projection (wood lines 1685-1686)
        auto& jvol = *joints[id].joint_volumes_pair_a_pair_b[0];
        Line l0 = Line::from_points(jvol.get_point(0), jvol.get_point(1));
        Line l1 = Line::from_points(jvol.get_point(1), jvol.get_point(2));

        // Determine which edge is parallel to which projection (wood 1703-1730)
        // Project volume edges onto plane01_near and check parallelism
        Point proj_p0 = plane01_near.project(jvol.get_point(0));
        Point proj_p1 = plane01_near.project(jvol.get_point(1));
        Point proj_p2 = plane01_near.project(jvol.get_point(2));
        Vector proj_l1_dir(proj_p1[0]-proj_p2[0], proj_p1[1]-proj_p2[1], proj_p1[2]-proj_p2[2]);
        // is_parallel_01: 0 means NOT parallel (projection collapsed the edge)
        bool is_parallel_01 = (proj_l1_dir.is_parallel_to(l1.to_vector()) == 0);
        // Wood: if l1's projection is NOT parallel → use l1 first
        std::array<Line, 2> ll = is_parallel_01
            ? std::array<Line, 2>{l1, l0}
            : std::array<Line, 2>{l0, l1};

        // Find translation endpoints via line-plane intersection (infinite).
        // Wood does not check for failure here; it just calls line_plane unconditionally
        // and then overrides p10/p11 when e20==e31. We must skip the e20==e31 checks
        // to avoid a spurious continue when ll[1] is parallel to the plane.
        Point p00, p01, p10, p11;
        if (!Intersection::line_plane(ll[0], plane00_far, p00, false)) { continue; }
        if (!Intersection::line_plane(ll[0], plane01_near, p01, false)) { continue; }
        if (e20 == e31) {
            p10 = p00; p11 = p01;
        } else {
            if (!Intersection::line_plane(ll[1], plane10_far, p10, false)) { continue; }
            if (!Intersection::line_plane(ll[1], plane11_near, p11, false)) { continue; }
        }

        Vector trans0(p00[0]-p01[0], p00[1]-p01[1], p00[2]-p01[2]);
        Vector trans1(p10[0]-p11[0], p10[1]-p11[1], p10[2]-p11[2]);

        // Validate translations (wood lines 1767-1778, signed sum check)
        double vsum = trans0[0] + trans0[1] + trans0[2]
                    + trans1[0] + trans1[1] + trans1[2];
        if (vsum < -1e8 || vsum > 1e8) { continue; }

        // Copy joint volumes (wood lines 1761-1762)
        auto copy_vols = [&]() -> std::array<Polyline, 4> {
            std::array<Polyline, 4> vols;
            for (int k = 0; k < 4; k++) {
                if (joints[id].joint_volumes_pair_a_pair_b[k].has_value()) {
                    vols[k] = *joints[id].joint_volumes_pair_a_pair_b[k];
                }
            }
            return vols;
        };
        auto jv0_copy = copy_vols();
        auto jv1_copy = copy_vols();

        // Shift jv1 volumes to align with translation direction (wood 1782-1796)
        int shift_amt = 0;
        for (int j = 0; j < 4; j++) {
            Vector v(jv1_copy[0].get_point(j)[0] - jv1_copy[0].get_point(j+1)[0],
                     jv1_copy[0].get_point(j)[1] - jv1_copy[0].get_point(j+1)[1],
                     jv1_copy[0].get_point(j)[2] - jv1_copy[0].get_point(j+1)[2]);
            if (v.is_parallel_to(trans1) == 1) {
                shift_amt = j; break;
            }
        }
        for (size_t k = 0; k < 4; k++) {
            if (jv1_copy[k].point_count() == 5) {
                jv1_copy[k].shift(shift_amt);
            }
        }

        // Copy joint lines (wood lines 1798-1799)
        auto jlines0 = joints[id].joint_lines;
        auto jlines1 = joints[id].joint_lines;

        // FIX #3: Translate BOTH volumes AND lines (wood lines 1802-1806)
        for (int k = 0; k < 2; k++) {
            jv0_copy[k].translate(trans0);
            jv1_copy[k].translate(trans1);
            // Translate joint lines too
            Point js0 = jlines0[k].start();
            Point je0 = jlines0[k].end();
            jlines0[k] = Line::from_points(
                Point(js0[0]+trans0[0], js0[1]+trans0[1], js0[2]+trans0[2]),
                Point(je0[0]+trans0[0], je0[1]+trans0[1], je0[2]+trans0[2]));
            Point js1 = jlines1[k].start();
            Point je1 = jlines1[k].end();
            jlines1[k] = Line::from_points(
                Point(js1[0]+trans1[0], js1[1]+trans1[1], js1[2]+trans1[2]),
                Point(je1[0]+trans1[0], je1[1]+trans1[1], je1[2]+trans1[2]));
        }

        // Check if joint order was reversed (wood line 1813)
        if (index_of(elements, joints[id].element_a) == s1) {
            std::swap(e20, e31);
            std::swap(s0, s1);
        }

        // Shadow joints go to j_mf.back() — the extra slot beyond planes (wood line 1827-1828).
        // Face indices = -1 so the j_mf build loop routes them via the link==true path.
        // (wood: joints.emplace_back(..., -1, -1, -1, -1, ...))
        // A shadow joint borrows the original's area but was never detected on a
        // face pair of its own, so its contact type stays `unknown`.

        // Create shadow joint 0 (s0 ↔ e20) — wood lines 1824-1829
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
        int shadow0_idx = (int)joints.size();
        joints.push_back(std::move(shadow0));
        joints_map[gcz_pair_key(s0, e20)] = shadow0_idx;

        // Create shadow joint 1 if e20 != e31 — wood lines 1831-1839
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

        // Wire linked_joints on the primary joint — wood line 1841
        if (e20 != e31) {
            joints[id].linked_joints = {shadow0_idx, shadow1_idx};
        } else {
            joints[id].linked_joints = {shadow0_idx};
        }
    }
}

// ───────────────────────────────────────────────────────────────────────────
// Three-valence joint alignment (Annen method).
// Shortens overlapping joint lines at 3-plate intersections to avoid collisions.
// Ported from wood_main.cpp three_valence_joint_alignment_annen.
// ───────────────────────────────────────────────────────────────────────────
static void three_valence_joint_alignment_annen(
    const std::vector<std::vector<int>>& tv_groups,
    const std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& joints,
    const std::vector<std::pair<int,int>>& /*adjacency_pairs*/)
{
    const std::unordered_map<uint64_t, int> joints_map = gcz_joints_map(elements, joints);

    for (size_t gi = 1; gi < tv_groups.size(); gi++) {
        auto& g = tv_groups[gi];
        if (g.size() != 4) { continue; }
        int s0 = g[0], s1 = g[1], e20 = g[2], e31 = g[3];

        auto it0 = joints_map.find(gcz_pair_key(s0, s1));
        auto it1 = joints_map.find(gcz_pair_key(e20, e31));
        if (it0 == joints_map.end() || it1 == joints_map.end()) { continue; }

        int id_0 = it0->second, id_1 = it1->second;
        auto& j0 = joints[id_0];
        auto& j1 = joints[id_1];

        // Compute overlap of the two joint lines.
        Line l0 = j0.joint_lines[0];
        // Orient l1 to match l0 direction.
        double d_s = Point::distance(l0.start(), j1.joint_lines[0].start());
        double d_e = Point::distance(l0.start(), j1.joint_lines[0].end());
        Line l1 = (d_s <= d_e)
            ? j1.joint_lines[0]
            : Line::from_points(j1.joint_lines[0].end(), j1.joint_lines[0].start());

        Line overlap;
        // A degenerate overlap (parallel-but-disjoint joint lines at a
        // 3-plate corner) used to fall through with whatever 'overlap'
        // contained and then get shrunk below, planting end caps at garbage
        // positions. Skip the group instead.
        if (!l0.overlap_average(l1, overlap)) { continue; }

        // Shorten by element thickness.
        double thickness = 0;
        int e0_idx = index_of(elements, j0.element_a);
        if (e0_idx >= 0 && e0_idx < (int)elements.size()) {
            Plate& el = *elements[e0_idx];
            if (el.polylines.size() >= 2 && el.polylines[0].point_count() > 0 && el.polylines[1].point_count() > 0) {
                auto p0 = el.polylines[0].get_point(0);
                auto p1_proj = el.planes[1].project(p0);
                thickness = Point::distance(p0, p1_proj);
            }
        }
        // Line::extend with a negative amount has no clamp: shrinking a short
        // shared edge by more than half its length INVERTS the segment
        // (start passes end) and the end-cap clip planes built from it then
        // clip the joint volumes inside-out.
        thickness = std::min(thickness, overlap.length() * 0.5 - 1e-9);
        if (thickness < 0.0) { thickness = 0.0; }
        overlap.extend(-thickness, -thickness);

        // Update both joint lines to the shortened overlap.
        j0.joint_lines[0] = overlap;
        j1.joint_lines[0] = overlap;

        // Clip joint volumes using planes at the overlap endpoints.
        auto vol_normal = [](const Polyline& vol) -> Vector {
            if (vol.point_count() < 3) { return Vector(0,0,1); }
            Point p0 = vol.get_point(0), p1 = vol.get_point(1), p2 = vol.get_point(2);
            Vector a(p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]);
            Vector b(p0[0]-p1[0], p0[1]-p1[1], p0[2]-p1[2]);
            return a.cross(b);
        };

        // Clip j0 volumes
        if (j0.joint_volumes_pair_a_pair_b[0].has_value()) {
            Vector cross0 = vol_normal(*j0.joint_volumes_pair_a_pair_b[0]);
            cross0.normalize_self();
            Point ol_s = overlap.start(), ol_e = overlap.end();
            Plane pl0_0 = Plane::from_point_normal(ol_s, cross0);
            Plane pl0_1 = Plane::from_point_normal(ol_e, cross0);
            for (int vp = 0; vp < 4; vp += 2) {
                if (!j0.joint_volumes_pair_a_pair_b[vp].has_value() || !j0.joint_volumes_pair_a_pair_b[vp+1].has_value()) { continue; }
                auto& v0 = *j0.joint_volumes_pair_a_pair_b[vp];
                auto& v1 = *j0.joint_volumes_pair_a_pair_b[vp+1];
                Line s0l = Line::from_points(v0.get_point(0), v1.get_point(0));
                Line s1l = Line::from_points(v0.get_point(1), v1.get_point(1));
                Line s2l = Line::from_points(v0.get_point(2), v1.get_point(2));
                Line s3l = Line::from_points(v0.get_point(3), v1.get_point(3));
                Intersection::plane_4lines(pl0_0, s0l, s1l, s2l, s3l, v0);
                Intersection::plane_4lines(pl0_1, s0l, s1l, s2l, s3l, v1);
            }
        }

        // Clip j1 volumes
        if (j1.joint_volumes_pair_a_pair_b[0].has_value()) {
            Vector cross1 = vol_normal(*j1.joint_volumes_pair_a_pair_b[0]);
            cross1.normalize_self();
            Point ol_s = overlap.start(), ol_e = overlap.end();
            Plane pl1_0 = Plane::from_point_normal(ol_e, cross1);
            Plane pl1_1 = Plane::from_point_normal(ol_s, cross1);
            for (int vp = 0; vp < 4; vp += 2) {
                if (!j1.joint_volumes_pair_a_pair_b[vp].has_value() || !j1.joint_volumes_pair_a_pair_b[vp+1].has_value()) { continue; }
                auto& v0 = *j1.joint_volumes_pair_a_pair_b[vp];
                auto& v1 = *j1.joint_volumes_pair_a_pair_b[vp+1];
                Line s0l = Line::from_points(v0.get_point(0), v1.get_point(0));
                Line s1l = Line::from_points(v0.get_point(1), v1.get_point(1));
                Line s2l = Line::from_points(v0.get_point(2), v1.get_point(2));
                Line s3l = Line::from_points(v0.get_point(3), v1.get_point(3));
                Intersection::plane_4lines(pl1_0, s0l, s1l, s2l, s3l, v0);
                Intersection::plane_4lines(pl1_1, s0l, s1l, s2l, s3l, v1);
            }
        }
    }
}

// ───────────────────────────────────────────────────────────────────────────
// merge_joints: merge oriented joint cuts into element plate polylines.
// Produces per-element output: [merged_top, merged_bottom, hole0_t, hole0_b, ...]
//
// j_mf[element_id][face_id] = vector of (joint_index, is_male)
// ───────────────────────────────────────────────────────────────────────────
using JMF = std::vector<std::vector<std::vector<std::pair<int,bool>>>>;
// Verbatim port of `wood::element::merge_joints` (wood_element.cpp:654-1424).
// Operates on CLOSED polylines (size = open_count + 1) so the wood indexing
// `id = i-2`, `prev = (n+id-1)%n`, `next = (id+1)%n`, `n = pline0.size()-1`
// applies 1:1. Mutates the joints vector in two places (joint.reverse swaps
// the m[0]↔m[1] or f[0]↔f[1] outline slots) — same mutation strategy as wood.
//
// Handles both case (2) — line joint with 2-point endpoint marker — and
// case (5) — rectangle joint with 5-point endpoint marker — matching
// `wood_element.cpp:816-1108`.

} // anonymous namespace

//  definitions live in wood_globals.cpp.
//  helpers (session_data_dir, load_plates, plates_exist) live in
// wood_internal.cpp. Declarations for both live in wood_session.h.

// Public pipeline — takes pre-built WoodElements (use internal::load_plates or
// build_wood_element directly). Mutates each element's `features` with the
// merged top/bottom outlines. Returns every detected WoodJoint with full
// geometry (area, lines, volumes, male/female cut outlines).
//
// Reads dataset auxiliary files (adjacency, tv, iv, jt) via DATA_SET_INPUT_NAME global.
//
// To visualize the result, call fill_session(session, elements, joints, true).
// [GCZ] pipeline trace. Off unless WOOD_TRACE is set. The trace emitted one
// fprintf + fflush per adjacency pair and per joint, and an unbuffered write
// syscall each is measurable on assemblies with hundreds of joints.
static bool wood_trace_enabled() {
    static const bool on = (std::getenv("WOOD_TRACE") != nullptr);
    return on;
}

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

/// Detection counters: successes, failures and successes per joint type.
struct GczDetectStats {
    int counts[6] = {0, 0, 0, 0, 0, 0}; // [11, 12, 13, 20, 30, 40]
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

/// Stage boundaries for the verbose timing report.
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

/// WOOD_EL_DUMP=<path>: dump element polylines for comparison with wood.
static void gcz_element_dump(const std::vector<std::shared_ptr<Plate>>& wood_elems) {
    if (const char* ep = std::getenv("WOOD_EL_DUMP")) {
        const char* df = std::getenv("DIAG_TEST");
        if (!df || wood_session::globals::DATA_SET_INPUT_NAME == df) {
            std::ofstream el_log(ep);
            for (size_t ei = 0; ei < wood_elems.size(); ei++) {
                const Plate& we = *wood_elems[ei];
                el_log << "ELEMENT " << ei << " reversed=" << (we.reversed ? 1 : 0)
                       << " thickness=" << we.thickness
                       << " n_polylines=" << we.polylines.size() << "\n";
                for (size_t pi = 0; pi < we.polylines.size(); pi++) {
                    el_log << "  poly[" << pi << "] pts=" << we.polylines[pi].point_count() << ":";
                    for (size_t k = 0; k < we.polylines[pi].point_count(); k++) {
                        Point p = we.polylines[pi].get_point(k);
                        el_log << " (" << p[0] << "," << p[1] << "," << p[2] << ")";
                    }
                    el_log << "\n";
                }
            }
        }
    }
}

/// Stage 1: candidate pairs from the adjacency sidecar, the thread-local override or the OBB+BVH search.
static std::vector<std::pair<int, int>> gcz_adjacency(
    const std::string& adj_name,
    const std::vector<std::shared_ptr<Plate>>& wood_elems,
    const bool verbose) {
    // The OBB+BVH path reads each Plate's top/bottom polylines
    // (we.polylines[0]/[1]) directly — they carry every corner the bounding
    // volumes need.
    std::vector<std::pair<int, int>> adjacency_pairs;
    if (!adj_name.empty()) {
        std::ifstream adj_in(adj_name);
        int a;
        int b;
        while (adj_in >> a >> b) { adjacency_pairs.emplace_back(a, b); }
        if (verbose) { fmt::print("adjacency: {} pairs from {}\n", adjacency_pairs.size(), adj_name); }
    }
    // In-memory override: set by ChevronJoineryData overload to skip BVH search.
    if (adjacency_pairs.empty() && !tl_adjacency_override.empty()) {
        adjacency_pairs = tl_adjacency_override;
    }

    if (adjacency_pairs.empty()) {
        const double distance = wood_session::globals::DISTANCE;
        if (wood_trace_enabled()) { fprintf(stderr, "[GCZ] adjacency_search start  DISTANCE=%g\n", distance); fflush(stderr); }
        std::vector<wood_session::ContactElement> view;
        view.reserve(wood_elems.size());
        for (const std::shared_ptr<Plate>& plate : wood_elems) {
            view.emplace_back(*plate);
        }
        adjacency_pairs = wood_session::adjacency_search(view, distance);
        if (wood_trace_enabled()) { fprintf(stderr, "[GCZ] adjacency pairs=%zu\n", adjacency_pairs.size()); fflush(stderr); }
        if (verbose) { fmt::print("adjacency: {} pairs from OBB+BVH\n", adjacency_pairs.size()); }
    }
    return adjacency_pairs;
}

/// Stage 2: per-element insertion vectors from the sidecar into each Plate, reversed plates flipped.
static void gcz_load_insertion_vectors(
    const std::string& iv_name,
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    const bool verbose) {
    // Wood's annen XML stores one <insertion_vectors> block per element, each
    // with `n_faces` <vector> entries (faces 0/1 = top/bottom = (0,0,0);
    // faces 2..N = side faces with the assembly direction the carpenter
    // slides the joint along). We pre-extract these to a flat .txt file (one
    // element per line, all vectors as space-separated `x y z x y z ...`).
    //
    // Empty file → empty per-element vectors → face_to_face_wood's
    // `dir_set` stays false and the algorithm falls back to the orthogonal
    // perpendicular vector helper (which is geometrically valid but
    // produces only orthogonal joints — same as before this load).
    std::vector<std::vector<Vector>> per_element_insertion_vectors(
        wood_elems.size(), std::vector<Vector>{});
    if (!iv_name.empty()) {
        std::ifstream iv_in(iv_name);
        std::string iv_line;
        size_t ei = 0;
        size_t total_loaded = 0;
        while (std::getline(iv_in, iv_line) && ei < per_element_insertion_vectors.size()) {
            std::istringstream iss(iv_line);
            std::vector<Vector>& vecs = per_element_insertion_vectors[ei];
            double x;
            double y;
            double z;
            while (iss >> x >> y >> z) {
                vecs.emplace_back(x, y, z);
                total_loaded++;
            }
            ei++;
        }
        if (verbose) { fmt::print("insertion_vectors: {} vectors across {} elements from {}\n",
                               total_loaded, ei, iv_name); }
    }
    // Move insertion vectors into Plate so face_to_face_wood has one object per plate.
    // Skip assignment when the element already has vectors pre-set by the caller
    // (_joinery_solver.cpp iv-only path); reversal still applies in that case.
    for (size_t ei = 0; ei < wood_elems.size(); ei++) {
        if (wood_elems[ei]->insertion_vectors().empty()) {
            wood_elems[ei]->insertion_vectors() = per_element_insertion_vectors[ei];
        }
        if (wood_elems[ei]->reversed) {
            auto& vecs = wood_elems[ei]->insertion_vectors();
            if (vecs.size() > 2) {
                std::reverse(vecs.begin() + 2, vecs.end());
            }
        }
    }
}

/// Stage 3: run face_to_face_wood on every adjacent pair; joints stay in adjacency-pair order.
static std::vector<WoodJoint> gcz_detect(
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    const std::vector<std::pair<int, int>>& adjacency_pairs,
    const GczDetectParams& params,
    const SearchType search_type,
    const bool verbose,
    GczDetectStats& stats) {
    std::vector<WoodJoint> all_joints;
    // WoodJoint is heavyweight (several Polyline vectors + strings);
    // growth reallocation moves the whole population repeatedly.
    all_joints.reserve(adjacency_pairs.size());
    if (wood_trace_enabled()) { fprintf(stderr, "[GCZ] joint detection loop  pairs=%zu\n", adjacency_pairs.size()); fflush(stderr); }
    const int n_wood_elems = static_cast<int>(wood_elems.size());
    for (size_t k = 0; k < adjacency_pairs.size(); ++k) {
        const int ia = adjacency_pairs[k].first;
        const int ib = adjacency_pairs[k].second;
        if (wood_trace_enabled()) { fprintf(stderr, "[GCZ]   pair k=%zu  ia=%d ib=%d\n", k, ia, ib); fflush(stderr); }

        // The adjacency list need not agree with the element list. It may come
        // from a file next to the dataset, or straight from a caller through
        // the Python bindings, so its indices are input rather than an
        // invariant. Indexing wood_elems with them unchecked segfaulted on a
        // reference to element 0 of an empty vector - vector::size() with
        // this=0x18 - so a dataset whose .obj had not loaded took the entire
        // sweep down instead of skipping one entry.
        if (ia < 0 || ib < 0 || ia >= n_wood_elems || ib >= n_wood_elems) {
            fprintf(stderr,
                    "  WARNING: adjacency pair %zu references elements (%d, %d) "
                    "but only %d were loaded - skipping.\n",
                    k, ia, ib, n_wood_elems);
            fflush(stderr);
            continue;
        }

        WoodJoint joint;
        bool swap_planes_b = false;
        const bool ok = face_to_face_wood(
            k,
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
        if (wood_trace_enabled()) {
            fprintf(stderr, "[GCZ]   face_to_face_wood done  ok=%d  type=%d\n",
                    (int)ok, ok ? joint.joint_type : -1); fflush(stderr);
        }
        if (swap_planes_b) {
            std::swap(wood_elems[ib]->planes[0], wood_elems[ib]->planes[1]);
            std::swap(wood_elems[ib]->polylines[0], wood_elems[ib]->polylines[1]);
        }
        if (!ok) {
            if (verbose && !joint.dbg_fail_reason.empty()) {
                fmt::print("  FAIL pair ({},{}) coplanar={} boolean={} reason={}\n",
                           ia, ib, joint.dbg_coplanar, joint.dbg_boolean, joint.dbg_fail_reason);
            }
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
        while (iss >> v) { row.push_back(v); }
        if (!row.empty()) { rows.push_back(row); }
    }
    return rows;
}

/// Stage 4: three-valence groups from the sidecar or the thread-local override; 0 = annen alignment, 1 = vidy addition.
static void gcz_three_valence(
    const std::string& tv_name,
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    std::vector<WoodJoint>& all_joints,
    const std::vector<std::pair<int, int>>& adjacency_pairs,
    const bool verbose) {
    if (!tv_name.empty()) {
        const std::vector<std::vector<int>> tv_groups = gcz_read_int_rows(tv_name);
        if (tv_groups.size() > 1) {
            std::unordered_map<uint64_t, int> joints_map = gcz_joints_map(wood_elems, all_joints);
            // The first group's first element is the instruction flag:
            //   0 = annen alignment only
            //   1 = vidy addition (create shadow joints) + alignment
            const int instruction = tv_groups[0].empty() ? 0 : tv_groups[0][0];
            if (instruction == 1) {
                // Wood switch case 1 — only this runs, NOT annen alignment.
                const size_t before_vidy = all_joints.size();
                three_valence_joint_addition_vidy(tv_groups, wood_elems, all_joints, joints_map, adjacency_pairs);
                if (verbose) { fmt::print("vidy_addition: {} shadow joints created (total {})\n",
                                       all_joints.size() - before_vidy, all_joints.size()); }
            } else {
                three_valence_joint_alignment_annen(tv_groups, wood_elems, all_joints, adjacency_pairs);
            }
        }
        if (verbose) { fmt::print("three_valence: {} groups applied\n", tv_groups.size()); }
    } else if (!tl_three_valence_override.empty()) {
        // In-memory override: no TV file was found, but ChevronJoineryData overload
        // provided three_valence groups directly — run the same alignment logic.
        const std::vector<std::vector<int>>& tv_groups = tl_three_valence_override;
        if (tv_groups.size() > 1) {
            std::unordered_map<uint64_t, int> joints_map = gcz_joints_map(wood_elems, all_joints);
            const int instruction = tv_groups[0].empty() ? 0 : tv_groups[0][0];
            if (instruction == 1) {
                three_valence_joint_addition_vidy(tv_groups, wood_elems, all_joints, joints_map, adjacency_pairs);
            } else {
                three_valence_joint_alignment_annen(tv_groups, wood_elems, all_joints, adjacency_pairs);
            }
        }
    }
}

/// Stage 5: per-element per-face joint type ids (the wood JOINTS_TYPES filter).
static std::vector<std::vector<int>> gcz_joint_types(
    const std::string& jt_name,
    const std::vector<std::shared_ptr<Plate>>& wood_elems,
    const bool verbose) {
    // 6 ints per line, one line per element. Values:
    //   0  → no joint on this face (skip joint construction)
    //   1-9   → ss_e_ip variant id  (side-side in-plane,    type 12)
    //   10-19 → ss_e_op variant id  (side-side out-of-plane, type 11)
    //   20-29 → ts_e_p  variant id  (top-side,               type 20)
    //   30-39 → cr_c_ip variant id  (cross,                  type 30)
    //   40-49 → tt_e_p  variant id  (top-top,                type 40)
    //   50-59 → ss_e_r  variant id  (side-side rotated,      type 13)
    //   60-69 → b       variant id  (boundary,               type 60)
    std::vector<std::vector<int>> per_element_joints_types(wood_elems.size());
    if (!jt_name.empty()) {
        std::ifstream jt_in(jt_name);
        std::string jt_line;
        size_t ei = 0;
        size_t total_loaded = 0;
        while (std::getline(jt_in, jt_line) && ei < per_element_joints_types.size()) {
            std::istringstream iss(jt_line);
            int v;
            while (iss >> v) {
                per_element_joints_types[ei].push_back(v);
                total_loaded++;
            }
            ei++;
        }
        if (verbose) {
            fmt::print("joints_types: {} ids across {} elements from {}\n",
                       total_loaded, ei, jt_name);
        }
    }
    // In-memory override: element.joint_types set directly by _joinery_solver.cpp
    // (direct path, no temp files). Takes precedence only when no file data exists.
    for (size_t ei = 0; ei < wood_elems.size(); ++ei) {
        if (per_element_joints_types[ei].empty() && !wood_elems[ei]->joint_types.empty()) {
            per_element_joints_types[ei] = wood_elems[ei]->joint_types;
        }
    }
    return per_element_joints_types;
}

/// Wood's id_representing_joint_name: max of the two face ids in the JOINTS_TYPES table, -1 when the table says nothing.
static int gcz_representing_id(
    const WoodJoint& j,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const std::vector<std::shared_ptr<Plate>>& wood_elems) {
    // Sentinel `-1` = no JOINTS_TYPES file → topology-based default in
    // `joint_create_geometry`. Empty per-element vector = same effect.
    int id_representing_joint_name = -1;
    if (!per_element_joints_types.empty()) {
        const int e0 = index_of(wood_elems, j.element_a);
        const int e1 = index_of(wood_elems, j.element_b);
        const int f0 = j.contact.face_a;
        const int f1 = j.contact.face_b;
        // Remap post-reversal face index back to original face index for
        // JOINTS_TYPES lookup. build_wood_element may reverse the winding,
        // which reorders the side planes: orig_side_j = n_sides-1 - rev_side_j.
        // The joints_types file uses original (pre-reversal) face indices.
        auto orig_face = [&](int ei, int fi) -> int {
            if (ei < 0 || ei >= (int)wood_elems.size()) {
                return fi;
            }
            if (!wood_elems[ei]->reversed) {
                return fi;
            }
            if (fi < 2) {
                return 1 - fi; // top(0)↔bottom(1)
            }
            int n_sides = (int)wood_elems[ei]->planes.size() - 2;
            return 2 + (n_sides - 1 - (fi - 2));
        };
        const int of0 = orig_face(e0, f0);
        const int of1 = orig_face(e1, f1);
        const int id0 = (e0 >= 0 && e0 < (int)per_element_joints_types.size()
                         && of0 >= 0 && of0 < (int)per_element_joints_types[e0].size())
                        ? std::abs(per_element_joints_types[e0][of0]) : 0;
        const int id1 = (e1 >= 0 && e1 < (int)per_element_joints_types.size()
                         && of1 >= 0 && of1 < (int)per_element_joints_types[e1].size())
                        ? std::abs(per_element_joints_types[e1][of1]) : 0;
        // Only treat the file as authoritative if either element actually
        // had a non-empty per-face id list. Elements with an empty list
        // (e.g. parsing skipped a line) fall through to the topology
        // default rather than getting silently filtered.
        if (e0 >= 0 && e0 < (int)per_element_joints_types.size() &&
            e1 >= 0 && e1 < (int)per_element_joints_types.size() &&
            (per_element_joints_types[e0].size() > 0 ||
             per_element_joints_types[e1].size() > 0)) {
            id_representing_joint_name = std::max(id0, id1);
            // id == 0 means the face slot exists in the jt table but was not
            // explicitly assigned (was -1 → 0 after Python sentinel conversion).
            // Fall through to auto-detection (-1 path) instead of skipping.
            if (id_representing_joint_name == 0) {
                id_representing_joint_name = -1;
            }
        }
    }
    return id_representing_joint_name;
}

/// Per-family row of JOINTS_PARAMETERS_AND_TYPES: division length, shift and the default id when none was given.
static GczFamilyParams gcz_family_params(const int joint_type, const int id_representing_joint_name) {
    // Wood's dispatcher (wood_joint_lib.cpp:6191-6196) reads
    // `default_parameters_for_four_types[group*3+{0,1,2}]` where group is
    // derived from joint_type:
    //   11 → 1 (ss_e_op)   12 → 0 (ss_e_ip)   13 → 5 (ss_e_r)
    //   20 → 2 (ts_e_p)    30 → 3 (cr_c_ip)   40 → 4 (tt_e_p)   60 → 6 (b)
    auto row_for_type = [](int t) -> int {
        switch (t) {
            case 11: return 1;
            case 12: return 0;
            case 13: return 5;
            case 20: return 2;
            case 30: return 3;
            case 40: return 4;
            case 60: return 6;
            default: return 1;
        }
    };
    // JPT is input, not an invariant: it starts EMPTY at namespace scope,
    // reset_defaults() fills 21 entries, and YAML replaces it with a list
    // of any length. Indexing row*3+2 (up to 20) into a short vector was
    // UB reachable from a config edit. Fall back to the reset_defaults
    // table (kept in sync by inspection) when the global is unusable.
    static constexpr double JPT_DEFAULTS[21] = {
        300, 0.5,  3,
        450, 0.64, 15,
        450, 0.5,  20,
        300, 0.5,  30,
          6, 0.95, 40,
        300, 0.5,  58,
        300, 1.0,  60,
    };
    const auto& JPT_global = wood_session::globals::JOINTS_PARAMETERS_AND_TYPES;
    const bool jpt_ok = JPT_global.size() >= 21;
    static thread_local bool jpt_warned = false;
    if (!jpt_ok && !jpt_warned) {
        fprintf(stderr,
                "  WARNING: JOINTS_PARAMETERS_AND_TYPES has %zu entries, expected 21 - "
                "using built-in defaults.\n", JPT_global.size());
        fflush(stderr);
        jpt_warned = true;
    }
    auto JPT = [&](size_t idx) -> double {
        return jpt_ok ? JPT_global[idx] : JPT_DEFAULTS[idx];
    };
    const int row = row_for_type(joint_type);

    // id fallback: when no JOINTS_TYPES file contributed a per-face id, read
    // the default id from the per-type row (col 2). Wood defaults:
    // ss_e_ip=3, ss_e_op=15, ts_e_p=20, cr_c_ip=30, tt_e_p=40, ss_e_r=58, b=60.
    GczFamilyParams fam;
    fam.id = id_representing_joint_name;
    if (fam.id == -1) {
        fam.id = (int)JPT(row*3 + 2);
    }
    fam.div_dist = JPT(row*3 + 0);
    fam.shift = JPT(row*3 + 1);
    return fam;
}

/// Wood's get_key number format: std::to_string truncated at two decimals.
static std::string gcz_key_num(double v) {
    v += 1e-9;
    const std::string s = std::to_string(v);
    const auto dot = s.find('.');
    if (dot != std::string::npos && dot + 3 <= s.size()) {
        return s.substr(0, dot + 3);
    }
    return s;
}

/// Unit-geometry cache key: the id stands in for `name` (id→constructor is deterministic).
static std::string gcz_cache_key(const int id_representing_joint_name, const WoodJoint& j) {
    return std::to_string(id_representing_joint_name)
         + ";" + gcz_key_num(j.shift)
         + ";" + gcz_key_num((double)j.divisions);
}

/// Stage 6, one joint: unit geometry (cached for butterflies), orient to the connection area, merge linked shadows.
static void gcz_build_one_joint(
    WoodJoint& j,
    const GczFamilyParams& fam,
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    std::vector<WoodJoint>& all_joints,
    GczJointCache& unique_joints_cache) {
    // Multiplicative scale (sx, sy, sz) used by ss_e_ip_2 (edge_length *= scale[2]),
    // ss_e_r_0/impl, ts_e_p_5. Mirrors wood_joint_lib.cpp:6181 — but as a SEPARATE
    // YAML knob (joint_scale) so additive extension and multiplicative scale don't
    // share one field.
    j.scale = { wood_session::globals::JOINT_SCALE[0],
                wood_session::globals::JOINT_SCALE[1],
                wood_session::globals::JOINT_SCALE[2] };

    if (j.joint_type == 13 || j.joint_type == 12) {
        // Pre-set element thickness for ss_e_r_2/3 (type 13) and
        // ss_e_ip_2 (type 12, butterfly). Wood's ss_e_ip_2 (line 765)
        // and ss_e_r_2/3 both use `joint.unit_scale_distance =
        // elements[joint.v0]->thickness` as `joint_volume_edge_length`
        // for the division formula. Without this pre-set, session uses
        // the hardcoded default of 40mm and teeth land off-position.
        const int ei = index_of(wood_elems, j.element_a);
        if (ei >= 0 && ei < (int)wood_elems.size()) {
            j.unit_scale_distance = wood_elems[ei]->thickness;
        }
    }

    // Compute divisions first so the cache key matches wood's get_key().
    joint_get_divisions(j, fam.div_dist);
    j.shift = fam.shift;
    const std::string cache_key = gcz_cache_key(fam.id, j);

    // Only apply caching for type-12 (ss_e_ip, butterflies) — wood caches
    // all joint types, but session's implementation diverges enough in
    // downstream handling (Phase B/C hole extraction, orient quirks) that
    // enabling cache for other types causes regressions in datasets like
    // top_to_side_box and vda_floor_0.
    const bool use_cache = (j.joint_type == 12) && j.linked_joints.empty();

    const auto cache_it = use_cache ? unique_joints_cache.find(cache_key)
                                    : unique_joints_cache.end();
    if (!use_cache) {
        joint_create_geometry(j, fam.div_dist, fam.shift, fam.id, &all_joints, &wood_elems);
    } else if (cache_it != unique_joints_cache.end()) {
        // Cache hit: transfer unit-cube geometry, skip recompute.
        const auto& u = cache_it->second;
        j.name = u.name;
        j.m_outlines = u.m_outlines;
        j.f_outlines = u.f_outlines;
        j.m_cut_types = u.m_cut_types;
        j.f_cut_types = u.f_cut_types;
        j.unit_scale = u.unit_scale;
        j.unit_scale_distance = u.unit_scale_distance;
    } else {
        // Cache miss: compute geometry and store pre-orient state.
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
    if (wood_trace_enabled()) { fprintf(stderr, "[GCZ]   after joint_create_geometry  no_orient=%d\n", (int)j.no_orient); fflush(stderr); }
    if (!j.no_orient) {
        if (wood_trace_enabled()) { fprintf(stderr, "[GCZ]   calling joint_orient_to_connection_area\n"); fflush(stderr); }
        joint_orient_to_connection_area(j);
        if (wood_trace_enabled()) { fprintf(stderr, "[GCZ]   joint_orient done\n"); fflush(stderr); }
    }
    // wood_joint_lib.cpp:6621-6626: orient shadows then interleave geometry into primary
    if (!j.linked_joints.empty() &&
        (fam.id == 15 || fam.id == 16)) {
        for (int sid : j.linked_joints) {
            if (!all_joints[sid].no_orient) {
                joint_orient_to_connection_area(all_joints[sid]);
            }
        }
        merge_linked_joints(j, all_joints);
    }
}

/// WOOD_DUMP=<path> (DIAG_TEST=<short_name>): per-joint volumes and oriented outlines, mirroring the wood-side dump.
static void gcz_joint_dump(const std::vector<WoodJoint>& all_joints) {
    // Lets us line-by-line diff wood↔session state for a failing dataset.
    // DIAG_TEST filters so only the targeted dataset's joints are written
    // (otherwise the last-run test overwrites the dump).
    const char* diag_filter = std::getenv("DIAG_TEST");
    const char* dump_path = std::getenv("WOOD_DUMP");
    const bool should_dump = dump_path && (!diag_filter ||
                              wood_session::globals::DATA_SET_INPUT_NAME == diag_filter);
    if (!should_dump) { return; }
    std::ofstream df(dump_path);
    if (!df) { return; }
    df << "# session joints after joint_create_geometry + orient\n";
    df << "# count=" << all_joints.size() << "\n";
    for (size_t ji = 0; ji < all_joints.size(); ji++) {
        const auto& j = all_joints[ji];
        df << "joint " << ji << " type=" << j.joint_type
           << " v0=" << j.element_a << " v1=" << j.element_b
           << " f0_0=" << j.contact.face_a << " f1_0=" << j.contact.face_b
           << " name=" << (j.name.empty() ? "undefined" : j.name)
           << " orient=" << (j.no_orient ? 0 : 1)
           << " div=" << j.divisions
           << " shift=" << j.shift << "\n";
        auto dump_pl = [&](const char* tag, const Polyline& pl) {
            df << "  " << tag << " pts=" << pl.point_count();
            for (size_t k = 0; k < pl.point_count(); k++) {
                Point p = pl.get_point(k);
                df << " (" << p[0] << "," << p[1] << "," << p[2] << ")";
            }
            df << "\n";
        };
        for (int k = 0; k < 4; k++) {
            if (j.joint_volumes_pair_a_pair_b[k].has_value()) {
                char tag[32];
                snprintf(tag, sizeof(tag), "jv[%d]", k);
                dump_pl(tag, *j.joint_volumes_pair_a_pair_b[k]);
            }
        }
        for (int face = 0; face < 2; face++) {
            for (size_t k = 0; k < j.m_outlines[face].size(); k++) {
                char tag[32];
                snprintf(tag, sizeof(tag), "m[%d][%zu]", face, k);
                dump_pl(tag, j.m_outlines[face][k]);
            }
            for (size_t k = 0; k < j.f_outlines[face].size(); k++) {
                char tag[32];
                snprintf(tag, sizeof(tag), "f[%d][%zu]", face, k);
                dump_pl(tag, j.f_outlines[face][k]);
            }
        }
    }
    fmt::print("[session_dump] wrote {} joints to {}\n", all_joints.size(), dump_path);
}

/// Stages 6-7: unit joinery geometry and orientation for every detected joint, in detection order.
static void gcz_geometry(
    std::vector<WoodJoint>& all_joints,
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    const std::vector<std::vector<int>>& per_element_joints_types) {
    // NO sort here: wood's construct_joint_by_index iterates joints in the
    // order they were generated (adjacency-pair order). The unique_joints
    // cache depends on iteration order — the FIRST joint with a given
    // (name, shift, divisions) key determines the edge_length used by all
    // subsequent joints with matching key. Sorting here would change which
    // joint gets cached first and make session diverge from wood.
    //
    // Wood caches unit-cube joint geometry by key (name, shift, divisions)
    // — NOT including edge_length. Subsequent joints with matching key COPY
    // the cached geometry instead of recomputing. See wood_joint_lib.cpp:6262-6294
    // + wood_joint.cpp:86 (get_key formatting).
    GczJointCache unique_joints_cache;
    if (wood_trace_enabled()) { fprintf(stderr, "[GCZ] geometry loop start  all_joints=%zu\n", all_joints.size()); fflush(stderr); }
    for (auto& j : all_joints) {
        if (wood_trace_enabled()) {
            fprintf(stderr, "[GCZ]   geom joint type=%d  e0=%s e1=%s\n",
                    j.joint_type, j.element_a.c_str(), j.element_b.c_str()); fflush(stderr);
        }
        const int id_representing_joint_name = gcz_representing_id(j, per_element_joints_types, wood_elems);
        const GczFamilyParams fam = gcz_family_params(j.joint_type, id_representing_joint_name);
        if (j.link) {
            continue; // shadow joints: geometry set by ss_e_op_5, orient+merge below
        }
        gcz_build_one_joint(j, fam, wood_elems, all_joints, unique_joints_cache);
    }
    gcz_joint_dump(all_joints);
}

/// Stage 8: j_mf[element][face] = [(joint index, is_male)]; shadow joints go to the extra last slot.
static JMF gcz_build_jmf(
    const std::vector<std::shared_ptr<Plate>>& wood_elems,
    const std::vector<WoodJoint>& all_joints) {
    // Wood sizes j_mf as (sides)+2+1: +1 is the extra slot for shadow joints (j_mf.back()).
    // Phase C in merge_joints_for_element processes j_mf.back() — shadow joints carry
    // f_outlines (holes) set by ss_e_op_4 called inside ss_e_op_5 for the primary joint.
    const size_t n_elems = wood_elems.size();
    JMF j_mf(n_elems);
    for (size_t ei = 0; ei < n_elems; ei++) {
        j_mf[ei].resize(wood_elems[ei]->planes.size() + 1); // +1 = extra slot for shadow joints
    }
    for (size_t ji = 0; ji < all_joints.size(); ji++) {
        const auto& j = all_joints[ji];
        const int e0 = index_of(wood_elems, j.element_a);
        const int e1 = index_of(wood_elems, j.element_b);
        if (j.link) {
            // Shadow joints → j_mf.back() (wood line 1827-1828)
            if (e0 >= 0 && e0 < (int)n_elems) {
                j_mf[e0].back().push_back({(int)ji, true});
            }
            if (e1 >= 0 && e1 < (int)n_elems) {
                j_mf[e1].back().push_back({(int)ji, false});
            }
        } else {
            const int f0 = j.contact.face_a;
            const int f1 = j.contact.face_b;
            if (e0 >= 0 && e0 < (int)n_elems && f0 >= 0 && f0 < (int)j_mf[e0].size()) {
                j_mf[e0][f0].push_back({(int)ji, true});
            }
            if (e1 >= 0 && e1 < (int)n_elems && f1 >= 0 && f1 < (int)j_mf[e1].size()) {
                j_mf[e1][f1].push_back({(int)ji, false});
            }
        }
    }
    return j_mf;
}

/// Stage 9: merge joint cuts into each plate's polylines and de-interleave into `features`.
static void gcz_merge(
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    const JMF& j_mf,
    std::vector<WoodJoint>& all_joints) {
    // Layout that merge_joints_for_element returns per element:
    //   [hole0_top, hole0_bot, hole1_top, hole1_bot, ..., outer_top, outer_bot]
    const size_t n_elems = wood_elems.size();
    for (size_t ei = 0; ei < n_elems; ei++) {
        auto merged = merge_joints_for_element(*wood_elems[ei], j_mf[ei], all_joints, (int)ei);
        auto& feat = wood_elems[ei]->features;
        feat.top.clear();
        feat.bottom.clear();
        if (merged.size() >= 2) {
            // `merged` is a local about to die and every entry is consumed
            // exactly once, so the outlines move across instead of being copied.
            const size_t n_holes = merged.size() - 2;
            feat.top.reserve(1 + n_holes / 2);
            feat.bottom.reserve(1 + n_holes / 2);
            feat.top.push_back(std::move(merged[merged.size() - 2]));     // outer top
            feat.bottom.push_back(std::move(merged[merged.size() - 1]));  // outer bot
            for (size_t hi = 0; hi + 2 <= n_holes; hi += 2) {
                feat.top.push_back(std::move(merged[hi]));
                feat.bottom.push_back(std::move(merged[hi + 1]));
            }
        }
    }
}

/// WOOD_VERBOSE summary: counts per type and per-stage timings.
static void gcz_report(
    const bool verbose,
    const std::vector<std::shared_ptr<Plate>>& wood_elems,
    const std::vector<std::pair<int, int>>& adjacency_pairs,
    const GczDetectStats& stats,
    const GczTimes& t) {
    auto ms = [](auto a, auto b) {
        return std::chrono::duration<double, std::milli>(b - a).count();
    };
    if (verbose) {
        fmt::print("{} elements -> {} adjacency pairs\n",
                   wood_elems.size(), adjacency_pairs.size());
        fmt::print("  joints: {} success / {} failed\n", stats.n_success, stats.n_failed);
        fmt::print("  by type: 11={} 12={} 13={} 20={} 30={} 40={}\n",
                   stats.counts[0], stats.counts[1], stats.counts[2], stats.counts[3], stats.counts[4], stats.counts[5]);
        fmt::print("  time: {:.0f}ms\n", ms(t.t0, t.t4));
        fmt::print("  stages(ms): setup={:.1f} adjacency={:.1f} detect={:.1f} tv={:.1f} geom={:.1f} jmf={:.1f} merge={:.1f}\n",
                   ms(t.t0, t.t2) - ms(t.t1, t.t2), ms(t.t1, t.t2), ms(t.t2, t.t3), ms(t.t3, t.t3a), ms(t.t3a, t.t3b), ms(t.t3b, t.t3c), ms(t.t3c, t.t4));
    }
}

} // anonymous namespace

std::vector<WoodJoint> get_connection_zones(
    std::vector<std::shared_ptr<Plate>>& wood_elems,
    SearchType search_type) {

    if (wood_trace_enabled()) {
        fprintf(stderr, "[GCZ] enter  n_elems=%zu  search_type=%d\n",
                wood_elems.size(), (int)search_type);
        fflush(stderr);
    }

    using namespace wood_session::globals;
    const std::string short_name = DATA_SET_INPUT_NAME;
    const double dihedral_threshold = FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE;

    GczTimes times;
    times.t0 = GczClock::now();

    // Sidecar tables the dataset yaml named; empty means derive it.
    const std::string adj_name = DATA_SET_ADJACENCY;
    const std::string tv_name = DATA_SET_THREE_VALENCE;
    const std::string iv_name = DATA_SET_INSERTION_VECTORS;
    const std::string jt_name = DATA_SET_JOINTS_TYPES;
    const std::vector<double> ext_vec = JOINT_VOLUME_EXTENSION;
    const bool verbose = std::getenv("WOOD_VERBOSE") != nullptr;

    if (verbose) { fmt::print("\n=== {}.obj ===\n", short_name); }

    gcz_element_dump(wood_elems);
    times.t1 = GczClock::now();

    const std::vector<std::pair<int, int>> adjacency_pairs = gcz_adjacency(adj_name, wood_elems, verbose);
    times.t2 = GczClock::now();

    // Every detection tunable is named here; the names match the original
    // wood::GLOBALS::* fields one-for-one.
    const GczDetectParams params{
        ext_vec,
        LIMIT_MIN_JOINT_LENGTH,                                   // YAML-tunable
        1e-6,                                                     // minimum joint-line squared length
        DISTANCE_SQUARED,                                         // squared-distance coplanar test
        dihedral_threshold,
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED,
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE};

    // Sync cross-joint near-coplanar threshold with current session globals.
    // Hexboxes multiplies DISTANCE_SQUARED by 100; the cross-joint detector
    // must honour the test-time value.
    wood_session::set_cross_joint_distance_squared(DISTANCE_SQUARED);

    gcz_load_insertion_vectors(iv_name, wood_elems, verbose);

    GczDetectStats stats;
    std::vector<WoodJoint> all_joints = gcz_detect(wood_elems, adjacency_pairs, params, search_type, verbose, stats);
    times.t3 = GczClock::now();

    gcz_three_valence(tv_name, wood_elems, all_joints, adjacency_pairs, verbose);

    const std::vector<std::vector<int>> per_element_joints_types = gcz_joint_types(jt_name, wood_elems, verbose);

    times.t3a = GczClock::now();
    gcz_geometry(all_joints, wood_elems, per_element_joints_types);
    times.t3b = GczClock::now();
    if (wood_trace_enabled()) { fprintf(stderr, "[GCZ] geometry dispatch done  all_joints=%zu\n", all_joints.size()); fflush(stderr); }

    const JMF j_mf = gcz_build_jmf(wood_elems, all_joints);
    times.t3c = GczClock::now();
    if (wood_trace_enabled()) { fprintf(stderr, "[GCZ] j_mf built  starting merge\n"); fflush(stderr); }

    gcz_merge(wood_elems, j_mf, all_joints);
    times.t4 = GczClock::now();

    gcz_report(verbose, wood_elems, adjacency_pairs, stats, times);
    // The kernel view of every joint - its two ElementFeatures - is derived from the solver
    // fields, which are final only now that the merge has run. Refresh it here so a joint
    // handed to a caller (or to fill_session) is never stale.
    for (WoodJoint& j : all_joints) { j.sync_features(); }
    return all_joints;
}

// ═══════════════════════════════════════════════════════════════════════════
// get_connection_zones — in-memory chevron data overload.
//
// Writes the ChevronJoineryData fields to temporary txt files in
// session_data_dir(), then delegates to the file-based overload.  This
// avoids duplicating the large detection + geometry + merge pipeline while
// still feeding adjacency, insertion vectors, joint types, and three-valence
// groups to the algorithm.
//
// Temporary files are removed after the call.  The prefix "_wood_nano_chevron"
// is chosen to avoid collisions with any existing named dataset.
// ═══════════════════════════════════════════════════════════════════════════
std::vector<wood_session::WoodJoint> get_connection_zones(
        std::vector<std::shared_ptr<wood_session::Plate>>& elements,
        SearchType search_type,
        const wood_session::ChevronJoineryData& joinery_data)
{
    // In-memory path — no temp files written or read.
    //
    // Previously this overload wrote adjacency/IV/JT/TV to temp files under
    // internal::session_data_dir() (a path computed from __FILE__ at compile
    // time) and then called the file-based 2-arg overload.  On PyPI/wheel
    // installs the compile-time source path does not exist on the user's
    // machine, so the ofstream writes silently fail, the 2-arg overload finds
    // no files, falls back to full BVH adjacency search, and returns wrong joints.
    //
    // Fix: set IVs and JTs directly on the WoodElements (the 2-arg overload
    // already has in-memory fallback logic for both), and inject adjacency /
    // three_valence via thread-local overrides that the 2-arg overload checks
    // before falling back to BVH / file respectively.

    // 1. Insertion vectors — convert array<double,18> → vector<Vector> per element.
    for (size_t ei = 0; ei < elements.size(); ++ei) {
        if (elements[ei]->insertion_vectors().empty() &&
            ei < joinery_data.insertion_vectors.size()) {
            const auto& iv18 = joinery_data.insertion_vectors[ei];
            auto& ivec = elements[ei]->insertion_vectors();
            for (int s = 0; s < 6; ++s)
                ivec.emplace_back(iv18[s*3+0], iv18[s*3+1], iv18[s*3+2]);
        }
    }

    // 2. Joint types — convert array<int,6> → vector<int> per element.
    for (size_t ei = 0; ei < elements.size(); ++ei) {
        if (elements[ei]->joint_types.empty() &&
            ei < joinery_data.joints_per_face.size()) {
            const auto& jt6 = joinery_data.joints_per_face[ei];
            elements[ei]->joint_types.assign(jt6.begin(), jt6.end());
        }
    }

    // 3+4. Adjacency and three-valence — injected via thread-locals so the
    // 2-arg overload skips the BVH / file paths. Cleared by RAII: if the
    // delegated pipeline throws (bad geometry, misconfigured globals), a
    // plain clear-after-call was skipped and the NEXT solve on this thread
    // silently reused this model's adjacency — wrong joints, no diagnostic.
    // Swap-with-empty in the destructor also releases the capacity that
    // clear() would pin per thread for the process lifetime.
    struct TlOverrideGuard {
        ~TlOverrideGuard() {
            std::vector<std::pair<int,int>>().swap(tl_adjacency_override);
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
