#include "pch.h"
#include "wood_session.h"

using namespace session_cpp;

namespace wood_session {

namespace {

/// Near-coplanar rejection threshold, synced from config::DISTANCE_SQUARED by the caller.
double g_cross_distance_squared = 0.01;

/// Polyline-plane crossings, none when any vertex lies within the cross-joint distance of the plane; exactly two are a chord.
bool polyline_plane_chord(const Polyline& polyline, const Plane& plane, std::vector<Point>& points, std::vector<int>& edge_ids) {

    for (size_t i = 0; i < polyline.point_count(); i++)
        if (plane.squared_distance(polyline.get_point(i)) < g_cross_distance_squared)
            return false;

    return Intersection::polyline_plane(polyline, plane, points, edge_ids) && points.size() == 2;
}

/// Positions of the test points inside or on the polygon in its plane; their count out.
int points_inside(const Polyline& polygon, const Plane& plane, const std::vector<Point>& test_points, std::vector<int>& inside) {

    const Xform to_xy = Xform::world_to_frame(plane.origin(), plane.base1(), plane.base2(), plane.z_axis());
    const Polyline flat = polygon.transformed(to_xy);
    for (size_t i = 0; i < test_points.size(); i++) {

        const Point moved = test_points[i].transformed(to_xy);
        const Point q(moved[0], moved[1], 0.0);
        size_t edge = 0;
        Point closest;
        if (flat.point_in_polygon_2d(q) || flat.closest_distance_and_point(q, edge, closest) < Tolerance::ZERO_TOLERANCE)
            inside.push_back(static_cast<int>(i));
    }

    return static_cast<int>(inside.size());
}

/// Cross-joint chord between two polylines via reciprocal polyline-plane intersections; (edge in c0, edge in c1) pair out.
bool polyline_plane_cross_joint(const Polyline& c0, const Polyline& c1, const Plane& p0, const Plane& p1, Line& contact, std::pair<int, int>& edges) {

    std::vector<Point> pts0;
    std::vector<int> edge_ids_0;
    if (!polyline_plane_chord(c0, p1, pts0, edge_ids_0))
        return false;

    std::vector<Point> pts1;
    std::vector<int> edge_ids_1;
    if (!polyline_plane_chord(c1, p0, pts1, edge_ids_1))
        return false;

    std::vector<int> ID1;
    const int count0 = points_inside(c0, p0, pts1, ID1);

    std::vector<int> ID0;
    const int count1 = points_inside(c1, p1, pts0, ID0);

    if (count0 == 0 && count1 == 0)
        return false;

    if (std::abs(count0 - count1) == 2) {
        if (count0 == 2) {
            contact = Line::from_points(pts0[0], pts0[1]);
            edges = std::pair<int, int>(edge_ids_0[0], edge_ids_0[1]);
        } else {
            contact = Line::from_points(pts1[0], pts1[1]);
            edges = std::pair<int, int>(edge_ids_1[0], edge_ids_1[1]);
        }
        return true;
    }

    if (count0 == 1 && count1 == 1) {
        contact = Line::from_points(pts0[ID0[0]], pts1[ID1[0]]);
        edges = std::pair<int, int>(edge_ids_0[ID0[0]], edge_ids_1[ID1[0]]);
        return true;
    }

    if (count0 > 1 || count1 > 1) {
        std::vector<Point> pts;
        pts.reserve(ID0.size() + ID1.size());
        for (int i : ID0)
            pts.push_back(pts0[i]);
        for (int i : ID1)
            pts.push_back(pts1[i]);

        double xmin = pts[0][0];
        double ymin = pts[0][1];
        double zmin = pts[0][2];
        double xmax = xmin;
        double ymax = ymin;
        double zmax = zmin;
        for (const Point& q : pts) {
            xmin = std::min(xmin, q[0]);
            ymin = std::min(ymin, q[1]);
            zmin = std::min(zmin, q[2]);
            xmax = std::max(xmax, q[0]);
            ymax = std::max(ymax, q[1]);
            zmax = std::max(zmax, q[2]);
        }

        const Point lo(xmin, ymin, zmin);
        const Point hi(xmax, ymax, zmax);
        contact = Line::from_points(lo, hi);

        // lo/hi are bbox corners, so e0/e1 stay -1 for plates crossing in general position; type 30 does not consume them.
        int e0 = -1;
        int e1 = -1;
        for (size_t i = 0; i < ID0.size(); i++) {
            const Point& q = pts0[ID0[i]];
            if ((q - lo).magnitude_squared() < 0.001 || (q - hi).magnitude_squared() < 0.001) {
                e0 = edge_ids_0[ID0[i]];
                break;
            }
        }
        for (size_t i = 0; i < ID1.size(); i++) {
            const Point& q = pts1[ID1[i]];
            if ((q - lo).magnitude_squared() < 0.001 || (q - hi).magnitude_squared() < 0.001) {
                e1 = edge_ids_1[ID1[i]];
                break;
            }
        }
        edges = std::pair<int, int>(e0, e1);
        return true;
    }

    return false;
}

} // anonymous namespace

void set_cross_joint_distance_squared(double dist_sq) {
    g_cross_distance_squared = dist_sq;
}

bool plane_to_face(
    const Polyline& cx0, const Polyline& cx1,
    const Polyline& cy0, const Polyline& cy1,
    const Plane& px0, const Plane& px1,
    const Plane& py0, const Plane& py1,
    CrossJoint& result,
    double angle_tol,
    const std::array<double, 3>& extension) {

    result.face_ids_a = {-1, -1};
    result.face_ids_b = {-1, -1};
    result.type = 30;

    const double raw_angle = px0.z_axis().angle(py0.z_axis(), false);
    const double angle = 90.0 - std::fabs(raw_angle - 90.0);
    if (angle < angle_tol)
        return false;

    Line cx0_py0__cy0_px0;
    std::pair<int, int> e0_0__e1_0;
    if (!polyline_plane_cross_joint(cx0, cy0, px0, py0, cx0_py0__cy0_px0, e0_0__e1_0))
        return false;

    Line cx0_py1__cy1_px0;
    std::pair<int, int> e0_0__e1_1;
    if (!polyline_plane_cross_joint(cx0, cy1, px0, py1, cx0_py1__cy1_px0, e0_0__e1_1))
        return false;

    Line cx1_py0__cy0_px1;
    std::pair<int, int> e0_1__e1_0;
    if (!polyline_plane_cross_joint(cx1, cy0, px1, py0, cx1_py0__cy0_px1, e0_1__e1_0))
        return false;

    Line cx1_py1__cy1_px1;
    std::pair<int, int> e0_1__e1_1;
    if (!polyline_plane_cross_joint(cx1, cy1, px1, py1, cx1_py1__cy1_px1, e0_1__e1_1))
        return false;

    result.face_ids_a.first = e0_0__e1_0.first + 2;
    result.face_ids_b.first = e0_0__e1_0.second + 2;
    result.face_ids_a.second = e0_1__e1_1.first + 2;
    result.face_ids_b.second = e0_1__e1_1.second + 2;

    const Vector ref_v = cx0_py0__cy0_px0.to_vector();
    if (ref_v.dot(cx0_py1__cy1_px0.to_vector()) < 0.0)
        cx0_py1__cy1_px0 = -cx0_py1__cy1_px0;
    if (ref_v.dot(cx1_py0__cy0_px1.to_vector()) < 0.0)
        cx1_py0__cy0_px1 = -cx1_py0__cy0_px1;
    if (ref_v.dot(cx1_py1__cy1_px1.to_vector()) < 0.0)
        cx1_py1__cy1_px1 = -cx1_py1__cy1_px1;

    Line c;
    Line::get_middle_line(cx0_py1__cy1_px0, cx1_py0__cy0_px1, c);
    if (c.length() < Tolerance::ZERO_TOLERANCE)
        return false;

    c.scale(10.0);

    const Point c_start = c.start();
    const Point c_end = c.end();
    const auto parameter_of = [&](const Point& point) {
        double t = 0.0;
        Polyline::closest_point_to_line(point, c_start, c_end, t);
        return t;
    };

    double cpt0[4] = {
        parameter_of(cx0_py0__cy0_px0.start()),
        parameter_of(cx0_py1__cy1_px0.start()),
        parameter_of(cx1_py0__cy0_px1.start()),
        parameter_of(cx1_py1__cy1_px1.start())
    };
    std::sort(cpt0, cpt0 + 4);

    double cpt1[4] = {
        parameter_of(cx0_py0__cy0_px0.end()),
        parameter_of(cx0_py1__cy1_px0.end()),
        parameter_of(cx1_py0__cy0_px1.end()),
        parameter_of(cx1_py1__cy1_px1.end())
    };
    std::sort(cpt1, cpt1 + 4);

    double cpt[8] = {cpt0[0], cpt0[1], cpt0[2], cpt0[3], cpt1[0], cpt1[1], cpt1[2], cpt1[3]};
    std::sort(cpt, cpt + 8);

    // cpt0[3] > cpt1[0] is the normal X-crossing case; only lMin's center and axis are consumed.
    const Line lMin = Line::from_points(c.point_at(cpt0[3]), c.point_at(cpt1[0]));
    const Line lMax = Line::from_points(c.point_at(cpt[0]), c.point_at(cpt[7]));

    const Point lMin_mid = lMin.center();
    const Vector lMin_dir = lMin.to_vector();
    if (lMin_dir.magnitude() < Tolerance::ZERO_TOLERANCE)
        return false;

    Vector lMin_z = lMin_dir;
    lMin_z.normalize_self();
    const Vector helper = (std::fabs(lMin_z[0]) < 0.9) ? Vector(1, 0, 0) : Vector(0, 1, 0);
    Vector mid_x = helper.cross(lMin_z);
    mid_x.normalize_self();
    Vector mid_y = lMin_z.cross(mid_x);
    mid_y.normalize_self();
    const Plane midPlane(lMin_mid, mid_x, mid_y);

    Point midPlane_lMax;
    if (!Intersection::line_plane(lMax, midPlane, midPlane_lMax, false))
        return false;

    const Point lMax_a = lMax.start();
    const Point lMax_b = lMax.end();
    const int maxID = ((lMax_b - midPlane_lMax).magnitude_squared() > (lMax_a - midPlane_lMax).magnitude_squared()) ? 1 : 0;
    Vector v = (maxID == 1) ? (lMax_b - midPlane_lMax) : -(lMax_a - midPlane_lMax);

    if (extension[2] != 0.0) {
        const double length = v.magnitude();
        if (length > Tolerance::ZERO_TOLERANCE) {
            const double target = length + extension[2];
            v = v * target / length;
        }
    }

    if (!Intersection::plane_4lines(
            midPlane,
            cx0_py0__cy0_px0,
            cx0_py1__cy1_px0,
            cx1_py1__cy1_px1,
            cx1_py0__cy0_px1,
            result.joint_area))
        return false;

    const Polyline& jpts = result.joint_area;
    result.joint_lines[0] = Polyline({Point::mid_point(jpts[0], jpts[1]), Point::mid_point(jpts[2], jpts[3])});
    result.joint_lines[1] = Polyline({Point::mid_point(jpts[1], jpts[2]), Point::mid_point(jpts[3], jpts[0])});

    result.joint_volumes[0] = result.joint_area.translated(v);
    result.joint_volumes[1] = result.joint_area.translated(-v);

    if (extension[0] != 0.0 || extension[1] != 0.0) {
        for (int k = 0; k < 2; k++) {
            Polyline& pl = result.joint_volumes[k];
            pl.extend_segment(0, extension[0], extension[0], 0.0, 0.0);
            pl.extend_segment(2, extension[0], extension[0], 0.0, 0.0);
            pl.extend_segment(1, extension[1], extension[1], 0.0, 0.0);
            pl.extend_segment(3, extension[1], extension[1], 0.0, 0.0);
        }
    }

    return true;
}

bool plane_to_face(
    const std::array<Polyline, 2>& polylines_a,
    const std::array<Polyline, 2>& polylines_b,
    const std::array<Plane, 2>& planes_a,
    const std::array<Plane, 2>& planes_b,
    CrossJoint& result,
    double angle_tol,
    const std::array<double, 3>& extension) {
    return plane_to_face(
        polylines_a[0], polylines_a[1],
        polylines_b[0], polylines_b[1],
        planes_a[0], planes_a[1],
        planes_b[0], planes_b[1],
        result, angle_tol, extension);
}

} // namespace wood_session
