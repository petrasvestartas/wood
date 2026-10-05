#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

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
static BayEdge bay_edge(const FloorPlan& plan, size_t k, const FloorSizes& sizes) {

    BayEdge edge;
    edge.line = Line::from_points(plan.corners[k], plan.corners[(k + 1) % 4]);
    edge.midpoint = plan.midpoint(k);
    edge.band = pair(Plane::from_point_normal(edge.midpoint, edge.line.to_direction().cross(-Vector::z_axis())), sizes.outer_ribs);

    return edge;
}

/// Seam k: the line from the midpoint of edge k to the centre, its plane into quarter k at the half edge up to the oculus corner, and the beam thickness.
static Seam seam(const FloorPlan& plan, size_t k, const Point& centre, const Point& oculus_corner, const FloorSizes& sizes) {

    Seam result;
    result.index = k;
    result.line = Line::from_points(plan.midpoint(k), centre);
    result.oculus_corner = oculus_corner;
    result.thickness = sizes.inner_beams;
    result.plane = result.plane_into(k);

    return result;
}

/// Oculus edge q from oculus corner q to oculus corner q - 1: its tilted bearing plane, the back face offset into the quarter and the ring's inner plane.
static OculusEdge oculus_edge(const Point& corner, const Point& previous, const FloorSizes& sizes) {

    OculusEdge edge;
    edge.line = Line::from_points(corner, previous);
    const Plane plane = edge_plane(edge.line, -Vector::z_axis());
    edge.tilted = rotate(plane, -sizes.oculus_plane_angle * M_PI / 180.0, edge.line.to_direction(), edge.line.center());
    edge.back = plane.translate_by_normal(sizes.inner_beams);
    edge.ring_inner = edge.back.translate_by_normal(-sizes.inner_beams * 2.0);

    return edge;
}

/// The corner frame of column k: the edge directions at a right corner, the two axes symmetric about the corner bisector otherwise.
static std::array<Vector, 2> corner_frame(const FloorPlan& plan, size_t k) {

    const Vector after = (plan.corners[(k + 1) % 4] - plan.corners[k]).normalized();
    const Vector before = (plan.corners[(k + 3) % 4] - plan.corners[k]).normalized();

    if (std::abs(plan.corner_angle(k) - 90.0) <= RIGHT_ANGLE)
        return {after, before};

    const Vector bisector = (after + before).normalized();
    const Xform to_x = Xform::rotation(Vector::z_axis(), -45.0, true);
    const Xform to_y = Xform::rotation(Vector::z_axis(), 45.0, true);

    return {bisector.transformed(to_x), bisector.transformed(to_y)};
}

/// Column corner k before its fan: frame, head polygon, chamfer direction, the head's boundary sides, levels, axis and support plane.
static ColumnCorner column_corner(const FloorPlan& plan, size_t k, const FloorSizes& sizes) {

    ColumnCorner column;
    column.corner = plan.corners[k];
    const std::array<Vector, 2> frame = corner_frame(plan, k);
    column.x_axis = frame[0];
    column.y_axis = frame[1];

    const Vector& x = column.x_axis;
    const Vector& y = column.y_axis;
    const double head = sizes.column_head;
    const double chamfer = sizes.column_head_chamfer;
    column.head = {column.corner, column.corner + x * head, column.corner + x * head + y * chamfer, column.corner + x * chamfer + y * head, column.corner + y * head};
    column.chamfer_direction = (column.head[3] - column.head[2]).normalized();
    column.sides = {edge_plane(edge(column.head, 0), -Vector::z_axis()), edge_plane(edge(column.head, 4), -Vector::z_axis())};
    column.levels = {0.0, 0.0, -sizes.column_head_depth};
    column.axis_point = column.corner + (x + y) * (head * 0.5);
    column.support_plane = Plane::from_frame(column.axis_point, x, y, Vector::z_axis());
    column.axis = Line::from_points(column.axis_point, column.axis_point + Vector::z_axis() * sizes.bay_height);

    return column;
}

/// The wedge fan of a column corner: the tilted chamfer plane and the two side planes through the head edges, each leaning parallel to the chamfer plane's crease with the inner rib's central face.
static std::array<std::array<Plane, 2>, 3> wedge_fan(const ColumnCorner& column, const ConstructionPlanes& cp, const FloorSizes& sizes) {

    const Line side0 = edge(column.head, 1);
    const Line side1 = edge(column.head, 2);
    const Line side2 = edge(column.head, 3);

    const Plane tilted = rotate(edge_plane(side1, Vector::z_axis()), sizes.wedge_plane_angle * M_PI / 180.0, side1.to_direction(), side1.center());
    const Line line0 = plane_plane(cp.inner_ribs[0][1], tilted).value();
    const Line line1 = plane_plane(cp.inner_ribs[1][1], tilted).value();
    const Plane wedge0 = Plane::from_point_normal(side0.center(), line0.to_direction().cross(side0.to_direction()));
    const Plane wedge2 = Plane::from_point_normal(side2.center(), (-line1.to_direction()).cross(side2.to_direction()));

    return {pair(wedge0, sizes.wedge), pair(tilted, sizes.wedge * sizes.middle_wedge_factor), pair(wedge2, sizes.wedge)};
}

/// The signed offset of the column's outer faces from the bay edges and the three wedge seats left on the head beyond the rib bands (R8).
static void column_seats(ColumnCorner& column, const FloorPlan& plan, size_t k, const ConstructionPlanes& cp, const FloorSizes& sizes) {

    const double phi = (plan.corner_angle(k) - 90.0) * 0.5 * M_PI / 180.0;
    const double offset = sizes.column_head * std::sin(phi);
    const double band = (sizes.outer_ribs - offset) / std::cos(phi);
    column.column_offset = {offset, offset};

    const double chamfer_length = (column.head[3] - column.head[2]).magnitude();
    const double sin0 = std::abs(cp.inner_ribs[0][0].z_axis().dot(column.chamfer_direction));
    const double sin1 = std::abs(cp.inner_ribs[1][0].z_axis().dot(column.chamfer_direction));
    column.wedge_seat = {sizes.column_head_chamfer - band, chamfer_length - sizes.inner_ribs / sin0 - sizes.inner_ribs / sin1, sizes.column_head_chamfer - band};
}

// ═══════════════════════════════════════════════════════════════════════════
// Quarter geometry
// ═══════════════════════════════════════════════════════════════════════════

/// The member planes of quarter q from the shared entities, every one at the quarter's own points: the bands at the half-edge midpoints, the seams at the half seams, the oculus edge and the column fan as they are.
static ConstructionPlanes construction_planes(const Floor& floor, size_t q, ColumnCorner& column) {

    const FloorSizes& sizes = floor.sizes;
    const std::vector<Point>& polygon = floor.geometry[q].polygon;
    ConstructionPlanes cp;

    const Plane outer0 = reoriginated(floor.edges[q].band[0], edge(polygon, 0).center());
    const Plane outer1 = reoriginated(floor.edges[(q + 3) % 4].band[0], edge(polygon, 4).center());
    cp.outer_ribs = {pair(outer0, sizes.outer_ribs), pair(outer1, sizes.outer_ribs)};

    const OculusEdge& oculus = floor.oculus_edges[q];
    cp.inner_beams = {floor.seams[q].faces_into(q), {oculus.tilted, oculus.back}, floor.seams[(q + 3) % 4].faces_into(q)};

    const Plane xy = level(0.0);
    const Point p0 = plane_plane_plane(xy, cp.inner_beams[0][1], cp.inner_beams[1][1]).value();
    const Point p1 = plane_plane_plane(xy, cp.inner_beams[1][1], cp.inner_beams[2][1]).value();
    const Point p2 = column.head[2];
    const Point p3 = column.head[3];
    const Plane rib0 = Plane::from_point_normal(p2 + (p0 - p2) * 0.5, (p0 - p2).cross(-Vector::z_axis()));
    const Plane rib1 = Plane::from_point_normal(p3 + (p1 - p3) * 0.5, (p1 - p3).cross(Vector::z_axis()));
    cp.inner_ribs = {pair(rib0, sizes.inner_ribs), pair(rib1, sizes.inner_ribs)};

    column.wedge_fan = wedge_fan(column, cp, sizes);
    cp.wedges = {column.wedge_fan[0], column.wedge_fan[1], column.wedge_fan[2], pair(cp.inner_beams[0][1], sizes.inner_beams), pair(cp.inner_beams[1][1], sizes.inner_beams), pair(cp.inner_beams[2][1], sizes.inner_beams)};

    cp.t_sections = {
        pair(cp.outer_ribs[0][1], sizes.tsections),
        pair(cp.inner_ribs[0][0], -sizes.tsections),
        pair(cp.inner_ribs[0][1], sizes.tsections),
        pair(cp.inner_ribs[1][0], -sizes.tsections),
        pair(cp.inner_ribs[1][1], sizes.tsections),
        pair(cp.outer_ribs[1][1], sizes.tsections),
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
        {cp.wedges[3][0], cp.inner_ribs[0][0], cp.wedges[3][1], cp.outer_ribs[0][1]},
        {cp.wedges[4][0], cp.inner_ribs[1][1], cp.wedges[4][1], cp.inner_ribs[0][1]},
        {cp.wedges[5][0], cp.outer_ribs[1][1], cp.wedges[5][1], cp.inner_ribs[1][0]},
    });
    result.t_sections = quads({
        {cp.t_sections[0][0], cp.inner_beams[0][1], cp.t_sections[0][1], cp.wedges[0][1]},
        {cp.t_sections[1][0], cp.inner_beams[0][1], cp.t_sections[1][1], cp.wedges[0][1]},
        {cp.t_sections[2][0], cp.inner_beams[1][1], cp.t_sections[2][1], cp.wedges[1][1]},
        {cp.t_sections[3][0], cp.inner_beams[2][1], cp.t_sections[3][1], cp.wedges[2][1]},
        {cp.t_sections[4][0], cp.inner_beams[1][1], cp.t_sections[4][1], cp.wedges[1][1]},
        {cp.t_sections[5][0], cp.inner_beams[2][1], cp.t_sections[5][1], cp.wedges[2][1]},
    });

    return result;
}

/// The outer parabola over a rib quad, a 7-point Bezier: from -height at the run-in along the axis past the fan plane's datum trace, controlled at the axis midpoint at -static_h, to the seam at -static_h.
static Polyline outer_parabola(const Polyline& quad, double run_in, const FloorSizes& sizes) {

    const Point start = quad.get_point(0);
    const Point end = quad.get_point(1);
    const Point trimmed = start + (end - start).normalized() * run_in;
    const Point middle = trimmed + (end - trimmed) * 0.5;

    return Polyline::quadratic_points(trimmed + Vector(0.0, 0.0, -sizes.height), middle + Vector(0.0, 0.0, -sizes.static_h()), end + Vector(0.0, 0.0, -sizes.static_h()));
}

/// The z where an outer rib's soffit, its first chord extended, meets its fan plane: the bottom of the rib's column end face.
static double fan_end(const Polyline& quad, double run_in, const Plane& fan, const Plane& seam, const FloorSizes& sizes) {

    const std::vector<Point> pts = trim(outer_parabola(quad, run_in, sizes), fan, seam).get_points();
    const double d0 = std::abs((pts.front() - fan.origin()).dot(fan.z_axis()));
    const double d1 = std::abs((pts.back() - fan.origin()).dot(fan.z_axis()));

    return d0 > d1 ? pts.back()[2] : pts.front()[2];
}

/// The run-in that lands an outer rib's end on the level, by the secant from the wedge; throws when it leaves the axis or does not converge.
static double run_in_to_level(const Polyline& quad, const Plane& fan, const Plane& seam, double level, const FloorSizes& sizes) {

    const double axis = (quad.get_point(1) - quad.get_point(0)).magnitude();
    double x0 = sizes.wedge;
    double f0 = fan_end(quad, x0, fan, seam, sizes) - level;

    if (std::abs(f0) <= RUN_IN_TOLERANCE)
        return x0;

    double x1 = x0 + 1.0;
    double f1 = fan_end(quad, x1, fan, seam, sizes) - level;

    for (size_t i = 0; i < RUN_IN_STEPS; i++) {
        if (std::abs(f1) <= RUN_IN_TOLERANCE)
            return x1;

        const double x2 = x1 - f1 * (x1 - x0) / (f1 - f0);

        if (x2 <= 0.0 || x2 >= axis)
            throw std::runtime_error(fmt::format("an outer rib's run-in to the column level {:.3f} leaves its axis: {:.3f} of {:.3f} mm", level, x2, axis));

        x0 = x1;
        f0 = f1;
        x1 = x2;
        f1 = fan_end(quad, x1, fan, seam, sizes) - level;
    }

    throw std::runtime_error(fmt::format("an outer rib's run-in to the column level {:.3f} did not converge: {:.3e} mm off", level, f1));
}

/// Per outer rib the run-in that lands its end on the corner's shared level, the shallower of the two ends at the wedge.
static std::array<double, 2> run_ins(const ConstructionPlanes& cp, const ConstructionQuads& quads, const FloorSizes& sizes) {

    const std::array<Plane, 2> fans = {cp.wedges[0][0], cp.wedges[2][0]};
    const std::array<Plane, 2> seams = {cp.inner_beams[0][0], cp.inner_beams[2][0]};
    const double level = std::max(fan_end(quads.outer_ribs[0], sizes.wedge, fans[0], seams[0], sizes), fan_end(quads.outer_ribs[1], sizes.wedge, fans[1], seams[1], sizes));

    return {run_in_to_level(quads.outer_ribs[0], fans[0], seams[0], level, sizes), run_in_to_level(quads.outer_ribs[1], fans[1], seams[1], level, sizes)};
}

/// The column blocks' far planes over the ribs' run-ins: each side block its fan plane offset by its own rib's run-in, the middle block by middle_wedge_factor times their mean.
static void block_planes(ConstructionPlanes& cp, ColumnCorner& column, const std::array<double, 2>& run_in, const FloorSizes& sizes) {

    const std::array<double, 3> thickness = {run_in[0], sizes.middle_wedge_factor * (0.5 * (run_in[0] + run_in[1])), run_in[1]};

    for (size_t i = 0; i < 3; i++) {
        column.wedge_fan[i][1] = column.wedge_fan[i][0].translate_by_normal(thickness[i]);
        cp.wedges[i][1] = column.wedge_fan[i][1];
    }
}

/// Per rib axis (outer 0, outer 1, shadow 0, shadow 1) the parabola and its two offsets by tsections: the outer ones from the rib quads over their run-ins, the shadows projected onto the inner ribs' outer faces along the outer rib normals.
static std::vector<std::array<Polyline, 3>> boundary_parabolas(const ConstructionPlanes& cp, const ConstructionQuads& quads, const FloorSizes& sizes, const std::array<double, 2>& run_in) {

    std::vector<std::array<Polyline, 3>> parabolas;

    for (size_t k = 0; k < 2; k++) {
        const Polyline parabola = outer_parabola(quads.outer_ribs[k], run_in[k], sizes);
        parabolas.push_back({parabola, offset_polyline(parabola, sizes.tsections), offset_polyline(parabola, 2.0 * sizes.tsections)});
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
static void compute_quarter(Floor& floor, size_t q) {

    QuarterGeometry& geometry = floor.geometry[q];
    ColumnCorner& column = floor.columns[q];
    const FloorSizes& sizes = floor.sizes;

    geometry.planes = construction_planes(floor, q, column);
    column_seats(column, floor.plan, q, geometry.planes, sizes);
    geometry.quads = construction_quads(geometry.planes);
    geometry.run_in = run_ins(geometry.planes, geometry.quads, sizes);
    block_planes(geometry.planes, column, geometry.run_in, sizes);
    geometry.quads = construction_quads(geometry.planes);
    geometry.parabolas = boundary_parabolas(geometry.planes, geometry.quads, sizes, geometry.run_in);
    geometry.central_panel = central_panel(geometry.planes, geometry.parabolas, sizes);
    geometry.bed_top_planes = bed_top_planes(geometry.planes, geometry.parabolas, geometry.central_panel);
}

/// The middle cutter level of the quarter's column by the rib-bottom rule: the deeper of its two outer ribs' bottom corners on their fan planes, on either face.
static double rib_bottom_level(const Quarter& quarter) {

    double level = 0.0;

    for (const Outline& rib : quarter.outer_ribs())
        level = std::min({level, rib.top.get_point(2)[2], rib.bottom.get_point(2)[2]});

    return level;
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

Floor::Floor(const FloorPlan& floor_plan, const FloorSizes& floor_sizes) : plan(floor_plan), sizes(floor_sizes) {

    std::string why;

    if (!plan.valid(why))
        throw std::invalid_argument("invalid floor plan: " + why);

    centre = plan.centre();
    oculus_corners = plan.oculus_corners();

    for (size_t q = 0; q < 4; q++) {
        edges[q] = bay_edge(plan, q, sizes);
        seams[q] = seam(plan, q, centre, oculus_corners[q], sizes);
        oculus_edges[q] = oculus_edge(oculus_corners[q], oculus_corners[(q + 3) % 4], sizes);
        columns[q] = column_corner(plan, q, sizes);
    }

    for (size_t q = 0; q < 4; q++) {
        geometry[q].polygon = {plan.corners[q], edges[q].midpoint, oculus_corners[q], oculus_corners[(q + 3) % 4], edges[(q + 3) % 4].midpoint};
        compute_quarter(*this, q);
    }

    for (size_t q = 0; q < 4; q++)
        columns[q].levels[1] = rib_bottom_level(quarter(q));
}

Quarter Floor::quarter(size_t q) const {
    return Quarter{*this, q % 4};
}

const QuarterGeometry& Quarter::geometry() const {
    return floor.geometry[index];
}

const FloorSizes& Quarter::sizes() const {
    return floor.sizes;
}

const ColumnCorner& Quarter::column() const {
    return floor.columns[index];
}

const OculusEdge& Quarter::oculus_edge() const {
    return floor.oculus_edges[index];
}

const Seam& Quarter::seam(size_t side) const {
    return floor.seams[side == 0 ? index : (index + 3) % 4];
}

const BayEdge& Quarter::edge(size_t side) const {
    return floor.edges[side == 0 ? index : (index + 3) % 4];
}

}
