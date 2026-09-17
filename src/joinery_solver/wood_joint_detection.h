#pragma once

#include "../src/plane.h"
#include "../src/polyline.h"

#include <array>
#include <utility>

namespace wood_session {

/// One crossing of two plates: where their side faces pass through each other.
struct CrossJoint {
    /// Joint type code, 30 for a cross.
    int type = 30;

    /// The two side faces of element A the crossing involves.
    std::pair<int, int> face_ids_a{-1, -1};

    /// The two side faces of element B the crossing involves.
    std::pair<int, int> face_ids_b{-1, -1};

    /// Closed quad on the mid-plane, 5 points.
    session_cpp::Polyline joint_area;

    /// The two perpendicular centrelines of joint_area, 2 points each.
    std::array<session_cpp::Polyline, 2> joint_lines;

    /// The two parallel quads bounding the joint volume.
    std::array<session_cpp::Polyline, 2> joint_volumes;
};

/// Cross/lap joint detection between two plates from their bottom and top outlines and planes; by reference so the hot loop copies nothing.
bool plane_to_face(
    const session_cpp::Polyline& a_bottom, const session_cpp::Polyline& a_top,
    const session_cpp::Polyline& b_bottom, const session_cpp::Polyline& b_top,
    const session_cpp::Plane& a_plane_bottom, const session_cpp::Plane& a_plane_top,
    const session_cpp::Plane& b_plane_bottom, const session_cpp::Plane& b_plane_top,
    CrossJoint& result,
    double angle_tol = 5.0,
    const std::array<double, 3>& extension = {0.0, 0.0, 0.0});

/// The same, with each plate's bottom/top outlines and planes as arrays.
bool plane_to_face(
    const std::array<session_cpp::Polyline, 2>& polylines_a,
    const std::array<session_cpp::Polyline, 2>& polylines_b,
    const std::array<session_cpp::Plane, 2>& planes_a,
    const std::array<session_cpp::Plane, 2>& planes_b,
    CrossJoint& result,
    double angle_tol = 5.0,
    const std::array<double, 3>& extension = {0.0, 0.0, 0.0});

/// Near-coplanar rejection threshold used by plane_to_face; the caller syncs it from globals::DISTANCE_SQUARED.
void set_cross_joint_distance_squared(double dist_sq);

} // namespace wood_session
