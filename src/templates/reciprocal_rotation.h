#pragma once
#include "session.h"
#include "reciprocal_boundary.h"
#include "intersection.h"
#include "polyline.h"
#include "tolerance.h"

#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>

using namespace session_cpp;

/// A reciprocal beam frame from a mesh (rotation-based nexorade).
///
/// Every mesh edge is scaled about its midpoint and turned about the edge normal by angle, so the
/// beams around each vertex form a pinwheel; each end then stops at the first side face it would
/// enter among the other beams at that vertex, or at the frame on the boundary. Quads and even-gons
/// alike. Produces the mesh, all beam box meshes, and the side-face outlines.
///
/// Usage:
///   ReciprocalRotation rb;                          // default 12×10 dome
///   ReciprocalRotation rb(6, 5, 6.0, 5.0, 2.0);    // smaller dome
class ReciprocalRotation {
public:
    Mesh dome_mesh;
    std::vector<Mesh>     beams;
    std::vector<Polyline> side0;        // right-face outlines (+right direction)
    std::vector<Polyline> side1;        // left-face outlines  (-right direction)
    std::vector<Polyline> beam_bottom;  // bottom-face outlines (-up direction, for joinery)
    std::vector<Polyline> beam_top;     // top-face outlines    (+up direction, for joinery)
    std::vector<std::array<double,3>> beam_dirs;  // unit axis direction per beam
    std::vector<std::array<double,3>> beam_ups;   // unit up (thickness) direction per beam
    std::vector<Mesh>     boundary_beams;        // straight beams on the naked edges, in boundary-loop order, mitred at the shared vertices
    std::vector<Polyline> boundary_side0;        // right-face outlines of the boundary beams
    std::vector<Polyline> boundary_side1;        // left-face outlines of the boundary beams
    std::vector<Polyline> boundary_beam_bottom;  // bottom-face outlines of the boundary beams
    std::vector<Polyline> boundary_beam_top;     // top-face outlines of the boundary beams, corner i above boundary_beam_bottom[i]

    /// Parametric sinusoidal dome constructor.
    ReciprocalRotation(int    nx                = 12,
                       int    ny                = 10,
                       double W                 = 12.0,
                       double D                 = 10.0,
                       double h                 = 3.0,
                       double angle             = 0.35,
                       double scale             = 1.4,
                       double beam_w            = 0.10,
                       double beam_h            = 0.0,
                       double extend_factor     = 5.0,
                       double cut_offset_factor = 1.0,
                       wood_reciprocal::BoundaryTwist boundary_twist = wood_reciprocal::BoundaryTwist::AtBends,
                       const std::map<std::pair<size_t, size_t>, Vector>& boundary_ups = {},
                       wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Mitre,
                       const std::map<std::pair<size_t, size_t>, int>& through_priority = {})
    {

        if (nx < 1 || ny < 1) {
            throw std::invalid_argument("ReciprocalRotation: nx and ny must be >= 1");
        }

        dome_mesh = make_dome(nx, ny, W, D, h);
        _build(dome_mesh, nx, angle, scale, beam_w, beam_h, extend_factor, cut_offset_factor, boundary_twist, boundary_ups, corner_joint, through_priority);
    }

    /// External mesh constructor — use any quad mesh as the base.
    /// v-edge stagger is disabled (pass nx > 0 explicitly if needed).
    explicit ReciprocalRotation(Mesh ext_mesh,
                                double angle             = 0.35,
                                double scale             = 1.4,
                                double beam_w            = 0.10,
                                double beam_h            = 0.0,
                                double extend_factor     = 5.0,
                                double cut_offset_factor = 1.0,
                                wood_reciprocal::BoundaryTwist boundary_twist = wood_reciprocal::BoundaryTwist::AtBends,
                       const std::map<std::pair<size_t, size_t>, Vector>& boundary_ups = {},
                       wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Mitre,
                       const std::map<std::pair<size_t, size_t>, int>& through_priority = {})
    {
        dome_mesh = std::move(ext_mesh);
        _build(dome_mesh, -1, angle, scale, beam_w, beam_h, extend_factor, cut_offset_factor, boundary_twist, boundary_ups, corner_joint, through_priority);
    }

private:

    void _build(const Mesh& m, int /*nx_stagger*/,
                double angle, double scale, double beam_w, double beam_h,
                double extend_factor, double cut_offset_factor,
                wood_reciprocal::BoundaryTwist boundary_twist,
                const std::map<std::pair<size_t, size_t>, Vector>& boundary_ups,
                wood_reciprocal::CornerJoint corner_joint,
                const std::map<std::pair<size_t, size_t>, int>& through_priority)
    {

        if (beam_w <= 0.0) {
            throw std::invalid_argument("ReciprocalRotation: beam_w must be positive");
        }

        if (beam_h <= 0.0)
            beam_h = beam_w * 2.0;
        const double extend = beam_w * extend_factor;         // how far beyond its rotated ends a beam may run to reach the beam it bears on
        const double face_offset = beam_w * 0.5 * cut_offset_factor;  // where an end stops, from the crossing beam's axis: 1.0 is flush with its side face
        std::vector<size_t> fkeys = m.faces();
        std::vector<std::vector<size_t>> faces(fkeys.size());  // vertex keys per face, in winding order
        std::vector<Vector> face_normals(fkeys.size());
        std::map<size_t, Point> vertex_points;
        for (size_t fi = 0; fi < fkeys.size(); fi++) {
            faces[fi] = m.face_vertices(fkeys[fi]).value();
            face_normals[fi] = m.face_normal(fkeys[fi]).value_or(Vector(0, 0, 0));
            for (size_t vk : faces[fi])
                vertex_points.emplace(vk, m.vertex_point(vk).value());
        }

        std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>> owners = wood_reciprocal::edge_owners(faces);
        std::vector<std::pair<size_t,size_t>> ekeys = m.edges();
        wood_reciprocal::BoundaryFrame frame = wood_reciprocal::boundary_frame(faces, owners, vertex_points, face_normals, beam_w, beam_h, boundary_twist, boundary_ups, corner_joint, through_priority);
        for (size_t k = 0; k < frame.naked.size(); k++) {
            const Vector& dir = frame.directions[k];
            if (dir.is_zero())
                continue;

            const Point& pu = vertex_points[wood_reciprocal::half_edge_start(faces, frame.naked[k])];
            const Point& pv = vertex_points[wood_reciprocal::half_edge_end(faces, frame.naked[k])];
            wood_reciprocal::BeamGeom bg = wood_reciprocal::cut_beam(pu, pv, dir, frame.ups[k], beam_w, beam_h, wood_reciprocal::unbounded(frame.cut_from[k]), wood_reciprocal::unbounded(frame.cut_to[k]));
            wood_reciprocal::store_beam(bg, boundary_beams, boundary_side0, boundary_side1, boundary_beam_bottom, boundary_beam_top);
        }
        size_t ne = ekeys.size();
        std::vector<Line> axes(ne);
        std::vector<Vector> ups(ne, Vector(0, 0, 1));
        std::vector<bool> interior(ne, false);
        std::map<size_t, std::vector<size_t>> edges_at;  // vertex key → the edges meeting there
        for (size_t ei = 0; ei < ne; ei++) {
            std::pair<size_t, size_t> key = wood_reciprocal::edge_key(ekeys[ei].first, ekeys[ei].second);
            std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>::const_iterator own = owners.find(key);
            interior[ei] = own != owners.end() && own->second.size() > 1;
            Vector normal(0, 0, 0);
            if (own != owners.end())
                for (const auto& [fi, fj] : own->second)
                    normal += face_normals[fi];

            if (!normal.is_zero())
                ups[ei] = normal.normalized();

            const Point& pu = vertex_points.at(ekeys[ei].first);
            const Point& pv = vertex_points.at(ekeys[ei].second);
            Point mid = Point::mid_point(pu, pv);
            Vector edge = pv - pu;
            if (edge.is_zero()) {
                axes[ei] = Line::from_points(pu, pv);
                continue;
            }

            Vector dir = wood_reciprocal::rotated_about(edge.normalized(), ups[ei], angle);
            double half = edge.magnitude() * 0.5 * scale;
            axes[ei] = Line::from_points(mid - dir * half, mid + dir * half);
            edges_at[ekeys[ei].first].push_back(ei);
            edges_at[ekeys[ei].second].push_back(ei);
        }
        for (size_t ei = 0; ei < ne; ei++) {
            const Line& axis = axes[ei];
            const Vector& up = ups[ei];
            Vector dir = axis.to_direction();
            if (dir.is_zero()) {
                wood_reciprocal::BeamGeom bg = wood_reciprocal::cut_beam(axis.start(), axis.end(), Vector(1, 0, 0), up, beam_w, beam_h, {}, {});
                wood_reciprocal::store_beam(bg, beams, side0, side1, beam_bottom, beam_top);
                beam_dirs.push_back({0.0, 0.0, 0.0});
                beam_ups.push_back({up[0], up[1], up[2]});
                continue;
            }

            std::vector<wood_reciprocal::CutFace> cuts_start = wood_reciprocal::unbounded(Plane::from_point_normal(axis.start(), dir));
            std::vector<wood_reciprocal::CutFace> cuts_end   = wood_reciprocal::unbounded(Plane::from_point_normal(axis.end(), dir));
            if (interior[ei]) {
                cuts_start = end_cuts(ei, ekeys[ei].first,  vertex_points.at(ekeys[ei].first),  axes, ups, interior, edges_at, frame, beam_w, face_offset, axis.start(), dir);
                cuts_end   = end_cuts(ei, ekeys[ei].second, vertex_points.at(ekeys[ei].second), axes, ups, interior, edges_at, frame, beam_w, face_offset, axis.end(),   dir);
            }

            wood_reciprocal::BeamGeom bg = wood_reciprocal::cut_beam(axis.start() - dir * extend, axis.end() + dir * extend, dir, up, beam_w, beam_h, cuts_start, cuts_end);
            wood_reciprocal::store_beam(bg, beams, side0, side1, beam_bottom, beam_top);
            beam_dirs.push_back({dir[0], dir[1], dir[2]});
            beam_ups.push_back({up[0],   up[1],   up[2]});
        }
    }

    /// The faces one end of the beam on edge ei can stop at, at the vertex the end swings past: the near side face of every other interior beam meeting there, bounded to that beam's length and to the far side of the vertex, since a neighbour whose axis crosses ours before the vertex is a beam resting on us, not one we run into; and the frame beams' inner faces at a boundary vertex; a flat cap at the axis end when there is none.
    static std::vector<wood_reciprocal::CutFace> end_cuts(size_t ei, size_t vertex, const Point& vertex_point,
                                                          const std::vector<Line>& axes, const std::vector<Vector>& ups, const std::vector<bool>& interior,
                                                          const std::map<size_t, std::vector<size_t>>& edges_at,
                                                          const wood_reciprocal::BoundaryFrame& frame,
                                                          double beam_w, double face_offset, const Point& axis_end, const Vector& dir)
    {

        std::vector<wood_reciprocal::CutFace> cuts;
        Point our_mid = axes[ei].center();
        Vector outward = axis_end - our_mid;
        outward = outward.is_zero() ? dir : outward.normalized();
        Plane beyond_vertex = Plane::from_point_normal(vertex_point, outward);
        std::map<size_t, std::vector<size_t>>::const_iterator around = edges_at.find(vertex);
        if (around != edges_at.end())
            for (size_t ej : around->second) {
                if (ej == ei || !interior[ej])
                    continue;

                Vector dir_j = axes[ej].to_direction();
                if (dir_j.is_zero())
                    continue;

                Vector side = dir_j.cross(ups[ej]);
                if (side.is_zero())
                    continue;

                side = side.normalized();
                Point mid_j = axes[ej].center();
                if ((our_mid - mid_j).dot(side) < 0.0)
                    side = -side;  // the face of beam j that looks at our beam

                std::vector<Plane> bounds = {Plane::from_point_normal(axes[ej].start() - dir_j * beam_w, dir_j),
                                             Plane::from_point_normal(axes[ej].end() + dir_j * beam_w, -dir_j),
                                             beyond_vertex};
                cuts.push_back(wood_reciprocal::CutFace{Plane::from_point_normal(mid_j + side * face_offset, side), bounds});
            }

        std::map<size_t, std::vector<wood_reciprocal::CutFace>>::const_iterator at_frame = frame.inner_faces_at_vertex.find(vertex);
        if (at_frame != frame.inner_faces_at_vertex.end())
            for (const wood_reciprocal::CutFace& face : at_frame->second)
                if (std::abs(face.plane.z_axis().dot(dir)) >= 0.15)
                    cuts.push_back(face);

        if (cuts.empty())
            return wood_reciprocal::unbounded(Plane::from_point_normal(axis_end, dir));

        return cuts;
    }

    static Mesh make_dome(int nx, int ny, double W, double D, double h) {

        std::vector<Point> pts;
        pts.reserve((nx + 1) * (ny + 1));
        for (int j = 0; j <= ny; j++) {
            for (int i = 0; i <= nx; i++) {
                double x = W * i / nx;
                double y = D * j / ny;
                double z = h * std::sin(Tolerance::PI * x / W)
                             * std::sin(Tolerance::PI * y / D);
                pts.push_back(Point(x, y, z));
            }
        }

        std::vector<std::vector<size_t>> faces;
        faces.reserve(nx * ny);
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                faces.push_back({
                    (size_t)( j      * (nx + 1) + i    ),
                    (size_t)( j      * (nx + 1) + i + 1),
                    (size_t)((j + 1) * (nx + 1) + i + 1),
                    (size_t)((j + 1) * (nx + 1) + i    ),
                });
            }
        }

        return Mesh::from_vertices_and_faces(pts, faces);
    }
};
