// examples/main_face_to_face.cpp — contact detection and face-to-face joints.
//
// Two scenarios, both ending in the same place: WoodElements handed to
// get_connection_zones.
//
//   A. Plates built in code. The (bottom, top) outline pairs are known, so a
//      WoodElement is constructed directly and the face-to-face algorithm runs
//      on them.
//
//   B. Loose closed polylines loaded from a .pb, grouped one list per element.
//      Nothing says which loop is bottom and which is top, so WoodElement does
//      not fit. These become BlockElements — outlines plus a plane each — and
//      only contact detection runs on them. That is the whole point of the
//      type: contact detection needs no plate convention, joint classification
//      does.
//
// The collision-detection entry points used here all live in
// wood_face_to_face.h alongside face_to_face_wood:
//   adjacency_search  — element-level broad phase, candidate pairs
//   face_planes       — unpack an element's face planes once
//   faces_coplanar    — do two faces touch back-to-back?
//   face_overlap_area — do two coplanar faces actually overlap, and where?
#include "wood_session.h"
#include "wood_face_to_face.h"
#include "wood_element.h"
#include "../src/session.h"
#include "../src/polyline.h"
#include "../src/plane.h"

#include <fmt/core.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

using namespace session_cpp;
using namespace wood_session;

// ───────────────────────────────────────────────────────────────────────────
// Shared reporting
// ───────────────────────────────────────────────────────────────────────────

// Contact detection, and nothing else. Works on any element type carrying
// outlines and planes — WoodElement in scenario A, BlockElement in scenario B.
template <class Element>
static std::vector<std::pair<int, int>> report_contacts(
    const std::vector<Element>& elements) {

    std::vector<std::pair<int, int>> pairs =
        adjacency_search(elements, globals::DISTANCE);
    fmt::print("  broad phase : {} candidate pairs from {} elements\n",
               pairs.size(), elements.size());

    // Narrow phase, spelled out rather than left to face_to_face_wood, so the
    // contact test is visible on its own. Same two steps the classifier runs:
    // coplanar back-to-back faces, then a real 2D overlap between them.
    const double cos_angle = std::cos(globals::ANGLE);
    int touching_faces = 0;
    for (const auto& [ia, ib] : pairs) {
        const std::vector<FacePlane> fa = face_planes(elements[ia]);
        const std::vector<FacePlane> fb = face_planes(elements[ib]);
        for (size_t i = 0; i < fa.size(); ++i) {
            for (size_t j = 0; j < fb.size(); ++j) {
                if (!faces_coplanar(fa[i], fb[j], cos_angle, globals::DISTANCE_SQUARED)) {
                    continue;
                }
                Polyline overlap(std::vector<Point>{});
                if (face_overlap_area(elements[ia].polylines[i],
                                      elements[ib].polylines[j],
                                      elements[ia].planes[i],
                                      i < 2 && j < 2, overlap)) {
                    ++touching_faces;
                }
            }
        }
    }
    fmt::print("  narrow phase: {} face pairs in real contact\n", touching_faces);
    return pairs;
}

// Full pipeline: contact detection + joint classification.
static void report_joints(std::vector<WoodElement>& elements, const char* out_name) {
    std::vector<WoodJoint> joints = get_connection_zones(elements, face_to_face);

    int n_ss = 0, n_ts = 0, n_tt = 0;
    for (const WoodJoint& j : joints) {
        if (j.joint_type >= 10 && j.joint_type < 20)      { ++n_ss; }
        else if (j.joint_type >= 20 && j.joint_type < 40) { ++n_ts; }
        else                                              { ++n_tt; }
    }
    fmt::print("  joints      : {} total ({} side-side, {} top-side, {} top-top)\n",
               joints.size(), n_ss, n_ts, n_tt);

    Session session(out_name);
    fill_session(session, elements, joints, true);
    const std::string out = (internal::output_dir() / (std::string(out_name) + ".pb")).string();
    session.pb_dump(out);
    fmt::print("  wrote       : {}\n", out);
}

// ───────────────────────────────────────────────────────────────────────────
// Scenario A — plates created in code
// ───────────────────────────────────────────────────────────────────────────
//
// WoodElement's constructor takes the outline pair directly, in (bottom, top)
// order. Three plates: a floor, and two walls standing on it that also meet
// each other along a shared vertical edge.

static Polyline rect(double ax, double ay, double az,
                     double bx, double by, double bz,
                     double cx, double cy, double cz,
                     double dx, double dy, double dz) {
    return Polyline(std::vector<Point>{
        Point(ax, ay, az), Point(bx, by, bz),
        Point(cx, cy, cz), Point(dx, dy, dz), Point(ax, ay, az)});
}

static void scenario_a() {
    fmt::print("\nA. plates built in code\n");

    // Same layout main_hello.cpp uses, because it is known to produce joints:
    // plates are 15 mm thick, given as the outline at z=0 and its copy at
    // z=-15. Two coplanar plates meeting along y=0 give a side-to-side
    // IN-PLANE joint; the angled pair at x>=1000 folds over a shared edge and
    // gives a side-to-side OUT-OF-PLANE joint.
    //
    // Plates must TOUCH, not interpenetrate: a wall sunk into a slab shares no
    // coplanar face pair and is detected as nothing at all.
    std::vector<WoodElement> elements;

    // Coplanar pair, sharing the edge y = 0.
    elements.emplace_back(
        rect(-500,   0,   0,   500,   0,   0,   500, 500,   0,  -500, 500,   0),
        rect(-500,   0, -15,   500,   0, -15,   500, 500, -15,  -500, 500, -15));
    elements.emplace_back(
        rect(-500, -500,   0,  500, -500,   0,  500,   0,   0,  -500,   0,   0),
        rect(-500, -500, -15,  500, -500, -15,  500,   0, -15,  -500,   0, -15));

    // Folded pair, sharing the edge x = 1000..2000, y = 0.
    elements.emplace_back(
        rect(1000,   0,   0,  2000,   0,   0,  2000, 500,   0,  1000, 500,   0),
        rect(1000,   0, -15,  2000,   0, -15,  2000, 500, -15,  1000, 500, -15));
    elements.emplace_back(
        rect(1000, -500, 134, 2000, -500, 134, 2000,   0,   0,  1000,   0,   0),
        rect(1000, -500, 119, 2000, -500, 119, 2000,   0, -15,  1000,   0, -15));

    report_contacts(elements);
    report_joints(elements, "face_to_face_scenario_a");
}

// ───────────────────────────────────────────────────────────────────────────
// Scenario B — loose closed polylines, grouped per element
// ───────────────────────────────────────────────────────────────────────────
//
// Input is a list of lists: each inner list is the closed loops of ONE
// colliding element, unordered and unlabelled. That is what
// data/face_to_face_detection/face_to_face_detection.pb holds — the face loops
// of each plate solid, one session group per element.
//
// No bottom/top means no WoodElement, so these become BlockElements and only
// the contact detector runs. Feeding such loops to WoodElement would silently
// invent a plate convention that the geometry never had.

static void scenario_b() {
    fmt::print("\nB. loose closed polylines -> BlockElement, contact detection only\n");

    const std::filesystem::path pb_path = internal::session_data_dir()
        / "face_to_face_detection" / "face_to_face_detection.pb";

    // pb_load returns an EMPTY session for a missing file rather than throwing,
    // so a silent zero-polyline run is indistinguishable from a bad path.
    if (!std::filesystem::exists(pb_path)) {
        fmt::print(stderr, "  not found: {}\n", pb_path.string());
        fmt::print(stderr, "  generate it with:\n"
                           "    cd data/face_to_face_detection && .venv/bin/python brep_to_pb.py\n");
        return;
    }
    Session session = Session::pb_load(pb_path.string());

    // Recover the list of lists: each child of the tree root is one element,
    // and its children are geometry nodes whose `name` is the polyline's GUID.
    std::vector<BlockElement> elements;
    std::shared_ptr<TreeNode> root = session.tree.root();
    if (root) {
        for (TreeNode* group : root->children()) {
            std::vector<Polyline> loops;
            for (TreeNode* leaf : group->children()) {
                if (auto pl = session.get_object<Polyline>(leaf->name)) { loops.push_back(*pl); }
            }
            if (!loops.empty()) { elements.emplace_back(loops); }
        }
    }
    fmt::print("  loaded      : {} elements, {} loops total\n", elements.size(),
               session.get_geometry().polylines->size());
    if (elements.empty()) {
        fmt::print("  nothing grouped in the tree, stopping\n");
        return;
    }

    std::vector<std::pair<int, int>> pairs = report_contacts(elements);

    // The contact regions themselves, which is what a BlockElement run is for.
    Session out("face_to_face_scenario_b");
    auto g = out.add_group("ContactAreas");
    const double cos_angle = std::cos(globals::ANGLE);
    int n_areas = 0;
    for (const auto& [ia, ib] : pairs) {
        const std::vector<FacePlane> fa = face_planes(elements[ia]);
        const std::vector<FacePlane> fb = face_planes(elements[ib]);
        for (size_t i = 0; i < fa.size(); ++i) {
            for (size_t j = 0; j < fb.size(); ++j) {
                if (!faces_coplanar(fa[i], fb[j], cos_angle, globals::DISTANCE_SQUARED)) {
                    continue;
                }
                Polyline area(std::vector<Point>{});
                // include_triangles is false: without the plate convention
                // there is no "these two are the outer faces" exception.
                if (face_overlap_area(elements[ia].polylines[i],
                                      elements[ib].polylines[j],
                                      elements[ia].planes[i], false, area)) {
                    out.add_polyline(std::make_shared<Polyline>(area), g);
                    ++n_areas;
                }
            }
        }
    }
    const std::string path = (internal::output_dir() / "face_to_face_scenario_b.pb").string();
    out.pb_dump(path);
    fmt::print("  contact areas: {}\n  wrote       : {}\n", n_areas, path);
}

int main() {
    globals::globals_yaml("hello");
    scenario_a();
    scenario_b();
    return 0;
}
