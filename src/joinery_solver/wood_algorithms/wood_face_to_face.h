#pragma once

#include "pch.h"

#include "wood_config.h"
#include "wood_face_to_face_element.h"
#include "wood_joint.h"
#include "wood_face_to_face_stats.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Contact detection - broad phase
// ═══════════════════════════════════════════════════════════════════════════

/// Candidate element pairs (i, j) with i < j: inflated OBB per element, a BVH over their AABBs, then an OBB/OBB SAT test; `names` selects by name, empty means everything.
std::vector<std::pair<int, int>> adjacency_search(
    const std::vector<ContactElement>& elements,
    double inflate,
    const std::vector<std::string>& names = {});

// ═══════════════════════════════════════════════════════════════════════════
// Contact detection - narrow phase
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
// Contact detection - both phases
// ═══════════════════════════════════════════════════════════════════════════

/// Every contacting face pair between ONE element pair, ordered by face index; call it inside the caller's loop over element pairs, since get_connection_zones swaps faces 0 and 1 mid-run.
std::vector<ContactFace> face_contacts_for_pair(
    const ContactElement& ea,
    const ContactElement& eb,
    double cos_angle,
    double coplanar_tolerance,
    PairScanStats* stats = nullptr);

/// Every face pair in contact across a set of elements, as (position of the first element, position of the second, the contact): adjacency_search, then faces_coplanar + face_overlap_area over each candidate; `angle` in radians, `coplanar_tolerance` a squared distance.
std::vector<std::tuple<int, int, ContactFace>> face_contacts(
    const std::vector<ContactElement>& elements,
    const std::vector<std::string>& names = {},
    double inflate            = config::DISTANCE,
    double angle              = config::ANGLE,
    double coplanar_tolerance = config::DISTANCE_SQUARED);

}  // namespace wood_session

// ═══════════════════════════════════════════════════════════════════════════
// Joint classification
// ═══════════════════════════════════════════════════════════════════════════

/// Classifies one element pair as a wood joint with every tunable explicit; true fills out_joint, and out_swap_planes_1 asks the caller to swap el1's faces 0 and 1. A joint line no longer than sqrt(zero_length_squared) is degenerate; coplanar_tolerance is the squared distance within which two faces are coplanar.
bool face_to_face_wood(
    const wood_session::Plate& el0,
    const wood_session::Plate& el1,
    std::pair<int, int> el_ids_in,
    const std::vector<double>& joint_volume_extension,
    double limit_min_joint_length,
    double zero_length_squared,
    double coplanar_tolerance,
    double dihedral_angle_threshold,
    bool all_treated_as_rotated,
    bool rotated_joint_as_average,
    int  search_type,
    wood_session::WoodJoint& out_joint,
    bool& out_swap_planes_1);
