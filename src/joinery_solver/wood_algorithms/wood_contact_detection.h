#pragma once

#include "pch.h"

#include "wood_config.h"
#include "wood_element_plate.h"
#include "wood_interaction_contact_face.h"
#include "wood_interaction_contact_cross.h"
#include "wood_interaction_feature_plate.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Face contacts - broad phase
// ═══════════════════════════════════════════════════════════════════════════

/// Candidate element pairs (i, j) with i < j: inflated OBB per element, a BVH over their AABBs, then an OBB/OBB SAT test; `names` selects by name, empty means everything.
std::vector<std::pair<int, int>> adjacency_search(
    const std::vector<std::shared_ptr<session_cpp::Element>>& elements,
    double inflate,
    const std::vector<std::string>& names = {});

// ═══════════════════════════════════════════════════════════════════════════
// Face contacts - narrow phase
// ═══════════════════════════════════════════════════════════════════════════

/// True when two faces touch back-to-back: z axes antiparallel within `cos_angle` (cos of config::ANGLE, radians) and each origin within `coplanar_tolerance` (a squared distance) of the other's plane; the z axes need not be unit length.
bool faces_coplanar(
    const session_cpp::Plane& face0,
    const session_cpp::Plane& face1,
    double cos_angle,
    double coplanar_tolerance);

/// Largest overlap of two coplanar outlines as a closed polygon in `plane0`, via Clipper2 on the CLIPPER_SCALE grid; coplanarity is a precondition, triangles count only when `include_triangles`.
bool face_overlap_area(
    const session_cpp::Polyline& outline0,
    const session_cpp::Polyline& outline1,
    const session_cpp::Plane& plane0,
    bool include_triangles,
    session_cpp::Polyline& out_area);

// ═══════════════════════════════════════════════════════════════════════════
// Face contacts - both phases
// ═══════════════════════════════════════════════════════════════════════════

/// Every contacting face pair between ONE element pair, ordered by face index; call it inside the caller's loop over element pairs, since compute_joints swaps faces 0 and 1 mid-run. `trace`, when given, counts the coplanar and overlapping pairs into its dbg fields.
std::vector<ContactFace> face_contacts_for_pair(
    session_cpp::Element& ea,
    session_cpp::Element& eb,
    double cos_angle,
    double coplanar_tolerance,
    FeaturePlate* trace = nullptr);

/// Every face pair in contact across a set of elements, as (position of the first element, position of the second, the contact): adjacency_search, then faces_coplanar + face_overlap_area over each candidate; `angle` in radians, `coplanar_tolerance` a squared distance.
std::vector<std::tuple<int, int, ContactFace>> face_contacts(
    const std::vector<std::shared_ptr<session_cpp::Element>>& elements,
    const std::vector<std::string>& names = {},
    double inflate            = config::DISTANCE,
    double angle              = config::ANGLE,
    double coplanar_tolerance = config::DISTANCE_SQUARED);

// ═══════════════════════════════════════════════════════════════════════════
// Cross contacts
// ═══════════════════════════════════════════════════════════════════════════

/// Cross/lap contact detection between two plates from their bottom and top outlines and planes; by reference so the hot loop copies nothing.
bool plane_to_face(
    const session_cpp::Polyline& a_bottom, const session_cpp::Polyline& a_top,
    const session_cpp::Polyline& b_bottom, const session_cpp::Polyline& b_top,
    const session_cpp::Plane& a_plane_bottom, const session_cpp::Plane& a_plane_top,
    const session_cpp::Plane& b_plane_bottom, const session_cpp::Plane& b_plane_top,
    ContactCross& result,
    double angle_tol = 5.0,
    const std::array<double, 3>& extension = {0.0, 0.0, 0.0});

/// The same, with each plate's bottom/top outlines and planes as arrays.
bool plane_to_face(
    const std::array<session_cpp::Polyline, 2>& polylines_a,
    const std::array<session_cpp::Polyline, 2>& polylines_b,
    const std::array<session_cpp::Plane, 2>& planes_a,
    const std::array<session_cpp::Plane, 2>& planes_b,
    ContactCross& result,
    double angle_tol = 5.0,
    const std::array<double, 3>& extension = {0.0, 0.0, 0.0});

/// Near-coplanar rejection threshold used by plane_to_face; the caller syncs it from config::DISTANCE_SQUARED.
void set_cross_joint_distance_squared(double dist_sq);

}  // namespace wood_session
