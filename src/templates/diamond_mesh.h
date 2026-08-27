#pragma once
#include "session.h"
#include "nurbssurface.h"
#include "wood_session.h"

#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

using namespace session_cpp;
using namespace wood_session;

/// Diamond-pattern triangular mesh from a NURBS surface, with plate elements.
///
/// Subdivides a NURBS surface into a diamond (rhombus) pattern by
/// placing triangle pairs that share alternating edge-midpoint vertices.
/// Each cell produces 6 triangles (first row) or 4 triangles (other rows).
/// Result mesh is welded to merge coincident vertices.
/// After building the mesh, miter_contours generates a WoodElement (bottom +
/// top polyline) for every triangular face. Chamfer is applied to any corner
/// whose edge-vector angle is less than chamfer_angle degrees.
///
/// Usage:
///   DiamondMesh dm;                                       // defaults
///   DiamondMesh dm(my_surface, 8, 4, 15.0, 2.0, 90.0);  // custom
class DiamondMesh {
public:
    Mesh mesh;
    std::vector<WoodElement> elements;

    DiamondMesh(NurbsSurface surface = default_surface(),
                int u_div = 8,
                int v_div = 4,
                double thickness     = 10.0,
                double chamfer       = 1.0,
                double chamfer_angle = 180.0)
    {
        if (u_div < 1) {
            u_div = 1;
        }
        if (v_div < 1) {
            v_div = 1;
        }

        std::pair<double,double> dom_u = surface.domain(0);
        double u0 = dom_u.first, u1 = dom_u.second;
        std::pair<double,double> dom_v = surface.domain(1);
        double v0 = dom_v.first, v1 = dom_v.second;
        double su = (u1 - u0) / u_div;
        double sv = (v1 - v0) / v_div;

        std::vector<Point> pts;
        std::vector<std::vector<size_t>> faces;
        size_t idx = 0;

        std::function<void(const Point&, const Point&, const Point&)> add_tri = [&](const Point& a, const Point& b, const Point& c) {
            pts.push_back(a); pts.push_back(b); pts.push_back(c);
            faces.push_back({idx, idx + 1, idx + 2});
            idx += 3;
        };

        for (int i = 0; i < u_div; i++) {
            // u = u0 + i*su, not u += su: the accumulated form drifts by one
            // rounding step per cell across the surface.
            double u = u0 + i * su;
            for (int j = 0; j < v_div; j++) {
                double v = v0 + j * sv;
                Point p0 = surface.point_at(u,          v);
                Point p1 = surface.point_at(u + su,     v);
                Point p2 = surface.point_at(u,          v + sv);
                Point p3 = surface.point_at(u + su,     v + sv);
                Point p5 = surface.point_at(u + su*0.5, v + sv*0.5);
                // Interior rows use p4 = next row's midpoint (in-domain for
                // j < v_div-1). Boundary rows previously sampled half a cell
                // OUTSIDE the domain (v - sv*0.5 / v + sv*1.5 - point_at
                // extrapolates, it does not clamp) and averaged with p5, so
                // border plates missed the true surface edge by
                // O(curvature*sv^2) - tens of mm at default divisions.
                // Evaluate the boundary parameter directly instead.
                Point p4 = (j != v_div - 1)
                    ? surface.point_at(u + su*0.5, v + sv*1.5)
                    : Point(0, 0, 0);   // unused on the last row
                Point p7 = surface.point_at(u + su*0.5, v0);
                Point p8 = surface.point_at(u + su*0.5, v1);

                if (j == 0) {
                    add_tri(p1, p5, p7);
                    add_tri(p7, p5, p0);
                }

                add_tri(p1, p3, p5);
                add_tri(p0, p5, p2);

                if (j != v_div - 1) {
                    add_tri(p3, p4, p5);
                    add_tri(p4, p2, p5);
                } else {
                    add_tri(p3, p8, p5);
                    add_tri(p8, p2, p5);
                }
            }
        }

        // Weld tolerance is a DISTANCE in model units. 3.14159 was pi
        // pasted into a millimetre slot: 300x the 0.01 the sibling templates
        // use, and enough to fuse genuinely distinct vertices once cells
        // shrink below ~6 mm. The vertices to merge differ only by floating
        // point rounding, so the small tolerance is the correct one.
        mesh = Mesh::from_vertices_and_faces(pts, faces).weld(0.01);

        // Generate WoodElement plates via miter_contours.
        // chamfer_mask/chamfer_apply are applied after to maintain equal
        // point counts in both top and bottom contours.
        using MiterTuple = std::tuple<std::vector<Point>, std::vector<Point>,
                                      std::vector<Point>, std::vector<Point>, Vector>;
        for (const MiterTuple& plate :
                Mesh::miter_contours(mesh, thickness, 0.0, 0.0, false)) {
            const std::vector<Point>& top_raw = std::get<2>(plate);
            const std::vector<Point>& bot_raw = std::get<3>(plate);
            if (top_raw.empty() || bot_raw.empty()) {
                continue;
            }

            std::vector<bool>  mask   = chamfer_mask(bot_raw, chamfer_angle);
            std::vector<Point> top_ch = chamfer_apply(top_raw, chamfer, mask);
            std::vector<Point> bot_ch = chamfer_apply(bot_raw, chamfer, mask);

            if (!bot_ch.empty()) {
                bot_ch.push_back(bot_ch[0]);
            }
            if (!top_ch.empty()) {
                top_ch.push_back(top_ch[0]);
            }
            elements.emplace_back(Polyline(bot_ch), Polyline(top_ch));
        }
    }

    /// Degree-3 bicubic arch surface: 3000×5000 mm, height 1500 mm (arc extrusion).
    static NurbsSurface default_surface() {
        const double W = 3000.0, L = 5000.0, H = 1500.0;
        NurbsSurface srf;
        srf.create_raw(3, false, 4, 4, 4, 4);
        const double ku[] = {0.0, 0.0, 0.0, W, W, W};
        const double kv[] = {0.0, 0.0, 0.0, L, L, L};
        for (int i = 0; i < 6; i++) {
            srf.set_nurbsknot(0, i, ku[i]);
            srf.set_nurbsknot(1, i, kv[i]);
        }
        const double us[] = {0.0, W/3.0, 2.0*W/3.0, W};
        const double zs[] = {0.0, H*4.0/3.0, H*4.0/3.0, 0.0};
        const double vs[] = {0.0, L/3.0, 2.0*L/3.0, L};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                srf.set_cv(i, j, Point(us[i], vs[j], zs[i]));
            }
        }
        srf.transpose();   // arch along V so u_div runs across the long span
        return srf;
    }

private:
    static std::vector<bool> chamfer_mask(const std::vector<Point>& pts,
                                           double max_angle_deg) {
        size_t n = pts.size();
        std::vector<bool> mask(n, false);
        constexpr double TO_DEG = 180.0 / 3.14159265358979323846;
        for (size_t i = 0; i < n; ++i) {
            size_t prev = (i + n - 1) % n;
            size_t next = (i + 1) % n;
            double dpx = pts[prev][0]-pts[i][0], dpy = pts[prev][1]-pts[i][1], dpz = pts[prev][2]-pts[i][2];
            double dnx = pts[next][0]-pts[i][0], dny = pts[next][1]-pts[i][1], dnz = pts[next][2]-pts[i][2];
            double lp = std::sqrt(dpx*dpx + dpy*dpy + dpz*dpz);
            double ln = std::sqrt(dnx*dnx + dny*dny + dnz*dnz);
            if (lp < 1e-12 || ln < 1e-12) {
                continue;
            }
            double cosA = std::max(-1.0, std::min(1.0,
                (dpx*dnx+dpy*dny+dpz*dnz)/(lp*ln)));
            mask[i] = (std::acos(cosA) * TO_DEG < max_angle_deg);
        }
        return mask;
    }

    static std::vector<Point> chamfer_apply(const std::vector<Point>& pts,
                                             double s,
                                             const std::vector<bool>& mask) {
        size_t n = pts.size();
        if (s <= 0.0) {
            return pts;
        }
        double min_edge = std::numeric_limits<double>::max();
        for (size_t i = 0; i < n; ++i) {
            size_t j = (i + 1) % n;
            double dx = pts[j][0]-pts[i][0], dy = pts[j][1]-pts[i][1], dz = pts[j][2]-pts[i][2];
            min_edge = std::min(min_edge, std::sqrt(dx*dx+dy*dy+dz*dz));
        }
        double sc = std::min(s, min_edge / 3.0);
        std::vector<Point> result;
        result.reserve(2 * n);
        for (size_t i = 0; i < n; ++i) {
            size_t prev = (i + n - 1) % n;
            size_t next = (i + 1) % n;
            double dpx = pts[prev][0]-pts[i][0], dpy = pts[prev][1]-pts[i][1], dpz = pts[prev][2]-pts[i][2];
            double dnx = pts[next][0]-pts[i][0], dny = pts[next][1]-pts[i][1], dnz = pts[next][2]-pts[i][2];
            double lp = std::sqrt(dpx*dpx+dpy*dpy+dpz*dpz);
            double ln = std::sqrt(dnx*dnx+dny*dny+dnz*dnz);
            if (mask[i]) {
                double sp = (lp > 1e-12) ? sc/lp : 0.0;
                double sn = (ln > 1e-12) ? sc/ln : 0.0;
                result.push_back(Point(pts[i][0]+dpx*sp, pts[i][1]+dpy*sp, pts[i][2]+dpz*sp));
                result.push_back(Point(pts[i][0]+dnx*sn, pts[i][1]+dny*sn, pts[i][2]+dnz*sn));
            } else {
                result.push_back(pts[i]);
            }
        }
        return result;
    }
};
