#pragma once

#include "pch.h"

namespace wood_session {

/// The feature_type names an element writes for its own geometry: "outline", "axis", "section"; every other type is joinery. The centroid is not one: `Element::point()` computes and caches it.
bool is_geometry_feature(std::string_view feature_type);

/// A feature of one polyline, whole element unless a face is given.
session_cpp::ElementFeature polyline_feature(std::string_view feature_type, const session_cpp::Polyline& polyline, int face_index = -1);

/// The "joint" features the session put on the element, guids kept; what every compute_geometry() carries over.
std::vector<session_cpp::ElementFeature> joint_features(const session_cpp::Element& element);

/// A closed square of half-width `radius` centred at `at`, in the plane normal to `direction`, one side along `up` projected into that plane.
session_cpp::Polyline square_section(const session_cpp::Point& at, const session_cpp::Vector& direction, const session_cpp::Vector& up, double radius);

/// The closed solid through consecutive closed sections of the same point count: one quad strip per pair, a cap at each end.
session_cpp::Mesh sweep_sections(const std::vector<session_cpp::Polyline>& sections);

} // namespace wood_session
