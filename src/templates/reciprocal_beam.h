#pragma once
#include "session.h"
#include "reciprocal.h"
#include "intersection.h"
#include "polyline.h"
#include "tolerance.h"

#include <cmath>
#include <stdexcept>

using namespace session_cpp;

/// A reciprocal beam frame from a sinusoidal dome mesh.
///
/// Generates a rectangular box beam at each mesh edge, cutting each end
/// against the side face of its topological neighbor beam. Produces the
/// dome mesh, all beam box meshes, and the side-face outlines.
///
/// Usage:
///   ReciprocalBeam rb;                          // default 12×10 dome
///   ReciprocalBeam rb(6, 5, 6.0, 5.0, 2.0);    // smaller dome
class ReciprocalBeam {
public:
    Mesh dome_mesh;
    std::vector<Mesh>     beams;
    std::vector<Polyline> side0;   // right-face outlines (+right direction)
    std::vector<Polyline> side1;   // left-face outlines  (-right direction)

    /// Parametric sinusoidal dome constructor.
    ReciprocalBeam(int    nx                = 12,
                   int    ny                = 10,
                   double W                 = 12.0,
                   double D                 = 10.0,
                   double h                 = 3.0,
                   double angle             = 0.35,
                   double scale             = 1.4,
                   double beam_w            = 0.10,
                   double extend_factor     = 5.0,
                   double cut_offset_factor = 1.0)
    {
        if (nx < 1 || ny < 1)
            throw std::invalid_argument("ReciprocalBeam: nx and ny must be >= 1");
        dome_mesh = make_dome(nx, ny, W, D, h);
        _build(dome_mesh, nx, angle, scale, beam_w, extend_factor, cut_offset_factor);
    }

    /// External mesh constructor — use any quad mesh as the base.
    /// v-edge stagger is disabled (pass nx > 0 explicitly if needed).
    explicit ReciprocalBeam(Mesh ext_mesh,
                            double angle             = 0.35,
                            double scale             = 1.4,
                            double beam_w            = 0.10,
                            double extend_factor     = 5.0,
                            double cut_offset_factor = 1.0)
    {
        dome_mesh = std::move(ext_mesh);
        _build(dome_mesh, -1, angle, scale, beam_w, extend_factor, cut_offset_factor);
    }

private:
    void _build(const Mesh& m, int nx_stagger,
                double angle, double scale, double beam_w,
                double extend_factor, double cut_offset_factor)
    {
        if (beam_w <= 0.0)
            throw std::invalid_argument("ReciprocalBeam: beam_w must be positive");

        const double beam_h     = beam_w * 2.0;
        const double extend     = beam_w * extend_factor;
        const double cut_offset = beam_w * cut_offset_factor;

        auto r = Reciprocal::from_mesh(m, angle, scale, true, beam_h);

        auto ekeys = m.edges();
        for (int ei = 0; ei < (int)r.center.size(); ei++) {
            auto [u, v]    = ekeys[ei];
            bool is_v_edge = (nx_stagger > 0) &&
                             ((size_t)std::abs((long long)u - (long long)v)
                              == (size_t)(nx_stagger + 1));

            Line          ln  = r.center[ei];
            const Vector& up  = r.lineplanes[ei].y_axis();
            if (is_v_edge)
                ln += up * (beam_h * 0.5);
            const Vector dir = ln.to_direction();

            Plane ps = side_cut_plane(
                r.endplanes[ei][0], ln.start(), dir, true,  beam_w, cut_offset);
            Plane pe = side_cut_plane(
                r.endplanes[ei][1], ln.end(),   dir, false, beam_w, cut_offset);

            auto bg = make_beam(ln, up, beam_w, beam_h, extend, ps, pe);

            beams.push_back(std::move(bg.mesh));

            std::vector<Point> s0 = bg.side0; s0.push_back(s0[0]);
            side0.emplace_back(s0);

            std::vector<Point> s1 = bg.side1; s1.push_back(s1[0]);
            side1.emplace_back(s1);
        }
    }
    struct BeamGeom {
        Mesh               mesh;
        std::vector<Point> side0;
        std::vector<Point> side1;
    };

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
        for (int j = 0; j < ny; j++)
            for (int i = 0; i < nx; i++)
                faces.push_back({
                    (size_t)( j      * (nx + 1) + i    ),
                    (size_t)( j      * (nx + 1) + i + 1),
                    (size_t)((j + 1) * (nx + 1) + i + 1),
                    (size_t)((j + 1) * (nx + 1) + i    ),
                });
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
        if (right_nb.is_zero())
            right_nb = endplane.x_axis().cross(Vector(0, 0, 1)).normalized();

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

    static BeamGeom make_beam(const Line& line, const Vector& up,
                               double w, double h, double extend,
                               const Plane& cut_s, const Plane& cut_e)
    {
        Vector dir   = line.to_direction();
        Vector right = dir.cross(up);
        if (right.is_zero())
            right = dir.cross(Vector(0, 0, 1));
        right = right.normalized() * (w * 0.5);
        Vector n = up * (h * 0.5);

        Point s = Point(line.start()[0] - extend * dir[0],
                        line.start()[1] - extend * dir[1],
                        line.start()[2] - extend * dir[2]);
        Point e = Point(line.end()[0]   + extend * dir[0],
                        line.end()[1]   + extend * dir[1],
                        line.end()[2]   + extend * dir[2]);

        auto corner = [](const Point& p, const Vector& r, int sr,
                         const Vector& nn, int sn) {
            return Point(p[0] + sr*r[0] + sn*nn[0],
                         p[1] + sr*r[1] + sn*nn[1],
                         p[2] + sr*r[2] + sn*nn[2]);
        };
        auto cut = [&](const Point& p, const Plane& pl) -> Point {
            Point pt;
            Line ray = Line::from_points(
                p, Point(p[0]+dir[0], p[1]+dir[1], p[2]+dir[2]));
            if (Intersection::line_plane(ray, pl, pt, false))
                return pt;
            return p;
        };

        std::array<Point, 4> sc = {
            cut(corner(s, right, -1, n, -1), cut_s),
            cut(corner(s, right, +1, n, -1), cut_s),
            cut(corner(s, right, +1, n, +1), cut_s),
            cut(corner(s, right, -1, n, +1), cut_s),
        };
        std::array<Point, 4> ec = {
            cut(corner(e, right, -1, n, -1), cut_e),
            cut(corner(e, right, +1, n, -1), cut_e),
            cut(corner(e, right, +1, n, +1), cut_e),
            cut(corner(e, right, -1, n, +1), cut_e),
        };

        std::vector<Point> pts = {
            sc[0], sc[1], sc[2], sc[3],
            ec[0], ec[1], ec[2], ec[3],
        };
        std::vector<std::vector<size_t>> faces = {
            {0, 1, 2, 3},  // start cap
            {4, 7, 6, 5},  // end cap
            {0, 4, 5, 1},  // bottom
            {1, 5, 6, 2},  // right
            {2, 6, 7, 3},  // top
            {3, 7, 4, 0},  // left
        };

        BeamGeom bg;
        bg.mesh  = Mesh::from_vertices_and_faces(pts, faces);
        bg.side0 = {sc[1], sc[2], ec[2], ec[1]};  // right face (+right)
        bg.side1 = {sc[3], sc[0], ec[0], ec[3]};  // left  face (-right)
        return bg;
    }
};
