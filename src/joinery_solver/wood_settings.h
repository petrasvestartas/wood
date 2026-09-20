#pragma once

#include "pch.h"

/// Which detection pass compute_features runs.
enum SearchType : int {
    face_to_face = 0, // Coplanar faces: ss_e_ip / ss_e_op / ss_e_r / ts_e_p / tt_e_p.
    cross_joint = 1, // Elements passing through each other: plane_to_face, type 30.
    face_to_face_then_cross = 2, // Face-to-face first, cross as the fallback.
};

namespace wood_session {

/// Every tunable of the solver: read from the dataset yml by config::load_yaml, held by the WoodSession that solves, passed by reference into every algorithm and joint builder, written with the scene. No algorithm reads a global.
struct Settings {
    SearchType search_type = face_to_face; // The detection pass (yml `search_type`).
    std::vector<double> joint_parameters = {300, 0.5, 3, 450, 0.64, 15, 450, 0.5, 20, 300, 0.5, 30, 6, 0.95, 40, 300, 0.5, 58, 300, 1.0, 60}; // Joint-family triples [division_length (mm), shift, joint id]; families 0=ss_e_ip 1=ss_e_op 2=ts_e_p 3=cr_c_ip 4=tt_e_p 5=ss_e_r 6=b.
    std::vector<double> joint_volume_extension = {0.0, 0.0, 0.0}; // Additive [width, height, length] extension (mm) of joint volumes: one triple for every joint type, or one per type (side-side, top-side, top-top, cross).
    std::array<double, 3> joint_scale = {1.0, 1.0, 1.0}; // Multiplicative [sx, sy, sz] scale of joint geometry before insertion; 1 = no change.
    double dihedral_angle = 150.0; // Degrees; rotated-joint threshold.
    bool all_treated_as_rotated = false; // Force the rotated geometry path.
    bool rotated_joint_as_average = false; // Averaged plane for rotated joints.
    double distance = 0.1; // Inflate AABBs / point-merge tolerance (mm).
    double distance_squared = 0.01; // Squared coplanarity tolerance (mm²).
    double angle = 0.11; // Angular tolerance, radians.
    double duplicate_points_tolerance = 0.0; // Consecutive duplicate points removed when an obj is read.
    double limit_min_joint_length = 0.0; // Joints whose centreline is shorter are dropped.
    int64_t clipper_scale = 1000000; // Mm -> int64 scale for the 2D boolean (1e6 = nanometre grid).
    double clipper_area = 0.01; // Overlap areas at or below this (mm²) are not a contact.
    std::vector<double> beams; // Beam datasets (yml `beams`): [radius, allowed joint type, min_distance, volume_length, cross_or_side_to_end, flip_male].
    std::map<std::string, std::array<std::vector<session_cpp::Polyline>, 2>> custom_joints; // User joint outlines per family ("ss_e_ip", "ss_e_op", "ts_e_p", "cr_c_ip", "tt_e_p", "ss_e_r", "b"): [0] male, [1] female, pairs (face0, face1).

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The male and female user outlines of one family; both empty when none were given.
    const std::array<std::vector<session_cpp::Polyline>, 2>& custom(const std::string& family) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The settings as JSON.
    nlohmann::ordered_json jsondump() const;

    /// Settings from their JSON.
    static Settings jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The settings as wood_proto.Settings bytes.
    std::string pb_dumps() const;

    /// Settings from wood_proto.Settings bytes.
    static Settings pb_loads(const std::string& data);
};

} // namespace wood_session
