
#include "cgal_inscribe_util.h"

#include "../../../stdafx.h"  //go up to the folder where the CMakeLists.txt is

namespace cgal
{
namespace inscribe_util
{

//! \cond NO_DOXYGEN
namespace internal
{

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal helpers (session_cpp types only)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool
unitize (session_cpp::Vector &v)
{
    return v.normalize_self ();
}

// Build plane_to_xy transform from origin and axes
inline session_cpp::Xform
plane_to_xy (session_cpp::Point &origin, session_cpp::Vector &x0, session_cpp::Vector &y0, session_cpp::Vector &z0)
{
    return session_cpp::Xform::plane_to_xy (origin, x0, y0, z0);
}

// Get average normal of a polyline (sum of consecutive cross-products)
void
get_average_normal (const Polyline &polyline, session_cpp::Vector &average_normal)
{
    size_t len = polyline.front ().squared_distance (polyline.back ()) < wood::GLOBALS::DISTANCE_SQUARED ? polyline.size () - 1 : polyline.size ();
    average_normal = session_cpp::Vector (0, 0, 0);

    for (size_t i = 0; i < len; i++)
        {
            auto prev = ((i - 1) + len) % len;
            auto next = ((i + 1) + len) % len;
            auto v1 = polyline[i] - polyline[prev];
            auto v2 = polyline[next] - polyline[i];
            average_normal = average_normal + v1.cross (v2);
        }
}

// Get fast plane from the first point of a polyline and the average normal
void
get_fast_plane (const Polyline &polyline, session_cpp::Point &center, session_cpp::Plane &plane)
{
    center = polyline[0];
    session_cpp::Vector average_normal;
    get_average_normal (polyline, average_normal);
    plane = session_cpp::Plane::from_point_normal (center, average_normal);
}

// Get fast plane axes from first point of a polyline and the average normal
void
get_fast_plane (const Polyline &polyline, session_cpp::Point &origin, session_cpp::Vector &x_axis, session_cpp::Vector &y_axis, session_cpp::Vector &z_axis)
{
    session_cpp::Plane plane;
    cgal::inscribe_util::internal::get_fast_plane (polyline, origin, plane);
    x_axis = plane.x_axis ();
    y_axis = plane.y_axis ();
    z_axis = plane.z_axis ();
}

// Plane axes oriented to the longest edge
void
get_average_plane_axes_oriented_to_longest_edge (const Polyline &polyline, session_cpp::Point &origin, session_cpp::Vector &x_axis, session_cpp::Vector &y_axis, session_cpp::Vector &z_axis)
{
    size_t len = polyline.front ().squared_distance (polyline.back ()) < wood::GLOBALS::DISTANCE_SQUARED ? polyline.size () - 1 : polyline.size ();
    z_axis = session_cpp::Vector (0, 0, 0);

    double max_length = 0;
    for (size_t i = 0; i < len; i++)
        {
            auto prev = ((i - 1) + len) % len;
            auto next = ((i + 1) + len) % len;
            auto v1 = polyline[i] - polyline[prev];
            auto v2 = polyline[next] - polyline[i];
            z_axis = z_axis + v1.cross (v2);

            double temp_len = polyline[i].squared_distance (polyline[next]);
            if (temp_len > max_length)
                {
                    max_length = temp_len;
                    x_axis = polyline[next] - polyline[i];
                    origin = polyline[i];
                }
        }
    y_axis = x_axis.cross (z_axis);
}

// Create circle division points in 3D
void
circle_points (const session_cpp::Vector &origin, const session_cpp::Vector &x_axis, const session_cpp::Vector &y_axis, const session_cpp::Vector &z_axis, std::vector<session_cpp::Point> &points, int number_of_chunks, double radius)
{
    double pi_to_radians = 3.14159265358979323846 / 180.0;
    double chunk_angle = 360.0 / number_of_chunks;
    points.reserve (number_of_chunks);

    session_cpp::Point orig_pt (origin[0], origin[1], origin[2]);
    session_cpp::Vector xa = x_axis, ya = y_axis, za = z_axis;
    session_cpp::Xform xy_to_plane = session_cpp::Xform::xy_to_plane (orig_pt, xa, ya, za);

    for (int i = 0; i < number_of_chunks; i++)
        {
            double degrees = i * chunk_angle;
            double radian = (45.0 + degrees) * pi_to_radians;
            session_cpp::Point p (radius * cos (radian), radius * sin (radian), 0);
            xy_to_plane.transform_point (p);
            points.emplace_back (p);
        }
}

// Orient polyline to XY and compute transform + inverse
bool
orient_polyline_to_xy_and_back (Polyline &polyline, session_cpp::Xform &xform_to_xy, session_cpp::Xform &xform_to_xy_inv)
{
    if (polyline.size () < 3)
        return false;

    session_cpp::Point center;
    session_cpp::Plane plane;
    session_cpp::get_fast_plane (polyline, center, plane);

    session_cpp::Point orig = plane.origin ();
    session_cpp::Vector xa = plane.x_axis (), ya = plane.y_axis (), za = plane.z_axis ();
    xform_to_xy = session_cpp::Xform::plane_to_xy (orig, xa, ya, za);
    auto inv = xform_to_xy.inverse ();
    if (!inv)
        return false;
    xform_to_xy_inv = *inv;
    return true;
}

// Axis-angle rotation matrix
inline session_cpp::Xform
axis_rotation (const double &angle, session_cpp::Vector &axis)
{
    return session_cpp::Xform::axis_rotation (angle, axis);
}

// ── Grid-based largest empty iso-rectangle approximation ─────────────────────
// Projects the already-2D polyline points (after transform), computes AABB,
// and finds the largest rectangle that fits inside the polygon by grid search.

// 2D point-in-polygon test (ray casting, polygon in XY plane z≈0)
static bool
point_in_polygon_2d (double px, double py, const std::vector<std::pair<double, double>> &poly)
{
    int n = (int)poly.size ();
    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++)
        {
            double xi = poly[i].first, yi = poly[i].second;
            double xj = poly[j].first, yj = poly[j].second;
            if (((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi))
                inside = !inside;
        }
    return inside;
}

// Find largest empty axis-aligned rectangle inside a 2D polygon (grid search)
// Returns {xmin, ymin, xmax, ymax} of best rectangle, with scale shrink applied
static std::array<double, 4>
largest_empty_rect_approx (const std::vector<std::pair<double, double>> &pts_2d, const Polyline &poly_2d_pts, double precision)
{
    // Build AABB
    double xmin = pts_2d[0].first, xmax = xmin;
    double ymin = pts_2d[0].second, ymax = ymin;
    for (auto &p : pts_2d)
        {
            xmin = std::min (xmin, p.first);
            xmax = std::max (xmax, p.first);
            ymin = std::min (ymin, p.second);
            ymax = std::max (ymax, p.second);
        }

    // Build polygon for inside test
    std::vector<std::pair<double, double>> polygon;
    polygon.reserve (pts_2d.size ());
    for (auto &p : pts_2d)
        polygon.push_back (p);

    // Grid search: try candidate rectangle centers and sizes
    int steps = std::max (4, (int)(20.0 * precision));
    double dx = (xmax - xmin) / steps;
    double dy = (ymax - ymin) / steps;

    double best_area = 0;
    std::array<double, 4> best = { xmin, ymin, xmax, ymax };

    for (int ix = 0; ix < steps; ix++)
        {
            for (int iy = 0; iy < steps; iy++)
                {
                    double cx = xmin + (ix + 0.5) * dx;
                    double cy = ymin + (iy + 0.5) * dy;
                    if (!point_in_polygon_2d (cx, cy, polygon))
                        continue;

                    // Expand rectangle from center until it hits the boundary
                    double rx = dx * 0.5, ry = dy * 0.5;
                    // Shrink to fit inside polygon
                    double step_x = dx * 0.1, step_y = dy * 0.1;
                    while (rx > step_x)
                        {
                            // Check all 4 corners
                            bool ok = point_in_polygon_2d (cx - rx, cy - ry, polygon) && point_in_polygon_2d (cx + rx, cy - ry, polygon)
                                      && point_in_polygon_2d (cx - rx, cy + ry, polygon) && point_in_polygon_2d (cx + rx, cy + ry, polygon);
                            if (ok)
                                break;
                            rx -= step_x;
                            ry -= step_y;
                        }

                    double area = (2 * rx) * (2 * ry);
                    if (area > best_area)
                        {
                            best_area = area;
                            best = { cx - rx, cy - ry, cx + rx, cy + ry };
                        }
                }
        }
    return best;
}

} // namespace internal
//! \endcond

std::tuple<session_cpp::Point, session_cpp::Plane, double>
get_polylabel (const std::vector<Polyline> &polylines, double precision)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Copy polylines for orienting from 3D to 2D
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<Polyline> polylines_copy = polylines;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create Transformation to XY plane
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    session_cpp::Point origin;
    session_cpp::Plane plane;
    session_cpp::get_fast_plane (polylines_copy[0], origin, plane);

    session_cpp::Point orig = plane.origin ();
    session_cpp::Vector xa = plane.x_axis (), ya = plane.y_axis (), za = plane.z_axis ();
    session_cpp::Xform xform_toXY = session_cpp::Xform::plane_to_xy (orig, xa, ya, za);
    auto inv = xform_toXY.inverse ();
    if (!inv)
        return { session_cpp::Point (), plane, 0.0 };
    session_cpp::Xform xform_toXY_Inv = *inv;

    for (auto &polyline : polylines_copy)
        session_cpp::transform (polyline, xform_toXY);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sort polylines by bounding-box diagonal to detect which are holes
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<double> len (polylines_copy.size ());
    std::vector<int> ids ((int)len.size ());
    std::iota (begin (ids), end (ids), 0);

    for (size_t i = 0; i < polylines_copy.size (); i++)
        {
            auto   bb = session_cpp::BoundingBox::from_points (polylines_copy[i]);
            auto   mn = bb.min_point ();
            auto   mx = bb.max_point ();
            double dx = mx[0] - mn[0], dy = mx[1] - mn[1], dz = mx[2] - mn[2];
            len[i] = dx * dx + dy * dy + dz * dz;
        }
    std::sort (begin (ids), end (ids), [&] (int ia, int ib) { return len[ia] < len[ib]; });
    std::reverse (begin (ids), end (ids));

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convert to Polylabel datastructure (skip last closing point)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    mapbox::geometry::polygon<double> polygon_with_holes (polylines_copy.size ());

    for (size_t i = 0; i < polylines_copy.size (); i++)
        for (size_t j = 0; j < polylines_copy[ids[i]].size () - 1; j++)
            polygon_with_holes[i].emplace_back (polylines_copy[ids[i]][j][0], polylines_copy[ids[i]][j][1]);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Run Polylabel algorithm
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::array<double, 3> center_and_radius = mapbox::polylabel (polygon_with_holes, 1.0);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convert back to 3D
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    session_cpp::Point center_2d (center_and_radius[0], center_and_radius[1], 0);
    xform_toXY_Inv.transform_point (center_2d);

    return std::tuple<session_cpp::Point, session_cpp::Plane, double> (center_2d, plane, center_and_radius[2]);
}

void
get_polylabel_circle_division_points (const session_cpp::Vector &division_direction_in_3d, const std::vector<Polyline> &polylines, std::vector<session_cpp::Point> &points, int division, double scale, double precision,
                                      bool orient_to_closest_edge)
{
    // Run the polylabel algorithm
    auto circle = get_polylabel (polylines, precision);
    session_cpp::Point &center_pt = std::get<0> (circle);
    session_cpp::Vector center (center_pt[0], center_pt[1], center_pt[2]);

    bool is_direction_valid = division_direction_in_3d[0] != 0.0 || division_direction_in_3d[1] != 0.0 || division_direction_in_3d[2] != 0.0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Find closest edge direction to the center
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    size_t edge_id0 = 0, edge_id1 = 0;
    double distance_from_center_to_polyline_edge = std::numeric_limits<double>::max ();

    if (orient_to_closest_edge)
        {
            for (size_t i = 0; i < polylines.size (); i++)
                {
                    for (size_t j = 0; j < polylines[i].size () - 1; j++)
                        {
                            const auto &pa = polylines[i][j];
                            const auto &pb = polylines[i][j + 1];
                            session_cpp::Vector seg_vec = pb - pa;
                            double seg_len2 = seg_vec[0] * seg_vec[0] + seg_vec[1] * seg_vec[1] + seg_vec[2] * seg_vec[2];

                            if (seg_len2 < 1e-20)
                                continue;

                            // Project center onto segment
                            session_cpp::Vector to_center (center[0] - pa[0], center[1] - pa[1], center[2] - pa[2]);
                            double t = (to_center[0] * seg_vec[0] + to_center[1] * seg_vec[1] + to_center[2] * seg_vec[2]) / seg_len2;
                            if (t < 0 || t > 1)
                                continue;

                            session_cpp::Point proj (pa[0] + t * seg_vec[0], pa[1] + t * seg_vec[1], pa[2] + t * seg_vec[2]);
                            double dist2 = center_pt.squared_distance (proj);
                            if (dist2 < distance_from_center_to_polyline_edge)
                                {
                                    distance_from_center_to_polyline_edge = dist2;
                                    edge_id0 = i;
                                    edge_id1 = j;
                                }
                        }
                }
        }

    // Orientation direction
    session_cpp::Vector division_direction = orient_to_closest_edge ? (polylines[edge_id0][edge_id1 + 1] - polylines[edge_id0][edge_id1]) : division_direction_in_3d;

    session_cpp::Vector x_axis = (is_direction_valid || orient_to_closest_edge) ? division_direction : std::get<1> (circle).x_axis ();
    session_cpp::Vector y_axis = (is_direction_valid || orient_to_closest_edge) ? division_direction.cross (std::get<1> (circle).z_axis ()) : std::get<1> (circle).y_axis ();
    session_cpp::Vector z_axis = std::get<1> (circle).z_axis ();

    internal::circle_points (center, x_axis, y_axis, z_axis, points, division, std::get<2> (circle) * scale);
}

bool
inscribe_rectangle_in_convex_polygon (const std::vector<Polyline> &polylines, Polyline &result, std::vector<session_cpp::Point> &points, session_cpp::Line &direction, double scale, double precision,
                                      double rectangle_division_distance)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Copy polylines
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<Polyline> polylines_copy = polylines;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Determine plane axes
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    session_cpp::Point o;
    session_cpp::Vector x, y, z;

    double seg_len2 = direction.length () * direction.length ();
    if (seg_len2 > wood::GLOBALS::DISTANCE_SQUARED)
        {
            internal::get_fast_plane (polylines_copy[0], o, x, y, z);
            x = direction.end () - direction.start ();
            y = x.cross (z);
        }
    else
        {
            internal::get_average_plane_axes_oriented_to_longest_edge (polylines_copy[0], o, x, y, z);
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create Transformation to XY plane (with tiny rotation to avoid degeneracy)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    session_cpp::Xform xform_toXY = internal::plane_to_xy (o, x, y, z);

    session_cpp::Vector rot_axis (0, 0, 1);
    session_cpp::Xform rot = internal::axis_rotation (0.0001, rot_axis);
    xform_toXY = rot * xform_toXY;

    auto inv = xform_toXY.inverse ();
    if (!inv)
        return false;
    session_cpp::Xform xform_toXY_Inv = *inv;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Project polylines to 2D and collect points
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    size_t point_count = 0;
    for (auto &polyline : polylines_copy)
        point_count += polyline.size ();

    std::vector<std::pair<double, double>> points_2d;
    points_2d.reserve (point_count);

    for (auto &polyline : polylines_copy)
        {
            session_cpp::transform (polyline, xform_toXY);
            for (int i = 0; i < (int)polyline.size () - 1; i++)
                points_2d.push_back ({ polyline[i][0], polyline[i][1] });
        }

    // Compute step_division from diagonal
    double xmin = points_2d[0].first, xmax = xmin;
    double ymin = points_2d[0].second, ymax = ymin;
    for (auto &p : points_2d)
        {
            xmin = std::min (xmin, p.first);
            xmax = std::max (xmax, p.first);
            ymin = std::min (ymin, p.second);
            ymax = std::max (ymax, p.second);
        }
    double diag = std::sqrt ((xmax - xmin) * (xmax - xmin) + (ymax - ymin) * (ymax - ymin));
    double step_division = diag / (50.0 * precision);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Interpolate edge division points (for the density of the grid search)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Polyline division_points_temp;
    for (auto &polyline : polylines_copy)
        for (int i = 0; i < (int)polyline.size () - 1; i++)
            {
                int divisions = (int)std::min (100.0, polyline[i].distance (polyline[i + 1]) / step_division);
                std::vector<session_cpp::Point> div_pts;
                session_cpp::interpolate_points (polyline[i], polyline[i + 1], divisions, div_pts, 2);
                for (auto &pt : div_pts)
                    {
                        points_2d.push_back ({ pt[0], pt[1] });
                        division_points_temp.push_back (pt);
                    }
            }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Find largest empty rectangle via grid approximation
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto rect_bounds = internal::largest_empty_rect_approx (points_2d, division_points_temp, precision);
    double rx_min = rect_bounds[0], ry_min = rect_bounds[1], rx_max = rect_bounds[2], ry_max = rect_bounds[3];

    // Apply scale shrink
    double x_dist = (1.0 - scale) * std::abs (rx_max - rx_min) * 0.5;
    double y_dist = (1.0 - scale) * std::abs (ry_max - ry_min) * 0.5;
    x_dist = std::min (x_dist, y_dist);
    y_dist = x_dist;

    Polyline rect = {
        session_cpp::Point (rx_min + x_dist, ry_min + y_dist, 0),
        session_cpp::Point (rx_min + x_dist, ry_max - y_dist, 0),
        session_cpp::Point (rx_max - x_dist, ry_max - y_dist, 0),
        session_cpp::Point (rx_max - x_dist, ry_min + y_dist, 0),
        session_cpp::Point (rx_min + x_dist, ry_min + y_dist, 0),
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Orient back to 3D
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (auto &point : rect)
        xform_toXY_Inv.transform_point (point);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Divide edges into points or grid
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (rectangle_division_distance > wood::GLOBALS::DISTANCE)
        {
            for (int i = 0; i < (int)rect.size () - 1; i++)
                {
                    int divisions = (int)std::min (25.0, rect[i].distance (rect[i + 1]) / rectangle_division_distance);
                    std::vector<session_cpp::Point> div_pts;
                    session_cpp::interpolate_points (rect[i], rect[i + 1], divisions, div_pts, 2);
                    for (auto &pt : div_pts)
                        points.push_back (pt);
                }
        }
    else if (rectangle_division_distance < -wood::GLOBALS::DISTANCE)
        {
            double unsigned_dist = std::abs (rectangle_division_distance);
            double edge_dist_00 = rect[0].distance (rect[1]);
            double edge_dist_01 = rect[3].distance (rect[2]);
            double edge_dist_10 = rect[1].distance (rect[2]);
            double edge_dist_11 = rect[0].distance (rect[3]);

            int divisions_u = (int)(std::min (std::ceil (edge_dist_00 / unsigned_dist), std::ceil (edge_dist_01 / unsigned_dist)));
            int divisions_v = (int)(std::min (std::ceil (edge_dist_10 / unsigned_dist), std::ceil (edge_dist_11 / unsigned_dist)));

            std::vector<session_cpp::Point> edge_0, edge_2;
            session_cpp::interpolate_points (rect[0], rect[1], divisions_u, edge_0, 1);
            session_cpp::interpolate_points (rect[3], rect[2], divisions_u, edge_2, 1);

            points.reserve ((divisions_u + 2) * (divisions_v + 2));
            for (int i = 0; i < (divisions_u + 2); i++)
                {
                    std::vector<session_cpp::Point> tmp;
                    session_cpp::interpolate_points (edge_0[i], edge_2[i], divisions_v, tmp, 1);
                    for (auto &pt : tmp)
                        points.push_back (pt);
                }
        }

    result = rect;
    return true;
}

} // namespace inscribe_util
} // namespace cgal
