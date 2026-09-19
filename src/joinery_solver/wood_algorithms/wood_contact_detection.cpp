#include "pch.h"
#include "wood_contact_detection.h"
#include "../src/clipper2/clipper.h"
using namespace session_cpp;
using namespace wood_session;

constexpr bool TRACE = false;

// ═══════════════════════════════════════════════════════════════════════════
// Face contacts
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_session {

namespace {

void add_outline(const Polyline& pl, std::vector<Point>& corners) {
    const size_t n = pl.is_closed() ? pl.point_count() - 1 : pl.point_count();
    for (size_t k = 0; k < n; k++)
        corners.push_back(pl.get_point(k));
}

/// Whether the element follows the plate face convention: [0] bottom, [1] top, [2..] sides.
bool is_plate(const Element& e) { return dynamic_cast<const Plate*>(&e) != nullptr; }

/// A plate's outlines as it holds them now (the solver swaps faces mid-run, past the kernel cache), any other element's cached face outlines.
std::vector<Polyline> outlines_of(Element& e) {

    if (const Plate* plate = dynamic_cast<const Plate*>(&e))
        return plate->polylines;

    return e.polylines();
}

/// A plate's planes as it holds them now, any other element's cached face planes.
std::vector<Plane> planes_of(Element& e) {

    if (const Plate* plate = dynamic_cast<const Plate*>(&e))
        return plate->planes;

    return e.planes();
}

/// The points that bound an element: a plate by its two outlines, anything else by every loop.
void bounding_points(Element& e, std::vector<Point>& out) {

    const std::vector<Polyline> loops = outlines_of(e);
    if (is_plate(e) && loops.size() > 1) {
        out.reserve(loops[0].point_count() + loops[1].point_count());
        add_outline(loops[1], out);
        add_outline(loops[0], out);
        return;
    }

    for (const Polyline& loop : loops)
        add_outline(loop, out);
}

/// Whether face i is an outer (top/bottom) face, where a triangular overlap is accepted.
bool outer_face(const Element& e, size_t i) { return is_plate(e) && i < 2; }

/// Topology class of a face pair; unknown unless both sides follow the plate convention.
ContactType contact_type(const Element& a, size_t i, const Element& b, size_t j) {

    if (!is_plate(a) || !is_plate(b))
        return ContactType::unknown;

    return static_cast<ContactType>(int(outer_face(a, i)) + int(outer_face(b, j)));
}

}  // namespace

/// Whether element i takes part in the search: every element when no names were given, else only the named ones.
static bool element_included(const std::vector<std::shared_ptr<Element>>& elements, const std::unordered_set<std::string>& wanted, size_t i) {
    return wanted.empty() || wanted.count(elements[i]->name) != 0;
}

std::vector<std::pair<int, int>> adjacency_search(
    const std::vector<std::shared_ptr<Element>>& elements,
    double inflate,
    const std::vector<std::string>& names) {

    std::vector<std::pair<int, int>> pairs;
    const size_t n_el = elements.size();

    if (n_el == 0)
        return pairs;

    const std::unordered_set<std::string> wanted(names.begin(), names.end());

    std::vector<OBB> obbs(n_el);
    std::vector<AABB> aabbs(n_el);
    std::vector<Point> corners;
    for (size_t i = 0; i < n_el; i++) {

        if (!element_included(elements, wanted, i))
            continue;

        corners.clear();
        bounding_points(*elements[i], corners);
        const std::vector<Plane> planes = planes_of(*elements[i]);
        if (!planes.empty())
            obbs[i] = OBB::from_points(corners, planes[0], inflate);
        else
            obbs[i] = OBB::from_points(corners, inflate);
        aabbs[i] = obbs[i].aabb();
    }

    double ws = 0;
    for (const AABB& a : aabbs) {
        ws = std::max(ws, std::abs(a.cx + a.hx));
        ws = std::max(ws, std::abs(a.cy + a.hy));
        ws = std::max(ws, std::abs(a.cz + a.hz));
        ws = std::max(ws, std::abs(a.cx - a.hx));
        ws = std::max(ws, std::abs(a.cy - a.hy));
        ws = std::max(ws, std::abs(a.cz - a.hz));
    }

    SpatialBVH bvh;
    bvh.build_from_aabbs(aabbs.data(), n_el, ws * 2);

    for (size_t i = 0; i < n_el; i++) {

        if (!element_included(elements, wanted, i))
            continue;

        for (int j : bvh.query_aabb(aabbs[i]))
            if ((int)i < j && element_included(elements, wanted, (size_t)j) && obbs[i].collides_with(obbs[j]))
                pairs.emplace_back((int)i, j);
    }

    return pairs;
}

bool faces_coplanar(
    const Plane& face0,
    const Plane& face1,
    double cos_angle,
    double coplanar_tolerance) {

    const Vector& n0 = face0.z_axis();
    const Vector& n1 = face1.z_axis();
    const double mag0 = n0.magnitude_squared();
    const double mag1 = n1.magnitude_squared();
    const double ll = std::sqrt(mag0 * mag1);
    if (ll <= 0.0 || n0.dot(n1) / ll > -cos_angle)
        return false;

    const Vector offset = face1.origin() - face0.origin();
    const double dot0 = n0.dot(offset);
    const double dot1 = n1.dot(offset);
    const double sq_dist0 = mag0 > 1e-20 ? dot0 * dot0 / mag0 : 1e30;
    const double sq_dist1 = mag1 > 1e-20 ? dot1 * dot1 / mag1 : 1e30;

    return sq_dist0 < coplanar_tolerance && sq_dist1 < coplanar_tolerance;
}

/// An outline as a Clipper path in the plane's 2D frame, scaled to integers and without its closing vertex.
static Clipper2Lib::Path64 outline_to_clipper_path(const Polyline& outline, const Point& origin, const Vector& x_axis, const Vector& y_axis, double scale) {

    Clipper2Lib::Path64 path;
    const size_t n = outline.is_closed() ? outline.point_count() - 1 : outline.point_count();
    path.reserve(n);
    for (size_t k = 0; k < n; ++k) {
        const Vector d = outline.get_point(k) - origin;
        const double u = d.dot(x_axis);
        const double v = d.dot(y_axis);
        path.emplace_back(
            static_cast<int64_t>(std::llround(u * scale)),
            static_cast<int64_t>(std::llround(v * scale))
        );
    }

    return path;
}

bool face_overlap_area(
    const Polyline& outline0,
    const Polyline& outline1,
    const Plane& plane0,
    bool include_triangles,
    Polyline& out_area) {

    if (outline0.point_count() < 3 || outline1.point_count() < 3)
        return false;

    const Point origin = outline0.get_point(0);
    const Vector xax = plane0.base1();
    const Vector yax = plane0.base2();
    const double scale = static_cast<double>(config::CLIPPER_SCALE);

    const Clipper2Lib::Paths64 subject{outline_to_clipper_path(outline0, origin, xax, yax, scale)};
    const Clipper2Lib::Paths64 clip{outline_to_clipper_path(outline1, origin, xax, yax, scale)};
    const Clipper2Lib::Paths64 solution = Clipper2Lib::Intersect(subject, clip, Clipper2Lib::FillRule::NonZero);

    if (solution.empty())
        return false;

    const Clipper2Lib::Path64* best = nullptr;
    double best_area = -1.0;
    for (const Clipper2Lib::Path64& path : solution) {
        const double a = std::abs(Clipper2Lib::Area(path));
        if (a > best_area) {
            best_area = a;
            best = &path;
        }
    }

    const Clipper2Lib::Path64 cleaned = Clipper2Lib::SimplifyPath(*best, scale / 1024.0, true);
    const size_t nc = cleaned.size();

    if (nc < 3)
        return false;

    if (nc == 3 && !include_triangles)
        return false;

    if (std::abs(Clipper2Lib::Area(cleaned)) / (scale * scale) <= config::CLIPPER_AREA)
        return false;

    std::vector<Point> pts;
    pts.reserve(nc + 1);
    for (const Clipper2Lib::Point64& q : cleaned) {
        const double u = static_cast<double>(q.x) / scale;
        const double v = static_cast<double>(q.y) / scale;
        pts.push_back(origin + xax * u + yax * v);
    }
    pts.push_back(pts.front());

    out_area = Polyline(pts);
    return true;
}

std::vector<ContactFace> face_contacts_for_pair(
    Element& ea,
    Element& eb,
    double cos_angle,
    double coplanar_tolerance,
    FeaturePlate* trace) {

    const std::vector<Plane> planes_a = planes_of(ea);
    const std::vector<Plane> planes_b = planes_of(eb);
    const std::vector<Polyline> outlines_a = outlines_of(ea);
    const std::vector<Polyline> outlines_b = outlines_of(eb);

    std::vector<ContactFace> contacts;
    for (size_t i = 0; i < planes_a.size(); ++i) {
        for (size_t j = 0; j < planes_b.size(); ++j) {

            if (!faces_coplanar(planes_a[i], planes_b[j], cos_angle, coplanar_tolerance))
                continue;

            if (trace)
                trace->dbg_coplanar++;

            Polyline polygon;
            const bool triangles = outer_face(ea, i) && outer_face(eb, j);
            if (!face_overlap_area(outlines_a[i], outlines_b[j], planes_a[i], triangles, polygon)) {
                if (TRACE && trace)
                    trace->dbg_fail_reason = fmt::format("bool_empty f({},{})", i, j);
                continue;
            }

            if (trace)
                trace->dbg_boolean++;

            contacts.emplace_back(static_cast<int>(i), static_cast<int>(j), contact_type(ea, i, eb, j), std::move(polygon));
        }
    }

    return contacts;
}

std::vector<std::tuple<int, int, ContactFace>> face_contacts(
    const std::vector<std::shared_ptr<Element>>& elements,
    const std::vector<std::string>& names,
    double inflate,
    double angle,
    double coplanar_tolerance) {

    std::vector<std::tuple<int, int, ContactFace>> contacts;
    const double cos_angle = std::cos(angle);

    for (const auto& [ia, ib] : adjacency_search(elements, inflate, names))
        for (ContactFace& face : face_contacts_for_pair(*elements[ia], *elements[ib], cos_angle, coplanar_tolerance))
            contacts.emplace_back(ia, ib, std::move(face));

    return contacts;
}

// ═══════════════════════════════════════════════════════════════════════════
// Cross contacts
// ═══════════════════════════════════════════════════════════════════════════

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
    ContactCross& result,
    double angle_tol,
    const std::array<double, 3>& extension) {

    result.faces_a = {-1, -1};
    result.faces_b = {-1, -1};

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

    result.faces_a[0] = e0_0__e1_0.first + 2;
    result.faces_b[0] = e0_0__e1_0.second + 2;
    result.faces_a[1] = e0_1__e1_1.first + 2;
    result.faces_b[1] = e0_1__e1_1.second + 2;

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
            result.polygon))
        return false;

    const Polyline& jpts = result.polygon;
    result.lines[0] = Polyline({Point::mid_point(jpts[0], jpts[1]), Point::mid_point(jpts[2], jpts[3])});
    result.lines[1] = Polyline({Point::mid_point(jpts[1], jpts[2]), Point::mid_point(jpts[3], jpts[0])});

    result.volumes[0] = result.polygon.translated(v);
    result.volumes[1] = result.polygon.translated(-v);

    if (extension[0] != 0.0 || extension[1] != 0.0) {
        for (int k = 0; k < 2; k++) {
            Polyline& pl = result.volumes[k];
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
    ContactCross& result,
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
