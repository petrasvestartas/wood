#pragma once

#include "pch.h"

#include "wood_interaction_contact_cross.h"
#include "wood_interaction_contact_face.h"
#include "wood_interaction_feature.h"

namespace wood_session {

/// A plate-to-plate joint: the pair, the face contact it was solved from, its parameters, the cut outlines per element per face and the two element features the hosts carry; name is the joint library variant ("ss_e_ip_2", "side_removal"), empty before construction. The solver builds it in place; the session stores it whole.
class InteractionFeaturePlate : public InteractionFeature {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionFeaturePlate"; // The tag the kernel writes and the registry reads.

    std::string element_a; // The male element, by guid; swapped with element_b by the solver, so not ordered. index_of_plate() gives a position.
    std::string element_b; // The female element, by guid.
    InteractionContactFace contact; // Which faces touched, and where: the solver's copy, oriented male to female.
    int joint_type = 0; // Refined solver code: 11/12/13 side-side, 20 top-side, 30 cross, 40 top-top.
    std::array<int, 2> cross_faces{-1, -1}; // Cross joints only: the second side face of each element in the crossing; {-1, -1} otherwise.
    std::array<session_cpp::Line, 2> joint_lines; // The two alignment lines, one per element, along the shared edge.
    std::array<std::optional<session_cpp::Polyline>, 4> joint_volumes; // The volume rectangles: [0] and [1] bound the male side, [2] and [3] the female side when it differs.
    std::array<std::vector<session_cpp::Polyline>, 2> male_outlines; // Male cut outlines per face, [0] bottom and [1] top; the last entry of each face is a 2-point endpoint marker.
    std::array<std::vector<session_cpp::Polyline>, 2> female_outlines; // Female cut outlines per face, laid out like male_outlines.
    std::array<std::vector<int>, 2> male_fabrication_types; // One FabricationType per male outline.
    std::array<std::vector<int>, 2> female_fabrication_types; // One FabricationType per female outline.
    int divisions = 1; // Number of teeth or notches along the joint line.
    double shift = 0.5; // Lateral offset of the pattern along the joint line, 0..1.
    double length = 0.0; // Length of the joint line.
    double division_length = 0.0; // Spacing between divisions along the joint line.
    std::array<double, 3> scale{1.0, 1.0, 1.0}; // Multiplicative scale of the unit-box geometry, x, y, z.
    bool unit_scale = false; // True when the variant pins its axial size to unit_scale_distance.
    double unit_scale_distance = 0.0; // The axial size a unit-scale variant is pinned to; 0 reads it off the volume rectangle.
    std::vector<std::string> linked_joints; // Guids of the joints this one is linked with (three-valence shadows).
    std::vector<std::vector<std::array<int, 4>>> linked_joints_seq; // Per linked joint, the vertex ranges merge_linked_joints interleaves.
    bool link = false; // True when this joint is the link of a three-valence group.
    bool no_orient = false; // True when the outlines are already in world space and must not be oriented.
    std::array<session_cpp::ElementFeature, 2> element_features; // The joint as each host element carries it: [0] male (element_a, face_a), [1] female; bodies current only after sync_features().
    mutable std::array<std::string, 2> feature_guids; // Identity of the two sides, minted on first read; kept here because an ElementFeature copy drops its guid.

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// An empty joint: type 0, one division, shift 0.5, unit scale off, zero-length lines.
    InteractionFeaturePlate();

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "plate".
    std::string_view kind() const override;

    /// The guid of one side, minted on first read.
    const std::string& feature_guid(int side) const;

    /// Rebuilds element_features from the solver fields.
    void sync_features();

    /// sync_features() applied to copies: identity preserved, the joint itself untouched.
    std::array<session_cpp::ElementFeature, 2> to_features() const;

    /// The contact this joint was solved from, oriented male to female: its face contact, or for a cross joint the crossing with both side faces per element, the mid-plane polygon, its two lines and its two volumes.
    std::shared_ptr<InteractionContact> to_contact() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The fields as wood_proto.InteractionFeaturePlate bytes: what the kernel carries in interaction_data.
    std::string interaction_data_dumps() const override;

    /// A plate joint from wood_proto.InteractionFeaturePlate bytes; the kernel sets the guid and the name.
    static InteractionFeaturePlate interaction_data_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel, so a Session load rebuilds plate joints.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionFeaturePlate(type, elements, faces, name)".
    std::string str() const override;
};

} // namespace wood_session
