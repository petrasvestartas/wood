#pragma once

#include "pch.h"

namespace wood_session {

/// A plate-to-plate joint as the joint library built it: the variant, its parameters and the cut outlines per element per face; the pair and the contact it was solved from live on the interaction.
struct FeaturePlate {
    int joint_type = 0; // Refined solver code: 11/12/13 side-side, 20 top-side, 30 cross, 40 top-top.
    std::string name; // The joint library variant that built the outlines ("ss_e_ip_2", "side_removal"), empty before construction.
    std::array<int, 2> cross_faces{-1, -1}; // Cross joints only: the second side face of each element in the crossing; {-1, -1} otherwise.
    std::array<session_cpp::Line, 2> joint_lines; // The two alignment lines, one per element, along the shared edge.
    std::array<std::optional<session_cpp::Polyline>, 4> joint_volumes; // The volume rectangles: [0] and [1] bound the male side, [2] and [3] the female side when it differs.
    std::array<std::vector<session_cpp::Polyline>, 2> male_outlines; // Male cut outlines per face, [0] bottom and [1] top; the last entry of each face is a 2-point endpoint marker.
    std::array<std::vector<session_cpp::Polyline>, 2> female_outlines; // Female cut outlines per face, laid out like male_outlines.
    std::array<std::vector<int>, 2> male_cut_types; // One CutType per male outline.
    std::array<std::vector<int>, 2> female_cut_types; // One CutType per female outline.
    int divisions = 1; // Number of teeth or notches along the joint line.
    double shift = 0.5; // Lateral offset of the pattern along the joint line, 0..1.
    double length = 0.0; // Length of the joint line.
    double division_length = 0.0; // Spacing between divisions along the joint line.
    std::array<double, 3> scale{1.0, 1.0, 1.0}; // Multiplicative scale of the unit-box geometry, x, y, z.
    bool unit_scale = false; // True when the variant pins its axial size to unit_scale_distance.
    double unit_scale_distance = 0.0; // The axial size a unit-scale variant is pinned to; 0 reads it off the volume rectangle.
    std::vector<int> linked_joints; // Solver-run indices of the joints this one is linked with (three-valence).
    std::vector<std::vector<std::array<int, 4>>> linked_joints_seq; // Per linked joint, the vertex ranges merge_linked_joints interleaves.
    bool link = false; // True when this joint is the link of a three-valence group.
    bool no_orient = false; // True when the outlines are already in world space and must not be oriented.

    /// An empty joint: type 0, one division, shift 0.5, unit scale off, zero-length lines.
    FeaturePlate();

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const FeaturePlate& feature);

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The joint as JSON, every solver field.
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

    /// "FeaturePlate(type, name, divisions)".
    std::string str() const;
};

} // namespace wood_session
