#pragma once

#include "pch.h"

#include "wood_settings.h"
#include "wood_element_beam.h"
#include "wood_element_plate.h"
#include "wood_interaction_contact_face.h"
#include "wood_interaction_contact_axis.h"
#include "wood_interaction_contact_cross.h"

namespace wood_session {

/// What detection counted and why it gave up on a pair; filled only when a caller asks for it.
struct DetectionTrace {
    int coplanar = 0; // Face pairs that passed the coplanarity test.
    int overlapping = 0; // Face pairs with a real overlap area.
    std::string fail_reason; // Why the pair was rejected, under TRACE.
};

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

/// Largest overlap of two coplanar outlines as a closed polygon in `plane0`, via Clipper2 on the `clipper_scale` grid; areas at or below `clipper_area` are none; coplanarity is a precondition, triangles count only when `include_triangles`.
bool face_overlap_area(
    const session_cpp::Polyline& outline0,
    const session_cpp::Polyline& outline1,
    const session_cpp::Plane& plane0,
    bool include_triangles,
    int64_t clipper_scale,
    double clipper_area,
    session_cpp::Polyline& out_area);

// ═══════════════════════════════════════════════════════════════════════════
// Face contacts - both phases
// ═══════════════════════════════════════════════════════════════════════════

/// Every contacting face pair between ONE element pair, ordered by face index; `trace`, when given, counts the coplanar and overlapping pairs.
std::vector<ContactFace> face_contacts_for_pair(
    session_cpp::Element& ea,
    session_cpp::Element& eb,
    const Settings& settings,
    DetectionTrace* trace = nullptr);

/// Every face pair in contact across a set of elements, as (position of the first element, position of the second, the contact): adjacency_search within settings.distance, then faces_coplanar + face_overlap_area over each candidate.
std::vector<std::tuple<int, int, ContactFace>> face_contacts(
    const std::vector<std::shared_ptr<session_cpp::Element>>& elements,
    const Settings& settings,
    const std::vector<std::string>& names = {});

// ═══════════════════════════════════════════════════════════════════════════
// Cross contacts
// ═══════════════════════════════════════════════════════════════════════════

/// Cross/lap contact detection between two plates from their bottom and top outlines and planes; by reference so the hot loop copies nothing. A vertex within sqrt(distance_squared) of the other plate's plane is near-coplanar and rejects the crossing.
bool plane_to_face(
    const session_cpp::Polyline& a_bottom, const session_cpp::Polyline& a_top,
    const session_cpp::Polyline& b_bottom, const session_cpp::Polyline& b_top,
    const session_cpp::Plane& a_plane_bottom, const session_cpp::Plane& a_plane_top,
    const session_cpp::Plane& b_plane_bottom, const session_cpp::Plane& b_plane_top,
    double distance_squared,
    ContactCross& result,
    double angle_tol = 5.0,
    const std::array<double, 3>& extension = {0.0, 0.0, 0.0});

// ═══════════════════════════════════════════════════════════════════════════
// Axis contacts
// ═══════════════════════════════════════════════════════════════════════════

/// The closest segment pair of every two beam axes within `min_distance`, one per beam pair, as (position of the first beam, position of the second, the contact).
std::vector<std::tuple<int, int, ContactAxis>> axis_contacts(const std::vector<std::shared_ptr<Beam>>& beams, double min_distance);

}  // namespace wood_session
