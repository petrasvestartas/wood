#pragma once

#include "pch.h"

#include "wood_interaction_contact_cross.h"

namespace wood_session {


/// Cross/lap joint detection between two plates from their bottom and top outlines and planes; by reference so the hot loop copies nothing.
bool plane_to_face(
    const session_cpp::Polyline& a_bottom, const session_cpp::Polyline& a_top,
    const session_cpp::Polyline& b_bottom, const session_cpp::Polyline& b_top,
    const session_cpp::Plane& a_plane_bottom, const session_cpp::Plane& a_plane_top,
    const session_cpp::Plane& b_plane_bottom, const session_cpp::Plane& b_plane_top,
    ContactCross& result,
    double angle_tol = 5.0,
    const std::array<double, 3>& extension = {0.0, 0.0, 0.0});

/// The same, with each plate's bottom/top outlines and planes as arrays.
bool plane_to_face(
    const std::array<session_cpp::Polyline, 2>& polylines_a,
    const std::array<session_cpp::Polyline, 2>& polylines_b,
    const std::array<session_cpp::Plane, 2>& planes_a,
    const std::array<session_cpp::Plane, 2>& planes_b,
    ContactCross& result,
    double angle_tol = 5.0,
    const std::array<double, 3>& extension = {0.0, 0.0, 0.0});

/// Near-coplanar rejection threshold used by plane_to_face; the caller syncs it from config::DISTANCE_SQUARED.
void set_cross_joint_distance_squared(double dist_sq);

} // namespace wood_session
