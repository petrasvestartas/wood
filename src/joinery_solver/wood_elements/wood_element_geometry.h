#pragma once

#include "pch.h"
#include "clipper2/clipper.h"
#include "wood_interaction_feature_solid.h"
#include "wood_interaction_feature_plane.h"

using namespace session_cpp;

namespace wood_session {

/// Closed polyhedral CSG by Manifold.
Mesh solid_boolean(
    const Mesh& source, const Mesh& cutter, SolidOperation operation, double tolerance = 1e-7
);

/// The source minus every cutter in one Manifold batch, keeping only the largest solid when the cuts split it.
Mesh solid_difference(const Mesh& source, const std::vector<Mesh>& cutters);

/// True when Manifold takes the mesh as a solid.
bool manifold_solid(const Mesh& mesh);

/// Several closed meshes as one.
Mesh shells_side_by_side(const std::vector<Mesh>& pieces);
std::optional<Mesh> compute_profile_cut(const Mesh& mesh, const InteractionFeatureSolid& cut);

/// Uses polygon booleans for matching extrusions.
Mesh apply_solid_features(
    Mesh mesh, const std::vector<InteractionFeatureSolid>& cuts, bool drills = true
);

/// The mesh with the cuts applied as a BRep.
BRep solid_features_brep(const Mesh& mesh, const std::vector<InteractionFeatureSolid>& cuts);
BRep mesh_brep(const Mesh& mesh);
void append_mesh(Mesh& target, const Mesh& source);
void append_brep(BRep& target, BRep source);
int circle_segments(double radius, double chord_tolerance);
Mesh drill_mesh(const Line& axis, double radius, double chord_tolerance);
BRep drill_brep(const Line& axis, double radius);

/// The feature_type names an element writes for its own geometry: "outline", "axis", "section", "top", "bottom"; every other type is joinery.
bool is_geometry_feature(std::string_view feature_type);

/// A feature of one polyline, whole element unless a face is given.
ElementFeature
polyline_feature(std::string_view feature_type, const Polyline& polyline, int face_index = -1);

/// The feature_type names WoodSession puts on elements as it stores contacts and joints.
bool is_session_feature(std::string_view feature_type);

/// The features the session put on the element, guids and visibility kept; what every compute_geometry_mesh() carries over.
std::vector<ElementFeature> session_features(const Element& element);

/// A closed square of half-width `radius` centred at `at`, in the plane normal to `direction`.
Polyline square_section(const Point& at, const Vector& direction, const Vector& up, double radius);

/// The closed solid through consecutive closed sections of the same point count: one quad strip per pair, a cap at each end.
Mesh sweep_sections(const std::vector<Polyline>& sections);

/// The closed solid through sections of one point count, wound outwards.
Mesh loft_stations(const std::vector<Polyline>& sections);

/// The same solid as a boundary representation: one quad face per section edge pair, the first and last section as caps.
BRep brep_sections(const std::vector<Polyline>& sections);

/// Unit Newell normal of a planar loop.
Vector compute_newell(const std::vector<Point>& points);

/// CGAL's Plane_3.
Vector cgal_base1(const Vector& normal);

/// An outline as a Clipper2 path in the 2D frame (origin, x_axis, y_axis).
Clipper2Lib::PathD clipper_path(
    const Polyline& outline,
    const Point& origin,
    const Vector& x_axis,
    const Vector& y_axis,
    bool open
);

/// One plane per face of a mesh, origin at the face centroid, Newell normal along the face ring; what contact detection compares.
std::vector<Plane> face_planes(const Mesh& mesh);

/// The enclosed volume of a closed mesh.
double compute_volume(const Mesh& mesh);

/// The solid between matching bottom and top loops as a boundary representation.
BRep brep_between_loops(const std::vector<Polyline>& bottom, const std::vector<Polyline>& top);

/// The solid cut by every plane in turn.
Mesh cut_mesh(const Mesh& geometry, const std::vector<Plane>& planes);
BRep cut_brep(const BRep& geometry, const std::vector<Plane>& planes);

/// An axis and its sections (one closed ring per axis point) trimmed to what the cuts leave.
std::pair<Polyline, std::vector<Polyline>> trim_to_cuts(
    const Polyline& axis, const std::vector<Polyline>& sections, const std::vector<Plane>& cuts
);

/// True when xform flips handedness, a mirror that turns a wood solid inside out.
bool is_mirror(const Xform& xform);

/// The frame at origin with z along z and x the part of along across it; none when the two are parallel.
std::optional<Plane> frame_along(const Point& origin, const Vector& along, const Vector& z);

/// Every polyline, plane or vector moved by xform, in order.
template <class T>
std::vector<T> transformed_list(const std::vector<T>& items, const Xform& xform) {

    std::vector<T> moved;
    moved.reserve(items.size());

    for (const T& item : items)
        moved.push_back(item.transformed(xform));

    return moved;
}

/// A copy of the features with every outline moved by xform, guids and visibility kept.
std::vector<ElementFeature>
transformed_features(const std::vector<ElementFeature>& features, const Xform& xform);

}

/// Returns contact corners without the closing point or collinear vertices.
std::vector<Point> merge_collinear(const Polyline& polygon);

/// The chord tolerance that makes a drill of radius a prism of sides sides.
double sides_tolerance(double radius, int sides);
