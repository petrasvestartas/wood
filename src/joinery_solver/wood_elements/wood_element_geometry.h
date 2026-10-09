#pragma once

#include "pch.h"
#include "wood_interaction_feature_solid.h"
#include "wood_interaction_feature_plane.h"

using namespace session_cpp;

namespace wood_session {

/// Closed polyhedral CSG by Manifold, every piece kept, faces merged back per input face; curved inputs must be meshed to their requested tolerance first.
Mesh solid_boolean(const Mesh& source, const Mesh& cutter,
                               SolidOperation operation, double tolerance = 1e-7);

/// The source minus every cutter in one Manifold batch, keeping only the largest solid when the cuts split it: the offcuts fall away.
Mesh solid_difference(const Mesh& source, const std::vector<Mesh>& cutters);

/// True when Manifold takes the mesh as a solid: closed, every edge on two faces, no face through another; what every cutter must be.
bool manifold_solid(const Mesh& mesh);

/// Several closed meshes as one, every shell with its own vertices and faces: what a cutter of touching, overlapping or nested pieces is, since a boolean takes a cutter's bodies apart and unites them inside Manifold, where a union made first and read back loses to its own coincident faces.
Mesh shells_side_by_side(const std::vector<Mesh>& pieces);
std::optional<Mesh> compute_profile_cut(const Mesh& mesh, const InteractionFeatureSolid& cut);

/// Uses polygon booleans for matching extrusions; the other differences in a row go to Manifold as one batch keeping the largest solid, intersections and unions one by one.
Mesh apply_solid_features(Mesh mesh, const std::vector<InteractionFeatureSolid>& cuts, bool drills = true);

/// The mesh with the cuts applied as a BRep: the round holes the cuts carry made exact, cylindrical faces with circle or ellipse loops, where they are clear of every edge and of each other, else the cut mesh's planar faces with their holes as inner loops.
BRep solid_features_brep(const Mesh& mesh, const std::vector<InteractionFeatureSolid>& cuts);
BRep mesh_brep(const Mesh& mesh);
void append_mesh(Mesh& target, const Mesh& source);
void append_brep(BRep& target, BRep source);
int circle_segments(double radius, double chord_tolerance);
Mesh drill_mesh(const Line& axis, double radius, double chord_tolerance);
BRep drill_brep(const Line& axis, double radius);

/// The feature_type names an element writes for its own geometry: "outline", "axis", "section", "top", "bottom"; every other type is joinery. The centroid is not one: `Element::point()` computes and caches it.
bool is_geometry_feature(std::string_view feature_type);

/// A feature of one polyline, whole element unless a face is given.
ElementFeature polyline_feature(std::string_view feature_type, const Polyline& polyline, int face_index = -1);

/// The feature_type names WoodSession puts on elements as it stores contacts and joints: "joint", "contact", and the "drill" of every hole a joint makes; a solid another element takes away is that element, a child layer of the one it cuts.
bool is_session_feature(std::string_view feature_type);

/// The features the session put on the element, guids and visibility kept; what every compute_geometry_mesh() carries over.
std::vector<ElementFeature> session_features(const Element& element);

/// A closed square of half-width `radius` centred at `at`, in the plane normal to `direction`, one side along `up` projected into that plane, wound counter-clockwise about `direction` so sweep_sections faces outward.
Polyline square_section(
    const Point& at,
    const Vector& direction,
    const Vector& up,
    double radius
);

/// The closed solid through consecutive closed sections of the same point count: one quad strip per pair, a cap at each end.
Mesh sweep_sections(const std::vector<Polyline>& sections);

/// The closed solid through sections of one point count, wound outwards: both end caps, one face per side strip whose points share a plane, else one per quad; repeated points collapse, so a section equal to its neighbour along part of the ring gives a stepped solid.
Mesh loft_stations(const std::vector<Polyline>& sections);

/// The same solid as a boundary representation: one quad face per section edge pair, the first and last section as caps.
BRep brep_sections(const std::vector<Polyline>& sections);

/// Unit Newell normal of a planar loop, the closing point ignored: right for a concave loop, where the corner-cross sum of Vector::average_normal can flip.
Vector compute_newell(const std::vector<Point>& points);

/// One plane per face of a mesh, origin at the face centroid, Newell normal along the face ring; what contact detection compares.
std::vector<Plane> face_planes(const Mesh& mesh);

/// The enclosed volume of a closed mesh, a holed cap summed over its face triangulation where the loft or a plane cut left one; Mesh::volume() fans the outer ring alone and counts the hole as solid.
double compute_volume(const Mesh& mesh);

/// The solid between matching bottom and top loops as a boundary representation: loop 0 the outer outline, the rest holes; one quad per edge of every loop.
BRep brep_between_loops(const std::vector<Polyline>& bottom, const std::vector<Polyline>& top);

/// The solid cut by every plane in turn, each keeping the side its normal points to; a mesh stays a mesh, a BRep a BRep.
Mesh cut_mesh(const Mesh& geometry, const std::vector<Plane>& planes);
BRep cut_brep(const BRep& geometry, const std::vector<Plane>& planes);

/// An axis and its sections (one closed ring per axis point) trimmed to what the cuts leave, each keeping the side its normal points to: the axis clipped to its one kept run, a ring whose point was cut away dropped, every other ring clipped by the cuts, and at a cut end the ring moved along the axis onto that cut and clipped by the others, the member's end face; one ring per trimmed axis point, empty where no face is left, none when the sections do not match the axis.
std::pair<Polyline, std::vector<Polyline>> trim_to_cuts(
    const Polyline& axis,
    const std::vector<Polyline>& sections,
    const std::vector<Plane>& cuts
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
std::vector<ElementFeature> transformed_features(const std::vector<ElementFeature>& features, const Xform& xform);

} // namespace wood_session
