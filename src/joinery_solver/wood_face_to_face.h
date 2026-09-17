#pragma once

#include "wood_pch.h"

#include "wood_globals.h"
#include "wood_joint.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Contact detection - what detection reads off an element
// ═══════════════════════════════════════════════════════════════════════════

/// One element as detection sees it: face outlines, their planes, the name to filter on, and whether the plate face convention ([0] bottom, [1] top, [2..] sides) applies.
struct ContactElement {
    std::vector<session_cpp::Polyline> polylines; // The face outlines.
    std::vector<session_cpp::Plane> planes; // One plane per outline.
    std::string name; // The element name, what `names` filters on.
    bool plate_convention = false; // True when polylines follow the plate convention: [0] bottom, [1] top, [2..] sides.

    /// An empty view.
    ContactElement() = default;

    /// A plate's own outlines and planes, with the plate convention.
    explicit ContactElement(const Plate& plate);

    /// A plate through its own fields, any other element through the face outlines of its mesh.
    explicit ContactElement(session_cpp::Element& element);
};

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

/// One face plane unpacked to plain doubles so the O(faces²) scan never calls Point/Vector::operator[]; the normal need not be unit length, `mag_sq` carries its scale.
struct FacePlane {
    double ox; // Origin x.
    double oy; // Origin y.
    double oz; // Origin z.
    double nx; // Normal x.
    double ny; // Normal y.
    double nz; // Normal z.
    double mag_sq; // Squared length of the normal.
};

/// Every face plane of an element unpacked, once per element pair rather than once per face pair.
std::vector<FacePlane> face_planes(const ContactElement& element);

/// True when two faces touch back-to-back: normals antiparallel within `cos_angle` (cos of globals::ANGLE, radians) and each origin within `coplanar_tolerance` (a squared distance) of the other's plane.
bool faces_coplanar(
    const FacePlane& face0,
    const FacePlane& face1,
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

/// Tallies from one element-pair scan; face_to_face_wood reports them as dbg_coplanar / dbg_boolean.
struct PairScanStats {
    int coplanar = 0; // Face pairs that passed the coplanarity test.
    int overlapping = 0; // Of those, the ones with a real overlap area.
    int empty_i = -1; // Face of the first element in the last pair whose boolean came back empty, -1 when none.
    int empty_j = -1; // Face of the second element in that pair, -1 when none.
};

/// Every contacting face pair between ONE element pair, ordered by face index; call it inside the caller's loop over element pairs, since get_connection_zones swaps faces 0 and 1 mid-run.
std::vector<FaceContact> face_contacts_for_pair(
    const ContactElement& ea,
    const ContactElement& eb,
    double cos_angle,
    double coplanar_tolerance,
    PairScanStats* stats = nullptr);

/// Every face pair in contact across a set of elements: adjacency_search, then faces_coplanar + face_overlap_area over each candidate; `angle` in radians, `coplanar_tolerance` a squared distance.
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

/// Classifies one element pair as a wood joint with every tunable explicit; true fills out_joint, and out_swap_planes_1 asks the caller to swap el1's faces 0 and 1.
bool face_to_face_wood(
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
