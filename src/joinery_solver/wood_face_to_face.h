// wood/wood_face_to_face.h — plate contact detection and face-to-face joints.
//
// Two layers, in the order the pipeline runs them:
//
//   Contact detection — does anything touch?
//     adjacency_search : element-level broad phase (oriented boxes + BVH + SAT)
//                        producing the candidate pairs everything below consumes.
//     face_planes / faces_coplanar / face_overlap_area
//                      : the per-face-pair contact test - antiparallel coplanar
//                        planes, then a Clipper2 boolean of the two outlines.
//     face_contacts    : the two phases wired together for any element type.
//
//   Joint classification — what does that contact become? (face_to_face_wood)
//
// Stage 4 of 9 (face_to_face_wood): core joint topology detector. Classifies
//   one face-pair from two WoodElements as joint type 11/12/13/20/30/40 and
//   computes joint area, alignment lines, and volume rectangles.
//
// (Stage 1 — building a WoodElement from a bottom/top polyline pair — is now
//  a WoodElement constructor in wood_element.h.)
//
// Geometry helpers that moved to the session kernel:
//   polyline_two_rects_from_frame   → src/polyline.h
//   Intersection::line_line_classified → src/intersection.h
#pragma once

#include "wood_element.h"
#include "../src/point.h"
#include "../src/line.h"
#include "../src/polyline.h"
#include "../src/vector.h"

#include "../src/plane.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace wood_session {

// ── Contact detection: broad phase ─────────────────────────────────────────

// Candidate element pairs, as (i, j) with i < j, in ascending i then j order.
//
// Each element gets an OBB in its own frame (planes[0]) built from its
// bounding points and inflated by `inflate` — wood::GLOBALS::DISTANCE, 0.1 mm
// by default. The AABBs of those boxes go into a SpatialBVH (static, built
// once per call — the R-tree in the kernel is for sets that change, and this
// one does not) for the candidate query; every candidate is then confirmed by
// an OBB/OBB SAT test.
//
// Using the element's own frame matters: the (points, inflate) overload of
// OBB::from_points is obb_from_aabb(AABB::from_points(...)) — a WORLD-axis
// box, so a thin plate lying diagonal to the world axes got a box the size of
// its diagonal extent and the narrow phase paid for a storm of false candidate
// pairs. Candidate sets only shrink under the oriented box (both boxes contain
// the element plus the same inflation), so no true contact is lost.
//
// Which points bound an element depends on the type. A WoodElement's side
// quads are built from the very corners its top and bottom outlines hold, so
// those two outlines give the identical box at a fraction of the work. A
// BlockElement has no such convention - any of its loops may stick out - so
// every loop counts. (Reading only the first two loops of a block, as this
// used to, boxed a six-face block by two of its faces and could miss a
// contact on the other four.)
//
// Elements with no bounding points contribute an empty box and never pair.
//
// Generic over the element type: anything with `polylines` and `planes` works.
// Instantiated for WoodElement (the plate convention the joint classifier
// needs) and BlockElement (loose loops, contact detection only); add an
// overload of `bounding_points` in wood_face_to_face.cpp for a new type.
template <class Element>
std::vector<std::pair<int, int>> adjacency_search(
    const std::vector<Element>& elements,
    double inflate);

extern template std::vector<std::pair<int, int>>
adjacency_search<WoodElement>(const std::vector<WoodElement>&, double);
extern template std::vector<std::pair<int, int>>
adjacency_search<BlockElement>(const std::vector<BlockElement>&, double);

// ── Contact detection: narrow phase ────────────────────────────────────────

// One face plane with its coordinates unpacked. Plane::origin()/z_axis() hand
// back references, but Point/Vector::operator[] is an out-of-line call in
// session_core, so the O(faces²) scan reads plain doubles instead.
// The normal is not assumed to be unit length; `mag_sq` carries its scale.
struct FacePlane {
    double ox, oy, oz;
    double nx, ny, nz;
    double mag_sq;
};

// Unpack every face plane of an element, once per element pair rather than
// once per face pair. Generic over the element type, as adjacency_search is.
template <class Element>
std::vector<FacePlane> face_planes(const Element& element);

extern template std::vector<FacePlane> face_planes<WoodElement>(const WoodElement&);
extern template std::vector<FacePlane> face_planes<BlockElement>(const BlockElement&);

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

// One face pair in real contact: which faces of which elements, and the
// overlap region between them (closed, in element_a's face plane).
struct FaceContact {
    int element_a;
    int element_b;   // element_a < element_b
    int face_a;
    int face_b;
    session_cpp::Polyline area;
};

// Every face pair in contact across a set of elements: adjacency_search for
// the candidate pairs, then faces_coplanar + face_overlap_area over each
// candidate's face pairs. `angle` is in RADIANS (wood::GLOBALS::ANGLE),
// `coplanar_tolerance` a SQUARED distance (wood::GLOBALS::DISTANCE_SQUARED).
// Triangular overlaps count only between a WoodElement's top/bottom faces;
// a BlockElement has no outer faces, so never.
//
// This is what face_to_face_wood runs before it classifies, spelled out for
// callers that want the contacts and not the joints - block assemblies, or a
// viewer showing where things touch. Ordered by element pair, then face pair.
template <class Element>
std::vector<FaceContact> face_contacts(
    const std::vector<Element>& elements,
    double inflate,
    double angle,
    double coplanar_tolerance);

extern template std::vector<FaceContact>
face_contacts<WoodElement>(const std::vector<WoodElement>&, double, double, double);
extern template std::vector<FaceContact>
face_contacts<BlockElement>(const std::vector<BlockElement>&, double, double, double);

}  // namespace wood_session

// ── Joint classification ───────────────────────────────────────────────────

// Stage 4 of 9: Classify one face-pair from two elements as a wood joint.
// Returns true if a valid joint was found; populates out_joint and out_swap_planes_1.
// All parameters that the original wood reads from GLOBALS are explicit here so
// the function is pure and re-entrant.
bool face_to_face_wood(
    size_t joint_id,
    const wood_session::WoodElement& el0,
    const wood_session::WoodElement& el1,
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
