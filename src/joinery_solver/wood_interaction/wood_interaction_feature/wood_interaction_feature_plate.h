#pragma once

#include "pch.h"

#include "wood_interaction_contact_face.h"

namespace wood_session {

class WoodSession;

/// A plate-to-plate joint: the pair, the face contact it was solved from, the variant the joint library built, its parameters, the cut outlines per element per face and the two element features the hosts carry. The solver builds it in place; the interaction stores it whole.
struct FeaturePlate {
    WoodSession* _session = nullptr; // The scene this record was stored in; null until it is added, never written.
    std::string guid; // Identity of the joint; minted by detection, the key its InteractionFeature is stored under.
    std::string element_a; // The male element, by guid; swapped with element_b by the solver, so not ordered. index_of() gives a position.
    std::string element_b; // The female element, by guid.
    ContactFace contact; // Which faces touched, and where.
    int joint_type = 0; // Refined solver code: 11/12/13 side-side, 20 top-side, 30 cross, 40 top-top.
    std::string name; // The joint library variant that built the outlines ("ss_e_ip_2", "side_removal"), empty before construction.
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

    /// An empty joint: type 0, one division, shift 0.5, unit scale off, zero-length lines.
    FeaturePlate();


    /// The scene this record belongs to; throws std::logic_error before the record is added to one.
    WoodSession& session() const;

    /// True once the record has been stored in a scene.
    bool has_session() const { return _session != nullptr; }

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const FeaturePlate& feature);

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
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The joint as JSON: the pair, the contact, every solver field, the two element features.
    nlohmann::ordered_json jsondump() const;

    /// A joint from its JSON.
    static FeaturePlate jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The joint as wood_proto.FeaturePlate bytes.
    std::string pb_dumps() const;

    /// A joint from wood_proto.FeaturePlate bytes.
    static FeaturePlate pb_loads(const std::string& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "FeaturePlate(type, elements, faces, name)".
    std::string str() const;
};

} // namespace wood_session
