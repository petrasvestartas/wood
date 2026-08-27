#pragma once
#include "session.h"
#include "wood_session.h"

#include <cmath>
#include <limits>
#include <stdexcept>

using namespace session_cpp;
using namespace wood_session;

/// A translation shell from two polylines with thickness.
///
/// Sweeps a cross_section curve along a profile by accumulating the
/// displacement steps of the profile, producing a quad mesh. Each quad face
/// is then offset into a top/bottom plate pair (WoodElement) using miter chamfers.
///
/// chamfer_angle controls which corners are chamfered (interior angle < chamfer_angle
/// in degrees). The same mask is applied to both top and bottom contours so that
/// both always have the same number of points regardless of geometry.
///
/// Usage:
///   TranslationShell ts;
///   TranslationShell ts(my_cross_section, my_profile, 15.0, 2.0, 8.0, 90.0);
class TranslationShell {
public:
    Mesh mesh;
    std::vector<WoodElement> elements;

    TranslationShell(const Polyline& cross_section = default_cross_section(),
                     const Polyline& profile        = default_profile(),
                     double thickness      = 10.0,
                     double chamfer        = 1.0,
                     double chamfer_angle  = 180.0)
    {
        if (cross_section.point_count() < 2) {
            throw std::invalid_argument("TranslationShell: cross_section must have at least 2 points");
        }
        if (profile.point_count() < 2) {
            throw std::invalid_argument("TranslationShell: profile must have at least 2 points");
        }
        if (thickness == 0.0) {
            throw std::invalid_argument("TranslationShell: thickness must not be zero");
        }

        mesh = sweep(cross_section, profile);

        // Get raw miter contours (no internal chamfering: pass 0,0 for distances).
        // Mask is computed once from bot_raw, then applied to both top and bot
        // so both always have the same number of points.
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

    static Polyline default_cross_section() {
        return Polyline(std::vector<Point>{
            Point(1343.472686,   0.0,   0.0      ),
            Point(1237.791431,   0.0, 121.964275 ),
            Point(1117.698859,   0.0, 229.686024 ),
            Point( 981.901426,   0.0, 316.618767 ),
            Point( 831.471484,   0.0, 374.347649 ),
            Point( 671.736343,   0.0, 394.768581 ),
            Point( 512.001202,   0.0, 374.347649 ),
            Point( 361.571259,   0.0, 316.618767 ),
            Point( 225.773829,   0.0, 229.686026 ),
            Point( 105.681255,   0.0, 121.964275 ),
            Point(   0.0,        0.0,   0.0      ),
        });
    }

    static Polyline default_profile() {
        return Polyline(std::vector<Point>{
            Point(0.0,    0.0,          0.0       ),
            Point(0.0, -154.339980,   121.873175  ),
            Point(0.0, -321.617334,   225.228387  ),
            Point(0.0, -500.643329,   306.496555  ),
            Point(0.0, -689.070490,   362.559337  ),
            Point(0.0, -883.558001,   391.165838  ),
            Point(0.0, -1080.134636,  391.165838  ),
            Point(0.0, -1274.622136,  362.559339  ),
            Point(0.0, -1463.049306,  306.496555  ),
            Point(0.0, -1642.075299,  225.228388  ),
            Point(0.0, -1809.352653,  121.873177  ),
            Point(0.0, -1963.692636,    0.0       ),
        });
    }

private:
    /// Returns a mask where mask[i]=true if the interior angle at corner i
    /// is less than max_angle_deg (i.e. the corner is sharp enough to chamfer).
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

    /// Applies a chamfer of distance s at each masked corner, inserting two
    /// points per chamfered corner and one point per un-chamfered corner.
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

    static Mesh sweep(const Polyline& cross_section, const Polyline& profile) {
        size_t nC = cross_section.point_count();
        size_t nP = profile.point_count();

        std::vector<Point> all_pts;
        all_pts.reserve(nC * nP);
        for (size_t j = 0; j < nC; ++j) {
            all_pts.push_back(cross_section[j]);
        }

        std::vector<std::vector<size_t>> faces;
        for (size_t i = 1; i < nP; ++i) {
            // Closed form per row: building row i from row i-1 compounded a
            // rounding step per row; the offset from profile[0] is exact and
            // equally cheap.
            Vector off(profile[i][0]-profile[0][0],
                       profile[i][1]-profile[0][1],
                       profile[i][2]-profile[0][2]);

            size_t row = all_pts.size();
            for (size_t j = 0; j < nC; ++j) {
                const Point& base = all_pts[j];
                all_pts.push_back(Point(base[0]+off[0], base[1]+off[1], base[2]+off[2]));
            }
            for (size_t j = 0; j + 1 < nC; ++j) {
                size_t new_j  = row + j;
                size_t old_j  = row - nC + j;
                faces.push_back({new_j, old_j, old_j+1, new_j+1});
            }
        }
        return Mesh::from_vertices_and_faces(all_pts, faces);
    }
};
