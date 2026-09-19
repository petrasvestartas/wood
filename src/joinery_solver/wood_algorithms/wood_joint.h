#pragma once

#include "pch.h"

#include "wood_element_plate.h"
#include "wood_interaction_contact_face.h"
#include "wood_interaction_feature_plate.h"
#include "wood_joint_cut_type.h"

namespace wood_session {

/// The solver's working joint: a FeaturePlate with the pair and the face contact it is being solved from. compute_joints stores it into its interaction, where the pair is the graph edge and the contact a stored InteractionContact; the record itself is never serialised.
struct WoodJoint : FeaturePlate {
    std::string element_a; // The male element, by guid; swapped with element_b by the solver, so not ordered. index_of() gives a position.
    std::string element_b; // The female element, by guid.
    ContactFace contact; // Which faces touched, and where.
    int dbg_coplanar = 0; // Face pairs that passed the coplanarity test in detection.
    int dbg_boolean = 0; // Face pairs with a real overlap area in detection.
    std::string dbg_fail_reason; // Why detection rejected the pair, filled only under TRACE.
    std::array<session_cpp::ElementFeature, 2> element_features; // The joint as each host element carries it: [0] male (element_a, face_a), [1] female; bodies current only after sync_features().
    mutable std::array<std::string, 2> feature_guids; // Identity of the two sides, minted on first read; kept here because an ElementFeature copy drops its guid.

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const WoodJoint& j);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The guid of one side, minted on first read.
    const std::string& feature_guid(int side) const;

    /// Rebuilds element_features from the solver fields.
    void sync_features();

    /// sync_features() applied to copies: identity preserved, the joint itself untouched.
    std::array<session_cpp::ElementFeature, 2> to_features() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "WoodJoint(type, elements, faces, name)".
    std::string str() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Joint construction
// ═══════════════════════════════════════════════════════════════════════════

/// Moves the volume rectangles of a unit-scale joint to unit_scale_distance apart along the joint line.
void apply_unit_scale(WoodJoint& joint);

/// Maps the unit-box outlines onto the joint volumes by change of basis, after apply_unit_scale.
void joint_orient_to_connection_area(WoodJoint& joint);

/// Interleaves the outlines of the joints in linked_joints into this one, following linked_joints_seq.
void merge_linked_joints(WoodJoint& joint, std::vector<WoodJoint>& all_joints);

/// Sets divisions from the joint length and division_distance, at least one.
void joint_get_divisions(WoodJoint& joint, double division_distance);

/// The [width, height, length] extension for a joint type: side-side (11/12/13) reads triple 0, top-side (20) triple 1, top-top (40) triple 2, cross (30) triple 3; a 3-entry list serves every type.
std::array<double, 3> joint_volume_extension(const std::vector<double>& extension, int joint_type);

/// Position of the plate with this guid, or -1.
int index_of(const std::vector<std::shared_ptr<Plate>>& elements, const std::string& guid);

} // namespace wood_session
