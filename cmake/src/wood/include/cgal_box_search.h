#pragma once
#include "../../../ext/session_cpp/src/point.h"
#include "../../../ext/session_cpp/src/bvh.h"

// ToDo
// What happens when 3 beams meet in one node when only two joints are needed, maybe move ends apart
// from each other or give point id Check edge normals

namespace cgal
{
namespace box_search
{

inline double
remap (const double &value, const double &from1, const double &to1, const double &from2, const double &to2)
{
    return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
}

inline double
length (double x, double y, double z)
{
    double len;
    x = fabs (x);
    y = fabs (y);
    z = fabs (z);
    if (y >= x && y >= z)
        {
            len = x;
            x = y;
            y = len;
        }
    else if (z >= x && z >= y)
        {
            len = x;
            x = z;
            z = len;
        }

    if (x > ON_DBL_MIN)
        {
            y /= x;
            z /= x;
            len = x * sqrt (1.0 + y * y + z * z);
        }
    else if (x > 0.0 && ON_IS_FINITE (x))
        len = x;
    else
        len = 0.0;

    return len;
}

// ── Helper: project 3D point onto infinite line (segment used as line direction) ──
inline session_cpp::Point
project_point_to_line_3d (const session_cpp::Point &pt, const session_cpp::Line &seg)
{
    session_cpp::Vector d = seg.end () - seg.start ();
    session_cpp::Vector dp (pt[0] - seg.start ()[0], pt[1] - seg.start ()[1], pt[2] - seg.start ()[2]);
    double len2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    if (len2 < 1e-20)
        return seg.start ();
    double t = (dp[0] * d[0] + dp[1] * d[1] + dp[2] * d[2]) / len2;
    return session_cpp::Point (seg.start ()[0] + t * d[0], seg.start ()[1] + t * d[1], seg.start ()[2] + t * d[2]);
}

// ── plane_to_xy using session_cpp::Xform ─────────────────────────────────────
inline session_cpp::Xform
plane_to_xy (session_cpp::Point &O0, session_cpp::Plane &plane)
{
    session_cpp::Vector X0 = plane.x_axis ();
    session_cpp::Vector Y0 = plane.y_axis ();
    session_cpp::Vector Z0 = plane.z_axis ();
    return session_cpp::Xform::plane_to_xy (O0, X0, Y0, Z0);
}

// ── closest_point_to (private to cgal::box_search; renamed to avoid session_cpp ADL clash) ──
inline bool
_closest_t (const session_cpp::Point &point, const session_cpp::Line &s, double &t)
{
    session_cpp::Vector D = s.end () - s.start ();
    double DoD = D[0] * D[0] + D[1] * D[1] + D[2] * D[2];

    if (DoD > 0.0)
        {
            session_cpp::Vector to_pt (point[0] - s.start ()[0], point[1] - s.start ()[1], point[2] - s.start ()[2]);
            session_cpp::Vector to_end (point[0] - s.end ()[0], point[1] - s.end ()[1], point[2] - s.end ()[2]);
            if (to_pt[0] * to_pt[0] + to_pt[1] * to_pt[1] + to_pt[2] * to_pt[2] <= to_end[0] * to_end[0] + to_end[1] * to_end[1] + to_end[2] * to_end[2])
                t = (to_pt[0] * D[0] + to_pt[1] * D[1] + to_pt[2] * D[2]) / DoD;
            else
                t = 1.0 + (to_end[0] * D[0] + to_end[1] * D[1] + to_end[2] * D[2]) / DoD;
            return true;
        }
    else
        {
            t = 0.0;
            return true;
        }
}

// ── 2D line intersection (used in skew-line case) ────────────────────────────
// Returns true and sets t_out (parameter on L0) if lines intersect non-parallel
inline bool
line_line_2d (double x00, double y00, double x01, double y01, double x10, double y10, double x11, double y11, double &t_out, double &px, double &py)
{
    double d0x = x01 - x00, d0y = y01 - y00;
    double d1x = x11 - x10, d1y = y11 - y10;
    double det = d0x * d1y - d0y * d1x;
    if (std::fabs (det) < 1e-12)
        return false;
    double rx = x10 - x00, ry = y10 - y00;
    t_out = (rx * d1y - ry * d1x) / det;
    px = x00 + t_out * d0x;
    py = y00 + t_out * d0y;
    return true;
}

// ── Segment-segment squared distance ─────────────────────────────────────────
inline double
segment_segment_sq_distance (const session_cpp::Line &s0, const session_cpp::Line &s1)
{
    // Using Eberly's method for segment-segment distance
    session_cpp::Vector d1 = s0.end () - s0.start ();
    session_cpp::Vector d2 = s1.end () - s1.start ();
    session_cpp::Vector r (s0.start ()[0] - s1.start ()[0], s0.start ()[1] - s1.start ()[1], s0.start ()[2] - s1.start ()[2]);
    double a = d1[0] * d1[0] + d1[1] * d1[1] + d1[2] * d1[2];
    double e = d2[0] * d2[0] + d2[1] * d2[1] + d2[2] * d2[2];
    double f = d2[0] * r[0] + d2[1] * r[1] + d2[2] * r[2];

    double s, t;
    if (a <= 1e-10 && e <= 1e-10)
        {
            s = t = 0.0;
        }
    else if (a <= 1e-10)
        {
            s = 0.0;
            t = std::max (0.0, std::min (1.0, f / e));
        }
    else
        {
            double c = d1[0] * r[0] + d1[1] * r[1] + d1[2] * r[2];
            if (e <= 1e-10)
                {
                    t = 0.0;
                    s = std::max (0.0, std::min (1.0, -c / a));
                }
            else
                {
                    double b = d1[0] * d2[0] + d1[1] * d2[1] + d1[2] * d2[2];
                    double denom = a * e - b * b;
                    if (denom != 0.0)
                        s = std::max (0.0, std::min (1.0, (b * f - c * e) / denom));
                    else
                        s = 0.0;
                    t = (b * s + f) / e;
                    if (t < 0.0)
                        {
                            t = 0.0;
                            s = std::max (0.0, std::min (1.0, -c / a));
                        }
                    else if (t > 1.0)
                        {
                            t = 1.0;
                            s = std::max (0.0, std::min (1.0, (b - c) / a));
                        }
                }
        }
    double dx = (s0.start ()[0] + s * d1[0]) - (s1.start ()[0] + t * d2[0]);
    double dy = (s0.start ()[1] + s * d1[1]) - (s1.start ()[1] + t * d2[1]);
    double dz = (s0.start ()[2] + s * d1[2]) - (s1.start ()[2] + t * d2[2]);
    return dx * dx + dy * dy + dz * dz;
}

inline void
two_rect_from_point_vector_and_zaxis (session_cpp::Point &p, session_cpp::Vector &segment_vector, session_cpp::Vector &zaxis, bool middle, double radius, double length, int flip_male, Polyline &rect0, Polyline &rect1)
{
    session_cpp::Vector x_axis = zaxis;
    session_cpp::Vector y_axis = zaxis.cross (segment_vector);
    x_axis = y_axis.cross (segment_vector);
    x_axis.normalize_self ();
    y_axis.normalize_self ();
    x_axis = x_axis * radius;
    y_axis = y_axis * radius;

    session_cpp::Vector sv0 = segment_vector * (length * -0.5);
    session_cpp::Vector sv1 = segment_vector * (length * 0.5);

    std::array<session_cpp::Vector, 4> v = {
        x_axis * -1.0 + y_axis * -1.0,
        x_axis + y_axis * -1.0,
        x_axis + y_axis,
        x_axis * -1.0 + y_axis,
    };

    if (!middle)
        {
            if (flip_male == 1)
                std::rotate (v.begin (), v.begin () + 1, v.end ());
            else if (flip_male == -1)
                std::rotate (v.rbegin (), v.rbegin () + 1, v.rend ());
        }

    rect0 = {
        session_cpp::Point (p[0] + sv0[0] + v[1][0], p[1] + sv0[1] + v[1][1], p[2] + sv0[2] + v[1][2]),
        session_cpp::Point (p[0] + sv1[0] + v[1][0], p[1] + sv1[1] + v[1][1], p[2] + sv1[2] + v[1][2]),
        session_cpp::Point (p[0] + sv1[0] + v[0][0], p[1] + sv1[1] + v[0][1], p[2] + sv1[2] + v[0][2]),
        session_cpp::Point (p[0] + sv0[0] + v[0][0], p[1] + sv0[1] + v[0][1], p[2] + sv0[2] + v[0][2]),
        session_cpp::Point (p[0] + sv0[0] + v[1][0], p[1] + sv0[1] + v[1][1], p[2] + sv0[2] + v[1][2]),
    };

    rect1 = {
        session_cpp::Point (p[0] + sv0[0] + v[2][0], p[1] + sv0[1] + v[2][1], p[2] + sv0[2] + v[2][2]),
        session_cpp::Point (p[0] + sv1[0] + v[2][0], p[1] + sv1[1] + v[2][1], p[2] + sv1[2] + v[2][2]),
        session_cpp::Point (p[0] + sv1[0] + v[3][0], p[1] + sv1[1] + v[3][1], p[2] + sv1[2] + v[3][2]),
        session_cpp::Point (p[0] + sv0[0] + v[3][0], p[1] + sv0[1] + v[3][1], p[2] + sv0[2] + v[3][2]),
        session_cpp::Point (p[0] + sv0[0] + v[2][0], p[1] + sv0[1] + v[2][1], p[2] + sv0[2] + v[2][2]),
    };
}

inline bool
line_line_intersection_with_properties (session_cpp::Line &s0, session_cpp::Line &s1, int number_of_polyline_segments0, int number_of_polyline_segments1, int current_polyline_segment0, int current_polyline_segment1,
                                        double above_closer_to_edge,

                                        session_cpp::Point &p0, session_cpp::Point &p1, session_cpp::Vector &v0, session_cpp::Vector &v1, session_cpp::Vector &normal, bool &type0, bool &type1, bool &is_parallel

)
{
    // Segment vectors
    v0 = s0.end () - s0.start ();
    v1 = s1.end () - s1.start ();

    // Normal = cross product, used to detect parallelism
    normal = v0.cross (v1);
    double normal_len2 = normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2];

    // Approximate angle between v0 and v1 (degrees)
    double len_v0 = std::sqrt (v0[0] * v0[0] + v0[1] * v0[1] + v0[2] * v0[2]);
    double len_v1 = std::sqrt (v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2]);
    double cos_angle = 0.0;
    if (len_v0 > 1e-10 && len_v1 > 1e-10)
        cos_angle = (v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2]) / (len_v0 * len_v1);
    cos_angle = std::max (-1.0, std::min (1.0, cos_angle));
    double angle_deg = std::acos (std::fabs (cos_angle)) * (180.0 / 3.14159265358979323846);

    is_parallel = normal_len2 < 1e-10 || (90.0 - std::fabs (angle_deg - 90.0)) < 1.0;

    if (is_parallel)
        {
            // Fall back to normal of s0's perpendicular plane
            session_cpp::Point orig = s0.start ();
            session_cpp::Vector v0u = v0;
            v0u.normalize_self ();
            // Create a plane whose normal is v0u
            session_cpp::Plane plane_s0 = session_cpp::Plane::from_point_normal (orig, v0u);
            session_cpp::Vector base1 = plane_s0.x_axis ();
            normal = base1;
        }
    normal.normalize_self ();

    // Check coincident endpoints
    if (s0.start ().squared_distance (s1.start ()) < wood::GLOBALS::DISTANCE)
        {
            p0 = s0.start ();
            p1 = s0.start ();
            v0 = s0.end () - s0.start ();
            v1 = s1.end () - s1.start ();
            v0.normalize_self ();
            v1.normalize_self ();
            type0 = 0;
            type1 = 0;
            return true;
        }
    else if (s0.start ().squared_distance (s1.end ()) < wood::GLOBALS::DISTANCE)
        {
            p0 = s0.start ();
            p1 = s0.start ();
            v0 = s0.end () - s0.start ();
            v1 = s1.start () - s1.end ();
            v0.normalize_self ();
            v1.normalize_self ();
            type0 = 0;
            type1 = 0;
            return true;
        }
    else if (s0.end ().squared_distance (s1.start ()) < wood::GLOBALS::DISTANCE)
        {
            p0 = s0.end ();
            p1 = s0.end ();
            v0 = s0.start () - s0.end ();
            v1 = s1.end () - s1.start ();
            v0.normalize_self ();
            v1.normalize_self ();
            type0 = 0;
            type1 = 0;
            return true;
        }
    else if (s0.end ().squared_distance (s1.end ()) < wood::GLOBALS::DISTANCE)
        {
            p0 = s0.end ();
            p1 = s0.end ();
            v0 = s0.start () - s0.end ();
            v1 = s1.start () - s1.end ();
            v0.normalize_self ();
            v1.normalize_self ();
            type0 = 0;
            type1 = 0;
            return true;
        }

    if (is_parallel)
        {
            // Parallel: find overlap midpoint
            session_cpp::Vector v0u = v0;
            session_cpp::Vector v1u = v1;
            v0u.normalize_self ();
            v1u.normalize_self ();

            // Project all 4 endpoints onto both lines
            auto proj_onto = [&] (const session_cpp::Point &pt, const session_cpp::Line &line, const session_cpp::Vector &unit_dir) -> double
            {
                session_cpp::Vector vec (pt[0] - line.start ()[0], pt[1] - line.start ()[1], pt[2] - line.start ()[2]);
                return vec[0] * unit_dir[0] + vec[1] * unit_dir[1] + vec[2] * unit_dir[2];
            };

            std::vector<std::pair<double, double>> pals;
            auto add_pt = [&] (const session_cpp::Point &pt)
            {
                double pos0 = proj_onto (pt, s0, v0u);
                // Project pt onto s1's line
                session_cpp::Point q0 (s0.start ()[0] + pos0 * v0u[0], s0.start ()[1] + pos0 * v0u[1], s0.start ()[2] + pos0 * v0u[2]);
                session_cpp::Point q1 = project_point_to_line_3d (pt, s1);
                double pos1 = proj_onto (q1, s1, v1u);
                pals.emplace_back (pos0, pos1);
            };
            add_pt (s0.start ());
            add_pt (s0.end ());
            add_pt (s1.start ());
            add_pt (s1.end ());
            std::sort (pals.begin (), pals.end (), [] (const auto &a, const auto &b) { return a.first < b.first; });

            // Midpoint of overlap (indices 1..2 out of 4 sorted)
            session_cpp::Point seg0_mid0 (s0.start ()[0] + pals[1].first * v0u[0], s0.start ()[1] + pals[1].first * v0u[1], s0.start ()[2] + pals[1].first * v0u[2]);
            session_cpp::Point seg0_mid1 (s0.start ()[0] + pals[2].first * v0u[0], s0.start ()[1] + pals[2].first * v0u[1], s0.start ()[2] + pals[2].first * v0u[2]);
            session_cpp::Point seg1_mid0 (s1.start ()[0] + pals[1].second * v1u[0], s1.start ()[1] + pals[1].second * v1u[1], s1.start ()[2] + pals[1].second * v1u[2]);
            session_cpp::Point seg1_mid1 (s1.start ()[0] + pals[2].second * v1u[0], s1.start ()[1] + pals[2].second * v1u[1], s1.start ()[2] + pals[2].second * v1u[2]);

            session_cpp::Point m0 = seg0_mid0.mid_point (seg0_mid1);
            session_cpp::Point m1 = seg1_mid0.mid_point (seg1_mid1);
            session_cpp::Point mid = m0.mid_point (m1);

            p0 = project_point_to_line_3d (mid, s0);
            p1 = project_point_to_line_3d (mid, s1);

            double t0, t1;
            _closest_t (p0, s0, t0);
            _closest_t (p1, s1, t1);
            if (t0 > 0.5)
                v0 = v0 * -1.0;
            if (t1 > 0.5)
                v1 = v1 * -1.0;

            type0 = 0;
            type1 = 0;
        }
    else
        {
            // Skew: project both segments to the plane normal=cross(v0,v1) through s0[0]
            v0.normalize_self ();
            v1.normalize_self ();

            session_cpp::Point plane_origin = s0.start ();
            session_cpp::Vector plane_normal = normal;
            session_cpp::Plane plane_skew = session_cpp::Plane::from_point_normal (plane_origin, plane_normal);
            session_cpp::Xform xform = plane_to_xy (plane_origin, plane_skew);
            auto xform_inv_opt = xform.inverse ();
            if (!xform_inv_opt)
                return false;
            session_cpp::Xform xform_Inv = *xform_inv_opt;

            auto apply = [&] (const session_cpp::Point &pt) -> session_cpp::Point
            {
                session_cpp::Point r = pt;
                xform.transform_point (r);
                return r;
            };

            session_cpp::Point p0_0 = apply (s0.start ());
            session_cpp::Point p0_1 = apply (s0.end ());
            session_cpp::Point p1_0 = apply (s1.start ());
            session_cpp::Point p1_1 = apply (s1.end ());

            double t_param, px, py;
            bool ok = line_line_2d (p0_0[0], p0_0[1], p0_1[0], p0_1[1], p1_0[0], p1_0[1], p1_1[0], p1_1[1], t_param, px, py);
            if (!ok)
                return false;

            // Backproject to 3D
            session_cpp::Point p0_3d (px, py, 0);
            xform_Inv.transform_point (p0_3d);
            p0 = p0_3d;

            double t0, t1;
            _closest_t (p0, s0, t0);

            bool outside0 = t0 > 1.0 || t0 < 0.0;
            if (outside0)
                {
                    t0 = std::max (0.0, std::min (t0, 1.0));
                    p0 = s0.point_at (t0);
                }

            // Project p0 onto s1 (infinite line)
            p1 = project_point_to_line_3d (p0, s1);
            _closest_t (p1, s1, t1);

            bool outside1 = t1 > 1.0 || t1 < 0.0;
            if (outside1)
                {
                    t1 = std::max (0.0, std::min (t1, 1.0));
                    p1 = s1.point_at (t1);
                }

            ////////////////////////////////////////////////////////////////////
            // Type and parameter
            ////////////////////////////////////////////////////////////////////
            t0 += current_polyline_segment0;
            t1 += current_polyline_segment1;
            t0 = remap (t0, 0.0, number_of_polyline_segments0 * 1.0, 0.0, 1.0);
            t1 = remap (t1, 0.0, number_of_polyline_segments1 * 1.0, 0.0, 1.0);
            double how_close_to_line_end_0 = 2 * std::abs (0.5 - t0);
            double how_close_to_line_end_1 = 2 * std::abs (0.5 - t1);

            if (above_closer_to_edge < 0.0)
                {
                    type0 = 1;
                    type1 = 1;
                }
            else if (above_closer_to_edge > 1.0)
                {
                    type0 = t0 < t1 ? 0 : 1;
                    type1 = t0 < t1 ? 1 : 0;
                }
            else
                {
                    type0 = how_close_to_line_end_0 > above_closer_to_edge ? 0 : 1;
                    type1 = how_close_to_line_end_1 > above_closer_to_edge ? 0 : 1;

                    if (how_close_to_line_end_0 > how_close_to_line_end_1 && type0 == 0 && type1 == 0)
                        {
                            type0 = 0;
                            type1 = 1;
                        }
                    else if (how_close_to_line_end_0 < how_close_to_line_end_1 && type0 == 0 && type1 == 0)
                        {
                            type0 = 1;
                            type1 = 0;
                        }
                }

            if (t0 > 0.5 && type0 == 0)
                v0 = v0 * -1.0;
            if (t1 > 0.5 && type1 == 0)
                v1 = v1 * -1.0;

            return true;
        }
    return true;
}

inline bool
line_line_intersection (session_cpp::Line &s0, session_cpp::Line &s1, bool finite, session_cpp::Point &p0, session_cpp::Point &p1)
{
    session_cpp::Vector v0 = s0.end () - s0.start ();
    session_cpp::Vector v1 = s1.end () - s1.start ();
    session_cpp::Vector normal = v0.cross (v1);

    printf ("line_line_intersection \n");

    double len_v0 = std::sqrt (v0[0] * v0[0] + v0[1] * v0[1] + v0[2] * v0[2]);
    double len_v1 = std::sqrt (v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2]);
    double cos_angle = 0.0;
    if (len_v0 > 1e-10 && len_v1 > 1e-10)
        cos_angle = (v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2]) / (len_v0 * len_v1);
    double angle_deg = std::acos (std::max (-1.0, std::min (1.0, std::fabs (cos_angle)))) * (180.0 / 3.14159265358979323846);
    double normal_len2 = normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2];

    // Coincident endpoints
    if (s0.start ().squared_distance (s1.start ()) < wood::GLOBALS::DISTANCE || s0.start ().squared_distance (s1.end ()) < wood::GLOBALS::DISTANCE)
        {
            p0 = s0.start ();
            p1 = s0.start ();
            return true;
        }
    else if (s0.end ().squared_distance (s1.start ()) < wood::GLOBALS::DISTANCE || s0.end ().squared_distance (s1.end ()) < wood::GLOBALS::DISTANCE)
        {
            p0 = s0.end ();
            p1 = s0.end ();
            return true;
        }

    bool is_parallel = normal_len2 < 1e-10 || (90.0 - std::fabs (angle_deg - 90.0)) < 1.0;

    if (is_parallel)
        {
            session_cpp::Vector v0u = v0;
            session_cpp::Vector v1u = v1;
            v0u.normalize_self ();
            v1u.normalize_self ();

            auto proj_t = [&] (const session_cpp::Point &pt, const session_cpp::Line &line, const session_cpp::Vector &udir) -> double
            {
                session_cpp::Vector vec (pt[0] - line.start ()[0], pt[1] - line.start ()[1], pt[2] - line.start ()[2]);
                return vec[0] * udir[0] + vec[1] * udir[1] + vec[2] * udir[2];
            };

            std::vector<std::pair<double, double>> pals;
            auto add = [&] (const session_cpp::Point &pt)
            {
                double t0 = proj_t (pt, s0, v0u);
                session_cpp::Point q1 = project_point_to_line_3d (pt, s1);
                double t1 = proj_t (q1, s1, v1u);
                pals.emplace_back (t0, t1);
            };
            add (s0.start ());
            add (s0.end ());
            add (s1.start ());
            add (s1.end ());
            std::sort (pals.begin (), pals.end (), [] (const auto &a, const auto &b) { return a.first < b.first; });

            session_cpp::Point seg0_a (s0.start ()[0] + pals[1].first * v0u[0], s0.start ()[1] + pals[1].first * v0u[1], s0.start ()[2] + pals[1].first * v0u[2]);
            session_cpp::Point seg0_b (s0.start ()[0] + pals[2].first * v0u[0], s0.start ()[1] + pals[2].first * v0u[1], s0.start ()[2] + pals[2].first * v0u[2]);
            session_cpp::Point seg1_a (s1.start ()[0] + pals[1].second * v1u[0], s1.start ()[1] + pals[1].second * v1u[1], s1.start ()[2] + pals[1].second * v1u[2]);
            session_cpp::Point seg1_b (s1.start ()[0] + pals[2].second * v1u[0], s1.start ()[1] + pals[2].second * v1u[1], s1.start ()[2] + pals[2].second * v1u[2]);

            session_cpp::Point m0 = seg0_a.mid_point (seg0_b);
            session_cpp::Point m1 = seg1_a.mid_point (seg1_b);
            session_cpp::Point mid = m0.mid_point (m1);

            p0 = project_point_to_line_3d (mid, s0);
            p1 = project_point_to_line_3d (mid, s1);
        }
    else
        {
            session_cpp::Point plane_origin = s0.start ();
            session_cpp::Vector plane_normal = normal;
            plane_normal.normalize_self ();
            session_cpp::Plane plane_skew = session_cpp::Plane::from_point_normal (plane_origin, plane_normal);
            session_cpp::Xform xform = plane_to_xy (plane_origin, plane_skew);
            auto xform_inv_opt = xform.inverse ();
            if (!xform_inv_opt)
                return false;
            session_cpp::Xform xform_Inv = *xform_inv_opt;

            auto apply = [&] (const session_cpp::Point &pt) -> session_cpp::Point
            {
                session_cpp::Point r = pt;
                xform.transform_point (r);
                return r;
            };

            session_cpp::Point p0_0 = apply (s0.start ());
            session_cpp::Point p0_1 = apply (s0.end ());
            session_cpp::Point p1_0 = apply (s1.start ());
            session_cpp::Point p1_1 = apply (s1.end ());

            double t_param, px, py;
            bool ok = line_line_2d (p0_0[0], p0_0[1], p0_1[0], p0_1[1], p1_0[0], p1_0[1], p1_1[0], p1_1[1], t_param, px, py);
            if (!ok)
                return false;

            session_cpp::Point p0_3d (px, py, 0);
            xform_Inv.transform_point (p0_3d);
            p0 = p0_3d;

            if (finite)
                {
                    double t0, t1;
                    _closest_t (p0, s0, t0);

                    bool outside0 = t0 > 1.0 || t0 < 0.0;
                    if (outside0)
                        {
                            t0 = std::max (0.0, std::min (t0, 1.0));
                            p0 = s0.point_at (t0);
                        }

                    p1 = project_point_to_line_3d (p0, s1);
                    _closest_t (p1, s1, t1);

                    bool outside1 = t1 > 1.0 || t1 < 0.0;
                    if (outside1)
                        {
                            t1 = std::max (0.0, std::min (t1, 1.0));
                            p1 = s1.point_at (t1);
                        }
                }
            else
                {
                    p1 = project_point_to_line_3d (p0, s1);
                }
            return true;
        }
    return true;
}

// https://github.com/CGAL/cgal/issues/6089
// Replaced CGAL::box_self_intersection_d with session_cpp BVH
inline void
intersecting_sequences_of_dD_iso_oriented_boxes (std::vector<Polyline> &polylines, std::vector<std::vector<double>> &polylines_segment_radii, double &min_distance,
                                                  std::vector<std::array<int, 4>> &polyline0_id_segment0_id_polyline1_id_segment1_id, std::vector<std::array<session_cpp::Point, 2>> &point_pairs)
{
    /////////////////////////////////////////////////////////////////////
    // Segment accessor
    /////////////////////////////////////////////////////////////////////
    auto segment = [&] (std::size_t pid, std::size_t sid) -> session_cpp::Line
    {
        return session_cpp::Line::from_points (polylines[pid][sid], polylines[pid][sid + 1]);
    };

    /////////////////////////////////////////////////////////////////////
    // Build inflated AABB bounding boxes for each segment
    /////////////////////////////////////////////////////////////////////
    // seg_refs[i] = {pid, sid} for box index i
    std::vector<std::pair<std::size_t, std::size_t>> seg_refs;
    std::vector<session_cpp::BoundingBox> boxes;

    for (std::size_t pid = 0; pid < polylines.size (); ++pid)
        {
            for (std::size_t sid = 0; sid < polylines[pid].size () - 1; ++sid)
                {
                    session_cpp::Line seg = segment (pid, sid);
                    double radius = polylines_segment_radii[pid][sid];

                    // Compute 8 inflated corners
                    session_cpp::Vector zaxis = seg.end () - seg.start ();
                    session_cpp::Point z_pt = seg.start ();
                    session_cpp::Vector z_normalized = zaxis;
                    z_normalized.normalize_self ();

                    // Build a local frame
                    session_cpp::Plane seg_plane = session_cpp::Plane::from_point_normal (z_pt, z_normalized);
                    session_cpp::Vector x_axis = seg_plane.x_axis () * radius;
                    session_cpp::Vector y_axis = seg_plane.y_axis () * radius;

                    std::array<session_cpp::Point, 8> pts = {
                        session_cpp::Point (seg.start ()[0] + x_axis[0] + y_axis[0], seg.start ()[1] + x_axis[1] + y_axis[1], seg.start ()[2] + x_axis[2] + y_axis[2]),
                        session_cpp::Point (seg.start ()[0] - x_axis[0] + y_axis[0], seg.start ()[1] - x_axis[1] + y_axis[1], seg.start ()[2] - x_axis[2] + y_axis[2]),
                        session_cpp::Point (seg.start ()[0] - x_axis[0] - y_axis[0], seg.start ()[1] - x_axis[1] - y_axis[1], seg.start ()[2] - x_axis[2] - y_axis[2]),
                        session_cpp::Point (seg.start ()[0] + x_axis[0] - y_axis[0], seg.start ()[1] + x_axis[1] - y_axis[1], seg.start ()[2] + x_axis[2] - y_axis[2]),
                        session_cpp::Point (seg.end ()[0] + x_axis[0] + y_axis[0], seg.end ()[1] + x_axis[1] + y_axis[1], seg.end ()[2] + x_axis[2] + y_axis[2]),
                        session_cpp::Point (seg.end ()[0] - x_axis[0] + y_axis[0], seg.end ()[1] - x_axis[1] + y_axis[1], seg.end ()[2] - x_axis[2] + y_axis[2]),
                        session_cpp::Point (seg.end ()[0] - x_axis[0] - y_axis[0], seg.end ()[1] - x_axis[1] - y_axis[1], seg.end ()[2] - x_axis[2] - y_axis[2]),
                        session_cpp::Point (seg.end ()[0] + x_axis[0] - y_axis[0], seg.end ()[1] + x_axis[1] - y_axis[1], seg.end ()[2] + x_axis[2] - y_axis[2]),
                    };

                    // Build AABB from 8 points
                    double xmin = pts[0][0], xmax = xmin;
                    double ymin = pts[0][1], ymax = ymin;
                    double zmin = pts[0][2], zmax = zmin;
                    for (int k = 1; k < 8; k++)
                        {
                            xmin = std::min (xmin, pts[k][0]);
                            xmax = std::max (xmax, pts[k][0]);
                            ymin = std::min (ymin, pts[k][1]);
                            ymax = std::max (ymax, pts[k][1]);
                            zmin = std::min (zmin, pts[k][2]);
                            zmax = std::max (zmax, pts[k][2]);
                        }

                    session_cpp::Point center ((xmin + xmax) * 0.5, (ymin + ymax) * 0.5, (zmin + zmax) * 0.5);
                    session_cpp::Vector half_size ((xmax - xmin) * 0.5, (ymax - ymin) * 0.5, (zmax - zmin) * 0.5);
                    boxes.emplace_back (center, session_cpp::Vector::x_axis (), session_cpp::Vector::y_axis (), session_cpp::Vector::z_axis (), half_size);
                    seg_refs.emplace_back (pid, sid);
                }
        }

    if (boxes.empty ())
        return;

    /////////////////////////////////////////////////////////////////////
    // BVH self-intersection
    /////////////////////////////////////////////////////////////////////
    double world_size = session_cpp::BVH::compute_world_size (boxes);
    session_cpp::BVH bvh = session_cpp::BVH::from_boxes (boxes, world_size);
    auto [collision_pairs, unused, count] = bvh.check_all_collisions (boxes);

    /////////////////////////////////////////////////////////////////////
    // Filter and process collisions
    /////////////////////////////////////////////////////////////////////
    std::map<uint64_t, std::tuple<double, int, int, int, int>> pair_collisions;

    for (auto &[bi, bj] : collision_pairs)
        {
            auto [pid0, sid0] = seg_refs[bi];
            auto [pid1, sid1] = seg_refs[bj];

            // Skip same polyline
            if (pid0 == pid1)
                continue;

            session_cpp::Line s0 = segment (pid0, sid0);
            session_cpp::Line s1 = segment (pid1, sid1);
            double distance = segment_segment_sq_distance (s0, s1);

            if (distance < min_distance * min_distance)
                {
                    bool flipped = pid1 > pid0;
                    uint64_t id = flipped ? ((uint64_t)pid1 << 32 | pid0) : ((uint64_t)pid0 << 32 | pid1);

                    auto dist_ids = flipped ? std::make_tuple (distance, (int)pid0, (int)sid0, (int)pid1, (int)sid1) : std::make_tuple (distance, (int)pid1, (int)sid1, (int)pid0, (int)sid0);

                    if (pair_collisions.find (id) == pair_collisions.end ())
                        pair_collisions.insert ({ id, dist_ids });
                    else if (distance < std::get<0> (pair_collisions[id]))
                        pair_collisions[id] = dist_ids;
                }
        }

    /////////////////////////////////////////////////////////////////////
    // Output
    /////////////////////////////////////////////////////////////////////
    polyline0_id_segment0_id_polyline1_id_segment1_id.reserve (pair_collisions.size ());
    point_pairs.reserve (pair_collisions.size ());

    for (auto const &x : pair_collisions)
        {
            auto &v = x.second;
            session_cpp::Line s0 = segment (std::get<1> (v), std::get<2> (v));
            session_cpp::Line s1 = segment (std::get<3> (v), std::get<4> (v));

            session_cpp::Point pp0, pp1;
            bool r = line_line_intersection (s0, s1, true, pp0, pp1);
            if (!r)
                continue;
            point_pairs.emplace_back (std::array<session_cpp::Point, 2>{ pp0, pp1 });
        }
}

// ── line_plane bridge (moved from stdafx.h) ──────────────────────────────────
// Delegates to session_cpp::Intersection::line_plane; exposed under the
// cgal::box_search namespace so existing call sites in wood_main.cpp are stable.
inline bool
line_plane (const session_cpp::Line &line, const session_cpp::Plane &plane, session_cpp::Point &output, bool is_finite = false)
{
    return session_cpp::Intersection::line_plane (line, plane, output, is_finite);
}

} // namespace box_search
} // namespace cgal
