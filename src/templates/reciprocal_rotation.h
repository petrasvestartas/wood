#pragma once
#include "session.h"
#include "reciprocal.h"
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

/// A reciprocal beam frame from a sinusoidal dome mesh (rotation-based nexorade).
///
/// Generates a rectangular box beam at each mesh edge, cutting each end
/// against the side face of its topological neighbor beam. Produces the
/// dome mesh, all beam box meshes, and the side-face outlines.
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
                       const std::map<wood_reciprocal::EdgeKey, Vector>& boundary_ups = {},
                       wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Mitre,
                       const std::map<wood_reciprocal::EdgeKey, int>& through_priority = {})
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
                       const std::map<wood_reciprocal::EdgeKey, Vector>& boundary_ups = {},
                       wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Mitre,
                       const std::map<wood_reciprocal::EdgeKey, int>& through_priority = {})
    {
        dome_mesh = std::move(ext_mesh);
        _build(dome_mesh, -1, angle, scale, beam_w, beam_h, extend_factor, cut_offset_factor, boundary_twist, boundary_ups, corner_joint, through_priority);
    }

private:
    using BeamGeom = wood_reciprocal::BeamGeom;
    using EdgeKey = wood_reciprocal::EdgeKey;
    using EdgeOwners = wood_reciprocal::EdgeOwners;

    void _build(const Mesh& m, int /*nx_stagger*/,
                double angle, double scale, double beam_w, double beam_h,
                double extend_factor, double cut_offset_factor,
                wood_reciprocal::BoundaryTwist boundary_twist,
                const std::map<wood_reciprocal::EdgeKey, Vector>& boundary_ups,
                wood_reciprocal::CornerJoint corner_joint,
                const std::map<wood_reciprocal::EdgeKey, int>& through_priority)
    {

        if (beam_w <= 0.0) {
            throw std::invalid_argument("ReciprocalRotation: beam_w must be positive");
        }

        if (beam_h <= 0.0) beam_h = beam_w * 2.0;
        const double extend     = beam_w * extend_factor;
        const double cut_offset = beam_w * cut_offset_factor;

        Reciprocal::Result r = Reciprocal::from_mesh(m, angle, scale, true, beam_h);

        // ── topology by vertex key, the same keys r.center is indexed by through m.edges() ──
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

        EdgeOwners owners = wood_reciprocal::edge_owners(faces);
        std::vector<std::pair<size_t,size_t>> ekeys = m.edges();

        // ── boundary beams: a straight beam on every naked edge, none of the reciprocal treatment ──
        // Axis = the mesh edge, section centred on the axis like the interior beams, up transported around
        // the loop by reflection in the mitres so consecutive prisms share their mitred end face; a warped loop's
        // closure twist goes where boundary_twist says. Consecutive beams are mitred with the bisector plane at their shared vertex.
        wood_reciprocal::BoundaryFrame frame = wood_reciprocal::boundary_frame(faces, owners, vertex_points, face_normals, beam_w, beam_h, boundary_twist, boundary_ups, corner_joint, through_priority);
        for (size_t k = 0; k < frame.naked.size(); k++) {
            const Vector& dir = frame.directions[k];
            if (dir.is_zero())
                continue;

            const Point& pu = vertex_points[wood_reciprocal::half_edge_start(faces, frame.naked[k])];
            const Point& pv = vertex_points[wood_reciprocal::half_edge_end(faces, frame.naked[k])];
            BeamGeom bg = wood_reciprocal::cut_beam(pu, pv, dir, frame.ups[k], beam_w, beam_h, wood_reciprocal::unbounded(frame.cut_from[k]), wood_reciprocal::unbounded(frame.cut_to[k]));
            wood_reciprocal::store_beam(bg, boundary_beams, boundary_side0, boundary_side1, boundary_beam_bottom, boundary_beam_top);
        }

        // ── one beam per mesh edge, in m.edges() order; an interior beam's end that reaches a boundary vertex stops at the inner faces of the frame beams meeting there ──
        for (int ei = 0; ei < (int)r.center.size(); ei++) {
            Line          ln  = r.center[ei];
            const Vector& up  = r.lineplanes[ei].y_axis();
            const Vector dir = ln.to_direction();

            Plane ps = side_cut_plane(
                r.endplanes[ei][0], ln.start(), dir, true,  beam_w, cut_offset);
            Plane pe = side_cut_plane(
                r.endplanes[ei][1], ln.end(),   dir, false, beam_w, cut_offset);
            std::vector<wood_reciprocal::CutFace> cuts_start = wood_reciprocal::unbounded(ps);
            std::vector<wood_reciprocal::CutFace> cuts_end   = wood_reciprocal::unbounded(pe);

            EdgeKey key = wood_reciprocal::edge_key(ekeys[ei].first, ekeys[ei].second);
            EdgeOwners::const_iterator own = owners.find(key);
            if (own != owners.end() && own->second.size() > 1) {
                size_t start_vertex = ln.start().distance(vertex_points[key.first]) <= ln.start().distance(vertex_points[key.second]) ? key.first : key.second;
                size_t end_vertex   = start_vertex == key.first ? key.second : key.first;
                cuts_start = wood_reciprocal::boundary_cuts_or(frame, start_vertex, dir, ps);
                cuts_end   = wood_reciprocal::boundary_cuts_or(frame, end_vertex,   dir, pe);
            }

            BeamGeom bg = wood_reciprocal::cut_beam(ln.start() - dir * extend, ln.end() + dir * extend, dir, up, beam_w, beam_h, cuts_start, cuts_end);
            wood_reciprocal::store_beam(bg, beams, side0, side1, beam_bottom, beam_top);
            beam_dirs.push_back({dir[0], dir[1], dir[2]});
            beam_ups.push_back({up[0],   up[1],   up[2]});
        }
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

    static Plane side_cut_plane(
        const Plane&  endplane,
        const Point&  endpoint,
        const Vector& dir_i,
        bool          is_start,
        double        beam_w,
        double        cut_offset = 0.0)
    {

        Vector right_nb = endplane.z_axis().normalized();
        if (right_nb.is_zero()) {
            right_nb = endplane.x_axis().cross(Vector(0, 0, 1)).normalized();
        }

        double s    = is_start ? 1.0 : -1.0;
        double side = s * (dir_i[0]*right_nb[0]
                         + dir_i[1]*right_nb[1]
                         + dir_i[2]*right_nb[2]);
        double half = beam_w * 0.5 + cut_offset;

        Point  face_pt;
        Vector face_normal;
        if (side > 0.0) {
            face_pt     = Point(endpoint[0] - right_nb[0]*half,
                                endpoint[1] - right_nb[1]*half,
                                endpoint[2] - right_nb[2]*half);
            face_normal = Vector(-right_nb[0], -right_nb[1], -right_nb[2]);
        } else {
            face_pt     = Point(endpoint[0] + right_nb[0]*half,
                                endpoint[1] + right_nb[1]*half,
                                endpoint[2] + right_nb[2]*half);
            face_normal = right_nb;
        }

        return Plane::from_point_normal(face_pt, face_normal);
    }
};
