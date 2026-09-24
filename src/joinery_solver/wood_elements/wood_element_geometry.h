#pragma once

#include "pch.h"

namespace wood_session {

/// The feature_type names an element writes for its own geometry: "outline", "axis", "section"; every other type is joinery. The centroid is not one: `Element::point()` computes and caches it.
bool is_geometry_feature(std::string_view feature_type);

/// A feature of one polyline, whole element unless a face is given.
session_cpp::ElementFeature polyline_feature(std::string_view feature_type, const session_cpp::Polyline& polyline, int face_index = -1);

/// The feature_type names WoodSession puts on elements as it stores contacts and joints: "joint", "contact".
bool is_session_feature(std::string_view feature_type);

/// The features the session put on the element, guids and visibility kept; what every compute_geometry_mesh() carries over.
std::vector<session_cpp::ElementFeature> session_features(const session_cpp::Element& element);

/// A closed square of half-width `radius` centred at `at`, in the plane normal to `direction`, one side along `up` projected into that plane, wound counter-clockwise about `direction` so sweep_sections faces outward.
session_cpp::Polyline square_section(const session_cpp::Point& at, const session_cpp::Vector& direction, const session_cpp::Vector& up, double radius);

/// The closed solid through consecutive closed sections of the same point count: one quad strip per pair, a cap at each end.
session_cpp::Mesh sweep_sections(const std::vector<session_cpp::Polyline>& sections);

/// The same solid as a boundary representation: one quad face per section edge pair, the first and last section as caps.
session_cpp::BRep brep_sections(const std::vector<session_cpp::Polyline>& sections);

/// Unit Newell normal of a planar loop, the closing point ignored: right for a concave loop, where the corner-cross sum of Vector::average_normal can flip.
session_cpp::Vector compute_newell(const std::vector<session_cpp::Point>& points);

/// One plane per face of a mesh, origin at the face centroid, Newell normal along the face ring; what contact detection compares.
std::vector<session_cpp::Plane> face_planes(const session_cpp::Mesh& mesh);

/// The enclosed volume of a closed mesh, a holed cap summed over its face triangulation where the loft or a plane cut left one; Mesh::volume() fans the outer ring alone and counts the hole as solid.
double compute_volume(const session_cpp::Mesh& mesh);

/// The solid between matching bottom and top loops as a boundary representation: loop 0 the outer outline, the rest holes; one quad per edge of every loop.
session_cpp::BRep brep_between_loops(const std::vector<session_cpp::Polyline>& bottom, const std::vector<session_cpp::Polyline>& top);

/// The solid cut by every plane in turn, each keeping the side its normal points to; a mesh stays a mesh, a BRep a BRep.
session_cpp::Mesh cut_mesh(const session_cpp::Mesh& geometry, const std::vector<session_cpp::Plane>& planes);
session_cpp::BRep cut_brep(const session_cpp::BRep& geometry, const std::vector<session_cpp::Plane>& planes);

/// True when xform flips handedness, a mirror that turns a wood solid inside out.
bool is_mirror(const session_cpp::Xform& xform);

/// Every polyline, plane or vector moved by xform, in order.
template <class T>
std::vector<T> transformed_list(const std::vector<T>& items, const session_cpp::Xform& xform) {

    std::vector<T> moved;
    moved.reserve(items.size());

    for (const T& item : items)
        moved.push_back(item.transformed(xform));

    return moved;
}

/// A copy of the features with every outline moved by xform, guids and visibility kept.
std::vector<session_cpp::ElementFeature> transformed_features(const std::vector<session_cpp::ElementFeature>& features, const session_cpp::Xform& xform);

} // namespace wood_session
