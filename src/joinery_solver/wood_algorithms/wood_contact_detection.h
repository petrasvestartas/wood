#pragma once

#include "pch.h"

#include "wood_settings.h"
#include "wood_element_beam.h"
#include "wood_element_plate.h"
#include "wood_interaction_contact_face.h"
#include "wood_interaction_contact_axis.h"
#include "wood_interaction_contact_cross.h"

using namespace session_cpp;

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

/// Candidate element pairs (i, j) with i < j, sorted by i then j.
std::vector<std::pair<int, int>> adjacency_search(
    const std::vector<std::shared_ptr<Element>>& elements,
    double inflate,
    const std::vector<std::string>& names = {}
);

// ═══════════════════════════════════════════════════════════════════════════
// Face contacts - narrow phase
// ═══════════════════════════════════════════════════════════════════════════

/// True when two faces touch back-to-back.
bool faces_coplanar(
    const Plane& face0, const Plane& face1, double cos_angle, double coplanar_tolerance
);

/// The first overlap of two coplanar outlines as a closed polygon in `plane0`, clipped as the 2024 solver clipped it.
bool face_overlap_area(
    const Polyline& outline0,
    const Polyline& outline1,
    const Plane& plane0,
    bool include_triangles,
    double clipper_area,
    Polyline& out_area,
    int decimals = 2
);

// ═══════════════════════════════════════════════════════════════════════════
// Face contacts - both phases
// ═══════════════════════════════════════════════════════════════════════════

/// Every contacting face pair between ONE element pair, ordered by face index.
std::vector<InteractionContactFace> face_contacts_for_pair(
    Element& ea,
    Element& eb,
    const Settings& settings,
    DetectionTrace* trace = nullptr,
    bool with_volumes = true
);

/// Every face pair in contact across a set of elements, as (position of the first element, position of the second.
std::vector<std::tuple<int, int, InteractionContactFace>> face_contacts(
    const std::vector<std::shared_ptr<Element>>& elements,
    const Settings& settings,
    const std::vector<std::string>& names = {}
);

// ═══════════════════════════════════════════════════════════════════════════
// Cross contacts
// ═══════════════════════════════════════════════════════════════════════════

/// Cross/lap contact detection between two plates from their bottom and top outlines and planes; by reference so the hot loop copies nothing.
bool plane_to_face(
    const Polyline& a_bottom,
    const Polyline& a_top,
    const Polyline& b_bottom,
    const Polyline& b_top,
    const Plane& a_plane_bottom,
    const Plane& a_plane_top,
    const Plane& b_plane_bottom,
    const Plane& b_plane_top,
    double distance_squared,
    InteractionContactCross& result,
    double angle_tol = 5.0,
    const std::array<double, 3>& extension = {0.0, 0.0, 0.0}
);

// ═══════════════════════════════════════════════════════════════════════════
// Axis contacts
// ═══════════════════════════════════════════════════════════════════════════

/// The closest segment pair of every two beam axes within `min_distance`.
std::vector<std::tuple<int, int, InteractionContactAxis>>
axis_contacts(const std::vector<std::shared_ptr<Beam>>& beams, double min_distance);

}
