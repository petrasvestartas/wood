#pragma once

#include "../src/line.h"
#include "../src/plane.h"
#include "../src/point.h"
#include "../src/polyline.h"
#include "../src/vector.h"
#include "wood_globals.h"
#include "wood_joint.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Contact detection - what detection reads off an element
// ═══════════════════════════════════════════════════════════════════════════

/// One element as detection sees it: face outlines, their planes, the name to filter on, and
/// whether the plate face convention ([0] bottom, [1] top, [2..] sides) applies. A plate
/// contributes its own outlines; any other element the face outlines of its mesh.
struct ContactElement {
    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;
    std::string                        name;
    bool plate_convention = false;

    ContactElement() = default;
    explicit ContactElement(const Plate& plate);
    explicit ContactElement(session_cpp::Element& element);
};

// ═══════════════════════════════════════════════════════════════════════════
// Contact detection - broad phase
// ═══════════════════════════════════════════════════════════════════════════

/// Candidate element pairs (i, j) with i < j: an oriented box per element in its own frame,
/// inflated by `inflate` (globals::DISTANCE), a BVH over their AABBs, then an OBB/OBB SAT test.
/// `names` selects which elements take part by name; empty searches everything, and an
/// excluded element keeps its index.
std::vector<std::pair<int, int>> adjacency_search(
    const std::vector<ContactElement>& elements,
    double inflate,
    const std::vector<std::string>& names = {});

// ═══════════════════════════════════════════════════════════════════════════
// Contact detection - narrow phase
// ═══════════════════════════════════════════════════════════════════════════

// One face plane with its coordinates unpacked. Plane::origin()/z_axis() hand
// back references, but Point/Vector::operator[] is an out-of-line call in
// session_core, so the O(faces²) scan reads plain doubles instead.
// The normal is not assumed to be unit length; `mag_sq` carries its scale.
struct FacePlane {
    double ox, oy, oz;
    double nx, ny, nz;
    double mag_sq;
};

/// Every face plane of an element unpacked, once per element pair rather than once per face pair.
std::vector<FacePlane> face_planes(const ContactElement& element);

// True when the two faces touch back-to-back: normals antiparallel to within
// `cos_angle` (= cos of wood::GLOBALS::ANGLE, 0.11 RADIANS ≈ 6.3°, NOT 0.11
// degrees) and each origin within `coplanar_tolerance` — a SQUARED distance —
// of the other's plane.
//
// This mirrors wood's cgal::plane_util::is_coplanar. Session's
// Vector::is_parallel_to uses ANGLE_TOLERANCE_DEGREES = 0.11°, which is far
// too strict and misses ts_e_p connections in the one_layer/full datasets, so
// the test is spelled out rather than delegated.
//
// Both faces are passed pre-unpacked so the O(faces²) scan over an element
// pair never touches Plane::origin()/z_axis() at all.
bool faces_coplanar(
    const FacePlane& face0,
    const FacePlane& face1,
    double cos_angle,
    double coplanar_tolerance);

// Overlap region of two coplanar face outlines, as a closed polygon in `plane0`.
// Returns false when they do not overlap, and `out_area` is then untouched.
//
// Both outlines are projected into plane0's canonical 2D frame and intersected
// with Clipper2 on int64 coordinates at wood::GLOBALS::CLIPPER_SCALE (1e6: a
// nanometre grid). Touching plates share edges exactly, and exactly-coincident
// edges are the case a floating-point sweep gets wrong - the previous engine
// needed a 1/1024 mm vertex-collapse pass to survive the hexbox datasets.
// Integer arithmetic has no such failure mode, and the result is exact on its
// grid. Of several overlap pieces (possible with concave outlines) the largest
// is returned. Results with area ≤ CLIPPER_AREA (0.01 mm²) are rejected.
//
// Coplanarity is a precondition, not a check: two parallel faces a metre apart
// would report a healthy overlap. Gate every call on faces_coplanar.
//
// `include_triangles` must be true only for top/bottom face pairs: wood
// accepts 3-vertex triangles there and rejects them for side-face pairs.
bool face_overlap_area(
    const session_cpp::Polyline& outline0,
    const session_cpp::Polyline& outline1,
    const session_cpp::Plane& plane0,
    bool include_triangles,
    session_cpp::Polyline& out_area);

// ── Contact detection: both phases ─────────────────────────────────────────

// Tallies from one element-pair scan. face_to_face_wood reports these on the
// joint it produces (dbg_coplanar / dbg_boolean); face_contacts ignores them.
struct PairScanStats {
    int coplanar    = 0;   // face pairs that passed the coplanarity test
    int overlapping = 0;   // of those, the ones with a real overlap area
    int empty_i     = -1;  // last face pair whose Clipper2 boolean came back empty
    int empty_j     = -1;
};

// Every contacting face pair between ONE element pair - the inner scan shared by
// face_contacts (which runs it over every adjacency pair) and face_to_face_wood
// (which runs it once and then classifies what comes back). Ordered by face
// index, `a` outer and `b` inner, which is the order both callers relied on when
// they each spelled this loop out.
//
// Call it INSIDE the caller's loop over element pairs, never over a precomputed
// list of every pair: get_connection_zones swaps an element's faces 0 and 1
// mid-run (wood_main.cpp, swap_planes_b), so a face index captured before that
// swap names a different face after it.

/// Every contacting face pair between ONE element pair, ordered by face index, `a` outer and
/// `b` inner. Call it inside the caller's loop over element pairs: get_connection_zones swaps an
/// element's faces 0 and 1 mid-run, so a face index captured earlier names a different face.
std::vector<FaceContact> face_contacts_for_pair(
    const ContactElement& ea,
    const ContactElement& eb,
    int ia,
    int ib,
    double cos_angle,
    double coplanar_tolerance,
    PairScanStats* stats = nullptr);

/// Every face pair in contact across a set of elements: adjacency_search, then faces_coplanar +
/// face_overlap_area over each candidate. `angle` in radians, `coplanar_tolerance` a squared
/// distance; triangular overlaps count only between two outer faces of plates.
std::vector<ContactPair> face_contacts(
    const std::vector<ContactElement>& elements,
    const std::vector<std::string>& names = {},
    double inflate            = globals::DISTANCE,
    double angle              = globals::ANGLE,
    double coplanar_tolerance = globals::DISTANCE_SQUARED);

}  // namespace wood_session

// ═══════════════════════════════════════════════════════════════════════════
// Joint classification
// ═══════════════════════════════════════════════════════════════════════════

// Stage 4 of 9: Classify one face-pair from two elements as a wood joint.
// Returns true if a valid joint was found; populates out_joint and out_swap_planes_1.
// All parameters that the original wood reads from GLOBALS are explicit here so
// the function is pure and re-entrant.
bool face_to_face_wood(
    size_t joint_id,
    const wood_session::Plate& el0,
    const wood_session::Plate& el1,
    std::pair<int, int> el_ids_in,
    const std::vector<double>& joint_volume_extension,
    double limit_min_joint_length,
    double distance_squared,
    double coplanar_tolerance,
    double dihedral_angle_threshold,
    bool all_treated_as_rotated,
    bool rotated_joint_as_average,
    int  search_type,
    wood_session::WoodJoint& out_joint,
    bool& out_swap_planes_1);

