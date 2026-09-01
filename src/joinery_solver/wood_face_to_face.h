// wood/wood_face_to_face.h — plate contact detection and face-to-face joints.
//
// Two layers, in the order the pipeline runs them:
//
//   Contact detection — does anything touch?
//     adjacency_search : element-level broad phase (oriented boxes + BVH + SAT)
//                        producing the candidate pairs everything below consumes.
//     face_planes / faces_coplanar / face_overlap_area
//                      : the per-face-pair contact test.
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
// Each plate gets an OBB in its own frame (planes[0]) built from its top and
// bottom outline corners and inflated by `inflate` — wood::GLOBALS::DISTANCE,
// 0.1 mm by default. The AABBs of those boxes go into a SpatialBVH for the
// candidate query; every candidate is then confirmed by an OBB/OBB SAT test.
//
// Using the plate's own frame matters: the (points, inflate) overload of
// OBB::from_points is obb_from_aabb(AABB::from_points(...)) — a WORLD-axis
// box, so a thin plate lying diagonal to the world axes got a box the size of
// its diagonal extent and the narrow phase paid for a storm of false candidate
// pairs. Candidate sets only shrink under the oriented box (both boxes contain
// the plate plus the same inflation), so no true contact is lost.
//
// Elements with fewer than two polylines contribute an empty box and simply
// never pair.
//
// Generic over the element type: anything with `polylines` and `planes` works.
// Instantiated for WoodElement (the plate convention the joint classifier
// needs) and BlockElement (loose loops, contact detection only).
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

// Overlap region of two coplanar face outlines, as a polygon in `plane0`.
// Returns false when they do not overlap, and `out_area` is then untouched.
//
// Coplanarity is a precondition, not a check: the 2D boolean projects both
// outlines along plane0's normal, so two parallel faces a metre apart would
// report a healthy overlap. Gate every call on faces_coplanar.
//
// `include_triangles` must be true only for top/bottom face pairs: wood
// accepts 3-vertex triangles there and rejects them for side-face pairs. The
// 1/1024 mm collapse epsilon removes Vatti FP duplicates on near-coincident
// edges (hexbox-family datasets).
bool face_overlap_area(
    const session_cpp::Polyline& outline0,
    const session_cpp::Polyline& outline1,
    const session_cpp::Plane& plane0,
    bool include_triangles,
    session_cpp::Polyline& out_area);

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
