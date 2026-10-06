// The timber floor, in the order it is built, every part declared in floor.h:
//   geometry helpers - planes, polylines and polygons the rest is made of
//   the guide        - corners, validity, the shared edges, seams, oculus and columns, then every quarter's planes,
//                      quads, run-ins and parabolas, the central panel, and the drawing
//   member outlines  - ribs, beams, wedges, t-sections, beds, the oculus ring and the column cutters
//   the report       - the relations the design relies on, measured
//   the model        - outlines into elements, placed and grouped as a Floor
//   relationships    - what every two members share, and the screws
//   checks           - contacts against the kernel's search, screw clearances, BReps
#include "pch.h"
#include "src/templates/floor/floor.h"
#include "closest.h"
#include "convex_hull.h"
#include "wood_brep_drill.h"
#include "wood_element_geometry.h"
#include <chrono>
#include <numeric>

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// GEOMETRY HELPERS
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor::geometry {

const double TOLERANCE = 1e-9; // parallel planes and lines below it have no intersection
const double EXTENSION = 1000.0; // how far parabola ends are pushed out before the panel planes trim them

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════


Plane level(double z) {
    return Plane::xy_plane() + Vector(0.0, 0.0, z);
}

Plane edge_plane(const Line& edge, const Vector& normal_z) {
    return Plane::from_point_normal(edge.center(), edge.to_direction().cross(normal_z));
}

// ═══════════════════════════════════════════════════════════════════════════
// Intersections
// ═══════════════════════════════════════════════════════════════════════════

std::optional<Line> plane_plane(const Plane& plane0, const Plane& plane1) {

    Line line;

    if (!Intersection::plane_plane(plane0, plane1, line))
        return std::nullopt;

    return line.to_vector().dot(plane0.z_axis().cross(plane1.z_axis())) < 0.0 ? Line::from_points(line.end(), line.start()) : line;
}

std::optional<Point> line_plane(const Line& line, const Plane& plane) {

    Point point;

    if (std::abs(plane.z_axis().dot(line.to_direction())) <= TOLERANCE || !Intersection::line_plane(line, plane, point, false))
        return std::nullopt;

    return point;
}

std::optional<Point> plane_plane_plane(const Plane& plane0, const Plane& plane1, const Plane& plane2) {

    Point point;

    if (!Intersection::plane_plane_plane(plane0, plane1, plane2, point))
        return std::nullopt;

    return point;
}

// ═══════════════════════════════════════════════════════════════════════════
// Polylines
// ═══════════════════════════════════════════════════════════════════════════


Polyline trim(const Polyline& polyline, const Plane& plane0, const Plane& plane1) {

    std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();
    const Point middle = polyline.center();

    pts[0] = pts[0] + (pts[0] - pts[1]).normalized() * EXTENSION;
    pts[n - 1] = pts[n - 1] + (pts[n - 1] - pts[n - 2]).normalized() * EXTENSION;

    // each cut keeps the side of the original's middle, not the extended one's, which a short member puts past the second plane
    return Polyline(pts).cut_by_plane(plane0, signed_distance(middle, plane0) >= 0.0).cut_by_plane(plane1, signed_distance(middle, plane1) >= 0.0);
}

/// The points pushed out at both ends by EXTENSION.
static std::vector<Point> extended(const Polyline& polyline) {

    std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();
    pts[0] = pts[0] + (pts[0] - pts[1]).normalized() * EXTENSION;
    pts[n - 1] = pts[n - 1] + (pts[n - 1] - pts[n - 2]).normalized() * EXTENSION;

    return pts;
}

/// The first segment of the points the plane crosses.
static size_t crossed(const std::vector<Point>& pts, const Plane& plane) {

    for (size_t i = 0; i + 1 < pts.size(); i++)
        if ((signed_distance(pts[i], plane) >= 0.0) != (signed_distance(pts[i + 1], plane) >= 0.0))
            return i;

    throw std::runtime_error("trim_alike: a plane misses the extended polyline");
}

std::vector<Polyline> trim_alike(const std::vector<Polyline>& polylines, const Plane& plane0, const Plane& plane1) {

    const std::vector<Point> first = extended(polylines[0]);
    const size_t a = crossed(first, plane0);
    const size_t b = crossed(first, plane1);
    const std::array<size_t, 2> ends = {std::min(a, b), std::max(a, b)};
    const std::array<const Plane*, 2> planes = a <= b ? std::array<const Plane*, 2>{&plane0, &plane1} : std::array<const Plane*, 2>{&plane1, &plane0};
    std::vector<Polyline> trimmed;

    for (const Polyline& polyline : polylines) {

        if (polyline.point_count() != polylines[0].point_count())
            throw std::invalid_argument("trim_alike: polylines of different vertex counts");

        const std::vector<Point> pts = extended(polyline);
        std::vector<Point> kept = {line_plane(Line::from_points(pts[ends[0]], pts[ends[0] + 1]), *planes[0]).value()};
        kept.insert(kept.end(), pts.begin() + ends[0] + 1, pts.begin() + ends[1] + 1);
        kept.push_back(line_plane(Line::from_points(pts[ends[1]], pts[ends[1] + 1]), *planes[1]).value());
        trimmed.push_back(Polyline(kept));
    }

    return trimmed;
}

Polyline offset_polyline(const Polyline& polyline, double distance) {

    const std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();
    const Vector z(0.0, 0.0, 1.0);

    std::vector<Plane> planes = {Plane::from_point_normal(pts[0], pts[1] - pts[0])};

    for (size_t i = 0; i + 1 < n; i++) {
        const Line line = Line::from_points(pts[i], pts[i + 1]);
        const Vector x = line.to_direction();
        const Vector y = z.cross(x);
        planes.push_back(Plane::from_point_normal(line.center(), x.cross(y)).translate_by_normal(distance));
    }

    planes.push_back(Plane::from_point_normal(pts[n - 1], pts[n - 2] - pts[n - 1]));

    const Plane base = Plane::from_point_normal(pts[0], z.cross(pts[n - 1] - pts[0]));

    std::vector<Point> result;

    for (size_t i = 0; i + 1 < planes.size(); i++)
        result.push_back(plane_plane_plane(planes[i], planes[i + 1], base).value());

    return Polyline(result);
}

std::vector<Point> open_points(const Polyline& polyline) {

    std::vector<Point> points = polyline.get_points();

    if (polyline.is_closed())
        points.pop_back();

    return points;
}

double signed_distance(const Point& point, const Plane& plane) {
    return (point - plane.origin()).dot(plane.z_axis());
}

double end_level(const Outline& outline, const Plane& end) {

    double level = 0.0;

    for (const Polyline& loop : {outline.top, outline.bottom})
        for (const Point& point : loop.get_points())
            if (std::abs(signed_distance(point, end)) <= 1e-6)
                level = std::min(level, point[2]);

    return level;
}

Point area_centroid(const Polyline& polyline) {

    const std::vector<Point> points = open_points(polyline);
    const Vector normal = wood_session::compute_newell(points).normalized();
    const Point origin = points[0];
    Vector sum(0.0, 0.0, 0.0);
    double area = 0.0;

    for (size_t i = 1; i + 1 < points.size(); i++) {
        const double weight = (points[i] - origin).cross(points[i + 1] - origin).dot(normal);
        sum += ((points[i] - origin) + (points[i + 1] - origin)) * (weight / 3.0);
        area += weight;
    }

    return origin + sum / area;
}

double polygon_area(const Polyline& polyline) {

    const std::vector<Point> points = open_points(polyline);
    Vector twice(0.0, 0.0, 0.0);

    for (size_t i = 1; i + 1 < points.size(); i++)
        twice += (points[i] - points[0]).cross(points[i + 1] - points[0]);

    return 0.5 * twice.magnitude();
}




std::vector<Point> overlap(const Polyline& a, const Polyline& b, const Plane& plane) {

    const std::vector<Point> loop = open_points(a);
    const std::vector<Polyline> shared = Polyline::boolean_op(a, b, plane, 0);

    if (shared.empty() || std::abs(polygon_area(shared.front()) - polygon_area(a)) <= 1e-6 * polygon_area(a))
        return loop;

    // the overlap in a's winding, from the corner nearest a's first, so a contact's top edge and normal read as a's do
    std::vector<Point> points = open_points(shared.front());

    if (wood_session::compute_newell(points).dot(wood_session::compute_newell(loop)) < 0.0)
        std::reverse(points.begin(), points.end());

    const auto nearest = std::min_element(points.begin(), points.end(), [&loop](const Point& p, const Point& q) { return p.distance(loop[0]) < q.distance(loop[0]); });
    std::rotate(points.begin(), nearest, points.end());

    return points;
}

std::vector<Point> above(const std::vector<Point>& points, double z) {

    std::vector<Point> result;
    const size_t n = points.size();

    for (size_t i = 0; i < n; i++) {
        const Point& a = points[i];
        const Point& b = points[(i + 1) % n];
        const bool a_in = a[2] >= z;
        const bool b_in = b[2] >= z;

        if (a_in)
            result.push_back(a);

        if (a_in != b_in)
            result.push_back(a + (b - a) * ((z - a[2]) / (b[2] - a[2])));
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

MemberRef quarter_member(size_t quarter, Family family, size_t index) {
    return MemberRef{static_cast<int>(quarter), family, index, -1};
}

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

Outline loft_planes(const std::vector<Plane>& planes, const Plane& bottom, const Plane& top, bool flip) {

    const size_t n = planes.size();
    std::vector<Point> pts_bottom;
    std::vector<Point> pts_top;

    for (size_t i = 0; i < n; i++) {
        const std::optional<Point> rb = plane_plane_plane(planes[i], planes[(i + 1) % n], bottom);
        const std::optional<Point> rt = plane_plane_plane(planes[i], planes[(i + 1) % n], top);

        if (rb)
            pts_bottom.push_back(*rb);

        if (rt)
            pts_top.push_back(*rt);
    }

    Outline outline{Polyline(pts_top).closed(), Polyline(pts_bottom).closed()};

    if (flip)
        std::swap(outline.top, outline.bottom);

    return outline;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE GUIDE: PLAN, CORNERS AND DRAWING
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

using namespace wood_floor::geometry;

// ═══════════════════════════════════════════════════════════════════════════
// Parameters
// ═══════════════════════════════════════════════════════════════════════════

double FloorGuide::static_h() const {
    return height - rise;
}

// ═══════════════════════════════════════════════════════════════════════════
// Guide
// ═══════════════════════════════════════════════════════════════════════════

FloorGuide FloorGuide::rectangle(double half_x, double half_y) {
    return FloorGuide({
        Point(-half_x, -half_y, 0.0), 
        Point(half_x, -half_y, 0.0), 
        Point(half_x, half_y, 0.0), 
        Point(-half_x, half_y, 0.0)
    });
}

Point FloorGuide::midpoint(size_t k) const {
    return Line::from_points(corners[k % 4], corners[(k + 1) % 4]).center();
}

double FloorGuide::corner_angle(size_t k) const {
    return (corners[(k + 1) % 4] - corners[k % 4]).angle(corners[(k + 3) % 4] - corners[k % 4], false);
}

double FloorGuide::oculus_corner_angle(size_t k) const {
    return (oculus_corners[(k + 3) % 4] - oculus_corners[k % 4]).angle(oculus_corners[(k + 1) % 4] - oculus_corners[k % 4], false);
}

double FloorGuide::oculus_seam_angle(size_t k) const {
    return (midpoint(k) - centre).angle(oculus_corners[(k + 1) % 4] - oculus_corners[k % 4], false);
}

/// One quarter family as the guide draws it: its plan quads and face planes in member order and the index of its first rib parabola, -1 for none.
struct DrawnFamily {
    Family family;
    const std::vector<Polyline>& quads;
    const std::vector<std::array<Plane, 2>>& planes;
    int parabola;
};

void FloorGuide::draw() {

    for (size_t q = 0; q < 4; q++) {
        const std::string suffix = fmt::format("_{}", q);
        const QuarterGeometry& drawn = geometry[q];
        const std::shared_ptr<TreeNode> quarter = wood_floor::add_group(*this, "quarter" + suffix, nullptr);
        const std::shared_ptr<TreeNode> plan = wood_floor::add_group(*this, "plan" + suffix, quarter);

        const auto line = [this](Polyline polyline, const std::string& name, const Color& color, double width, const std::shared_ptr<TreeNode>& group) {
            polyline.name = name;
            polyline.linecolor = color;
            polyline.width = width;
            add_polyline(polyline, group);
        };

        line(Polyline(drawn.polygon).closed(), "polygon" + suffix, Color::black(), 3.0, plan);
        line(Polyline(columns[q].head).closed(), "column_head" + suffix, Color::black(), 3.0, plan);
        Point corner = oculus_corners[q];
        corner.name = "oculus_corner" + suffix;
        corner.width = 10.0;
        add_point(corner, plan);

        const std::array<DrawnFamily, 5> families = {{
            {Family::outer_ribs, drawn.quads.outer_ribs, drawn.planes.outer_ribs, 0},
            {Family::inner_ribs, drawn.quads.inner_ribs, drawn.planes.inner_ribs, 2},
            {Family::inner_beams, drawn.quads.inner_beams, drawn.planes.inner_beams, -1},
            {Family::wedges, drawn.quads.wedges, drawn.planes.wedges, -1},
            {Family::tsections, drawn.quads.tsections, drawn.planes.tsections, -1},
        }};

        for (const DrawnFamily& family : families) {
            const std::string& name = FAMILY_NAMES[static_cast<size_t>(family.family)];
            const Color& color = FAMILY_COLORS[static_cast<size_t>(family.family)];
            const std::shared_ptr<TreeNode> group = wood_floor::add_group(*this, name + suffix, quarter);

            for (size_t i = 0; i < family.quads.size(); i++) {
                const std::shared_ptr<TreeNode> member = wood_floor::add_group(*this, fmt::format("{}_{}{}", name, i, suffix), group);
                line(family.quads[i].closed(), "quad", color, 2.0, member);

                for (size_t side = 0; side < 2; side++) {
                    Plane face = family.planes[i][side];
                    face.name = fmt::format("face_{}", side);
                    face.linecolor = color;
                    add_plane(face, member);
                }

                if (family.parabola < 0)
                    continue;

                const std::array<Polyline, 3>& parabola = drawn.parabolas[static_cast<size_t>(family.parabola) + i];
                line(parabola[0], "soffit", color, 2.0, member);
                line(parabola[1], "tsections_top", color, 1.0, member);
                line(parabola[2], "beds_top", color, 1.0, member);
            }
        }
    }
}

namespace geometry {

std::string invalid(const FloorGuide& guide) {

    const std::array<Point, 4>& corners = guide.corners;

    if (guide.rise <= 0.0 || guide.rise >= guide.height)
        return fmt::format("rise {:.3f} is not between 0 and height {:.3f}: the ribs need a parabola and a depth at the seam", guide.rise, guide.height);

    for (size_t k = 0; k < 4; k++) {
        const Vector after = corners[(k + 1) % 4] - corners[k];
        const Vector before = corners[(k + 3) % 4] - corners[k];
        const double turn = after.cross(corners[(k + 2) % 4] - corners[(k + 1) % 4])[2];

        if (std::abs(corners[k][2]) > 0.0)
            return fmt::format("corner {} is not at z 0", k);

        if (turn <= 0.0)
            return fmt::format("the corners are not counter-clockwise and convex at corner {}", (k + 1) % 4);

        if (after.magnitude() <= 0.0 || before.magnitude() <= 0.0)
            return fmt::format("corner {} repeats its neighbour", k);

        const double along = (guide.oculus_corners[k] - guide.centre).dot((guide.midpoint(k) - guide.centre).normalized());

        if (along <= 0.0 || along >= (guide.midpoint(k) - guide.centre).magnitude())
            return fmt::format("oculus corner {} is not between the centre and the midpoint of edge {}", k, k);
    }

    for (size_t k = 0; k < 4; k++)
        if (std::sin(guide.oculus_corner_angle(k) * M_PI / 180.0) < std::sin(guide.oculus_seam_angle(k) * M_PI / 180.0))
            return fmt::format("the ring beam leaves quarter {}'s oculus beam face uncovered at oculus corner {}: corner angle {:.3f}, seam angle {:.3f} degrees", (k + 1) % 4, k, guide.oculus_corner_angle(k), guide.oculus_seam_angle(k));

    return "";
}

}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE GUIDE: COMPUTATION
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

using namespace wood_floor::geometry;

const double RIGHT_ANGLE = 1e-9; // degrees off 90 within which a corner counts as right, so a rectangle keeps its exact edge directions
const double RUN_IN_TOLERANCE = 1e-11; // mm an outer rib's end may sit off its corner's shared level; within it the rib keeps the wedge as its run-in
const size_t RUN_IN_STEPS = 50; // secant steps a run-in may take to land its rib's end on the shared level

/// A plane pair: the plane and its copy moved by distance along the normal.
static std::array<Plane, 2> pair(const Plane& plane, double distance) {
    return {plane, plane.translate_by_normal(distance)};
}

/// The plane moved to a new origin with its axes kept, so its normal and offset read the same bits.
static Plane reoriginated(const Plane& plane, const Point& origin) {
    return Plane::from_frame(origin, plane.x_axis(), plane.y_axis(), plane.z_axis());
}

/// The plan quad of four planes at z 0: corners 3-0, 0-1, 1-2 and 2-3.
static Polyline quad(const std::array<Plane, 4>& planes) {

    const Plane xy = level(0.0);

    return Polyline({
        plane_plane_plane(xy, planes[0], planes[3]).value(),
        plane_plane_plane(xy, planes[0], planes[1]).value(),
        plane_plane_plane(xy, planes[1], planes[2]).value(),
        plane_plane_plane(xy, planes[2], planes[3]).value(),
    });
}

/// The plan quads of a family of quad planes.
static std::vector<Polyline> quads(const std::vector<std::array<Plane, 4>>& family) {

    std::vector<Polyline> result;

    for (const std::array<Plane, 4>& planes : family)
        result.push_back(quad(planes));

    return result;
}

/// The plane fitted to the deepest quad of a bed panel: its top layer on the panel's two side planes, each cut there by the panel planes.
static Plane panel_top_plane(const std::array<Polyline, 2>& faces, const Plane& cut_plane0, const Plane& cut_plane1) {

    std::array<std::vector<Point>, 2> pts = {trim(faces[0], cut_plane0, cut_plane1).get_points(), trim(faces[1], cut_plane0, cut_plane1).get_points()};

    if (pts[0].front()[2] > pts[0].back()[2]) {
        std::reverse(pts[0].begin(), pts[0].end());
        std::reverse(pts[1].begin(), pts[1].end());
    }

    const Plane plane = Plane::from_points_pca({pts[0][0], pts[0][1], pts[1][0], pts[1][1]});
    const Vector normal = plane.z_axis()[2] < 0.0 ? -plane.z_axis() : plane.z_axis();

    return Plane::from_point_normal(plane.origin(), normal);
}

// ═══════════════════════════════════════════════════════════════════════════
// Shared entities
// ═══════════════════════════════════════════════════════════════════════════

/// Bay edge k: the line from corner k to corner k + 1, its midpoint and the band of the edge plane, normal into the bay, with its offset by outer_ribs.
static BayEdge bay_edge(const FloorGuide& guide, size_t k) {

    BayEdge edge;
    edge.line = Line::from_points(guide.corners[k], guide.corners[(k + 1) % 4]);
    edge.midpoint = guide.midpoint(k);
    edge.band = pair(Plane::from_point_normal(edge.midpoint, edge.line.to_direction().cross(-Vector::z_axis())), guide.outer_ribs);

    return edge;
}

/// Seam k: the line from the midpoint of edge k to the centre, its plane into quarter k at the half edge up to the oculus corner, and the beam thickness.
static Seam seam(const FloorGuide& guide, size_t k, const Point& centre, const Point& oculus_corner) {

    Seam result;
    result.index = k;
    result.line = Line::from_points(guide.midpoint(k), centre);
    result.oculus_corner = oculus_corner;
    result.thickness = guide.inner_beams;
    result.plane = result.plane_into(k);

    return result;
}

/// Oculus edge q from oculus corner q to oculus corner q - 1: its tilted bearing plane, the back face offset into the quarter and the ring's inner plane.
static OculusEdge oculus_edge(const Point& corner, const Point& previous, const FloorGuide& guide) {

    OculusEdge edge;
    edge.line = Line::from_points(corner, previous);
    const Plane plane = edge_plane(edge.line, -Vector::z_axis());
    edge.tilted = plane.transformed(Xform::rotation_around_line(Line::from_points(edge.line.center(), edge.line.center() + edge.line.to_direction()), -guide.oculus_plane_angle * M_PI / 180.0));
    edge.back = plane.translate_by_normal(guide.inner_beams);
    edge.ring_inner = edge.back.translate_by_normal(-guide.inner_beams * 2.0);

    return edge;
}

/// The corner frame of column k: the edge directions at a right corner, the two axes symmetric about the corner bisector otherwise.
static std::array<Vector, 2> corner_frame(const FloorGuide& guide, size_t k) {

    const Vector after = (guide.corners[(k + 1) % 4] - guide.corners[k]).normalized();
    const Vector before = (guide.corners[(k + 3) % 4] - guide.corners[k]).normalized();

    if (std::abs(guide.corner_angle(k) - 90.0) <= RIGHT_ANGLE)
        return {after, before};

    const Vector bisector = (after + before).normalized();
    const Xform to_x = Xform::rotation(Vector::z_axis(), -45.0, true);
    const Xform to_y = Xform::rotation(Vector::z_axis(), 45.0, true);

    return {bisector.transformed(to_x), bisector.transformed(to_y)};
}

/// Column corner k before its fan: frame, head polygon, chamfer direction, the head's boundary sides, levels, axis and support plane.
static ColumnCorner column_corner(const FloorGuide& guide, size_t k) {

    ColumnCorner column;
    column.corner = guide.corners[k];
    const std::array<Vector, 2> frame = corner_frame(guide, k);
    column.x_axis = frame[0];
    column.y_axis = frame[1];

    const Vector& x = column.x_axis;
    const Vector& y = column.y_axis;
    const double head = guide.column_head;
    const double chamfer = guide.column_head_chamfer;
    column.head = {column.corner, column.corner + x * head, column.corner + x * head + y * chamfer, column.corner + x * chamfer + y * head, column.corner + y * head};
    column.chamfer_direction = (column.head[3] - column.head[2]).normalized();
    column.sides = {edge_plane(Line::from_points(column.head[0], column.head[1]), -Vector::z_axis()), edge_plane(Line::from_points(column.head[4], column.head[0]), -Vector::z_axis())};
    column.levels = {0.0, 0.0, -guide.column_head_depth};
    column.axis_point = column.corner + (x + y) * (head * 0.5);
    column.support_plane = Plane::from_frame(column.axis_point, x, y, Vector::z_axis());
    column.axis = Line::from_points(column.axis_point, column.axis_point + Vector::z_axis() * guide.bay_height);

    return column;
}

/// The wedge fan of a column corner: the tilted chamfer plane and the two side planes through the head edges, each leaning parallel to the chamfer plane's crease with the inner rib's central face.
static std::array<std::array<Plane, 2>, 3> wedge_fan(const ColumnCorner& column, const ConstructionPlanes& cp, const FloorGuide& guide) {

    const Line side0 = Line::from_points(column.head[1], column.head[2]);
    const Line side1 = Line::from_points(column.head[2], column.head[3]);
    const Line side2 = Line::from_points(column.head[3], column.head[4]);

    const Plane tilted = edge_plane(side1, Vector::z_axis()).transformed(Xform::rotation_around_line(Line::from_points(side1.center(), side1.center() + side1.to_direction()), guide.wedge_plane_angle * M_PI / 180.0));
    const Line line0 = plane_plane(cp.inner_ribs[0][1], tilted).value();
    const Line line1 = plane_plane(cp.inner_ribs[1][1], tilted).value();
    const Plane wedge0 = Plane::from_point_normal(side0.center(), line0.to_direction().cross(side0.to_direction()));
    const Plane wedge2 = Plane::from_point_normal(side2.center(), (-line1.to_direction()).cross(side2.to_direction()));

    return {pair(wedge0, guide.wedge), pair(tilted, guide.wedge * guide.middle_wedge_factor), pair(wedge2, guide.wedge)};
}

/// The signed offset of the column's outer faces from the bay edges and the three wedge seats left on the head beyond the rib bands (R8).
static void column_seats(ColumnCorner& column, const FloorGuide& guide, size_t k, const ConstructionPlanes& cp) {

    const double phi = (guide.corner_angle(k) - 90.0) * 0.5 * M_PI / 180.0;
    const double offset = guide.column_head * std::sin(phi);
    const double band = (guide.outer_ribs - offset) / std::cos(phi);
    column.column_offset = {offset, offset};

    const double chamfer_length = (column.head[3] - column.head[2]).magnitude();
    const double sin0 = std::abs(cp.inner_ribs[0][0].z_axis().dot(column.chamfer_direction));
    const double sin1 = std::abs(cp.inner_ribs[1][0].z_axis().dot(column.chamfer_direction));
    column.wedge_seat = {guide.column_head_chamfer - band, chamfer_length - guide.inner_ribs / sin0 - guide.inner_ribs / sin1, guide.column_head_chamfer - band};
}

// ═══════════════════════════════════════════════════════════════════════════
// Quarter geometry
// ═══════════════════════════════════════════════════════════════════════════

/// The member planes of quarter q from the shared entities, every one at the quarter's own points: the bands at the half-edge midpoints, the seams at the half seams, the oculus edge and the column fan as they are.
static ConstructionPlanes construction_planes(const FloorGuide& guide, size_t q, ColumnCorner& column) {

    const std::vector<Point>& polygon = guide.geometry[q].polygon;
    ConstructionPlanes cp;

    const Plane outer0 = reoriginated(guide.edges[q].band[0], Line::from_points(polygon[0], polygon[1]).center());
    const Plane outer1 = reoriginated(guide.edges[(q + 3) % 4].band[0], Line::from_points(polygon[4], polygon[0]).center());
    cp.outer_ribs = {pair(outer0, guide.outer_ribs), pair(outer1, guide.outer_ribs)};

    const OculusEdge& oculus = guide.oculus_edges[q];
    cp.inner_beams = {guide.seams[q].faces_into(q), {oculus.tilted, oculus.back}, guide.seams[(q + 3) % 4].faces_into(q)};

    const Plane xy = level(0.0);
    const Point p0 = plane_plane_plane(xy, cp.inner_beams[0][1], cp.inner_beams[1][1]).value();
    const Point p1 = plane_plane_plane(xy, cp.inner_beams[1][1], cp.inner_beams[2][1]).value();
    const Point p2 = column.head[2];
    const Point p3 = column.head[3];
    const Plane rib0 = Plane::from_point_normal(p2 + (p0 - p2) * 0.5, (p0 - p2).cross(-Vector::z_axis()));
    const Plane rib1 = Plane::from_point_normal(p3 + (p1 - p3) * 0.5, (p1 - p3).cross(Vector::z_axis()));
    cp.inner_ribs = {pair(rib0, guide.inner_ribs), pair(rib1, guide.inner_ribs)};

    column.wedge_fan = wedge_fan(column, cp, guide);
    cp.wedges = {column.wedge_fan[0], column.wedge_fan[1], column.wedge_fan[2]};

    cp.tsections = {
        pair(cp.outer_ribs[0][1], guide.tsections),
        pair(cp.inner_ribs[0][0], -guide.tsections),
        pair(cp.inner_ribs[0][1], guide.tsections),
        pair(cp.inner_ribs[1][1], guide.tsections),
        pair(cp.inner_ribs[1][0], -guide.tsections),
        pair(cp.outer_ribs[1][1], guide.tsections),
    };

    return cp;
}

/// The plan quad of every member at z 0.
static ConstructionQuads construction_quads(const ConstructionPlanes& cp) {

    ConstructionQuads result;

    result.outer_ribs = quads({
        {cp.outer_ribs[0][0], cp.inner_beams[0][0], cp.outer_ribs[0][1], cp.wedges[0][0]},
        {cp.outer_ribs[1][0], cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.wedges[2][0]},
    });
    result.inner_beams = quads({
        {cp.inner_beams[0][0], cp.inner_beams[1][0], cp.inner_beams[0][1], cp.outer_ribs[0][1]},
        {cp.inner_beams[1][0], cp.inner_beams[2][1], cp.inner_beams[1][1], cp.inner_beams[0][1]},
        {cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.inner_beams[2][1], cp.inner_beams[1][0]},
    });
    result.inner_ribs = quads({
        {cp.inner_ribs[0][1], cp.inner_beams[1][1], cp.inner_ribs[0][0], cp.wedges[1][0]},
        {cp.inner_ribs[1][1], cp.inner_beams[1][1], cp.inner_ribs[1][0], cp.wedges[1][0]},
    });
    result.wedges = quads({
        {cp.wedges[0][0], cp.outer_ribs[0][1], cp.wedges[0][1], cp.inner_ribs[0][0]},
        {cp.wedges[1][0], cp.inner_ribs[0][1], cp.wedges[1][1], cp.inner_ribs[1][1]},
        {cp.wedges[2][0], cp.inner_ribs[1][0], cp.wedges[2][1], cp.outer_ribs[1][1]},
    });
    result.tsections = quads({
        {cp.tsections[0][0], cp.inner_beams[0][1], cp.tsections[0][1], cp.wedges[0][1]},
        {cp.tsections[1][0], cp.inner_beams[0][1], cp.tsections[1][1], cp.wedges[0][1]},
        {cp.tsections[2][0], cp.inner_beams[1][1], cp.tsections[2][1], cp.wedges[1][1]},
        {cp.tsections[3][0], cp.inner_beams[1][1], cp.tsections[3][1], cp.wedges[1][1]},
        {cp.tsections[4][0], cp.inner_beams[2][1], cp.tsections[4][1], cp.wedges[2][1]},
        {cp.tsections[5][0], cp.inner_beams[2][1], cp.tsections[5][1], cp.wedges[2][1]},
    });

    return result;
}

/// The outer parabola over a rib quad, a 7-point Bezier: from -height at the run-in along the axis past the fan plane's datum trace, controlled at the axis midpoint at -static_h, to the seam at -static_h.
static Polyline outer_parabola(const Polyline& quad, double run_in, const FloorGuide& guide) {

    const Point start = quad.get_point(0);
    const Point end = quad.get_point(1);
    const Point trimmed = start + (end - start).normalized() * run_in;
    const Point middle = trimmed + (end - trimmed) * 0.5;

    return Polyline::quadratic_points(trimmed + Vector(0.0, 0.0, -guide.height), middle + Vector(0.0, 0.0, -guide.static_h()), end + Vector(0.0, 0.0, -guide.static_h()));
}

/// The z where an outer rib's soffit, its first chord extended, meets its fan plane: the bottom of the rib's column end face.
static double fan_end(const Polyline& quad, double run_in, const Plane& fan, const Plane& seam, const FloorGuide& guide) {

    const std::vector<Point> pts = trim(outer_parabola(quad, run_in, guide), fan, seam).get_points();
    const double d0 = std::abs((pts.front() - fan.origin()).dot(fan.z_axis()));
    const double d1 = std::abs((pts.back() - fan.origin()).dot(fan.z_axis()));

    return d0 > d1 ? pts.back()[2] : pts.front()[2];
}

/// The run-in that lands an outer rib's end on the level, by the secant from the wedge; throws when it leaves the axis or does not converge.
static double run_in_to_level(const Polyline& quad, const Plane& fan, const Plane& seam, double level, const FloorGuide& guide) {

    const double axis = (quad.get_point(1) - quad.get_point(0)).magnitude();
    double x0 = guide.wedge;
    double f0 = fan_end(quad, x0, fan, seam, guide) - level;

    if (std::abs(f0) <= RUN_IN_TOLERANCE)
        return x0;

    double x1 = x0 + 1.0;
    double f1 = fan_end(quad, x1, fan, seam, guide) - level;

    for (size_t i = 0; i < RUN_IN_STEPS; i++) {
        if (std::abs(f1) <= RUN_IN_TOLERANCE)
            return x1;

        const double x2 = x1 - f1 * (x1 - x0) / (f1 - f0);

        if (x2 <= 0.0 || x2 >= axis)
            throw std::runtime_error(fmt::format("an outer rib's run-in to the column level {:.3f} leaves its axis: {:.3f} of {:.3f} mm", level, x2, axis));

        x0 = x1;
        f0 = f1;
        x1 = x2;
        f1 = fan_end(quad, x1, fan, seam, guide) - level;
    }

    throw std::runtime_error(fmt::format("an outer rib's run-in to the column level {:.3f} did not converge: {:.3e} mm off", level, f1));
}

/// Per outer rib the run-in that lands its end on the corner's shared level, the shallower of the two ends at the wedge.
static std::array<double, 2> run_ins(const ConstructionPlanes& cp, const ConstructionQuads& quads, const FloorGuide& guide) {

    const std::array<Plane, 2> fans = {cp.wedges[0][0], cp.wedges[2][0]};
    const std::array<Plane, 2> seams = {cp.inner_beams[0][0], cp.inner_beams[2][0]};
    const double level = std::max(fan_end(quads.outer_ribs[0], guide.wedge, fans[0], seams[0], guide), fan_end(quads.outer_ribs[1], guide.wedge, fans[1], seams[1], guide));

    return {run_in_to_level(quads.outer_ribs[0], fans[0], seams[0], level, guide), run_in_to_level(quads.outer_ribs[1], fans[1], seams[1], level, guide)};
}

/// The column blocks' far planes over the ribs' run-ins: each side block its fan plane offset by its own rib's run-in, the middle block by middle_wedge_factor times their mean.
static void block_planes(ConstructionPlanes& cp, ColumnCorner& column, const std::array<double, 2>& run_in, const FloorGuide& guide) {

    const std::array<double, 3> thickness = {run_in[0], guide.middle_wedge_factor * (0.5 * (run_in[0] + run_in[1])), run_in[1]};

    for (size_t i = 0; i < 3; i++) {
        column.wedge_fan[i][1] = column.wedge_fan[i][0].translate_by_normal(thickness[i]);
        cp.wedges[i][1] = column.wedge_fan[i][1];
    }
}

/// Per rib axis (outer 0, outer 1, shadow 0, shadow 1) the parabola and its two offsets by tsections: the outer ones from the rib quads over their run-ins, the shadows projected onto the inner ribs' outer faces along the outer rib normals.
static std::vector<std::array<Polyline, 3>> boundary_parabolas(const ConstructionPlanes& cp, const ConstructionQuads& quads, const FloorGuide& guide, const std::array<double, 2>& run_in) {

    std::vector<std::array<Polyline, 3>> parabolas;

    for (size_t k = 0; k < 2; k++) {
        const Polyline parabola = outer_parabola(quads.outer_ribs[k], run_in[k], guide);
        parabolas.push_back({parabola, offset_polyline(parabola, guide.tsections), offset_polyline(parabola, 2.0 * guide.tsections)});
    }

    for (size_t i = 0; i < 2; i++) {
        const Xform projection = Xform::project_to_plane_by_axis(cp.inner_ribs[i][0], cp.outer_ribs[i][0].z_axis());
        const std::array<Polyline, 3>& outer = parabolas[i];
        parabolas.push_back({outer[0].transformed(projection), outer[1].transformed(projection), outer[2].transformed(projection)});
    }

    return parabolas;
}

/// Per bed panel, matching the three wedges, the plane fitted to its deepest quad, normal up: the outer panels' top layers projected along their outer rib normals, the central panel's read from its traces.
static std::vector<Plane> bed_top_planes(const ConstructionPlanes& cp, const std::vector<std::array<Polyline, 3>>& parabolas, const CentralPanel& panel) {

    const Xform side00 = Xform::project_to_plane_by_axis(cp.inner_ribs[0][0], cp.outer_ribs[0][0].z_axis());
    const Xform side01 = Xform::project_to_plane_by_axis(cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis());
    const Xform side20 = Xform::project_to_plane_by_axis(cp.inner_ribs[1][0], cp.outer_ribs[1][0].z_axis());
    const Xform side21 = Xform::project_to_plane_by_axis(cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis());

    return {
        panel_top_plane({parabolas[0][2].transformed(side00), parabolas[0][2].transformed(side01)}, cp.inner_beams[0][1], cp.wedges[0][0]),
        panel_top_plane({panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]),
        panel_top_plane({parabolas[1][2].transformed(side20), parabolas[1][2].transformed(side21)}, cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

/// Quarter q's geometry in dependency order: planes, quads, run-ins, the blocks' far planes over them and the quads again, parabolas and shadows, block levels, bed planes; the column fan and seats are written into its column.
static void compute_quarter(FloorGuide& guide, size_t q) {

    QuarterGeometry& geometry = guide.geometry[q];
    ColumnCorner& column = guide.columns[q];

    geometry.planes = construction_planes(guide, q, column);
    column_seats(column, guide, q, geometry.planes);
    geometry.quads = construction_quads(geometry.planes);
    geometry.run_in = run_ins(geometry.planes, geometry.quads, guide);
    block_planes(geometry.planes, column, geometry.run_in, guide);
    geometry.quads = construction_quads(geometry.planes);
    geometry.parabolas = boundary_parabolas(geometry.planes, geometry.quads, guide, geometry.run_in);
    geometry.central_panel = central_panel(geometry.planes, geometry.parabolas, guide);
    geometry.bed_top_planes = bed_top_planes(geometry.planes, geometry.parabolas, geometry.central_panel);
}

/// The middle cutter level of the quarter's column by the rib-bottom rule: the deeper of its two outer ribs' bottom corners on their fan planes, on either face.
static double rib_bottom_level(const Quarter& quarter) {

    double level = 0.0;

    for (const Outline& rib : quarter.outer_ribs())
        level = std::min({level, rib.top.get_point(2)[2], rib.bottom.get_point(2)[2]});

    return level;
}

/// Why the oculus is too close to a bay edge, empty when it is not: every quarter's inner beam corners, where the seam beams' far faces meet the oculus beam's back face at the datum, must lie inside the outer rib bands, or the inner ribs end inside the outer ribs.
static std::string beam_corners_in_bands(const FloorGuide& guide) {

    for (size_t q = 0; q < 4; q++) {
        const Plane& back = guide.oculus_edges[q].back;
        const std::array<std::pair<Point, size_t>, 2> corners = {{
            {plane_plane_plane(level(0.0), guide.seams[q].faces_into(q)[1], back).value(), q},
            {plane_plane_plane(level(0.0), back, guide.seams[(q + 3) % 4].faces_into(q)[1]).value(), (q + 3) % 4},
        }};

        for (const auto& [corner, k] : corners) {
            const double inside = signed_distance(corner, guide.edges[k].band[1]);

            if (inside <= 0.0)
                return fmt::format("quarter {}'s inner beam corner lies {:.3f} mm inside the outer rib band of edge {}: the oculus is too close to the bay edge", q, -inside, k);
        }
    }

    return "";
}

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

Plane Seam::plane_into(size_t quarter) const {

    const Point& midpoint = line.start();

    return quarter % 4 == index ? edge_plane(Line::from_points(midpoint, oculus_corner), -Vector::z_axis()) : edge_plane(Line::from_points(oculus_corner, midpoint), -Vector::z_axis());
}

std::array<Plane, 2> Seam::faces_into(size_t quarter) const {
    return pair(plane_into(quarter), thickness);
}

FloorGuide::FloorGuide(const std::array<Point, 4>& guide_corners) : wood_session::WoodSession("floor_guide"), corners(guide_corners) {
    compute();
}

void FloorGuide::compute() {

    // a recompute starts from an empty drawing
    static_cast<wood_session::WoodSession&>(*this) = wood_session::WoodSession("floor_guide");

    centre = Point::centroid({corners[0], corners[1], corners[2], corners[3]});

    for (size_t q = 0; q < 4; q++)
        oculus_corners[q] = centre + (midpoint(q) - centre).normalized() * oculus_radius;

    const std::string why = invalid(*this);

    if (!why.empty())
        throw std::invalid_argument("invalid floor guide: " + why);

    for (size_t q = 0; q < 4; q++) {
        edges[q] = bay_edge(*this, q);
        seams[q] = seam(*this, q, centre, oculus_corners[q]);
        oculus_edges[q] = oculus_edge(oculus_corners[q], oculus_corners[(q + 3) % 4], *this);
        columns[q] = column_corner(*this, q);
    }

    const std::string clash = beam_corners_in_bands(*this);

    if (!clash.empty())
        throw std::invalid_argument("invalid floor guide: " + clash);

    for (size_t q = 0; q < 4; q++) {
        geometry[q].polygon = {corners[q], edges[q].midpoint, oculus_corners[q], oculus_corners[(q + 3) % 4], edges[(q + 3) % 4].midpoint};
        compute_quarter(*this, q);
    }

    for (size_t q = 0; q < 4; q++)
        columns[q].levels[1] = rib_bottom_level(quarter(q));

    soffit = -static_h();

    for (size_t q = 0; q < 4; q++) {
        const Quarter view = quarter(q);
        const std::vector<Outline> outer = view.outer_ribs();
        const std::vector<Outline> inner = view.inner_ribs();

        for (size_t k = 0; k < 2; k++)
            soffit = std::min({soffit, end_level(outer[k], view.rib_seam_ends()[k]), end_level(inner[k], geometry[q].planes.inner_beams[1][1])});
    }

    draw();
}

Quarter FloorGuide::quarter(size_t q) const {
    return Quarter{*this, q % 4};
}

const QuarterGeometry& Quarter::geometry() const {
    return guide.geometry[index];
}

const ColumnCorner& Quarter::column() const {
    return guide.columns[index];
}

const OculusEdge& Quarter::oculus_edge() const {
    return guide.oculus_edges[index];
}

const Seam& Quarter::seam(size_t side) const {
    return guide.seams[side == 0 ? index : (index + 3) % 4];
}

const BayEdge& Quarter::edge(size_t side) const {
    return guide.edges[side == 0 ? index : (index + 3) % 4];
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE CENTRAL PANEL
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor::geometry {

const double SCAN_STEP = 0.5; // degrees between the sweep directions scanned for rule A's root
const double SCAN_RANGE = 85.0; // degrees either side of n0 - n1 the scan covers: the second root, where both ribs shift alike, lies at 90
const double GRAZING = 1e-3; // |n . r| below which a sweep runs along a rib face and is skipped
const size_t BISECTIONS = 200; // halvings of the bracket, far past the last bit of the angle

/// The vector flattened to the plan.
static Vector flat(const Vector& vector) {
    return Vector(vector[0], vector[1], 0.0);
}

/// The plan unit vector turned by degrees about z from the reference.
static Vector turned(const Vector& reference, double degrees) {

    const double a = degrees * M_PI / 180.0;
    const Vector x = flat(reference).normalized();

    return Vector(x[0] * std::cos(a) - x[1] * std::sin(a), x[0] * std::sin(a) + x[1] * std::cos(a), 0.0);
}

/// The farthest pair of same-index vertices of two polylines, mm.
static double largest_shift(const Polyline& a, const Polyline& b) {

    double shift = 0.0;

    for (size_t i = 0; i < std::min(a.point_count(), b.point_count()); i++)
        shift = std::max(shift, (a.get_point(i) - b.get_point(i)).magnitude());

    return shift;
}

// ═══════════════════════════════════════════════════════════════════════════
// Rule A
// ═══════════════════════════════════════════════════════════════════════════

/// The rib sweep's two shifts: how far along r each rib's outer face trace moves to reach its central face, thickness / (n . r).
static std::array<double, 2> shifts(const std::array<Vector, 2>& normals, double thickness, const Vector& r) {
    return {thickness / normals[0].dot(r), thickness / normals[1].dot(r)};
}

/// Rule A's closure for one sweep: the sine between the plan chords joining the two central traces at the start and at the vertex, zero when one ruling joins both.
static double closure(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& r) {

    const std::array<double, 2> a = shifts(normals, thickness, r);
    const size_t n = shadows[0].point_count() - 1;
    const Vector start = flat((shadows[1].get_point(0) + r * a[1]) - (shadows[0].get_point(0) + r * a[0]));
    const Vector vertex = flat((shadows[1].get_point(n) + r * a[1]) - (shadows[0].get_point(n) + r * a[0]));

    return start.cross(vertex)[2] / (start.magnitude() * vertex.magnitude());
}

/// Whether the sweep at degrees from the reference crosses both rib faces, and on which side of each.
static bool sweep_sides(const std::array<Vector, 2>& normals, const Vector& reference, double degrees, std::array<bool, 2>& sides) {

    const Vector r = turned(reference, degrees);
    sides = {normals[0].dot(r) > 0.0, normals[1].dot(r) > 0.0};

    return std::abs(normals[0].dot(r)) > GRAZING && std::abs(normals[1].dot(r)) > GRAZING;
}

/// The root of the closure between two scanned angles by bisection.
static double bisect(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& reference, double lo, double hi) {

    double f_lo = closure(shadows, normals, thickness, turned(reference, lo));

    for (size_t i = 0; i < BISECTIONS && lo != hi; i++) {
        const double mid = 0.5 * (lo + hi);
        const double f_mid = closure(shadows, normals, thickness, turned(reference, mid));

        if (f_mid == 0.0 || mid == lo || mid == hi)
            return mid;

        if ((f_mid > 0.0) == (f_lo > 0.0)) {
            lo = mid;
            f_lo = f_mid;
        } else
            hi = mid;
    }

    return 0.5 * (lo + hi);
}

/// Rule A's rib sweep: the root of the closure nearest the reference n0 - n1, scanned in steps without crossing a rib face and refined by bisection; the reference itself when no root is bracketed.
static Vector rib_sweep(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& reference) {

    double best = 0.0;
    bool found = false;

    for (double lo = -SCAN_RANGE; lo < SCAN_RANGE; lo += SCAN_STEP) {
        const double hi = lo + SCAN_STEP;
        std::array<bool, 2> sides_lo;
        std::array<bool, 2> sides_hi;

        if (!sweep_sides(normals, reference, lo, sides_lo) || !sweep_sides(normals, reference, hi, sides_hi) || sides_lo != sides_hi)
            continue;

        const double f_lo = closure(shadows, normals, thickness, turned(reference, lo));
        const double f_hi = closure(shadows, normals, thickness, turned(reference, hi));

        if ((f_lo > 0.0) == (f_hi > 0.0) && f_lo != 0.0 && f_hi != 0.0)
            continue;

        const double root = bisect(shadows, normals, thickness, reference, lo, hi);

        if (!found || std::abs(root) < std::abs(best))
            best = root;

        found = true;
    }

    return turned(reference, found ? best : 0.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Layers
// ═══════════════════════════════════════════════════════════════════════════

/// The central layers: the panel soffit's offsets by tsections and twice that in the panel's own cross-section, projected along the ruling onto both central faces.
static std::array<std::array<Polyline, 3>, 2> section_layers(const std::array<Polyline, 2>& soffits, const std::array<Plane, 2>& faces, const Vector& ruling, double tsections) {

    const Polyline section = soffits[0].transformed(Xform::project_to_plane_by_axis(Plane::from_point_normal(soffits[0].get_point(0), ruling), ruling));
    const Polyline offset1 = offset_polyline(section, tsections);
    const Polyline offset2 = offset_polyline(section, 2.0 * tsections);
    std::array<std::array<Polyline, 3>, 2> traces;

    for (size_t k = 0; k < 2; k++)
        traces[k] = {soffits[k], offset1.transformed(Xform::project_to_plane_by_axis(faces[k], ruling)), offset2.transformed(Xform::project_to_plane_by_axis(faces[k], ruling))};

    return traces;
}

CentralPanel central_panel(const ConstructionPlanes& cp, const std::vector<std::array<Polyline, 3>>& parabolas, const FloorGuide& guide) {

    const std::array<Plane, 2> faces = {cp.inner_ribs[0][1], cp.inner_ribs[1][1]};
    const std::array<Vector, 2> normals = {faces[0].z_axis(), faces[1].z_axis()};
    const std::array<Polyline, 2> shadows = {parabolas[2][0], parabolas[3][0]};
    const Vector reference = flat(normals[0] - normals[1]).normalized();

    CentralPanel panel;
    panel.rib_sweep = rib_sweep(shadows, normals, guide.inner_ribs, reference);
    const std::array<Polyline, 2> soffits = {shadows[0].transformed(Xform::project_to_plane_by_axis(faces[0], panel.rib_sweep)), shadows[1].transformed(Xform::project_to_plane_by_axis(faces[1], panel.rib_sweep))};
    panel.ruling = flat(soffits[1].get_point(0) - soffits[0].get_point(0)).normalized();

    for (size_t k = 0; k < 2; k++)
        panel.obliqueness[k] = std::acos(std::clamp(std::abs(normals[k].dot(panel.rib_sweep)), 0.0, 1.0)) * 180.0 / M_PI;

    panel.residual = largest_shift(soffits[0].transformed(Xform::project_to_plane_by_axis(faces[1], panel.ruling)), soffits[1]);

    panel.traces = section_layers(soffits, faces, panel.ruling, guide.tsections);

    return panel;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// MEMBER OUTLINES
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

using namespace wood_floor::geometry;

const double CUTTER_MARGIN = 100.0; // how far column cutter quads overshoot and how thick they are

// ═══════════════════════════════════════════════════════════════════════════
// Ribs
// ═══════════════════════════════════════════════════════════════════════════

/// A rib face's outline: the trimmed soffit trace closed up to z 0 over the end planes; an inner rib also ends its base on the second plane.
static Polyline rib_loop(const std::vector<Point>& pts, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

    const Vector span(pts.back()[0] - pts.front()[0], pts.back()[1] - pts.front()[1], 0.0);
    const Plane rib_plane = Plane::from_point_normal(pts.front(), span.cross(Vector(0.0, 0.0, 1.0)));
    const Point p0 = plane_plane_plane(cut_plane0, level(0.0), rib_plane).value();
    Point p1(pts.back()[0], pts.back()[1], 0.0);

    if (inner)
        p1 = line_plane(Line::from_points(p0, p1), cut_plane1).value();

    std::vector<Point> loop = {p1, p0};
    loop.insert(loop.end(), pts.begin(), pts.end());
    loop.push_back(p1);

    return Polyline(loop);
}

/// A rib: its soffit trace trimmed by the two end planes on its first face, and on its second face the trace swept along the rib with its end corners cut on the end planes, the first and last facet extended (R4).
static Outline rib(const Polyline& trace, const Plane& face1, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

    std::vector<Point> pts = trim(trace, cut_plane0, cut_plane1).get_points();
    const double d0 = std::abs((pts.front() - cut_plane0.origin()).dot(cut_plane0.z_axis()));
    const double d1 = std::abs((pts.back() - cut_plane0.origin()).dot(cut_plane0.z_axis()));

    if (d0 > d1)
        std::reverse(pts.begin(), pts.end());

    const Xform projection = Xform::project_to_plane_by_axis(face1, sweep);
    std::vector<Point> far;

    for (const Point& point : pts)
        far.push_back(point.transformed(projection));

    const size_t n = far.size();
    far[0] = line_plane(Line::from_points(far[0], far[1]), cut_plane0).value();
    far[n - 1] = line_plane(Line::from_points(far[n - 2], far[n - 1]), cut_plane1).value();

    return {rib_loop(pts, cut_plane0, cut_plane1, inner), rib_loop(far, cut_plane0, cut_plane1, inner)};
}

std::vector<Outline> Quarter::outer_ribs() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& parabolas = geometry().parabolas;
    const std::array<Plane, 2> ends = rib_seam_ends();

    return {
        rib(parabolas[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][1].z_axis(), cp.wedges[0][0], ends[0], false),
        rib(parabolas[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][1].z_axis(), cp.wedges[2][0], ends[1], false),
    };
}

std::array<Plane, 2> Quarter::rib_seam_ends() const {

    const ConstructionPlanes& cp = geometry().planes;
    const size_t face = guide.seam_through_ribs ? 1 : 0;

    return {cp.inner_beams[0][face], cp.inner_beams[2][face]};
}

std::vector<Outline> Quarter::inner_ribs() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& parabolas = geometry().parabolas;
    const Vector& sweep = geometry().central_panel.rib_sweep;

    return {
        rib(parabolas[2][0], cp.inner_ribs[0][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true),
        rib(parabolas[3][0], cp.inner_ribs[1][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Beams and wedges
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Outline> Quarter::inner_beams() const {

    const ConstructionPlanes& cp = geometry().planes;
    const Plane side0 = level(0.0);
    const Plane side1 = level(guide.soffit);
    const size_t face = guide.seam_through_ribs ? 0 : 1;

    return {
        loft_planes({cp.outer_ribs[0][face], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[0][0], cp.inner_beams[0][1]),
        loft_planes({cp.inner_beams[0][1], side0, cp.inner_beams[2][1], side1}, cp.inner_beams[1][0], cp.inner_beams[1][1]),
        loft_planes({cp.outer_ribs[1][face], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[2][0], cp.inner_beams[2][1]),
    };
}

std::vector<Outline> Quarter::wedges() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<Plane>& beds = geometry().bed_top_planes;
    const Plane top = level(0.0);
    const std::array<std::array<Plane, 2>, 3> ribs = {{
        {cp.outer_ribs[0][1], cp.inner_ribs[0][0]},
        {cp.inner_ribs[0][1], cp.inner_ribs[1][1]},
        {cp.inner_ribs[1][0], cp.outer_ribs[1][1]},
    }};

    std::vector<Outline> wedges;

    for (size_t i = 0; i < 3; i++)
        wedges.push_back(loft_planes({ribs[i][0], beds[i], ribs[i][1], top}, cp.wedges[i][0], cp.wedges[i][1]));

    return wedges;
}

// ═══════════════════════════════════════════════════════════════════════════
// T-sections and beds
// ═══════════════════════════════════════════════════════════════════════════

/// A t-section: its soffit and +t traces on its first face, each trimmed there, closed into one outline, and the same traces projected onto its second face and trimmed there.
static Outline tsection(const Polyline& soffit, const Polyline& layer, const Plane& cut_plane0, const Plane& cut_plane1, const Xform& projection10, const Xform& projection11) {

    const std::vector<Point> cut00 = trim(soffit, cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut01 = trim(soffit.transformed(projection10), cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut10 = trim(layer, cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut11 = trim(layer.transformed(projection11), cut_plane0, cut_plane1).get_points();

    std::vector<Point> top = cut00;
    top.insert(top.end(), cut10.rbegin(), cut10.rend());
    top.push_back(cut00.front());

    std::vector<Point> bottom = cut01;
    bottom.insert(bottom.end(), cut11.rbegin(), cut11.rend());
    bottom.push_back(cut01.front());

    return {Polyline(top), Polyline(bottom)};
}

/// The t-section beside an outer panel rib face: the outer parabola and its +t projected along the outer rib normal onto the face, the soffit continued to the far face along the rib's sweep, the +t along the panel.
static Outline outer_tsection(const std::array<Polyline, 3>& parabola, const std::array<Plane, 2>& faces, const Vector& outer, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1) {

    const Xform projection = Xform::project_to_plane_by_axis(faces[0], outer);

    return tsection(
        parabola[0].transformed(projection), parabola[1].transformed(projection), cut_plane0, cut_plane1,
        Xform::project_to_plane_by_axis(faces[1], sweep),
        Xform::project_to_plane_by_axis(faces[1], outer)
    );
}

std::vector<Outline> Quarter::tsections() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& pb = geometry().parabolas;
    const CentralPanel& panel = geometry().central_panel;
    const Vector outer0 = cp.outer_ribs[0][0].z_axis();
    const Vector outer1 = cp.outer_ribs[1][0].z_axis();
    const std::vector<std::array<Plane, 2>>& ts = cp.tsections;

    return {
        outer_tsection(pb[0], ts[0], outer0, outer0, cp.inner_beams[0][1], cp.wedges[0][0]),
        outer_tsection(pb[0], ts[1], outer0, panel.rib_sweep, cp.inner_beams[0][1], cp.wedges[0][0]),
        tsection(
            panel.traces[0][0], panel.traces[0][1], cp.inner_beams[1][1], cp.wedges[1][0],
            Xform::project_to_plane_by_axis(ts[2][1], panel.rib_sweep),
            Xform::project_to_plane_by_axis(ts[2][1], panel.ruling)
        ),
        tsection(
            panel.traces[1][0], panel.traces[1][1], cp.inner_beams[1][1], cp.wedges[1][0],
            Xform::project_to_plane_by_axis(ts[3][1], panel.rib_sweep),
            Xform::project_to_plane_by_axis(ts[3][1], panel.ruling)
        ),
        outer_tsection(pb[1], ts[4], outer1, panel.rib_sweep, cp.inner_beams[2][1], cp.wedges[2][0]),
        outer_tsection(pb[1], ts[5], outer1, outer1, cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

/// One bed row: the lower and upper layer on each of the panel's two side planes, trimmed alike, so on a skewed bay too, one quad pair per segment.
static std::vector<Outline> bed_row(const std::array<Polyline, 2>& lower_faces, const std::array<Polyline, 2>& upper_faces, const Plane& cut_plane0, const Plane& cut_plane1) {

    const std::vector<Polyline> layers = trim_alike({lower_faces[0], lower_faces[1], upper_faces[0], upper_faces[1]}, cut_plane0, cut_plane1);
    const std::array<std::vector<Point>, 2> lower = {layers[0].get_points(), layers[1].get_points()};
    const std::array<std::vector<Point>, 2> upper = {layers[2].get_points(), layers[3].get_points()};

    std::vector<Outline> plates;

    for (size_t i = 0; i + 1 < lower[0].size(); i++) {
        const Polyline bottom({lower[0][i], lower[0][i + 1], lower[1][i + 1], lower[1][i], lower[0][i]});
        const Polyline top({upper[0][i], upper[0][i + 1], upper[1][i + 1], upper[1][i], upper[0][i]});
        plates.push_back({top, bottom});
    }

    return plates;
}

/// An outer bed row: the parabola's +t and +2t projected along the outer rib normal onto the panel's two side planes.
static std::vector<Outline> outer_bed_row(const std::array<Polyline, 3>& parabola, const Plane& side0, const Plane& side1, const Vector& outer, const Plane& cut_plane0, const Plane& cut_plane1) {

    const Xform projection0 = Xform::project_to_plane_by_axis(side0, outer);
    const Xform projection1 = Xform::project_to_plane_by_axis(side1, outer);

    return bed_row({parabola[1].transformed(projection0), parabola[1].transformed(projection1)}, {parabola[2].transformed(projection0), parabola[2].transformed(projection1)}, cut_plane0, cut_plane1);
}

std::vector<std::vector<Outline>> Quarter::beds() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& pb = geometry().parabolas;
    const CentralPanel& panel = geometry().central_panel;

    return {
        outer_bed_row(pb[0], cp.inner_ribs[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis(), cp.inner_beams[0][1], cp.wedges[0][0]),
        bed_row({panel.traces[0][1], panel.traces[1][1]}, {panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]),
        outer_bed_row(pb[1], cp.inner_ribs[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis(), cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Outline> FloorGuide::oculus() const {

    const Plane side0 = level(0.0);
    const Plane side1 = level(soffit + tsections);
    const Plane side2 = level(soffit);
    const Plane side3 = level(soffit + tsections * 2.0);

    std::vector<Plane> tilted;
    std::vector<Plane> inner;

    for (const OculusEdge& edge : oculus_edges) {
        tilted.push_back(edge.tilted);
        inner.push_back(edge.ring_inner);
    }

    std::vector<Outline> plates;

    for (size_t i = 0; i < 4; i++)
        plates.push_back(loft_planes({side2, tilted[(i + 1) % 4], side0, inner[(i + 3) % 4]}, tilted[i], inner[i], true));

    for (size_t i = 0; i < 4; i++) {
        const std::vector<Plane> sides = {inner[i], inner[(i + 1) % 4], inner[i].translate_by_normal(-tsections), inner[(i + 3) % 4].translate_by_normal(-tsections)};
        plates.push_back(loft_planes(sides, side2, side1));
    }

    plates.push_back(loft_planes(inner, side1, side3));

    return plates;
}

// ═══════════════════════════════════════════════════════════════════════════
// Column cutters
// ═══════════════════════════════════════════════════════════════════════════

/// The cutter quad stretched in its own plane: its long sides by the margin at both ends, then its short sides inwards, both short sides for a top quad and only the first for a bottom one.
static std::vector<Point> stretch(std::vector<Point> quad, bool top) {

    const Vector d0 = (quad[1] - quad[0]).normalized() * CUTTER_MARGIN;
    const Vector d1 = (quad[3] - quad[2]).normalized() * CUTTER_MARGIN;
    quad[0] = quad[0] - d0;
    quad[1] = quad[1] + d0;
    quad[2] = quad[2] - d1;
    quad[3] = quad[3] + d1;

    const Vector d2 = (quad[2] - quad[1]).normalized() * CUTTER_MARGIN;
    const Vector d3 = (quad[0] - quad[3]).normalized() * CUTTER_MARGIN;
    quad[0] = quad[0] - d2;
    quad[1] = quad[1] - d2;

    if (top) {
        quad[2] = quad[2] - d3;
        quad[3] = quad[3] - d3;
    }

    return quad;
}

std::vector<Point> Quarter::column_face(size_t i) const {

    const ConstructionPlanes& cp = geometry().planes;
    const ColumnCorner& corner = column();
    const std::array<Plane, 5> fan = {corner.sides[0], cp.wedges[0][0], cp.wedges[1][0], cp.wedges[2][0], corner.sides[1]};
    const Plane xy0 = level(corner.levels[0]);
    const Plane xy1 = level(corner.levels[1]);

    return {
        plane_plane_plane(xy0, fan[i], fan[i + 1]).value(),
        plane_plane_plane(xy0, fan[i + 1], fan[i + 2]).value(),
        plane_plane_plane(xy1, fan[i + 1], fan[i + 2]).value(),
        plane_plane_plane(xy1, fan[i], fan[i + 1]).value(),
    };
}

std::vector<Outline> Quarter::column_cutters() const {

    const ColumnCorner& corner = column();
    const std::vector<Point>& column = corner.head;
    const Vector down(0.0, 0.0, -1.0);
    const Plane xy2 = level(corner.levels[2]);
    const std::vector<Plane> fan_bottom = {corner.sides[0], edge_plane(Line::from_points(column[1], column[2]), down), edge_plane(Line::from_points(column[3], column[4]), down), corner.sides[1]};
    const std::array<std::vector<Point>, 3> faces = {column_face(0), column_face(1), column_face(2)};
    const std::vector<Point> p1 = {faces[0][3], faces[0][2], faces[1][2], faces[2][2]};

    std::vector<Point> p2;

    for (size_t i = 0; i + 1 < fan_bottom.size(); i++)
        p2.push_back(plane_plane_plane(xy2, fan_bottom[i], fan_bottom[i + 1]).value());

    const Vector quarter = (p2[2] - p2[0]) * 0.25;
    const std::vector<std::vector<Point>> quads = {
        faces[0],
        faces[1],
        faces[2],
        {p1[0], p1[1], p2[1], p2[0]},
        {p1[1], p1[2], p2[1] + quarter, p2[1] - quarter},
        {p1[2], p1[3], p2[2], p2[1]},
    };

    std::vector<Outline> plates;

    for (size_t i = 0; i < quads.size(); i++) {
        const std::vector<Point> quad = stretch(quads[i], i < 3);
        const Vector normal = (quad[2] - quad[1]).cross(quad[1] - quad[0]).normalized() * CUTTER_MARGIN;
        const Polyline top = Polyline(quad).closed();
        plates.push_back({top, top.transformed(Xform::translation(normal[0], normal[1], normal[2]))});
    }

    return plates;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE FLOOR REPORT
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

using namespace wood_floor::geometry;

/// The signed plan angle in degrees from the line along a to the vector b, the line's sense picked to make it the smaller turn.
static double plan_angle(const Vector& a, const Vector& b) {

    const Vector x = a.dot(b) < 0.0 ? -a : a;
    const double cross = x[0] * b[1] - x[1] * b[0];
    const double dot = x[0] * b[0] + x[1] * b[1];

    return std::atan2(cross, dot) * 180.0 / M_PI;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plane polygons
// ═══════════════════════════════════════════════════════════════════════════

/// The total area of the polygons a boolean of two loops on a plane leaves: clip_type 0 their overlap, 2 a outside b.
static double boolean_area(const Polyline& a, const Polyline& b, const Plane& plane, int clip_type) {

    double area = 0.0;

    for (const Polyline& polygon : Polyline::boolean_op(a, b, plane, clip_type))
        area += polygon_area(polygon);

    return area;
}

// ═══════════════════════════════════════════════════════════════════════════
// Measures
// ═══════════════════════════════════════════════════════════════════════════

/// The farthest corner of a rib's two end faces from its end planes.
static double end_face_offset(const Outline& rib, const Plane& cut_plane0, const Plane& cut_plane1) {

    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    const size_t n = top.size();
    double worst = 0.0;

    for (const Point& point : {top[1], top[2], bottom[2], bottom[1]})
        worst = std::max(worst, std::abs(signed_distance(point, cut_plane0)));

    for (const Point& point : {top[0], top[n - 2], bottom[n - 2], bottom[0]})
        worst = std::max(worst, std::abs(signed_distance(point, cut_plane1)));

    return worst;
}

/// The farthest corner of the four ribs' end faces from their end planes.
static double end_face_planarity(const Quarter& quarter) {

    const ConstructionPlanes& cp = quarter.geometry().planes;
    const std::vector<Outline> outer = quarter.outer_ribs();
    const std::vector<Outline> inner = quarter.inner_ribs();

    return std::max({
        end_face_offset(outer[0], cp.wedges[0][0], quarter.rib_seam_ends()[0]),
        end_face_offset(outer[1], cp.wedges[2][0], quarter.rib_seam_ends()[1]),
        end_face_offset(inner[0], cp.wedges[1][0], cp.inner_beams[1][1]),
        end_face_offset(inner[1], cp.wedges[1][0], cp.inner_beams[1][1]),
    });
}

/// The farthest bed underside corner from the top loop of the flange beside it: per row, side 0 and side 1 against their two t-sections.
static double bed_flange_coincidence(const Quarter& quarter) {

    const std::vector<std::vector<Outline>> beds = quarter.beds();
    const std::vector<Outline> flanges = quarter.tsections();
    const std::array<std::array<size_t, 2>, 3> beside = {{{1, 0}, {2, 3}, {4, 5}}};
    double worst = 0.0;

    for (size_t row = 0; row < 3; row++)
        for (const Outline& bed : beds[row]) {
            const std::vector<Point> under = bed.bottom.get_points();
            worst = std::max({worst, std::get<2>(Closest::polyline_point(flanges[beside[row][0]].top, under[0])), std::get<2>(Closest::polyline_point(flanges[beside[row][0]].top, under[1]))});
            worst = std::max({worst, std::get<2>(Closest::polyline_point(flanges[beside[row][1]].top, under[2])), std::get<2>(Closest::polyline_point(flanges[beside[row][1]].top, under[3]))});
        }

    return worst;
}

/// The highest less the lowest of the eight rib face bottoms at the column head: each outer and inner rib's soffit corner on its column end plane, on both faces.
static double rib_level_spread(const Quarter& quarter) {

    std::vector<double> levels;

    for (const std::vector<Outline>& family : {quarter.outer_ribs(), quarter.inner_ribs()})
        for (const Outline& rib : family)
            levels.insert(levels.end(), {rib.top.get_point(2)[2], rib.bottom.get_point(2)[2]});

    return *std::max_element(levels.begin(), levels.end()) - *std::min_element(levels.begin(), levels.end());
}

/// The plan overlap of the four ring beams, pair by pair.
static double ring_overlap(const std::vector<Outline>& ring) {

    std::vector<Polyline> footprints;

    for (size_t i = 0; i < 4; i++) {
        std::vector<Point> points = open_points(ring[i].top);
        const std::vector<Point> bottom = open_points(ring[i].bottom);
        points.insert(points.end(), bottom.begin(), bottom.end());

        for (Point& point : points)
            point[2] = 0.0;

        footprints.push_back(Polyline(ConvexHull::hull_2d(points)).closed());
    }

    double overlap = 0.0;

    for (size_t i = 0; i < 4; i++)
        for (size_t j = i + 1; j < 4; j++)
            overlap += boolean_area(footprints[i], footprints[j], level(0.0), 0);

    return overlap;
}

/// The area of quarter q's oculus beam face outside ring beam q's outer face, on their shared tilted plane.
static double ring_uncovered(const FloorGuide& guide, size_t q, const Outline& ring_beam) {
    return boolean_area(guide.quarter(q).inner_beams()[1].bottom, ring_beam.top, guide.oculus_edges[q].tilted, 2);
}

/// The relations of quarter q and its corner written into the report.
static void measure_quarter(const FloorGuide& guide, size_t q, FloorReport& report) {

    const Quarter quarter = guide.quarter(q);
    const QuarterGeometry& geometry = quarter.geometry();
    const CentralPanel& panel = geometry.central_panel;
    const ColumnCorner& column = guide.columns[q];
    const size_t next = (q + 1) % 4;

    for (const Point& point : open_points(guide.quarter(next).inner_beams()[2].bottom))
        report.seam_plane_gap[q] = std::max(report.seam_plane_gap[q], std::abs(signed_distance(point, geometry.planes.inner_beams[0][0])));

    report.oculus_corner_gap[q] = (geometry.polygon[2] - guide.geometry[next].polygon[3]).magnitude();
    report.ruling_off_chamfer_deg[q] = plan_angle(column.chamfer_direction, panel.ruling);
    report.ruling_off_oculus_edge_deg[q] = plan_angle(guide.oculus_edges[q].line.to_direction(), panel.ruling);
    report.closure_residual_mm[q] = panel.residual;
    report.end_face_planarity_mm[q] = end_face_planarity(quarter);
    report.bed_flange_coincidence_mm[q] = bed_flange_coincidence(quarter);
    report.wedge_seat_mm[q] = column.wedge_seat;
    report.column_offset_mm[q] = column.column_offset;
    report.rib_level_spread_mm[q] = rib_level_spread(quarter);

    const std::vector<Outline> outer = quarter.outer_ribs();

    for (size_t k = 0; k < 2; k++) {
        report.rib_sweep_obliqueness_deg[q][k] = panel.obliqueness[k];
        report.rib_shear_mm[q][k] = guide.inner_ribs * std::tan(panel.obliqueness[k] * M_PI / 180.0);
        report.rib_bottom_clearance_mm[q][k] = std::min(outer[k].top.get_point(2)[2], outer[k].bottom.get_point(2)[2]) - column.levels[1];
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Report
// ═══════════════════════════════════════════════════════════════════════════

FloorReport FloorGuide::check() const {

    FloorReport report;
    const std::vector<Outline> ring = oculus();

    for (size_t q = 0; q < 4; q++) {
        measure_quarter(*this, q, report);
        report.ring_uncovered_mm2 += ring_uncovered(*this, q, ring[q]);
    }

    report.ring_overlap_mm2 = ring_overlap(ring);

    return report;
}

bool FloorReport::ok(double tolerance) const {

    for (size_t q = 0; q < 4; q++)
        if (seam_plane_gap[q] > tolerance || oculus_corner_gap[q] > tolerance || closure_residual_mm[q] > tolerance || end_face_planarity_mm[q] > tolerance || bed_flange_coincidence_mm[q] > tolerance)
            return false;

    return ring_overlap_mm2 <= tolerance && ring_uncovered_mm2 <= tolerance;
}

std::string FloorReport::str() const {

    std::string text = fmt::format("floor report: {}\n", ok() ? "ok" : "FAILING");

    for (size_t q = 0; q < 4; q++) {
        text += fmt::format("  quarter {}: seam gap {:.3e}, oculus corner gap {:.3e}, closure {:.3e}, end faces {:.3e}, beds on flanges {:.3e} mm\n", q, seam_plane_gap[q], oculus_corner_gap[q], closure_residual_mm[q], end_face_planarity_mm[q], bed_flange_coincidence_mm[q]);
        text += fmt::format("    ruling {:.3f} deg off the chamfer, {:.3f} off the oculus edge; rib sweep {:.3f} / {:.3f} deg oblique, shear {:.3f} / {:.3f} mm\n", ruling_off_chamfer_deg[q], ruling_off_oculus_edge_deg[q], rib_sweep_obliqueness_deg[q][0], rib_sweep_obliqueness_deg[q][1], rib_shear_mm[q][0], rib_shear_mm[q][1]);
        text += fmt::format("    rib bottoms {:.3f} / {:.3f} mm above the cutter level, the eight rib bottoms at the head span {:.3f} mm; seats {:.3f} / {:.3f} / {:.3f} mm; column offset {:.3f} / {:.3f} mm\n", rib_bottom_clearance_mm[q][0], rib_bottom_clearance_mm[q][1], rib_level_spread_mm[q], wedge_seat_mm[q][0], wedge_seat_mm[q][1], wedge_seat_mm[q][2], column_offset_mm[q][0], column_offset_mm[q][1]);
    }

    text += fmt::format("  ring: beams overlap {:.3e} mm2, quarter beam faces uncovered {:.3e} mm2", ring_overlap_mm2, ring_uncovered_mm2);

    return text;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// OUTLINES INTO ELEMENTS
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

using namespace wood_floor::geometry;

/// The point at z.
static Point at_level(const Point& point, double z) {
    return Point(point[0], point[1], z);
}

/// The closed square from the corner over side along both frame axes, at z.
static Polyline square(const ColumnCorner& corner, double side, double z) {

    const Point& o = corner.corner;
    const Vector x = corner.x_axis * side;
    const Vector y = corner.y_axis * side;

    return Polyline({at_level(o, z), at_level(o + x, z), at_level(o + x + y, z), at_level(o + y, z)}).closed();
}

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<wood_session::BeamVariable> to_rib(const Outline& outline, const std::string& name) {

    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const size_t stations = top.size() - 3;
    std::vector<Polyline> sections;

    for (size_t i = 0; i < stations; i++) {
        const Point& low = top[2 + i];
        const Point& far_low = bottom[2 + i];
        Point high = at_level(low, 0.0);
        Point far_high = at_level(far_low, 0.0);

        if (i == 0) {
            high = top[1];
            far_high = bottom[1];
        } else if (i + 1 == stations) {
            high = top[0];
            far_high = bottom[0];
        }

        sections.push_back(Polyline({low, high, far_high, far_low}).closed());
    }

    const Line axis = Line::from_points(Line::from_points(top[1], bottom[1]).center(), Line::from_points(top[0], bottom[0]).center());

    return std::make_shared<wood_session::BeamVariable>(axis, sections, name);
}

std::shared_ptr<wood_session::BeamVariable> to_beam(const Outline& outline, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name) {

    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const Polyline first = Polyline({top[start[0]], top[start[1]], bottom[start[1]], bottom[start[0]]}).closed();
    const Polyline last = Polyline({top[end[0]], top[end[1]], bottom[end[1]], bottom[end[0]]}).closed();
    const Point a = Point::centroid({top[start[0]], top[start[1]], bottom[start[1]], bottom[start[0]]});
    const Point b = Point::centroid({top[end[0]], top[end[1]], bottom[end[1]], bottom[end[0]]});

    return std::make_shared<wood_session::BeamVariable>(Line::from_points(a, b), std::vector<Polyline>{first, last}, name);
}

std::shared_ptr<wood_session::Plate> to_plate(const Outline& outline, const std::string& name) {
    return std::make_shared<wood_session::Plate>(outline.bottom, outline.top, name);
}

std::shared_ptr<wood_session::Support> to_support(const ColumnCorner& corner) {
    return std::make_shared<wood_session::Support>(corner.support_plane, "support");
}

std::shared_ptr<wood_session::Column> to_column(const ColumnCorner& corner, const FloorGuide& guide, const wood_session::Support& support) {

    const Point foot = support.column_foot();
    const double side = guide.column_head;
    const double head = side + guide.column_head_chamfer;
    const Line axis = Line::from_points(foot, Point(foot[0], foot[1], guide.bay_height));

    std::shared_ptr<wood_session::Column> column = std::make_shared<wood_session::Column>(axis, square(corner, side, foot[2]), "column");
    column->head = square(corner, head, foot[2]);
    column->head_height = guide.column_head_depth;

    return column;
}

std::vector<wood_session::SolidCut> column_cuts(const Quarter& quarter) {

    const Xform lift = Xform::translation(0.0, 0.0, quarter.guide.bay_height);
    std::vector<wood_session::SolidCut> cuts;

    for (const Outline& outline : quarter.column_cutters()) {
        wood_session::SolidCut cut;
        cut.mesh = to_plate(outline, "column_cutter")->element_geometry_mesh().transformed(lift);
        cut.operation = wood_session::SolidOperation::difference;
        cuts.push_back(cut);
    }

    return cuts;
}

double outline_thickness(const Outline& outline) {
    return (area_centroid(outline.top) - area_centroid(outline.bottom)).magnitude();
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE FLOOR MODEL
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

/// The member outlines of one family as members: ribs and inner beams as variable beams, every other family as plates, each with its outline's thickness.
static std::vector<Member> to_members(Family family, const std::vector<Outline>& outlines) {

    const std::string& name = FAMILY_NAMES[static_cast<size_t>(family)];
    std::vector<Member> members;

    for (const Outline& outline : outlines) {
        Member member;
        member.thickness = outline_thickness(outline);

        if (family == Family::outer_ribs || family == Family::inner_ribs)
            member.element = to_rib(outline, name);
        else if (family == Family::inner_beams)
            member.element = to_beam(outline, {0, 3}, {1, 2}, name);
        else
            member.element = to_plate(outline, name);

        members.push_back(member);
    }

    return members;
}

/// Names an element and adds it under the group.
static void add_named(wood_session::WoodSession& session, const std::shared_ptr<Element>& element, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->name = name;
    session.add(element, group);
}

/// Lifts an element to the floor, names it and adds it under the group.
static void add_placed(wood_session::WoodSession& session, const std::shared_ptr<Element>& element, const Xform& lift, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->place(lift);
    add_named(session, element, name, group);
}

/// Places the members under a new group <prefix><suffix>, each named <prefix>_<i><suffix>.
static void add_family(wood_session::WoodSession& session, const std::vector<Member>& members, const Xform& placement, const std::string& prefix, const std::string& suffix, const std::shared_ptr<TreeNode>& group) {

    const std::shared_ptr<TreeNode> node = add_group(session, prefix + suffix, group);

    for (size_t i = 0; i < members.size(); i++)
        add_placed(session, members[i].element, placement, fmt::format("{}_{}{}", prefix, i, suffix), node);
}

/// Colours the node and every node nested under it.
static void paint(wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& node, const Color& color) {

    session.set_node_color(node, color);

    for (TreeNode* child : node->descendants())
        session.set_node_color(child->shared_from_this(), color);
}

/// The live child of parent named name, null when there is none; a null parent is the tree's root.
static std::shared_ptr<TreeNode> child_named(const wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& parent, const std::string& name) {

    const std::shared_ptr<TreeNode> host = parent ? parent : session.tree.root();

    if (!host)
        return nullptr;

    for (TreeNode* child : host->children())
        if (child->name == name)
            return child->shared_from_this();

    return nullptr;
}

/// The group named name under parent, added after its other children the first time.
static std::shared_ptr<TreeNode> group_named(wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& parent, const std::string& name) {

    const std::shared_ptr<TreeNode> group = child_named(session, parent, name);

    return group ? group : add_group(session, name, parent);
}

/// The group of quarter q under the floor's group, made the first time; every member, column and connector of the quarter goes in it.
static std::shared_ptr<TreeNode> quarter_group(wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& floor, size_t q) {
    return group_named(session, floor, fmt::format("quarter_{}", q));
}

/// The next free number of a connector name prefix in the session: one past the highest <prefix>_<n> already there, 0 when there is none.
static size_t next_number(const wood_session::WoodSession& session, const std::string& prefix) {

    size_t next = 0;

    for (const std::shared_ptr<Element>& element : *session.objects.elements) {
        const std::string& name = element->name;

        if (name.size() > prefix.size() + 1 && name.compare(0, prefix.size() + 1, prefix + "_") == 0 && std::all_of(name.begin() + prefix.size() + 1, name.end(), ::isdigit))
            next = std::max(next, static_cast<size_t>(std::stoul(name.substr(prefix.size() + 1))) + 1);
    }

    return next;
}

std::shared_ptr<Element> uncut(const Element& member) {

    const std::shared_ptr<Element> copy = member.clone();

    if (wood_session::BeamVariable* beam = dynamic_cast<wood_session::BeamVariable*>(copy.get())) {
        beam->cuts.clear();
        beam->solid_cuts.clear();
    } else if (wood_session::Plate* plate = dynamic_cast<wood_session::Plate*>(copy.get())) {
        plate->solid_cuts.clear();
    } else if (wood_session::Column* column = dynamic_cast<wood_session::Column*>(copy.get())) {
        // the head carve stays, only what connectors cut goes
        std::erase_if(column->solid_cuts, [](const wood_session::SolidCut& cut) { return !cut.joint_guid.empty(); });
    }

    copy->invalidate_geometry();

    return copy;
}

// ═══════════════════════════════════════════════════════════════════════════
// Models
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<TreeNode> add_group(wood_session::WoodSession& session, const std::string& name, const std::shared_ptr<TreeNode>& parent) {

    std::shared_ptr<TreeNode> node = std::make_shared<TreeNode>(name);
    session.Session::add(node, parent);

    return node;
}

QuarterMembers add_quarter_model(wood_session::WoodSession& session, const Quarter& view, const std::shared_ptr<TreeNode>& group) {

    const Xform lift = Xform::translation(0.0, 0.0, view.guide.bay_height);
    const std::string suffix = fmt::format("_{}", view.index);
    const std::vector<std::vector<Outline>> beds = view.beds();
    const std::shared_ptr<TreeNode> bed_group = add_group(session, "beds" + suffix, group);
    QuarterMembers quarter;
    quarter.group = group;

    for (size_t row = 0; row < beds.size(); row++) {
        quarter.beds.push_back(to_members(Family::beds, beds[row]));
        add_family(session, quarter.beds.back(), lift, fmt::format("beds_{}", row), suffix, bed_group);
    }

    quarter.tsections = to_members(Family::tsections, view.tsections());
    add_family(session, quarter.tsections, lift, "tsections", suffix, group);
    quarter.outer_ribs = to_members(Family::outer_ribs, view.outer_ribs());
    add_family(session, quarter.outer_ribs, lift, "outer_ribs", suffix, group);
    quarter.inner_ribs = to_members(Family::inner_ribs, view.inner_ribs());
    add_family(session, quarter.inner_ribs, lift, "inner_ribs", suffix, group);
    quarter.wedges = to_members(Family::wedges, view.wedges());
    add_family(session, quarter.wedges, lift, "wedges", suffix, group);
    quarter.inner_beams = to_members(Family::inner_beams, view.inner_beams());
    add_family(session, quarter.inner_beams, lift, "inner_beams", suffix, group);

    return quarter;
}

std::vector<Member> add_oculus_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const std::vector<Outline> outlines = guide.oculus();
    std::vector<Member> beams;

    for (size_t i = 0; i < outlines.size(); i++) {
        const std::shared_ptr<Element> member = i < 4 ? std::static_pointer_cast<Element>(to_beam(outlines[i], {1, 0}, {2, 3}, "oculus")) : std::static_pointer_cast<Element>(to_plate(outlines[i], "oculus"));
        const std::shared_ptr<TreeNode> host = i < 8 ? group_named(session, quarter_group(session, group, i % 4), fmt::format("oculus_{}", i % 4)) : group_named(session, group, "oculus");
        add_placed(session, member, lift, fmt::format("oculus_{}", i), host);

        if (i < 4)
            beams.push_back({member, outline_thickness(outlines[i])});
    }

    return beams;
}

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

ColumnModel add_column_model(wood_session::WoodSession& session, const FloorGuide& guide, size_t corner, const std::shared_ptr<TreeNode>& group) {

    const std::string suffix = fmt::format("_{}", corner % 4);
    ColumnModel model;
    model.group = group;
    model.support = to_support(guide.columns[corner % 4]);
    model.column = to_column(guide.columns[corner % 4], guide, *model.support);
    add_named(session, model.support, "support" + suffix, group);
    add_named(session, model.column, "column" + suffix, group);

    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(*model.support, *model.column);
    session.add(joint, group);
    session.add_joint(joint);

    for (const wood_session::SolidCut& cut : column_cuts(guide.quarter(corner)))
        model.column->solid_cuts.push_back(cut);

    model.column->invalidate_geometry();

    return model;
}

FloorMembers add_floor(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group) {

    FloorMembers members;
    members.group = group;

    for (size_t q = 0; q < 4; q++)
        members.quarters[q] = add_quarter_model(session, guide.quarter(q), quarter_group(session, group, q));

    members.ring = add_oculus_model(session, guide, group);
    members.oculus = group_named(session, group, "oculus");

    return members;
}

void add_columns(wood_session::WoodSession& session, const FloorGuide& guide, FloorMembers& members) {

    members.columns.resize(4);

    for (size_t q = 0; q < 4; q++)
        members.columns[q] = add_column_model(session, guide, q, group_named(session, quarter_group(session, members.group, q), fmt::format("column_{}", q)));
}

/// The member a quarter reference names, null when the family or index is not in the quarter.
static const Member* quarter_member(const QuarterMembers& quarter, const MemberRef& ref) {

    const std::vector<Member>* family = nullptr;

    if (ref.family == Family::outer_ribs)
        family = &quarter.outer_ribs;
    else if (ref.family == Family::inner_ribs)
        family = &quarter.inner_ribs;
    else if (ref.family == Family::inner_beams)
        family = &quarter.inner_beams;
    else if (ref.family == Family::wedges)
        family = &quarter.wedges;
    else if (ref.family == Family::tsections)
        family = &quarter.tsections;
    else if (ref.family == Family::beds && ref.row >= 0 && static_cast<size_t>(ref.row) < quarter.beds.size())
        family = &quarter.beds[static_cast<size_t>(ref.row)];

    return family && ref.index < family->size() ? &(*family)[ref.index] : nullptr;
}

std::shared_ptr<Element> FloorMembers::get(const MemberRef& ref) const {

    if (ref.family == Family::ring)
        return ref.index < ring.size() ? ring[ref.index].element : nullptr;

    if (ref.family == Family::column)
        return ref.index < columns.size() ? columns[ref.index].column : nullptr;

    if (ref.family == Family::support)
        return ref.index < columns.size() ? columns[ref.index].support : nullptr;

    if (ref.quarter < 0 || ref.quarter > 3)
        return nullptr;

    const Member* member = quarter_member(quarters[static_cast<size_t>(ref.quarter)], ref);

    return member ? member->element : nullptr;
}

double FloorMembers::thickness(const MemberRef& ref) const {

    if (ref.family == Family::ring)
        return ref.index < ring.size() ? ring[ref.index].thickness : 0.0;

    if (ref.quarter < 0 || ref.quarter > 3)
        return 0.0;

    const Member* member = quarter_member(quarters[static_cast<size_t>(ref.quarter)], ref);

    return member ? member->thickness : 0.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Connectors
// ═══════════════════════════════════════════════════════════════════════════

std::array<std::shared_ptr<Element>, 2> FloorMembers::pair(const Relationship& row) const {

    const std::shared_ptr<Element> a = get(row.a);
    const std::shared_ptr<Element> b = get(row.b);

    if (!a || !b)
        throw std::runtime_error("the floor members do not hold both members of " + row.text());

    return {a, b};
}

/// The name prefix of a connector of that kind, as the examples name them.
static std::string connector_prefix(Relation kind) {

    if (kind == Relation::seam_wedge || kind == Relation::oculus_wedge)
        return "connector_wedge";

    if (kind == Relation::column_plate)
        return "connector";

    if (kind == Relation::cross_lap)
        return "connector_cross_lap";

    if (kind == Relation::seam_tie)
        return "outer_rib_connector";

    if (std::find(SCREW_RELATIONS.begin(), SCREW_RELATIONS.end(), kind) != SCREW_RELATIONS.end())
        return "connector_screws";

    return "connector_dowels";
}

/// The group a relationship's connector goes under, made the first time: connectors_q in the group of its quarter q.
static std::shared_ptr<TreeNode> connector_group(wood_session::WoodSession& session, const FloorMembers& members, const Relationship& row) {
    return group_named(session, quarter_group(session, members.group, row.seam_or_corner), fmt::format("connectors_{}", row.seam_or_corner));
}

/// The connector of one contact relationship through its factory: the wedge sized by the thicker member, the plate by the rib's thickness, the tie, the screws and the dowels by their defaults.
static std::shared_ptr<wood_session::JointBeam> connector_of(const Relationship& row, const FloorMembers& members) {

    const std::array<std::shared_ptr<Element>, 2> pair = members.pair(row);
    const wood_session::InteractionContactFace contact(-1, -1, row.type, row.contact);

    if (row.kind == Relation::seam_wedge || row.kind == Relation::oculus_wedge) {
        const double thickness = std::max(members.thickness(row.a), members.thickness(row.b));
        return wood_session::JointBeam::wedge(*pair[0], *pair[1], contact, 1.5 * thickness, 2.0 * thickness / 3.0, row.end);
    }

    if (row.kind == Relation::column_plate)
        return wood_session::JointBeam::rectangle_plate(*pair[0], *pair[1], contact, members.thickness(row.b));

    if (row.kind == Relation::seam_tie)
        return wood_session::JointBeam::tie(*pair[0], *pair[1], contact, TIE_TOP);

    if (!row.screws.empty()) {
        std::vector<const Element*> passed = {pair[0].get(), pair[1].get()};

        for (const MemberRef& ref : row.through)
            passed.push_back(members.get(ref).get());

        return wood_session::JointBeam::screws(passed, row.screws);
    }

    const std::shared_ptr<wood_session::JointBeam> dowels = wood_session::JointBeam::dowels(*pair[0], *pair[1], contact);

    if (!dowels)
        throw std::runtime_error("the inset leaves no room for the dowels of " + row.text());

    return dowels;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_connectors(wood_session::WoodSession& session, const FloorGuide& guide, const FloorMembers& members, const std::vector<Relation>& kinds) {

    std::map<size_t, std::vector<std::shared_ptr<wood_session::JointBeam>>> plates_of_corner;
    std::vector<std::pair<Relationship, std::shared_ptr<wood_session::JointBeam>>> built;

    // every connector first, so a missing member throws before anything is added or cut
    for (const Relationship& row : relationships(guide)) {
        if (row.kind == Relation::support || std::find(kinds.begin(), kinds.end(), row.kind) == kinds.end())
            continue;

        std::shared_ptr<wood_session::JointBeam> connector;

        if (row.kind == Relation::cross_lap) {
            const std::vector<std::shared_ptr<wood_session::JointBeam>>& plates = plates_of_corner[row.seam_or_corner];

            if (plates.size() != 2)
                throw std::runtime_error("the cross lap of corner " + std::to_string(row.seam_or_corner) + " needs its two column plates in the same call");

            connector = wood_session::JointBeam::cross_lap(*plates[0], *plates[1]);
        } else
            connector = connector_of(row, members);

        built.push_back({row, connector});

        if (row.kind == Relation::column_plate)
            plates_of_corner[row.seam_or_corner].push_back(connector);
    }

    std::map<std::string, size_t> numbers;
    std::vector<std::shared_ptr<wood_session::JointBeam>> connectors;

    for (const auto& [row, connector] : built) {
        const std::string prefix = connector_prefix(row.kind);

        if (!numbers.count(prefix))
            numbers[prefix] = next_number(session, prefix);

        connector->name = fmt::format("{}_{}", prefix, numbers[prefix]++);
        paint(session, session.add_connector(connector, connector_group(session, members, row)), CONNECTOR_COLOR);
        connectors.push_back(connector);
    }

    return connectors;
}

// ═══════════════════════════════════════════════════════════════════════════
// Model
// ═══════════════════════════════════════════════════════════════════════════

Floor::Floor(const FloorGuide& floor_guide, const std::string& name) : wood_session::WoodSession(name), guide(floor_guide) {
}

void Floor::add_column(size_t corner) {

    if (members.columns.size() < 4)
        members.columns.resize(4);

    members.columns[corner % 4] = add_column_model(*this, guide, corner % 4, group_named(*this, quarter_group(*this, members.group, corner % 4), fmt::format("column_{}", corner % 4)));
}

void Floor::add_columns() {

    for (size_t q = 0; q < 4; q++)
        add_column(q);
}

void Floor::add_quarters() {

    for (size_t q = 0; q < 4; q++)
        members.quarters[q] = add_quarter_model(*this, guide.quarter(q), quarter_group(*this, members.group, q));
}

void Floor::add_oculus() {

    members.ring = add_oculus_model(*this, guide, members.group);
    members.oculus = group_named(*this, members.group, "oculus");
}

void Floor::add_members() {

    add_quarters();
    add_oculus();
    add_columns();
}

void Floor::add_connectors(const std::vector<Relation>& kinds) {

    for (const std::shared_ptr<wood_session::JointBeam>& connector : wood_floor::add_connectors(*this, guide, members, kinds))
        (connector->name.starts_with("connector_screws_") ? screws : connectors).push_back(connector);
}

void Floor::add_screws() {

    add_connectors({SCREW_RELATIONS.begin(), SCREW_RELATIONS.end()});
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// RELATIONSHIPS
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

using namespace wood_floor::geometry;

/// A ring, column or support reference.
static MemberRef shared_member(Family family, size_t index) {
    return MemberRef{-1, family, index, -1};
}

// ═══════════════════════════════════════════════════════════════════════════
// References
// ═══════════════════════════════════════════════════════════════════════════

std::string MemberRef::name() const {

    if (family == Family::ring)
        return fmt::format("oculus_{}", index);

    if (family == Family::column)
        return fmt::format("column_{}", index);

    if (family == Family::support)
        return fmt::format("support_{}", index);

    if (family == Family::beds)
        return fmt::format("beds_{}_{}_{}", row, index, quarter);

    return fmt::format("{}_{}_{}", FAMILY_NAMES[static_cast<size_t>(family)], index, quarter);
}

double Relationship::area() const {
    return polygon_area(contact);
}

std::string relation_name(Relation kind) {

    const std::array<std::string, 12> kinds = {"support", "column_plate", "cross_lap", "seam_tie", "seam_wedge", "oculus_wedge", "block_dowels", "screw_rib_beam", "screw_beam_mitre", "screw_rib_corner", "screw_ring", "screw_oculus"};

    return kinds[static_cast<size_t>(kind)];
}

std::string Relationship::text() const {
    return fmt::format("{} {} - {}", relation_name(kind), a.name(), b.name());
}

// ═══════════════════════════════════════════════════════════════════════════
// Relationships
// ═══════════════════════════════════════════════════════════════════════════

/// The seam wedge of seam q: inner beam 0 of q and inner beam 2 of q + 1 on the seam plane, the contact where their end faces on it overlap; run on to the bay's outer face when the beams run through the rib band.
static Relationship seam_wedge(const FloorGuide& guide, size_t q) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    Relationship row;
    row.kind = Relation::seam_wedge;
    row.a = quarter_member(q, Family::inner_beams, 0);
    row.b = quarter_member((q + 1) % 4, Family::inner_beams, 2);
    row.plane = guide.seams[q].plane_into(q).transformed(lift);
    row.contact = Polyline(overlap(guide.quarter(q).inner_beams()[0].bottom, guide.quarter((q + 1) % 4).inner_beams()[2].bottom, guide.seams[q].plane_into(q))).closed().transformed(lift);
    row.type = wood_session::ContactType::side_side;
    row.seam_or_corner = q;

    if (guide.seam_through_ribs)
        row.end = guide.edges[q].band[0].transformed(lift);

    return row;
}

/// The oculus wedge of quarter q: inner beam 1 of q and ring beam q on the tilted plane, the contact beam 1's loop on it.
static Relationship oculus_wedge(const FloorGuide& guide, size_t q) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    Relationship row;
    row.kind = Relation::oculus_wedge;
    row.a = quarter_member(q, Family::inner_beams, 1);
    row.b = shared_member(Family::ring, q);
    row.plane = guide.oculus_edges[q].tilted.transformed(lift);
    row.contact = Polyline(open_points(guide.quarter(q).inner_beams()[1].bottom)).closed().transformed(lift);
    row.type = wood_session::ContactType::side_side;
    row.seam_or_corner = q;

    return row;
}

/// The column plate of corner q on outer rib k: the rib's column end face on the fan side plane where it meets the column's carved face, down to the middle cutter level.
static Relationship column_plate(const FloorGuide& guide, size_t q, size_t k) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const Outline rib = guide.quarter(q).outer_ribs()[k];
    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    Relationship row;
    row.kind = Relation::column_plate;
    row.a = shared_member(Family::column, q);
    row.b = quarter_member(q, Family::outer_ribs, k);
    row.plane = guide.columns[q].wedge_fan[k == 0 ? 0 : 2][0].transformed(lift);
    row.contact = Polyline(overlap(Polyline({top[1], top[2], bottom[2], bottom[1]}).closed(), Polyline(guide.quarter(q).column_face(k == 0 ? 0 : 2)).closed(), guide.columns[q].wedge_fan[k == 0 ? 0 : 2][0])).closed().transformed(lift);
    row.seam_or_corner = q;

    return row;
}

/// The cross lap of corner q: its two column plates, on outer ribs 0 and 1, crossing inside the column.
static Relationship cross_lap(size_t q) {

    Relationship row;
    row.kind = Relation::cross_lap;
    row.a = quarter_member(q, Family::outer_ribs, 0);
    row.b = quarter_member(q, Family::outer_ribs, 1);
    row.seam_or_corner = q;

    return row;
}

/// The tie of seam q: outer rib 0 of q and outer rib 1 of q + 1 meeting end to end on the seam plane, the contact rib 0's seam end face.
static Relationship seam_tie(const FloorGuide& guide, size_t q) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const Outline rib = guide.quarter(q).outer_ribs()[0];
    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    const size_t n = top.size();
    Relationship row;
    row.kind = Relation::seam_tie;
    row.a = quarter_member(q, Family::outer_ribs, 0);
    row.b = quarter_member((q + 1) % 4, Family::outer_ribs, 1);
    row.plane = guide.seams[q].plane_into(q).transformed(lift);
    row.contact = Polyline(std::vector<Point>{top[0], top[n - 2], bottom[n - 2], bottom[0]}).closed().transformed(lift);
    row.type = wood_session::ContactType::end_end;
    row.seam_or_corner = q;

    return row;
}

/// The dowels of block k of quarter q on one of its two ribs: the block's face on that rib's plane, corners 3 and 0 of its loops on the first rib plane, 1 and 2 on the second.
static Relationship block_dowels(const FloorGuide& guide, size_t q, size_t k, size_t side, const MemberRef& rib) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const Outline block = guide.quarter(q).wedges()[k];
    const std::vector<Point> top = block.top.get_points();
    const std::vector<Point> bottom = block.bottom.get_points();
    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const std::array<std::array<Plane, 2>, 3> ribs = {{{cp.outer_ribs[0][1], cp.inner_ribs[0][0]}, {cp.inner_ribs[0][1], cp.inner_ribs[1][1]}, {cp.inner_ribs[1][0], cp.outer_ribs[1][1]}}};
    Relationship row;
    row.kind = Relation::block_dowels;
    row.a = rib;
    row.b = quarter_member(q, Family::wedges, k);
    row.plane = ribs[k][side].transformed(lift);
    row.contact = side == 0 ? Polyline(std::vector<Point>{bottom[3], bottom[0], top[0], top[3]}).closed().transformed(lift) : Polyline(std::vector<Point>{bottom[1], bottom[2], top[2], top[1]}).closed().transformed(lift);
    row.seam_or_corner = q;

    return row;
}

/// The support of corner q under its column.
static Relationship support(const FloorGuide& guide, size_t q) {

    Relationship row;
    row.kind = Relation::support;
    row.a = shared_member(Family::support, q);
    row.b = shared_member(Family::column, q);
    row.plane = guide.columns[q].support_plane;
    row.seam_or_corner = q;

    return row;
}

std::vector<Relationship> relationships(const FloorGuide& guide) {

    std::vector<Relationship> rows;

    for (size_t q = 0; q < 4; q++)
        rows.push_back(seam_wedge(guide, q));

    for (size_t q = 0; q < 4; q++)
        rows.push_back(oculus_wedge(guide, q));

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            rows.push_back(column_plate(guide, q, k));

    for (size_t q = 0; q < 4; q++)
        rows.push_back(cross_lap(q));

    for (size_t q = 0; q < 4 && !guide.seam_through_ribs; q++)
        rows.push_back(seam_tie(guide, q));

    for (size_t q = 0; q < 4; q++) {
        rows.push_back(block_dowels(guide, q, 0, 0, quarter_member(q, Family::outer_ribs, 0)));
        rows.push_back(block_dowels(guide, q, 2, 1, quarter_member(q, Family::outer_ribs, 1)));
        rows.push_back(block_dowels(guide, q, 0, 1, quarter_member(q, Family::inner_ribs, 0)));
        rows.push_back(block_dowels(guide, q, 1, 0, quarter_member(q, Family::inner_ribs, 0)));
        rows.push_back(block_dowels(guide, q, 1, 1, quarter_member(q, Family::inner_ribs, 1)));
        rows.push_back(block_dowels(guide, q, 2, 0, quarter_member(q, Family::inner_ribs, 1)));
    }

    for (size_t q = 0; q < 4; q++)
        rows.push_back(support(guide, q));

    for (const Relationship& row : screw_relationships(guide))
        rows.push_back(row);

    return rows;
}

std::vector<Relationship> relationships(const FloorGuide& guide, Relation kind) {

    std::vector<Relationship> rows;

    for (const Relationship& row : relationships(guide))
        if (row.kind == kind)
            rows.push_back(row);

    return rows;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// SCREWS
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

using namespace wood_floor::geometry;

const double SCREW_LENGTH = 200.0; // mm, every assembly screw
const double RIB_END_MARGIN = 20.0; // mm a seam screw sits below the rib's top and above its bottom at its end when the seam runs through the rib band
const double SEAM_SCREW_OFFSET = 15.0; // mm the screws of the two ribs meeting at a seam sit either side of their axes, so their heads on the seam plane stay apart
const std::array<double, 2> RIB_BEAM_LEVELS = {0.25, 0.5}; // fractions of the seam depth, or of twice the tie's top less TIE_CLEARANCE when that is shallower: the outer rib's lower part at its seam end holds the tie key and its pocket
const double TIE_CLEARANCE = 10.0; // mm the lower tied rib screw stays above the tie key
const double CORNER_LEVELS = 7.0; // an oculus corner's depth in sevenths: six levels, one per screw on each side of the corner
const std::array<std::array<double, 2>, 2> MITRE_LEVELS = {{{2.0, 5.0}, {3.0, 6.0}}}; // per mitre k, the levels of its two screws; the two quarters' mitres at a seam put their heads on the seam plane at one point, so they differ
const std::array<double, 2> RIB_CORNER_LEVELS = {1.0, 4.0}; // the inner rib end screws at both corners, apart from that corner's mitre and oculus screws they cross
const std::array<std::array<double, 2>, 2> OCULUS_LEVELS = {{{3.0, 6.0}, {2.0, 5.0}}}; // per end k, the ring into the quarter's oculus beam, apart from that side's mitre and rib end screws
const std::array<double, 2> RING_LEVELS = {3.0, 6.0}; // the ring corner screws, apart from the oculus screws of the next quarter they cross
const double WEDGE_MARGIN = 1.5; // the wedge leaves this many beam thicknesses free at both ends of its contact, add_connectors' margin
const double COARSE_STEP = 5.0; // mm, the head positions an oculus screw first tries along the ring's inner face
const double COARSE_ANGLE = 2.0; // degrees, the directions it first tries
const double SEARCH_STEP = 0.25; // mm, the head positions it then tries around the best
const double ANGLE_STEP = 0.1; // degrees, the directions it then tries

/// The quarter's members a corner screw reads: the faces of the oculus beam, the seam beam end plane at that corner, the inner rib and their middles.
struct CornerFaces {
    std::array<Plane, 2> beam; // The oculus beam: the tilted face it shares with the ring, its back face.
    Plane beam_end; // The seam beam's inner face the oculus beam ends on at this corner.
    std::array<Plane, 2> rib; // The inner rib ending on the back face here.
    Point beam_body; // A point inside the oculus beam.
    Point rib_body; // A point inside the inner rib.
};

// ═══════════════════════════════════════════════════════════════════════════
// Lines in a level
// ═══════════════════════════════════════════════════════════════════════════

/// The horizontal line a plane cuts at level z.
static Line trace(const Plane& plane, double z) {
    return plane_plane(plane, level(z)).value();
}

/// The axis of a member between two faces at level z: the line midway between their traces.
static Line axis(const std::array<Plane, 2>& faces, double z) {

    const Line line0 = trace(faces[0], z);
    const Line line1 = trace(faces[1], z);
    const Vector d = line1.to_direction();
    const Point p0 = line0.start();
    const Point p1 = line1.start() + d * (p0 - line1.start()).dot(d);
    const Point middle = p0 + (p1 - p0) * 0.5;

    return Line::from_points(middle, middle + line0.to_direction());
}

/// The middle of a member outline: the mean of its two loops' area centroids.
static Point body(const Outline& outline) {
    return area_centroid(outline.top) + (area_centroid(outline.bottom) - area_centroid(outline.top)) * 0.5;
}

/// The distance of a point from a plane, positive on the side of inside.
static double depth(const Point& point, const Plane& plane, const Point& inside) {
    return signed_distance(inside, plane) < 0.0 ? -signed_distance(point, plane) : signed_distance(point, plane);
}

/// The level of screw i of a corner's level set: down from the datum in sevenths of the depth.
static double corner_level(double levels, double depth) {
    return -depth * levels / CORNER_LEVELS;
}

// ═══════════════════════════════════════════════════════════════════════════
// Screw rules
// ═══════════════════════════════════════════════════════════════════════════

/// A screw at level z through a side member into the member butting on it, along the butting member's axis: the head where that axis leaves the side member's far face, the tip on towards the butting member's body.
static Line along_axis(const std::array<Plane, 2>& butting, const Plane& far_face, const Point& butting_body, double z) {

    const Line line = axis(butting, z);
    const Point head = line_plane(line, far_face).value();
    Vector d = line.to_direction();

    if (d.dot(butting_body - head) < 0.0)
        d = -d;

    return Line::from_points(head, head + d * SCREW_LENGTH);
}

/// A screw at level z along a rib ending on a seam beam that runs through the rib band, its axis offset across the rib: from where that axis meets the beam's seam face, drilled before the wedge goes in, through the beam into the rib end.
static Line from_seam_face(const std::array<Plane, 2>& rib, const std::array<Plane, 2>& beam, double z, double offset) {

    const Line line = axis(rib, z);
    const Vector across = rib[0].z_axis() * offset;
    const Vector along = (line_plane(line, beam[1]).value() - line_plane(line, beam[0]).value()).normalized();
    const Point head = line_plane(Line::from_points(line.start() + across, line.end() + across), beam[0]).value();

    return Line::from_points(head, head + along * SCREW_LENGTH);
}

/// The ring's members an oculus screw reads beside the corner's: the ring beam's inner face and body, its end plane at this corner, and where the wedge starts along the contact.
struct RingFaces {
    Plane inner; // The ring beam's inner face, where the heads sit.
    Plane end; // The ring beam's end plane at this corner.
    Point body; // A point inside the ring beam.
    Point wedge_start; // The contact's top edge end at this corner moved along the edge by the wedge's margin.
    Vector along; // Along the contact's top edge, away from this corner.
    double band = 0.0; // Half the beam thickness: within it of the contact plane the wedge's pocket lies.
};

/// How far an oculus screw keeps inside: the head and the contact crossing inside the ring's end, the part within the pocket band short of the wedge, the tip inside the oculus beam's back face and the seam beam end.
static double oculus_clearance(const Point& head, const Vector& u, const CornerFaces& faces, const RingFaces& ring) {

    const Plane& contact = faces.beam[0];
    const Vector n = (ring.body - contact.origin()).dot(contact.z_axis()) < 0.0 ? -contact.z_axis() : contact.z_axis();
    const double s_head = (head - contact.origin()).dot(n);
    const double s_rate = u.dot(n);

    if (s_rate >= 0.0)
        return -1e300;

    const Point band_point = head + u * std::max((s_head - ring.band) / -s_rate, 0.0);
    const Point crossing = head + u * (s_head / -s_rate);
    const Point tip = head + u * SCREW_LENGTH;
    const double wedge = (ring.wedge_start - band_point).dot(ring.along);
    const double ring_part = std::min(depth(head, ring.end, ring.body), depth(crossing, ring.end, ring.body));
    const double beam = std::min({depth(tip, faces.beam[1], faces.beam_body), depth(tip, faces.beam[0], faces.beam_body), depth(tip, faces.beam_end, faces.beam_body)});

    return std::min({wedge, ring_part, beam});
}

/// The aim of an oculus screw: its head's offset along the ring's inner face from the corner and its angle off square to the contact, in degrees.
struct Aim {
    double offset = 0.0; // mm along the inner face from where the seam beam end plane meets it.
    double angle = 0.0; // Degrees from square to the contact towards the corner.
    double clearance = -1e300; // The aim's clearance.
};

/// The best aim on a grid of offsets and angles around a centre, each within its range.
static Aim best_aim(const Point& start, const Vector& across, const CornerFaces& faces, const RingFaces& ring, const Aim& centre, double offset_span, double angle_span, double offset_step, double angle_step) {

    Aim best = centre;

    for (double offset = std::max(centre.offset - offset_span, 0.0); offset <= centre.offset + offset_span; offset += offset_step)
        for (double angle = std::max(centre.angle - angle_span, 0.0); angle <= std::min(centre.angle + angle_span, 80.0); angle += angle_step) {
            const Point head = start + ring.along * offset;
            const Vector u = across * std::cos(angle * M_PI / 180.0) - ring.along * std::sin(angle * M_PI / 180.0);
            const double clearance = oculus_clearance(head, u, faces, ring);

            if (clearance > best.clearance + 1e-9)
                best = {offset, angle, clearance};
        }

    return best;
}

/// The oculus screw at level z: from the ring beam's inner face through the ring and the contact into the quarter's oculus beam towards the corner, crossing the wedge's band before the wedge starts, the 200 line with the largest clearance, found on a coarse grid and refined around its best.
static Line oculus_screw(const CornerFaces& faces, const RingFaces& ring, double z) {

    const Line inner = trace(ring.inner, z);
    const Point start = line_plane(inner, faces.beam_end).value();
    Vector across = (faces.beam_body - ring.body);
    across = Vector(across[0], across[1], 0.0);
    across = (across - ring.along * across.dot(ring.along)).normalized();

    const Aim coarse = best_aim(start, across, faces, ring, Aim{150.0, 40.0, -1e300}, 150.0, 40.0, COARSE_STEP, COARSE_ANGLE);
    const Aim fine = best_aim(start, across, faces, ring, Aim{coarse.offset, coarse.angle, -1e300}, COARSE_STEP, COARSE_ANGLE, SEARCH_STEP, ANGLE_STEP);
    const Point head = start + ring.along * fine.offset;
    const Vector u = across * std::cos(fine.angle * M_PI / 180.0) - ring.along * std::sin(fine.angle * M_PI / 180.0);

    return Line::from_points(head, head + u * SCREW_LENGTH);
}

// ═══════════════════════════════════════════════════════════════════════════
// Relationships
// ═══════════════════════════════════════════════════════════════════════════

/// A screw relationship: the two members, the face the second ends on and its end face there, the screws lifted to the floor.
static Relationship screw_row(const FloorGuide& guide, Relation kind, const MemberRef& a, const MemberRef& b, const Plane& plane, const std::vector<Point>& contact, const std::vector<Line>& screws, size_t corner) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    Relationship row;
    row.kind = kind;
    row.a = a;
    row.b = b;
    row.plane = plane.transformed(lift);
    row.contact = Polyline(contact).closed().transformed(lift);
    row.seam_or_corner = corner;

    for (const Line& screw : screws)
        row.screws.push_back(screw.transformed(lift));

    return row;
}

/// The corner faces of quarter q at end k: k 0 where seam beam 0 meets the oculus beam, k 1 where seam beam 2 does.
static CornerFaces corner_faces(const FloorGuide& guide, size_t q, size_t k) {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    CornerFaces faces;
    faces.beam = cp.inner_beams[1];
    faces.beam_end = cp.inner_beams[k == 0 ? 0 : 2][1];
    faces.rib = cp.inner_ribs[k];
    faces.beam_body = body(guide.quarter(q).inner_beams()[1]);
    faces.rib_body = body(guide.quarter(q).inner_ribs()[k]);

    return faces;
}

/// Outer rib k of quarter q and the seam beam it meets: two screws along the seam beam from the rib's outer face, the contact the beam's end where it meets the rib's inner face, so not the strip of the end below the rib when the soffit is deeper; when the seam runs through the rib band, two screws along the rib, 20 mm below its top and above its bottom and either side of its axis, from the beam's seam face through the beam into the rib end, the contact the rib's end on the beam.
static Relationship rib_beam(const FloorGuide& guide, size_t q, size_t k) {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const size_t beam = k == 0 ? 0 : 2;
    const Outline outline = guide.quarter(q).inner_beams()[beam];
    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    std::vector<Line> screws;

    if (guide.seam_through_ribs) {
        const Outline rib = guide.quarter(q).outer_ribs()[k];
        const std::vector<Point> rib_top = rib.top.get_points();
        const std::vector<Point> rib_bottom = rib.bottom.get_points();
        const size_t n = rib_top.size();

        for (double level : {-RIB_END_MARGIN, end_level(rib, guide.quarter(q).rib_seam_ends()[k]) + RIB_END_MARGIN})
            screws.push_back(from_seam_face(cp.outer_ribs[k], cp.inner_beams[beam], level, k == 0 ? -SEAM_SCREW_OFFSET : SEAM_SCREW_OFFSET));

        return screw_row(guide, Relation::screw_rib_beam, quarter_member(q, Family::outer_ribs, k), quarter_member(q, Family::inner_beams, beam), cp.inner_beams[beam][1], {rib_top[0], rib_top[n - 2], rib_bottom[n - 2], rib_bottom[0]}, screws, q);
    }

    const double depth = std::min(guide.static_h(), 2.0 * (TIE_TOP - TIE_CLEARANCE));

    for (double fraction : RIB_BEAM_LEVELS)
        screws.push_back(along_axis(cp.inner_beams[beam], cp.outer_ribs[k][0], body(outline), -depth * fraction));

    const std::vector<Point> contact = overlap(Polyline({top[3], top[0], bottom[0], bottom[3]}).closed(), guide.quarter(q).outer_ribs()[k].bottom, cp.outer_ribs[k][1]);

    return screw_row(guide, Relation::screw_rib_beam, quarter_member(q, Family::outer_ribs, k), quarter_member(q, Family::inner_beams, beam), cp.outer_ribs[k][1], contact, screws, q);
}

/// Seam beam 0 (k 0) or 2 (k 1) of quarter q into the oculus beam ending on it: two screws along the oculus beam from the seam plane, the contact the oculus beam's end.
static Relationship beam_mitre(const FloorGuide& guide, size_t q, size_t k) {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const size_t seam = k == 0 ? 0 : 2;
    const Outline outline = guide.quarter(q).inner_beams()[1];
    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const std::vector<Point> contact = k == 0 ? std::vector<Point>{top[3], top[0], bottom[0], bottom[3]} : std::vector<Point>{top[1], top[2], bottom[2], bottom[1]};
    std::vector<Line> screws;

    for (double levels : MITRE_LEVELS[k])
        screws.push_back(along_axis(cp.inner_beams[1], cp.inner_beams[seam][0], body(outline), corner_level(levels, guide.static_h())));

    return screw_row(guide, Relation::screw_beam_mitre, quarter_member(q, Family::inner_beams, seam), quarter_member(q, Family::inner_beams, 1), cp.inner_beams[seam][1], contact, screws, q);
}

/// The oculus beam of quarter q into inner rib k ending on its back face: two screws along the rib from where its axis leaves the tilted face, through the beam corner, so they also pass the seam beam's end where the corner needs it; the contact the rib's end face down to the beam's soffit.
static Relationship rib_corner(const FloorGuide& guide, size_t q, size_t k) {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const Outline outline = guide.quarter(q).inner_ribs()[k];
    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const CornerFaces faces = corner_faces(guide, q, k);
    const size_t seam = k == 0 ? 0 : 2;
    std::vector<Line> screws;
    bool through_seam = false;

    for (double levels : RIB_CORNER_LEVELS) {
        screws.push_back(along_axis(cp.inner_ribs[k], cp.inner_beams[1][0], faces.rib_body, corner_level(levels, guide.static_h())));
        through_seam = through_seam || depth(screws.back().start(), faces.beam_end, faces.beam_body) < 0.0;
        const double from_seam = signed_distance(screws.back().start(), cp.inner_beams[seam][0]);

        if (from_seam < 0.5 * SCREW_SPACING)
            throw std::runtime_error(fmt::format("quarter {}'s inner rib {} screw starts {:.3f} mm from the seam plane, less than half the screw spacing, where the next quarter's meets it: the bay is too narrow for the corner screws", q, k, from_seam));
    }

    const std::vector<Point> end = above({top[0], top[top.size() - 2], bottom[bottom.size() - 2], bottom[0]}, guide.soffit);
    Relationship row = screw_row(guide, Relation::screw_rib_corner, quarter_member(q, Family::inner_beams, 1), quarter_member(q, Family::inner_ribs, k), cp.inner_beams[1][1], end, screws, q);

    if (through_seam)
        row.through.push_back(quarter_member(q, Family::inner_beams, seam));

    return row;
}

/// Ring beam q into ring beam q + 1 starting on its inner face at oculus corner q: two screws along ring beam q + 1 from ring beam q's tilted face, the contact ring beam q + 1's start face.
static Relationship ring(const FloorGuide& guide, size_t q, const std::vector<Outline>& oculus) {

    const size_t next = (q + 1) % 4;
    const std::vector<Point> top = oculus[next].top.get_points();
    const std::vector<Point> bottom = oculus[next].bottom.get_points();
    const std::array<Plane, 2> faces = {guide.oculus_edges[next].tilted, guide.oculus_edges[next].ring_inner};
    std::vector<Line> screws;

    for (double levels : RING_LEVELS)
        screws.push_back(along_axis(faces, guide.oculus_edges[q].tilted, body(oculus[next]), corner_level(levels, guide.static_h())));

    return screw_row(guide, Relation::screw_ring, MemberRef{-1, Family::ring, q, -1}, MemberRef{-1, Family::ring, next, -1}, guide.oculus_edges[q].ring_inner, {top[2], top[3], bottom[3], bottom[2]}, screws, q);
}

/// Ring beam q into the oculus beam of quarter q at its end k: two aimed screws from the ring's inner face beyond the wedge, the contact the oculus wedge's.
static Relationship oculus(const FloorGuide& guide, size_t q, size_t k, const std::vector<Outline>& oculus) {

    const Outline outline = guide.quarter(q).inner_beams()[1];
    const std::vector<Point> loop = outline.bottom.get_points();
    const CornerFaces faces = corner_faces(guide, q, k);
    const double thickness = std::max(outline_thickness(outline), outline_thickness(oculus[q]));
    RingFaces ring;
    ring.inner = guide.oculus_edges[q].ring_inner;
    ring.end = k == 0 ? guide.oculus_edges[(q + 1) % 4].tilted : guide.oculus_edges[(q + 3) % 4].ring_inner;
    ring.body = body(oculus[q]);
    const Point end = k == 0 ? loop[0] : loop[1];
    ring.along = ((k == 0 ? loop[1] : loop[0]) - end).normalized();
    ring.wedge_start = end + ring.along * (WEDGE_MARGIN * thickness);
    ring.band = 0.5 * guide.inner_beams;
    std::vector<Line> screws;

    for (double levels : OCULUS_LEVELS[k])
        screws.push_back(oculus_screw(faces, ring, corner_level(levels, guide.static_h())));

    return screw_row(guide, Relation::screw_oculus, MemberRef{-1, Family::ring, q, -1}, quarter_member(q, Family::inner_beams, 1), guide.oculus_edges[q].tilted, {loop.begin(), loop.end() - 1}, screws, q);
}

std::vector<Relationship> geometry::screw_relationships(const FloorGuide& guide) {

    const std::vector<Outline> rings = guide.oculus();
    std::vector<Relationship> rows;

    for (size_t q = 0; q < 4; q++) {
        for (size_t k = 0; k < 2; k++)
            rows.push_back(rib_beam(guide, q, k));

        for (size_t k = 0; k < 2; k++)
            rows.push_back(beam_mitre(guide, q, k));

        for (size_t k = 0; k < 2; k++)
            rows.push_back(rib_corner(guide, q, k));
    }

    for (size_t q = 0; q < 4; q++)
        rows.push_back(ring(guide, q, rings));

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            rows.push_back(oculus(guide, q, k, rings));

    return rows;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// CONTACT VERIFICATION
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Verification
// ═══════════════════════════════════════════════════════════════════════════

/// The top edge of a polygon: its highest edge, the longest among those at the top.
static Line top_edge(const std::vector<Point>& points) {

    Line best;
    double best_height = -1e300;
    double best_length = -1.0;

    for (size_t i = 0; i < points.size(); i++) {
        const Point& a = points[i];
        const Point& b = points[(i + 1) % points.size()];
        const double height = 0.5 * (a[2] + b[2]);
        const double length = (b - a).magnitude();

        if (height > best_height + 1e-6 || (std::abs(height - best_height) <= 1e-6 && length > best_length)) {
            best = Line::from_points(a, b);
            best_height = height;
            best_length = length;
        }
    }

    return best;
}

/// The name of a contact type.
static std::string type_name(wood_session::ContactType type) {

    if (type == wood_session::ContactType::side_side)
        return "side_side";

    if (type == wood_session::ContactType::end_end)
        return "end_end";

    return fmt::format("type {}", static_cast<int>(type));
}

std::shared_ptr<wood_session::InteractionContactFace> require_contact(wood_session::WoodSession& session, const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b, wood_session::ContactType expected, const std::string& relation) {

    const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(uncut(*a), uncut(*b));

    if (!contact)
        throw std::runtime_error(fmt::format("no contact for {} between {} and {}", relation, a->name, b->name));

    if (expected != wood_session::ContactType::unknown && contact->type != expected)
        throw std::runtime_error(fmt::format("the contact for {} between {} and {} is {}, not {}", relation, a->name, b->name, type_name(contact->type), type_name(expected)));

    return contact;
}

/// How the searched contact disagrees with the constructed one: the plane normal, the top edge midpoint and length, and the area, beyond the tolerance; empty when it agrees.
static std::string disagreement(const Relationship& row, const wood_session::InteractionContactFace& found, double tolerance) {

    const std::vector<Point> mine = geometry::open_points(row.contact);
    const std::vector<Point> theirs = geometry::open_points(found.polygon);
    const Vector normal = wood_session::compute_newell(theirs).normalized();
    const double angle = std::acos(std::clamp(std::abs(normal.dot(row.plane.z_axis())), 0.0, 1.0));
    const Line edge_mine = top_edge(mine);
    const Line edge_theirs = top_edge(theirs);
    const double midpoint = (edge_mine.center() - edge_theirs.center()).magnitude();
    const double length = std::abs(edge_mine.length() - edge_theirs.length());
    const double area_mine = row.area();
    const double area_theirs = geometry::polygon_area(found.polygon);
    const double area = std::abs(area_theirs - area_mine) / std::max(area_mine, 1e-300);

    if (angle > tolerance)
        return fmt::format("plane {:.3e} rad off", angle);

    if (midpoint > tolerance || length > tolerance)
        return fmt::format("top edge {:.3e} mm off, {:.3e} mm longer", midpoint, edge_theirs.length() - edge_mine.length());

    if (area > tolerance)
        return fmt::format("area {:.6f} against {:.6f}", area_theirs, area_mine);

    return "";
}

ContactCheck verify_contacts(wood_session::WoodSession& session, const FloorGuide& guide, const FloorMembers& members, double tolerance, const std::vector<Relation>& kinds) {

    ContactCheck check;
    std::vector<ContactMismatch>& mismatches = check.mismatches;

    for (const Relationship& row : relationships(guide)) {
        if (row.contact.point_count() == 0 || std::find(kinds.begin(), kinds.end(), row.kind) == kinds.end())
            continue;

        check.count++;

        const std::array<std::shared_ptr<Element>, 2> pair = members.pair(row);
        const std::shared_ptr<wood_session::InteractionContactFace> found = session.compute_face_contact(uncut(*pair[0]), uncut(*pair[1]));

        if (!found) {
            mismatches.push_back({row.text(), "missing"});
            continue;
        }

        if (row.type != wood_session::ContactType::unknown && found->type != row.type) {
            mismatches.push_back({row.text(), type_name(found->type) + " instead of " + type_name(row.type)});
            continue;
        }

        const std::string what = disagreement(row, *found, tolerance);

        if (!what.empty())
            mismatches.push_back({row.text(), what});
    }

    return check;
}

bool ContactCheck::ok() const {
    return mismatches.empty();
}

std::string ContactCheck::str() const {

    std::string text = fmt::format("{} of {} contacts verified by the kernel's search", count - mismatches.size(), count);

    for (const ContactMismatch& mismatch : mismatches)
        text += fmt::format("\n  mismatch: {}: {}", mismatch.relation, mismatch.what);

    return text;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE SCREW CHECK
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

const double SAMPLE_STEP = 0.25; // mm between the points a screw is sampled at against a pocket
const double NEAR = 30.0; // mm a solid's box is inflated by before a screw is measured against it

/// A convex solid of another connector a screw must keep out of: its mesh as triangles and its box.
struct KeepOut {
    std::string name; // The connector it belongs to.
    std::vector<wood_session::PlanarFace> faces; // The solid's faces, for the inside test.
    std::vector<std::array<Point, 3>> triangles; // Its faces fanned into triangles.
    AABB box; // Its box, inflated by NEAR.
    std::optional<std::vector<wood_session::PlanarFace>> within; // For a cutter, its target's solid faces: only the screw's points inside the target meet the pocket.
    std::vector<std::string> targets; // The guids of the connector's members.
};

// ═══════════════════════════════════════════════════════════════════════════
// Distances
// ═══════════════════════════════════════════════════════════════════════════

/// The distance between two segments.
static double segment_distance(const Line& s, const Line& t) {

    const Vector d1 = s.to_vector();
    const Vector d2 = t.to_vector();
    const Vector r = s.start() - t.start();
    const double a = d1.dot(d1);
    const double e = d2.dot(d2);
    const double f = d2.dot(r);
    const double c = d1.dot(r);
    const double b = d1.dot(d2);
    const double denominator = a * e - b * b;
    double u = denominator > 1e-12 ? std::clamp((b * f - c * e) / denominator, 0.0, 1.0) : 0.0;
    double v = (b * u + f) / e;

    if (v < 0.0) {
        v = 0.0;
        u = std::clamp(-c / a, 0.0, 1.0);
    } else if (v > 1.0) {
        v = 1.0;
        u = std::clamp((b - c) / a, 0.0, 1.0);
    }

    return ((s.start() + d1 * u) - (t.start() + d2 * v)).magnitude();
}

/// The distance from a point to the segment a b.
static double point_distance(const Point& p, const Point& a, const Point& b) {
    return (p - Line::from_points(a, b).closest_point(p).second).magnitude();
}

/// The distance from a point to a triangle.
static double triangle_distance(const Point& p, const std::array<Point, 3>& tri) {

    const Vector ab = tri[1] - tri[0];
    const Vector ac = tri[2] - tri[0];
    const Vector n = ab.cross(ac);
    const double area = n.magnitude();

    if (area < 1e-12)
        return std::min({point_distance(p, tri[0], tri[1]), point_distance(p, tri[1], tri[2]), point_distance(p, tri[2], tri[0])});

    const Vector unit = n * (1.0 / area);
    const double height = (p - tri[0]).dot(unit);
    const Point q = p - unit * height;
    bool inside = true;

    for (size_t i = 0; i < 3; i++)
        inside = inside && (tri[(i + 1) % 3] - tri[i]).cross(q - tri[i]).dot(unit) >= 0.0;

    if (inside)
        return std::abs(height);

    double best = 1e300;

    for (size_t i = 0; i < 3; i++)
        best = std::min(best, point_distance(p, tri[i], tri[(i + 1) % 3]));

    return best;
}

/// The keep-out of one closed solid.
static KeepOut keep_out(const std::string& name, const Mesh& mesh) {

    KeepOut solid;
    solid.name = name;
    solid.faces = wood_session::planar_faces(mesh);
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.to_vertices_and_faces();

    for (const std::vector<size_t>& face : data.second)
        for (size_t i = 1; i + 1 < face.size(); i++)
            solid.triangles.push_back({data.first[face[0]], data.first[face[i]], data.first[face[i + 1]]});

    solid.box = AABB::from_mesh(mesh, NEAR);

    return solid;
}

/// The signed distance from a screw's surface to a solid: the closest sampled axis point's distance less the radius, negative where the screw enters it, a pocket's only over the points inside its target; 1e300 when the solid is out of reach.
static double solid_clearance(const Line& screw, double radius, const KeepOut& solid) {

    const AABB reach = AABB::from_line(screw);

    if (std::abs(reach.cx - solid.box.cx) > reach.hx + solid.box.hx || std::abs(reach.cy - solid.box.cy) > reach.hy + solid.box.hy || std::abs(reach.cz - solid.box.cz) > reach.hz + solid.box.hz)
        return 1e300;

    const size_t samples = static_cast<size_t>(std::ceil(screw.length() / SAMPLE_STEP));
    double best = 1e300;

    for (size_t i = 0; i <= samples; i++) {
        const Point p = screw.start() + screw.to_vector() * (static_cast<double>(i) / samples);
        double distance = 1e300;

        if (solid.within && !wood_session::is_inside(*solid.within, p))
            continue;

        for (const std::array<Point, 3>& triangle : solid.triangles)
            distance = std::min(distance, triangle_distance(p, triangle));

        best = std::min(best, wood_session::is_inside(solid.faces, p) ? -distance - radius : distance - radius);
    }

    return best;
}

// ═══════════════════════════════════════════════════════════════════════════
// Check
// ═══════════════════════════════════════════════════════════════════════════

/// Every bore and every pocket or part of the connectors that are not screws, each part once from its connector, not again from its part child: the bores as lines run on by their overshoot, with their radius, each pocket within its target.
static void collect(const wood_session::WoodSession& session, std::vector<std::pair<Line, double>>& bores, std::vector<KeepOut>& solids) {

    for (const std::shared_ptr<wood_session::JointBeam>& connector : session.get_elements<wood_session::JointBeam>()) {
        if (connector->pre_drill || std::dynamic_pointer_cast<wood_session::ConnectorPart>(connector))
            continue;

        for (const Line& dowel : connector->drill_lines) {
            const Vector d = dowel.to_vector().normalized() * connector->drill_overshoot;
            bores.push_back({Line::from_points(dowel.start() - d, dowel.end() + d), connector->line_radius});
        }

        for (size_t i = 0; i < connector->parts.size(); i++) {
            solids.push_back(keep_out(connector->name, connector->part_mesh(i)));
            solids.back().targets = connector->targets;
        }

        for (size_t side = 0; side < connector->cutters.size() && side < connector->targets.size(); side++) {
            const std::vector<wood_session::PlanarFace> target = wood_session::planar_faces(uncut(*session.get_element<Element>(connector->targets[side]))->element_geometry_mesh());

            for (const std::array<Polyline, 2>& cutter : connector->cutters[side]) {
                solids.push_back(keep_out(connector->name, Mesh::loft({cutter[0]}, {cutter[1]}, true)));
                solids.back().within = target;
                solids.back().targets = connector->targets;
            }
        }
    }
}

/// How much of a screw each member it names holds, their solids before any cut, the stretches clipped to the screw.
static std::vector<double> held(const wood_session::WoodSession& session, const wood_session::JointBeam& connector, const Line& screw) {

    std::vector<double> lengths;

    for (const std::string& guid : connector.targets) {
        const std::shared_ptr<Element> target = session.get_element<Element>(guid);
        lengths.push_back(0.0);

        for (const std::array<double, 2>& stretch : wood_session::inside_stretches(uncut(*target)->element_geometry_mesh(), screw))
            lengths.back() += std::max(0.0, std::min(stretch[1], screw.length()) - std::max(stretch[0], 0.0));
    }

    return lengths;
}

/// Measures one screw against the other screws after it, the bores and the solids, and how its members hold it; a screw drilled from a seam face before the wedge goes in, its member drilled_from, may cross the parts and pockets of the connectors on that member.
static void check_screw(const wood_session::WoodSession& session, const wood_session::JointBeam& connector, size_t index, const std::vector<std::pair<Line, double>>& bores, const std::vector<KeepOut>& solids, const std::string& drilled_from, ScrewCheck& check) {

    const Line& screw = connector.drill_lines[index];
    const std::string name = fmt::format("{} screw {}", connector.name, index);
    const std::vector<double> lengths = held(session, connector, screw);
    const double total = std::accumulate(lengths.begin(), lengths.end(), 0.0);
    check.embedded_min_mm = std::min(check.embedded_min_mm, total);
    check.member_min_mm = std::min({check.member_min_mm, lengths[0], lengths[1]});

    if (lengths[0] < 1e-6 || lengths[1] < 1e-6 || std::abs(total - screw.length()) > 1e-3)
        check.misfits.push_back(fmt::format("{}: held {:.3f} + {:.3f} of {:.3f} mm over {} members", name, lengths[0], lengths[1], screw.length(), lengths.size()));

    for (const std::pair<Line, double>& bore : bores) {
        const double clearance = segment_distance(screw, bore.first) - connector.line_radius - bore.second;
        check.screw_bore_mm = std::min(check.screw_bore_mm, clearance);

        if (clearance < 0.0)
            check.misfits.push_back(fmt::format("{}: into a bore, {:.3f} mm", name, clearance));
    }

    for (const KeepOut& solid : solids) {
        if (!drilled_from.empty() && std::find(solid.targets.begin(), solid.targets.end(), drilled_from) != solid.targets.end())
            continue;

        const double clearance = solid_clearance(screw, connector.line_radius, solid);
        check.screw_pocket_mm = std::min(check.screw_pocket_mm, clearance);

        if (clearance < 0.0)
            check.misfits.push_back(fmt::format("{}: into {}, {:.3f} mm", name, solid.name, clearance));
    }
}

ScrewCheck check_screws(const wood_session::WoodSession& session, const FloorGuide& guide, const std::vector<std::shared_ptr<wood_session::JointBeam>>& screws) {

    ScrewCheck check;
    std::vector<std::pair<Line, double>> bores;
    std::vector<KeepOut> solids;
    std::vector<std::pair<std::string, Line>> all;
    collect(session, bores, solids);
    std::vector<std::string> drilled_from;
    const std::vector<Relationship> rows = relationships(guide);
    const size_t screwed = static_cast<size_t>(std::count_if(rows.begin(), rows.end(), [](const Relationship& row) { return !row.screws.empty(); }));

    if (screws.size() != screwed)
        throw std::invalid_argument(fmt::format("check_screws reads one screw connector per screw row in relationship order: {} connectors for {} rows", screws.size(), screwed));

    for (const Relationship& row : rows)
        if (!row.screws.empty()) {
            check.counts[row.kind] += screws[drilled_from.size()]->drill_lines.size();
            drilled_from.push_back(row.kind == Relation::screw_rib_beam && guide.seam_through_ribs ? screws[drilled_from.size()]->targets[1] : "");
        }

    for (size_t c = 0; c < screws.size(); c++) {
        const wood_session::JointBeam& connector = *screws[c];

        for (size_t i = 0; i < connector.drill_lines.size(); i++) {
            check_screw(session, connector, i, bores, solids, drilled_from[c], check);
            all.push_back({fmt::format("{} screw {}", connector.name, i), connector.drill_lines[i]});
        }
    }

    for (size_t i = 0; i < all.size(); i++)
        for (size_t j = i + 1; j < all.size(); j++) {
            const double distance = segment_distance(all[i].second, all[j].second);
            check.screw_screw_mm = std::min(check.screw_screw_mm, distance);

            if (distance < SCREW_SPACING)
                check.misfits.push_back(fmt::format("{} and {}: {:.3f} mm apart", all[i].first, all[j].first, distance));
        }

    return check;
}

std::string ScrewCheck::str() const {

    std::string text = "screws:";
    size_t total = 0;

    for (const std::pair<const Relation, size_t>& count : counts) {
        text += fmt::format(" {} {},", relation_name(count.first), count.second);
        total += count.second;
    }

    text += fmt::format(" {} in all\n", total);
    text += fmt::format("  closest: screw to screw {:.3f} mm (axes), screw to bore {:.3f} mm, screw to pocket or part {:.3f} mm (surfaces)\n", screw_screw_mm, screw_bore_mm, screw_pocket_mm);
    text += fmt::format("  held: every screw {:.3f} mm in the members it names at the least, {:.3f} mm in one member of its joint\n", embedded_min_mm, member_min_mm);
    text += fmt::format("  misfits: {}", misfits.size());

    for (const std::string& misfit : misfits)
        text += "\n    " + misfit;

    return text;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE BREP CHECK
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Counting
// ═══════════════════════════════════════════════════════════════════════════

/// A dowel or a connector part: a child a connector draws on its own.
static bool is_connector_child(const std::shared_ptr<Element>& element) {

    return std::dynamic_pointer_cast<wood_session::Dowel>(element) || std::dynamic_pointer_cast<wood_session::ConnectorPart>(element);
}

/// A member its joints cut: not a joint, and its model differs from the uncut element.
static bool is_cut_member(const std::shared_ptr<Element>& element) {

    return !std::dynamic_pointer_cast<wood_session::Joint>(element) && element->model_geometry_mesh().number_of_vertices() != element->element_geometry_mesh().number_of_vertices();
}

size_t count_bores(const BRep& brep) {

    size_t bores = 0;

    for (const NurbsSurface& surface : brep.m_surfaces)
        if (surface.is_rational())
            bores++;

    return bores;
}

/// The bores the dowels ask for in one solid, already cut by its pockets: one per stretch of a dowel inside it, dowels meeting end to end on one axis joined into one.
static size_t bore_stretches(const Mesh& solid, const std::vector<wood_session::Drill>& drills) {

    size_t stretches = 0;

    for (const wood_session::Drill& drill : wood_session::merged_drills(drills))
        for (const std::array<double, 2>& stretch : wood_session::inside_stretches(solid, drill.axis))
            if (stretch[1] > 1e-6 && stretch[0] < drill.axis.length() - 1e-6)
                stretches++;

    return stretches;
}

/// The bores the dowels and screws ask for over the scene: every stretch of one inside a target's pocketed solid or inside its connector's own part.
static size_t dowel_stretches(const wood_session::WoodSession& session) {

    std::map<std::string, std::vector<wood_session::Drill>> drills;
    size_t stretches = 0;

    for (const std::shared_ptr<wood_session::Joint>& joint : session.get_elements<wood_session::Joint>()) {
        std::vector<wood_session::Drill> own;

        for (const Line& dowel : joint->drill_axes())
            own.push_back({dowel, joint->line_radius});

        for (const std::string& target : joint->targets)
            drills[target].insert(drills[target].end(), own.begin(), own.end());

        if (const std::shared_ptr<wood_session::JointBeam> connector = std::dynamic_pointer_cast<wood_session::JointBeam>(joint))
            for (size_t i = 0; i < connector->parts.size(); i++)
                stretches += bore_stretches(wood_session::apply_solid_cuts(connector->part_mesh(i), connector->solid_cuts, false), own);
    }

    for (const std::pair<const std::string, std::vector<wood_session::Drill>>& element : drills) {
        const std::shared_ptr<Element> target = session.get_element<Element>(element.first);
        const std::vector<wood_session::SolidCut>* cuts = wood_session::solid_cuts_of(*target);
        stretches += bore_stretches(cuts ? wood_session::apply_solid_cuts(target->element_geometry_mesh(), *cuts, false) : target->element_geometry_mesh(), element.second);
    }

    return stretches;
}

// ═══════════════════════════════════════════════════════════════════════════
// Check
// ═══════════════════════════════════════════════════════════════════════════

BrepCheck check_breps(const wood_session::WoodSession& session) {

    BrepCheck check;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    for (const std::shared_ptr<Element>& element : *session.objects.elements) {
        if (is_connector_child(element)) {
            if (std::dynamic_pointer_cast<wood_session::ConnectorPart>(element))
                check.part_bores += count_bores(element->model_geometry_brep());

            continue;
        }

        if (const std::shared_ptr<wood_session::JointBeam> connector = std::dynamic_pointer_cast<wood_session::JointBeam>(element)) {
            check.connectors += !connector->parts.empty() || !connector->drill_lines.empty();
            continue;
        }

        if (!is_cut_member(element))
            continue;

        const size_t bores = count_bores(element->model_geometry_brep());
        check.bores += bores;

        if (bores > 0)
            check.exact++;
        else
            check.faceted.push_back(element->name);
    }

    check.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    check.stretches = dowel_stretches(session);

    return check;
}

void compute_breps(wood_session::WoodSession& session) {

    for (const std::shared_ptr<Element>& element : *session.objects.elements)
        if (is_connector_child(element) || is_cut_member(element))
            element->compute_geometry_brep();
}

std::string BrepCheck::str() const {

    std::string text = fmt::format("BReps of the cut elements: {} with {} exact bores, {} faceted, and of {} connectors with {} exact bores through their parts, in {:.0f} ms\n", exact, bores, faceted.size(), connectors, part_bores, ms);
    text += fmt::format("Every dowel bores every element it passes: {} dowel stretches through members and parts, {} exact bores found", stretches, bores + part_bores);

    for (const std::string& name : faceted)
        text += "\n  faceted: " + name;

    return text;
}

}
