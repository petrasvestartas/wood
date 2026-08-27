#pragma once
#include "session.h"
#include "wood_session.h"

#include <cmath>
#include <stdexcept>

using namespace session_cpp;
using namespace wood_session;

/// A reflex-fold shell from two polylines with thickness.
///
/// Folds a profile along a cross_section by projecting each profile row
/// onto the perpendicular bisector plane at each cross-section point,
/// producing a quad mesh. Each quad face is then offset into a top/bottom
/// plate pair (WoodElement) using miter offsets.
///
/// chamfer_bot / chamfer_top  — miter offset distances on the two faces.
/// chamfer_angle              — corners with interior angle < this (degrees)
///                              receive the miter offset; others are kept sharp.
///
/// Usage:
///   ReflexFold rf;
///   ReflexFold rf(my_cross_section, my_profile, 10.0, 20.0, 20.0, 45.0);
class ReflexFold {
public:
    Mesh mesh;
    std::vector<WoodElement> elements;

    ReflexFold(const Polyline& cross_section = default_cross_section(),
               const Polyline& profile       = default_profile(),
               double thickness     = 10.0,
               double chamfer_bot   = 20.0,
               double chamfer_top   = 20.0,
               double chamfer_angle = 180.0)
    {
        if (cross_section.point_count() < 2) {
            throw std::invalid_argument("ReflexFold: cross_section must have at least 2 points");
        }
        if (profile.point_count() < 2) {
            throw std::invalid_argument("ReflexFold: profile must have at least 2 points");
        }
        if (thickness == 0.0) {
            throw std::invalid_argument("ReflexFold: thickness must not be zero");
        }

        mesh = reflex_fold(cross_section, profile);

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
            // chamfer_top was accepted by the constructor and then silently
            // ignored - both contours used chamfer_bot, so distinct values
            // produced wrong top-plate geometry with no error.
            std::vector<Point> top_ch = chamfer_apply(top_raw, chamfer_top, mask);
            std::vector<Point> bot_ch = chamfer_apply(bot_raw, chamfer_bot, mask);

            if (!bot_ch.empty()) {
                bot_ch.push_back(bot_ch[0]);
            }
            if (!top_ch.empty()) {
                top_ch.push_back(top_ch[0]);
            }
            elements.emplace_back(Polyline(bot_ch), Polyline(top_ch));
        }
    }

    static Polyline default_cross_section() {
        return Polyline(std::vector<Point>{
            Point(   0.0,        0.0, 0.0        ),
            Point( 232.466867,   0.0, 578.230273 ),
            Point( 966.431048,   0.0, 738.362403 ),
            Point(1714.771039,   0.0, 604.197646 ),
            Point(2037.199246,   0.0,   4.784134 ),
        });
    }

    static Polyline default_profile() {
        return Polyline(std::vector<Point>{
            Point(   0.0,         0.0,          0.0),
            Point( -77.091582,  -89.779688,     0.0),
            Point( -19.195901, -179.559376,     0.0),
            Point( -89.503039, -269.339064,     0.0),
            Point( -25.912208, -359.118752,     0.0),
            Point( -89.664508, -448.898440,     0.0),
            Point( -19.472054, -538.678128,     0.0),
            Point( -77.485143, -628.457816,     0.0),
            Point(  -0.817868, -718.237504,     0.0),
            Point( -54.785307, -808.017192,     0.0),
            Point(  26.377605, -897.796881,     0.0),
            Point( -25.921340, -987.576569,     0.0),
            Point(  55.558217,-1077.356257,     0.0),
            Point(   1.964204,-1167.135945,     0.0),
            Point(  79.167116,-1256.915633,     0.0),
            Point(  21.364954,-1346.695321,     0.0),
            Point(  91.462653,-1436.475009,     0.0),
            Point(  27.192231,-1526.254697,     0.0),
            Point(  89.830774,-1616.034385,     0.0),
        });
    }

private:
    static std::vector<bool> chamfer_mask(const std::vector<Point>& pts, double max_angle_deg) {
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
            double cosA = std::max(-1.0, std::min(1.0, (dpx*dnx+dpy*dny+dpz*dnz)/(lp*ln)));
            mask[i] = (std::acos(cosA) * TO_DEG < max_angle_deg);
        }
        return mask;
    }

    static std::vector<Point> chamfer_apply(const std::vector<Point>& pts, double s,
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

    /// Projects each profile row onto the bisector plane at each cross-section
    /// point and accumulates a quad mesh.
    ///
    /// Ported from session_cpp Mesh::reflex_fold (mesh.cpp).
    static Mesh reflex_fold(const Polyline& cross_section, const Polyline& profile) {
        size_t nCS = cross_section.point_count();
        size_t nP  = profile.point_count();

        // Plane at each cross-section point: normal bisects neighbouring edges
        // (Z-up fallback for endpoints).
        std::vector<Plane> planes;
        planes.reserve(nCS);
        for (size_t i = 0; i < nCS; ++i) {
            Vector normal(0, 0, 1);
            if (i > 0 && i < nCS - 1) {
                Point ci = cross_section[i];
                Point cp = cross_section[i - 1];
                Point cn = cross_section[i + 1];
                Vector v1(cp[0]-ci[0], cp[1]-ci[1], cp[2]-ci[2]);
                Vector v2(cn[0]-ci[0], cn[1]-ci[1], cn[2]-ci[2]);
                v1 = v1.normalized();
                v2 = v2.normalized();
                normal = Vector(v1[0]+v2[0], v1[1]+v2[1], v1[2]+v2[2]);
                if (!normal.normalize_self()) {
                    // Collinear neighbours: v1 ~ -v2, the bisector sum is
                    // ~zero and the old code used the zero normal anyway -
                    // a degenerate plane whose downstream dot-product guard
                    // forced t=0 and silently duplicated the previous
                    // profile row into zero-area quads. Use the edge
                    // direction as the section normal instead.
                    normal = Vector(cn[0]-cp[0], cn[1]-cp[1], cn[2]-cp[2]);
                    normal.normalize_self();
                }
            }
            Point origin = cross_section[i];
            planes.push_back(Plane::from_point_normal(origin, normal));
        }

        // First row = profile points as-is.
        std::vector<Point> all_pts;
        all_pts.reserve(nCS * nP);
        for (size_t j = 0; j < nP; ++j) {
            all_pts.push_back(profile[j]);
        }

        // Each subsequent cross-section step slides each profile point along
        // the edge direction until it lies on the bisector plane.
        std::vector<std::vector<size_t>> faces;
        for (size_t i = 1; i < nCS; ++i) {
            const Point& po = planes[i].origin();
            const Point& pp = planes[i - 1].origin();
            Vector n1(po[0]-pp[0], po[1]-pp[1], po[2]-pp[2]);
            const Vector& n2 = planes[i].z_axis();

            size_t row_start = all_pts.size();
            for (size_t j = 0; j < nP; ++j) {
                const Point& pvrt = all_pts[row_start - nP + j];
                Vector diff(po[0]-pvrt[0], po[1]-pvrt[1], po[2]-pvrt[2]);
                double denom = n2.dot(n1);
                double t = (std::abs(denom) > 1e-12) ? n2.dot(diff) / denom : 0.0;
                all_pts.push_back(Point(pvrt[0]+n1[0]*t, pvrt[1]+n1[1]*t, pvrt[2]+n1[2]*t));
            }
            for (size_t j = 0; j + 1 < nP; ++j) {
                size_t new_j = row_start + j;
                size_t old_j = row_start - nP + j;
                faces.push_back({new_j, old_j, old_j+1, new_j+1});
            }
        }
        return Mesh::from_vertices_and_faces(all_pts, faces);
    }
};
