#include "pch.h"
#include "src/templates/floor/floor_guide.h"

using namespace session_cpp;

namespace wood_floor {

const double EXTENSION = 1000.0; // how far a trace's ends are pushed out before the planes that end its member trim it

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// FLOORGUIDE: FROM FOUR CORNERS TO EVERY QUARTER'S GEOMETRY
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

FloorGuide::FloorGuide(const std::array<Point, 4>& guide_corners) : wood_session::WoodSession("floor_guide"), corners(guide_corners) {
    compute();
}

FloorGuide FloorGuide::rectangle(double half_x, double half_y) {
    return FloorGuide({
        Point(-half_x, -half_y, 0.0),
        Point(half_x, -half_y, 0.0),
        Point(half_x, half_y, 0.0),
        Point(-half_x, half_y, 0.0)
    });
}

void FloorGuide::compute() {

    // a recompute starts from an empty drawing
    static_cast<wood_session::WoodSession&>(*this) = wood_session::WoodSession("floor_guide");

    centre = Point::centroid({corners[0], corners[1], corners[2], corners[3]});

    for (size_t q = 0; q < 4; q++)
        oculus_corners[q] = centre + (midpoint(q) - centre).normalized() * oculus_radius;

    const std::string why = invalid();

    if (!why.empty())
        throw std::invalid_argument("invalid floor guide: " + why);

    edges.clear();
    seams.clear();
    oculus_edges.clear();
    columns.clear();
    geometry.clear();

    for (size_t q = 0; q < 4; q++) {
        edges.push_back(BayEdge(*this, q));
        seams.push_back(Seam(*this, q));
        oculus_edges.push_back(OculusEdge(*this, q));
        columns.push_back(ColumnCorner(*this, q));
    }

    const std::string clash = beam_corners_in_bands();

    if (!clash.empty())
        throw std::invalid_argument("invalid floor guide: " + clash);

    for (size_t q = 0; q < 4; q++)
        geometry.push_back(QuarterGeometry(*this, q, columns[q]));

    for (size_t q = 0; q < 4; q++)
        columns[q].levels[1] = rib_bottom_level(q);

    soffit = -static_h();

    for (size_t q = 0; q < 4; q++) {
        const Quarter view = quarter(q);
        const std::vector<Loops> outer = view.outer_ribs();
        const std::vector<Loops> inner = view.inner_ribs();

        for (size_t k = 0; k < 2; k++)
            soffit = std::min({soffit, Quarter::end_level(outer[k], view.rib_seam_ends()[k]), Quarter::end_level(inner[k], geometry[q].planes.inner_beams[1][1])});
    }

    draw();
}

std::string FloorGuide::invalid() const {

    if (rise <= 0.0 || rise >= height)
        return fmt::format("rise {:.3f} is not between 0 and height {:.3f}: the ribs need a parabola and a depth at the seam", rise, height);

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

        const double along = (oculus_corners[k] - centre).dot((midpoint(k) - centre).normalized());

        if (along <= 0.0 || along >= (midpoint(k) - centre).magnitude())
            return fmt::format("oculus corner {} is not between the centre and the midpoint of edge {}", k, k);
    }

    for (size_t k = 0; k < 4; k++)
        if (std::sin(oculus_corner_angle(k) * M_PI / 180.0) < std::sin(oculus_seam_angle(k) * M_PI / 180.0))
            return fmt::format("the ring beam leaves quarter {}'s oculus beam face uncovered at oculus corner {}: corner angle {:.3f}, seam angle {:.3f} degrees", (k + 1) % 4, k, oculus_corner_angle(k), oculus_seam_angle(k));

    return "";
}

std::string FloorGuide::beam_corners_in_bands() const {

    for (size_t q = 0; q < 4; q++) {
        const Plane& back = oculus_edges[q].back;
        const std::array<std::pair<Point, size_t>, 2> beam_corners = {{
            {Intersection::plane_plane_plane(Plane::xy_plane_at(0.0), seams[q].faces_into(q)[1], back).value(), q},
            {Intersection::plane_plane_plane(Plane::xy_plane_at(0.0), back, seams[(q + 3) % 4].faces_into(q)[1]).value(), (q + 3) % 4},
        }};

        for (const auto& [corner, k] : beam_corners) {
            const double inside = edges[k].band[1].signed_distance(corner);

            if (inside <= 0.0)
                return fmt::format("quarter {}'s inner beam corner lies {:.3f} mm inside the outer rib band of edge {}: the oculus is too close to the bay edge", q, -inside, k);
        }
    }

    return "";
}

double FloorGuide::rib_bottom_level(size_t q) const {

    double level = 0.0;

    for (const Loops& rib : quarter(q).outer_ribs())
        level = std::min({level, rib[0].get_point(2)[2], rib[1].get_point(2)[2]});

    return level;
}

double FloorGuide::static_h() const {
    return height - rise;
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

Quarter FloorGuide::quarter(size_t q) const {
    return Quarter(*this, q);
}

std::vector<Loops> FloorGuide::oculus() const {

    const Plane side0 = Plane::xy_plane_at(0.0);
    const Plane side1 = Plane::xy_plane_at(soffit + tsections);
    const Plane side2 = Plane::xy_plane_at(soffit);
    const Plane side3 = Plane::xy_plane_at(soffit + tsections * 2.0);

    std::vector<Plane> tilted;
    std::vector<Plane> inner;

    for (const OculusEdge& edge : oculus_edges) {
        tilted.push_back(edge.tilted);
        inner.push_back(edge.ring_inner);
    }

    std::vector<Loops> plates;

    for (size_t i = 0; i < 4; i++)
        plates.push_back(Quarter::loft({side2, tilted[(i + 1) % 4], side0, inner[(i + 3) % 4]}, tilted[i], inner[i], true));

    for (size_t i = 0; i < 4; i++) {
        const std::vector<Plane> sides = {inner[i], inner[(i + 1) % 4], inner[i].translate_by_normal(-tsections), inner[(i + 3) % 4].translate_by_normal(-tsections)};
        plates.push_back(Quarter::loft(sides, side2, side1));
    }

    plates.push_back(Quarter::loft(inner, side1, side3));

    return plates;
}

void FloorGuide::draw() {

    // one quarter family as it is drawn: its plan quads and face planes in member order and the index of its first rib parabola, -1 for none
    struct DrawnFamily {
        Family family;
        const std::vector<Polyline>& quads;
        const std::vector<std::array<Plane, 2>>& planes;
        int parabola;
    };

    const auto line = [this](Polyline polyline, const std::string& name, const Color& color, double width, const std::shared_ptr<TreeNode>& group) {
        polyline.name = name;
        polyline.linecolor = color;
        polyline.width = width;
        add_polyline(polyline, group);
    };

    for (size_t q = 0; q < 4; q++) {
        const std::string suffix = fmt::format("_{}", q);
        const QuarterGeometry& drawn = geometry[q];
        const std::shared_ptr<TreeNode> quarter = add_group("quarter" + suffix);
        const std::shared_ptr<TreeNode> plan = add_group("plan" + suffix, quarter);

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
            const std::shared_ptr<TreeNode> group = add_group(name + suffix, quarter);

            for (size_t i = 0; i < family.quads.size(); i++) {
                const std::shared_ptr<TreeNode> member = add_group(fmt::format("{}_{}{}", name, i, suffix), group);
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

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE SHARED ENTITIES
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

const double RIGHT_ANGLE = 1e-9; // degrees off 90 within which a corner counts as right, so a rectangle keeps its exact edge directions

BayEdge::BayEdge(const FloorGuide& guide, size_t k) {

    line = Line::from_points(guide.corners[k], guide.corners[(k + 1) % 4]);
    midpoint = guide.midpoint(k);
    band = ConstructionPlanes::pair(Plane::from_point_normal(midpoint, line.to_direction().cross(-Vector::z_axis())), guide.outer_ribs);
}

Seam::Seam(const FloorGuide& guide, size_t k) {

    index = k;
    line = Line::from_points(guide.midpoint(k), guide.centre);
    oculus_corner = guide.oculus_corners[k];
    thickness = guide.inner_beams;
    plane = plane_into(k);
}

Plane Seam::plane_into(size_t quarter) const {

    const Point& midpoint = line.start();

    return quarter % 4 == index ? Plane::from_line(Line::from_points(midpoint, oculus_corner), -Vector::z_axis()) : Plane::from_line(Line::from_points(oculus_corner, midpoint), -Vector::z_axis());
}

std::array<Plane, 2> Seam::faces_into(size_t quarter) const {
    return ConstructionPlanes::pair(plane_into(quarter), thickness);
}

OculusEdge::OculusEdge(const FloorGuide& guide, size_t q) {

    line = Line::from_points(guide.oculus_corners[q], guide.oculus_corners[(q + 3) % 4]);
    const Plane plane = Plane::from_line(line, -Vector::z_axis());
    tilted = plane.transformed(Xform::rotation_around_line(Line::from_points(line.center(), line.center() + line.to_direction()), -guide.oculus_plane_angle * M_PI / 180.0));
    back = plane.translate_by_normal(guide.inner_beams);
    ring_inner = back.translate_by_normal(-guide.inner_beams * 2.0);
}

ColumnCorner::ColumnCorner(const FloorGuide& guide, size_t k) {

    // the edge directions at a right corner, the two axes symmetric about the corner bisector otherwise
    corner = guide.corners[k];
    const Vector after = (guide.corners[(k + 1) % 4] - corner).normalized();
    const Vector before = (guide.corners[(k + 3) % 4] - corner).normalized();

    if (std::abs(guide.corner_angle(k) - 90.0) <= RIGHT_ANGLE) {
        x_axis = after;
        y_axis = before;
    } else {
        const Vector bisector = (after + before).normalized();
        x_axis = bisector.transformed(Xform::rotation(Vector::z_axis(), -45.0, true));
        y_axis = bisector.transformed(Xform::rotation(Vector::z_axis(), 45.0, true));
    }

    const double shaft = guide.column_head;
    const double chamfer = guide.column_head_chamfer;
    head = {corner, corner + x_axis * shaft, corner + x_axis * shaft + y_axis * chamfer, corner + x_axis * chamfer + y_axis * shaft, corner + y_axis * shaft};
    sides = {Plane::from_line(Line::from_points(head[0], head[1]), -Vector::z_axis()), Plane::from_line(Line::from_points(head[4], head[0]), -Vector::z_axis())};
    levels = {0.0, 0.0, -guide.column_head_depth};
    support_plane = Plane::from_frame(corner + (x_axis + y_axis) * (shaft * 0.5), x_axis, y_axis, Vector::z_axis());
}

void ColumnCorner::lean_fan(const std::array<Plane, 2>& inner_rib_faces, const FloorGuide& guide) {

    const Line side0 = Line::from_points(head[1], head[2]);
    const Line side1 = Line::from_points(head[2], head[3]);
    const Line side2 = Line::from_points(head[3], head[4]);

    const Plane tilted = Plane::from_line(side1, Vector::z_axis()).transformed(Xform::rotation_around_line(Line::from_points(side1.center(), side1.center() + side1.to_direction()), guide.wedge_plane_angle * M_PI / 180.0));
    const Line line0 = Intersection::plane_plane(tilted, inner_rib_faces[0]).value();
    const Line line1 = Intersection::plane_plane(tilted, inner_rib_faces[1]).value();
    const Plane wedge0 = Plane::from_point_normal(side0.center(), line0.to_direction().cross(side0.to_direction()));
    const Plane wedge2 = Plane::from_point_normal(side2.center(), (-line1.to_direction()).cross(side2.to_direction()));

    wedge_fan = {ConstructionPlanes::pair(wedge0, guide.wedge), ConstructionPlanes::pair(tilted, guide.wedge * guide.middle_wedge_factor), ConstructionPlanes::pair(wedge2, guide.wedge)};
}

std::shared_ptr<wood_session::Support> ColumnCorner::to_support() const {
    return std::make_shared<wood_session::Support>(support_plane, "support");
}

std::shared_ptr<wood_session::Column> ColumnCorner::to_column(const FloorGuide& guide, const wood_session::Support& support) const {

    const Point foot = support.column_foot();
    const Line axis = Line::from_points(foot, Point(foot[0], foot[1], guide.bay_height));
    const Plane frame = Plane::from_frame(corner, x_axis, y_axis, Vector::z_axis());

    return wood_session::Column::square(axis, frame, guide.column_head, guide.column_head + guide.column_head_chamfer, guide.column_head_depth);
}

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE GEOMETRY OF A QUARTER
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

const double RUN_IN_TOLERANCE = 1e-11; // mm an outer rib's end may sit off its corner's shared level; within it the rib keeps the wedge as its run-in
const size_t RUN_IN_STEPS = 50; // secant steps a run-in may take to land its rib's end on the shared level

QuarterGeometry::QuarterGeometry(const FloorGuide& guide, size_t q, ColumnCorner& column) {

    polygon = {guide.corners[q], guide.edges[q].midpoint, guide.oculus_corners[q], guide.oculus_corners[(q + 3) % 4], guide.edges[(q + 3) % 4].midpoint};
    planes = ConstructionPlanes(guide, q, polygon, column);
    quads = ConstructionQuads(planes);
    run_in = run_ins(guide);
    set_block_planes(column, guide);
    quads = ConstructionQuads(planes);
    parabolas = boundary_parabolas(guide);
    central_panel = CentralPanel(planes, parabolas, guide);
    bed_top_planes = fit_bed_top_planes();
}

// ═══════════════════════════════════════════════════════════════════════════
// Planes and quads
// ═══════════════════════════════════════════════════════════════════════════

ConstructionPlanes::ConstructionPlanes(const FloorGuide& guide, size_t q, const std::vector<Point>& polygon, ColumnCorner& column) {

    const Plane outer0 = guide.edges[q].band[0].moved_to(Line::from_points(polygon[0], polygon[1]).center());
    const Plane outer1 = guide.edges[(q + 3) % 4].band[0].moved_to(Line::from_points(polygon[4], polygon[0]).center());
    outer_ribs = {pair(outer0, guide.outer_ribs), pair(outer1, guide.outer_ribs)};

    const OculusEdge& oculus = guide.oculus_edges[q];
    inner_beams = {guide.seams[q].faces_into(q), {oculus.tilted, oculus.back}, guide.seams[(q + 3) % 4].faces_into(q)};

    const Plane xy = Plane::xy_plane_at(0.0);
    const Point p0 = Intersection::plane_plane_plane(xy, inner_beams[0][1], inner_beams[1][1]).value();
    const Point p1 = Intersection::plane_plane_plane(xy, inner_beams[1][1], inner_beams[2][1]).value();
    const Point p2 = column.head[2];
    const Point p3 = column.head[3];
    const Plane rib0 = Plane::from_point_normal(p2 + (p0 - p2) * 0.5, (p0 - p2).cross(-Vector::z_axis()));
    const Plane rib1 = Plane::from_point_normal(p3 + (p1 - p3) * 0.5, (p1 - p3).cross(Vector::z_axis()));
    inner_ribs = {pair(rib0, guide.inner_ribs), pair(rib1, guide.inner_ribs)};

    column.lean_fan({inner_ribs[0][1], inner_ribs[1][1]}, guide);
    wedges = {column.wedge_fan[0], column.wedge_fan[1], column.wedge_fan[2]};

    tsections = {
        pair(outer_ribs[0][1], guide.tsections),
        pair(inner_ribs[0][0], -guide.tsections),
        pair(inner_ribs[0][1], guide.tsections),
        pair(inner_ribs[1][1], guide.tsections),
        pair(inner_ribs[1][0], -guide.tsections),
        pair(outer_ribs[1][1], guide.tsections),
    };
}

std::array<Plane, 2> ConstructionPlanes::pair(const Plane& plane, double distance) {
    return {plane, plane.translate_by_normal(distance)};
}

ConstructionQuads::ConstructionQuads(const ConstructionPlanes& cp) {

    outer_ribs = quads({
        {cp.outer_ribs[0][0], cp.inner_beams[0][0], cp.outer_ribs[0][1], cp.wedges[0][0]},
        {cp.outer_ribs[1][0], cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.wedges[2][0]},
    });
    inner_beams = quads({
        {cp.inner_beams[0][0], cp.inner_beams[1][0], cp.inner_beams[0][1], cp.outer_ribs[0][1]},
        {cp.inner_beams[1][0], cp.inner_beams[2][1], cp.inner_beams[1][1], cp.inner_beams[0][1]},
        {cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.inner_beams[2][1], cp.inner_beams[1][0]},
    });
    inner_ribs = quads({
        {cp.inner_ribs[0][1], cp.inner_beams[1][1], cp.inner_ribs[0][0], cp.wedges[1][0]},
        {cp.inner_ribs[1][1], cp.inner_beams[1][1], cp.inner_ribs[1][0], cp.wedges[1][0]},
    });
    wedges = quads({
        {cp.wedges[0][0], cp.outer_ribs[0][1], cp.wedges[0][1], cp.inner_ribs[0][0]},
        {cp.wedges[1][0], cp.inner_ribs[0][1], cp.wedges[1][1], cp.inner_ribs[1][1]},
        {cp.wedges[2][0], cp.inner_ribs[1][0], cp.wedges[2][1], cp.outer_ribs[1][1]},
    });
    tsections = quads({
        {cp.tsections[0][0], cp.inner_beams[0][1], cp.tsections[0][1], cp.wedges[0][1]},
        {cp.tsections[1][0], cp.inner_beams[0][1], cp.tsections[1][1], cp.wedges[0][1]},
        {cp.tsections[2][0], cp.inner_beams[1][1], cp.tsections[2][1], cp.wedges[1][1]},
        {cp.tsections[3][0], cp.inner_beams[1][1], cp.tsections[3][1], cp.wedges[1][1]},
        {cp.tsections[4][0], cp.inner_beams[2][1], cp.tsections[4][1], cp.wedges[2][1]},
        {cp.tsections[5][0], cp.inner_beams[2][1], cp.tsections[5][1], cp.wedges[2][1]},
    });
}

std::vector<Polyline> ConstructionQuads::quads(const std::vector<std::array<Plane, 4>>& family) {

    std::vector<Polyline> result;

    for (const std::array<Plane, 4>& planes : family)
        result.push_back(Polyline::from_planes({planes[3], planes[0], planes[1], planes[2]}, Plane::xy_plane_at(0.0)));

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Run-ins and the column blocks
// ═══════════════════════════════════════════════════════════════════════════

std::array<double, 2> QuarterGeometry::run_ins(const FloorGuide& guide) const {

    const std::array<Plane, 2> fans = {planes.wedges[0][0], planes.wedges[2][0]};
    const std::array<Plane, 2> seams = {planes.inner_beams[0][0], planes.inner_beams[2][0]};
    const double level = std::max(fan_end(quads.outer_ribs[0], guide.wedge, fans[0], seams[0], guide), fan_end(quads.outer_ribs[1], guide.wedge, fans[1], seams[1], guide));

    return {run_in_to_level(quads.outer_ribs[0], fans[0], seams[0], level, guide), run_in_to_level(quads.outer_ribs[1], fans[1], seams[1], level, guide)};
}

double QuarterGeometry::run_in_to_level(const Polyline& quad, const Plane& fan, const Plane& seam, double level, const FloorGuide& guide) {

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

double QuarterGeometry::fan_end(const Polyline& quad, double run_in, const Plane& fan, const Plane& seam, const FloorGuide& guide) {

    const std::vector<Point> pts = outer_parabola(quad, run_in, guide).trimmed(fan, seam, EXTENSION).get_points();
    const double d0 = std::abs(fan.signed_distance(pts.front()));
    const double d1 = std::abs(fan.signed_distance(pts.back()));

    return d0 > d1 ? pts.back()[2] : pts.front()[2];
}

void QuarterGeometry::set_block_planes(ColumnCorner& column, const FloorGuide& guide) {

    const std::array<double, 3> thickness = {run_in[0], guide.middle_wedge_factor * (0.5 * (run_in[0] + run_in[1])), run_in[1]};

    for (size_t i = 0; i < 3; i++) {
        column.wedge_fan[i][1] = column.wedge_fan[i][0].translate_by_normal(thickness[i]);
        planes.wedges[i][1] = column.wedge_fan[i][1];
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Parabolas and bed planes
// ═══════════════════════════════════════════════════════════════════════════

Polyline QuarterGeometry::outer_parabola(const Polyline& quad, double run_in, const FloorGuide& guide) {

    const Point start = quad.get_point(0);
    const Point end = quad.get_point(1);
    const Point trimmed = start + (end - start).normalized() * run_in;
    const Point middle = trimmed + (end - trimmed) * 0.5;

    return Polyline::quadratic_points(trimmed + Vector(0.0, 0.0, -guide.height), middle + Vector(0.0, 0.0, -guide.static_h()), end + Vector(0.0, 0.0, -guide.static_h()));
}

std::vector<std::array<Polyline, 3>> QuarterGeometry::boundary_parabolas(const FloorGuide& guide) const {

    std::vector<std::array<Polyline, 3>> result;

    for (size_t k = 0; k < 2; k++) {
        const Polyline parabola = outer_parabola(quads.outer_ribs[k], run_in[k], guide);
        result.push_back({parabola, parabola.offset_toward(guide.tsections, Vector::z_axis()), parabola.offset_toward(2.0 * guide.tsections, Vector::z_axis())});
    }

    for (size_t i = 0; i < 2; i++) {
        const Xform projection = Xform::project_to_plane_by_axis(planes.inner_ribs[i][0], planes.outer_ribs[i][0].z_axis());
        const std::array<Polyline, 3>& outer = result[i];
        result.push_back({outer[0].transformed(projection), outer[1].transformed(projection), outer[2].transformed(projection)});
    }

    return result;
}

std::vector<Plane> QuarterGeometry::fit_bed_top_planes() const {

    // the plane through a panel's deepest quad, its top layer on the two side planes trimmed by the panel planes, normal up
    const auto fitted = [](const std::array<Polyline, 2>& faces, const Plane& cut_plane0, const Plane& cut_plane1) {
        std::array<std::vector<Point>, 2> pts = {faces[0].trimmed(cut_plane0, cut_plane1, EXTENSION).get_points(), faces[1].trimmed(cut_plane0, cut_plane1, EXTENSION).get_points()};

        if (pts[0].front()[2] > pts[0].back()[2]) {
            std::reverse(pts[0].begin(), pts[0].end());
            std::reverse(pts[1].begin(), pts[1].end());
        }

        const Plane plane = Plane::from_points_pca({pts[0][0], pts[0][1], pts[1][0], pts[1][1]});

        return Plane::from_point_normal(plane.origin(), plane.z_axis()[2] < 0.0 ? -plane.z_axis() : plane.z_axis());
    };

    const Xform side00 = Xform::project_to_plane_by_axis(planes.inner_ribs[0][0], planes.outer_ribs[0][0].z_axis());
    const Xform side01 = Xform::project_to_plane_by_axis(planes.outer_ribs[0][1], planes.outer_ribs[0][0].z_axis());
    const Xform side20 = Xform::project_to_plane_by_axis(planes.inner_ribs[1][0], planes.outer_ribs[1][0].z_axis());
    const Xform side21 = Xform::project_to_plane_by_axis(planes.outer_ribs[1][1], planes.outer_ribs[1][0].z_axis());

    return {
        fitted({parabolas[0][2].transformed(side00), parabolas[0][2].transformed(side01)}, planes.inner_beams[0][1], planes.wedges[0][0]),
        fitted({central_panel.traces[0][2], central_panel.traces[1][2]}, planes.inner_beams[1][1], planes.wedges[1][0]),
        fitted({parabolas[1][2].transformed(side20), parabolas[1][2].transformed(side21)}, planes.inner_beams[2][1], planes.wedges[2][0]),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// The central panel
// ═══════════════════════════════════════════════════════════════════════════

const double SCAN_STEP = 0.5; // degrees between the sweep directions scanned for rule A's root
const double SCAN_RANGE = 85.0; // degrees either side of n0 - n1 the scan covers: the second root, where both ribs shift alike, lies at 90
const double GRAZING = 1e-3; // |n . r| below which a sweep runs along a rib face and is skipped
const size_t BISECTIONS = 200; // halvings of the bracket, far past the last bit of the angle

CentralPanel::CentralPanel(const ConstructionPlanes& cp, const std::vector<std::array<Polyline, 3>>& parabolas, const FloorGuide& guide) {

    const std::array<Plane, 2> faces = {cp.inner_ribs[0][1], cp.inner_ribs[1][1]};
    shadows = {parabolas[2][0], parabolas[3][0]};
    normals = {faces[0].z_axis(), faces[1].z_axis()};
    thickness = guide.inner_ribs;
    reference = (normals[0] - normals[1]).flattened().normalized();

    rib_sweep = solve_rib_sweep();
    const std::array<Polyline, 2> soffits = {shadows[0].transformed(Xform::project_to_plane_by_axis(faces[0], rib_sweep)), shadows[1].transformed(Xform::project_to_plane_by_axis(faces[1], rib_sweep))};
    ruling = (soffits[1].get_point(0) - soffits[0].get_point(0)).flattened().normalized();

    // the layers: the soffit's offsets by tsections and twice that in the panel's own cross-section, projected along the ruling onto both central faces
    const Polyline section = soffits[0].transformed(Xform::project_to_plane_by_axis(Plane::from_point_normal(soffits[0].get_point(0), ruling), ruling));
    const Polyline layer1 = section.offset_toward(guide.tsections, Vector::z_axis());
    const Polyline layer2 = section.offset_toward(2.0 * guide.tsections, Vector::z_axis());

    for (size_t k = 0; k < 2; k++)
        traces[k] = {soffits[k], layer1.transformed(Xform::project_to_plane_by_axis(faces[k], ruling)), layer2.transformed(Xform::project_to_plane_by_axis(faces[k], ruling))};
}

Vector CentralPanel::solve_rib_sweep() const {

    double best = 0.0;
    bool found = false;

    for (double lo = -SCAN_RANGE; lo < SCAN_RANGE; lo += SCAN_STEP) {
        const double hi = lo + SCAN_STEP;
        std::array<bool, 2> sides_lo;
        std::array<bool, 2> sides_hi;

        if (!sweep_sides(lo, sides_lo) || !sweep_sides(hi, sides_hi) || sides_lo != sides_hi)
            continue;

        const double f_lo = closure(lo);
        const double f_hi = closure(hi);

        if ((f_lo > 0.0) == (f_hi > 0.0) && f_lo != 0.0 && f_hi != 0.0)
            continue;

        const double root = bisect(lo, hi);

        if (!found || std::abs(root) < std::abs(best))
            best = root;

        found = true;
    }

    return turned(found ? best : 0.0);
}

double CentralPanel::closure(double degrees) const {

    // each rib's outer face trace moves thickness / (n . r) along r to reach its central face
    const Vector r = turned(degrees);
    const std::array<double, 2> shift = {thickness / normals[0].dot(r), thickness / normals[1].dot(r)};
    const size_t n = shadows[0].point_count() - 1;
    const Vector start = ((shadows[1].get_point(0) + r * shift[1]) - (shadows[0].get_point(0) + r * shift[0])).flattened();
    const Vector vertex = ((shadows[1].get_point(n) + r * shift[1]) - (shadows[0].get_point(n) + r * shift[0])).flattened();

    return start.cross(vertex)[2] / (start.magnitude() * vertex.magnitude());
}

bool CentralPanel::sweep_sides(double degrees, std::array<bool, 2>& sides) const {

    const Vector r = turned(degrees);
    sides = {normals[0].dot(r) > 0.0, normals[1].dot(r) > 0.0};

    return std::abs(normals[0].dot(r)) > GRAZING && std::abs(normals[1].dot(r)) > GRAZING;
}

double CentralPanel::bisect(double lo, double hi) const {

    double f_lo = closure(lo);

    for (size_t i = 0; i < BISECTIONS && lo != hi; i++) {
        const double mid = 0.5 * (lo + hi);
        const double f_mid = closure(mid);

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

Vector CentralPanel::turned(double degrees) const {

    const double a = degrees * M_PI / 180.0;
    const Vector x = reference.flattened().normalized();

    return Vector(x[0] * std::cos(a) - x[1] * std::sin(a), x[0] * std::sin(a) + x[1] * std::cos(a), 0.0);
}


// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// THE MEMBERS OF A QUARTER
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

const double CUTTER_MARGIN = 100.0; // how far column cutter quads overshoot and how thick they are

Quarter::Quarter(const FloorGuide& floor_guide, size_t q) : guide(floor_guide), index(q % 4) {
}

const QuarterGeometry& Quarter::geometry() const {
    return guide.geometry[index];
}

const ColumnCorner& Quarter::column() const {
    return guide.columns[index];
}

std::vector<Loops> Quarter::outer_ribs() const {

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

std::vector<Loops> Quarter::inner_ribs() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& parabolas = geometry().parabolas;
    const Vector& sweep = geometry().central_panel.rib_sweep;

    return {
        rib(parabolas[2][0], cp.inner_ribs[0][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true),
        rib(parabolas[3][0], cp.inner_ribs[1][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true),
    };
}

std::vector<Loops> Quarter::inner_beams() const {

    const ConstructionPlanes& cp = geometry().planes;
    const Plane side0 = Plane::xy_plane_at(0.0);
    const Plane side1 = Plane::xy_plane_at(guide.soffit);
    const size_t face = guide.seam_through_ribs ? 0 : 1;

    return {
        loft({cp.outer_ribs[0][face], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[0][0], cp.inner_beams[0][1]),
        loft({cp.inner_beams[0][1], side0, cp.inner_beams[2][1], side1}, cp.inner_beams[1][0], cp.inner_beams[1][1]),
        loft({cp.outer_ribs[1][face], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[2][0], cp.inner_beams[2][1]),
    };
}

std::vector<Loops> Quarter::wedges() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<Plane>& beds = geometry().bed_top_planes;
    const Plane top = Plane::xy_plane_at(0.0);
    const std::array<std::array<Plane, 2>, 3> ribs = {{
        {cp.outer_ribs[0][1], cp.inner_ribs[0][0]},
        {cp.inner_ribs[0][1], cp.inner_ribs[1][1]},
        {cp.inner_ribs[1][0], cp.outer_ribs[1][1]},
    }};

    std::vector<Loops> blocks;

    for (size_t i = 0; i < 3; i++)
        blocks.push_back(loft({ribs[i][0], beds[i], ribs[i][1], top}, cp.wedges[i][0], cp.wedges[i][1]));

    return blocks;
}

std::vector<Loops> Quarter::tsections() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& pb = geometry().parabolas;
    const CentralPanel& panel = geometry().central_panel;
    const Vector outer0 = cp.outer_ribs[0][0].z_axis();
    const Vector outer1 = cp.outer_ribs[1][0].z_axis();
    const std::vector<std::array<Plane, 2>>& ts = cp.tsections;

    return {
        outer_tsection(pb[0], ts[0], outer0, outer0, cp.inner_beams[0][1], cp.wedges[0][0]),
        outer_tsection(pb[0], ts[1], outer0, panel.rib_sweep, cp.inner_beams[0][1], cp.wedges[0][0]),
        tsection(panel.traces[0][0], panel.traces[0][1], cp.inner_beams[1][1], cp.wedges[1][0], Xform::project_to_plane_by_axis(ts[2][1], panel.rib_sweep), Xform::project_to_plane_by_axis(ts[2][1], panel.ruling)),
        tsection(panel.traces[1][0], panel.traces[1][1], cp.inner_beams[1][1], cp.wedges[1][0], Xform::project_to_plane_by_axis(ts[3][1], panel.rib_sweep), Xform::project_to_plane_by_axis(ts[3][1], panel.ruling)),
        outer_tsection(pb[1], ts[4], outer1, panel.rib_sweep, cp.inner_beams[2][1], cp.wedges[2][0]),
        outer_tsection(pb[1], ts[5], outer1, outer1, cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

std::vector<std::vector<Loops>> Quarter::beds() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& pb = geometry().parabolas;
    const CentralPanel& panel = geometry().central_panel;

    return {
        outer_bed_row(pb[0], cp.inner_ribs[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis(), cp.inner_beams[0][1], cp.wedges[0][0]),
        bed_row({panel.traces[0][1], panel.traces[1][1]}, {panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]),
        outer_bed_row(pb[1], cp.inner_ribs[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis(), cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

std::vector<Point> Quarter::column_face(size_t i) const {

    const ConstructionPlanes& cp = geometry().planes;
    const ColumnCorner& corner = column();
    const std::array<Plane, 5> fan = {corner.sides[0], cp.wedges[0][0], cp.wedges[1][0], cp.wedges[2][0], corner.sides[1]};
    const Plane xy0 = Plane::xy_plane_at(corner.levels[0]);
    const Plane xy1 = Plane::xy_plane_at(corner.levels[1]);

    return {
        Intersection::plane_plane_plane(xy0, fan[i], fan[i + 1]).value(),
        Intersection::plane_plane_plane(xy0, fan[i + 1], fan[i + 2]).value(),
        Intersection::plane_plane_plane(xy1, fan[i + 1], fan[i + 2]).value(),
        Intersection::plane_plane_plane(xy1, fan[i], fan[i + 1]).value(),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Loops
// ═══════════════════════════════════════════════════════════════════════════

Loops Quarter::loft(const std::vector<Plane>& sides, const Plane& bottom, const Plane& top, bool flip) {

    const Polyline at_bottom = Polyline::from_planes(sides, bottom);
    const Polyline at_top = Polyline::from_planes(sides, top);

    return flip ? Loops{at_bottom, at_top} : Loops{at_top, at_bottom};
}

double Quarter::thickness(const Loops& loops) {
    return (loops[0].area_centroid() - loops[1].area_centroid()).magnitude();
}

Point Quarter::body(const Loops& loops) {
    return loops[0].area_centroid() + (loops[1].area_centroid() - loops[0].area_centroid()) * 0.5;
}

double Quarter::end_level(const Loops& loops, const Plane& end) {

    double level = 0.0;

    for (const Polyline& loop : loops)
        for (const Point& point : loop.get_points())
            if (std::abs(end.signed_distance(point)) <= 1e-6)
                level = std::min(level, point[2]);

    return level;
}

// ═══════════════════════════════════════════════════════════════════════════
// Ribs
// ═══════════════════════════════════════════════════════════════════════════

Loops Quarter::rib(const Polyline& trace, const Plane& face1, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

    std::vector<Point> near = trace.trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();

    if (std::abs(cut_plane0.signed_distance(near.front())) > std::abs(cut_plane0.signed_distance(near.back())))
        std::reverse(near.begin(), near.end());

    const Xform projection = Xform::project_to_plane_by_axis(face1, sweep);
    std::vector<Point> far;

    for (const Point& point : near)
        far.push_back(point.transformed(projection));

    // the first and last facet extended to the end planes
    const size_t n = far.size();
    far[0] = Intersection::line_plane(Line::from_points(far[0], far[1]), cut_plane0, false).value();
    far[n - 1] = Intersection::line_plane(Line::from_points(far[n - 2], far[n - 1]), cut_plane1, false).value();

    return {rib_loop(near, cut_plane0, cut_plane1, inner), rib_loop(far, cut_plane0, cut_plane1, inner)};
}

Polyline Quarter::rib_loop(const std::vector<Point>& pts, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

    const Vector span(pts.back()[0] - pts.front()[0], pts.back()[1] - pts.front()[1], 0.0);
    const Plane rib_plane = Plane::from_point_normal(pts.front(), span.cross(Vector(0.0, 0.0, 1.0)));
    const Point p0 = Intersection::plane_plane_plane(cut_plane0, Plane::xy_plane_at(0.0), rib_plane).value();
    Point p1(pts.back()[0], pts.back()[1], 0.0);

    if (inner)
        p1 = Intersection::line_plane(Line::from_points(p0, p1), cut_plane1, false).value();

    std::vector<Point> loop = {p1, p0};
    loop.insert(loop.end(), pts.begin(), pts.end());
    loop.push_back(p1);

    return Polyline(loop);
}

// ═══════════════════════════════════════════════════════════════════════════
// T-sections and beds
// ═══════════════════════════════════════════════════════════════════════════

Loops Quarter::tsection(const Polyline& soffit, const Polyline& layer, const Plane& cut_plane0, const Plane& cut_plane1, const Xform& projection10, const Xform& projection11) {

    const std::vector<Point> cut00 = soffit.trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();
    const std::vector<Point> cut01 = soffit.transformed(projection10).trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();
    const std::vector<Point> cut10 = layer.trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();
    const std::vector<Point> cut11 = layer.transformed(projection11).trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();

    std::vector<Point> top = cut00;
    top.insert(top.end(), cut10.rbegin(), cut10.rend());
    top.push_back(cut00.front());

    std::vector<Point> bottom = cut01;
    bottom.insert(bottom.end(), cut11.rbegin(), cut11.rend());
    bottom.push_back(cut01.front());

    return {Polyline(top), Polyline(bottom)};
}

Loops Quarter::outer_tsection(const std::array<Polyline, 3>& parabola, const std::array<Plane, 2>& faces, const Vector& outer, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1) {

    const Xform projection = Xform::project_to_plane_by_axis(faces[0], outer);

    return tsection(
        parabola[0].transformed(projection), parabola[1].transformed(projection), cut_plane0, cut_plane1,
        Xform::project_to_plane_by_axis(faces[1], sweep),
        Xform::project_to_plane_by_axis(faces[1], outer)
    );
}

std::vector<Loops> Quarter::bed_row(const std::array<Polyline, 2>& lower, const std::array<Polyline, 2>& upper, const Plane& cut_plane0, const Plane& cut_plane1) {

    const std::vector<Polyline> layers = Polyline::trimmed_alike({lower[0], lower[1], upper[0], upper[1]}, cut_plane0, cut_plane1, EXTENSION);
    const std::array<std::vector<Point>, 2> under = {layers[0].get_points(), layers[1].get_points()};
    const std::array<std::vector<Point>, 2> over = {layers[2].get_points(), layers[3].get_points()};
    std::vector<Loops> plates;

    for (size_t i = 0; i + 1 < under[0].size(); i++) {
        const Polyline bottom({under[0][i], under[0][i + 1], under[1][i + 1], under[1][i], under[0][i]});
        const Polyline top({over[0][i], over[0][i + 1], over[1][i + 1], over[1][i], over[0][i]});
        plates.push_back({top, bottom});
    }

    return plates;
}

std::vector<Loops> Quarter::outer_bed_row(const std::array<Polyline, 3>& parabola, const Plane& side0, const Plane& side1, const Vector& normal, const Plane& cut_plane0, const Plane& cut_plane1) {

    const Xform projection0 = Xform::project_to_plane_by_axis(side0, normal);
    const Xform projection1 = Xform::project_to_plane_by_axis(side1, normal);

    return bed_row({parabola[1].transformed(projection0), parabola[1].transformed(projection1)}, {parabola[2].transformed(projection0), parabola[2].transformed(projection1)}, cut_plane0, cut_plane1);
}

// ═══════════════════════════════════════════════════════════════════════════
// Column cutters
// ═══════════════════════════════════════════════════════════════════════════

ColumnCutters::ColumnCutters(const Quarter& quarter) {

    const ColumnCorner& corner = quarter.column();
    const std::vector<Point>& head = corner.head;
    const Vector down(0.0, 0.0, -1.0);
    const Plane xy2 = Plane::xy_plane_at(corner.levels[2]);
    const std::vector<Plane> fan_bottom = {corner.sides[0], Plane::from_line(Line::from_points(head[1], head[2]), down), Plane::from_line(Line::from_points(head[3], head[4]), down), corner.sides[1]};
    const std::array<std::vector<Point>, 3> faces = {quarter.column_face(0), quarter.column_face(1), quarter.column_face(2)};
    const std::vector<Point> p1 = {faces[0][3], faces[0][2], faces[1][2], faces[2][2]};

    std::vector<Point> p2;

    for (size_t i = 0; i + 1 < fan_bottom.size(); i++)
        p2.push_back(Intersection::plane_plane_plane(xy2, fan_bottom[i], fan_bottom[i + 1]).value());

    const Vector quarter_span = (p2[2] - p2[0]) * 0.25;
    const std::vector<std::vector<Point>> quads = {
        faces[0],
        faces[1],
        faces[2],
        {p1[0], p1[1], p2[1], p2[0]},
        {p1[1], p1[2], p2[1] + quarter_span, p2[1] - quarter_span},
        {p1[2], p1[3], p2[2], p2[1]},
    };

    for (size_t i = 0; i < quads.size(); i++) {
        const std::vector<Point> quad = stretch(quads[i], i < 3);
        const Vector normal = (quad[2] - quad[1]).cross(quad[1] - quad[0]).normalized() * CUTTER_MARGIN;
        const Polyline top = Polyline(quad).closed();
        plates.push_back({top, top.transformed(Xform::translation(normal[0], normal[1], normal[2]))});
    }
}

std::vector<wood_session::SolidCut> ColumnCutters::cuts(double bay_height) const {

    const Xform lift = Xform::translation(0.0, 0.0, bay_height);
    std::vector<wood_session::SolidCut> result;

    for (const Loops& plate : plates)
        result.push_back(wood_session::SolidCut::difference(wood_session::Plate(plate[1], plate[0], "column_cutter").element_geometry_mesh().transformed(lift)));

    return result;
}

std::vector<Point> ColumnCutters::stretch(std::vector<Point> quad, bool top) {

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

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// CONTACTS AND SCREWS
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

ContactFaces::ContactFaces(const FloorGuide& floor_guide) : guide(floor_guide), lift(Xform::translation(0.0, 0.0, floor_guide.bay_height)) {
}

std::shared_ptr<wood_session::InteractionContactFace> ContactFaces::face(ContactKind kind, const std::string& place, wood_session::ContactType type, const Polyline& polygon) const {

    std::shared_ptr<wood_session::InteractionContactFace> contact = std::make_shared<wood_session::InteractionContactFace>(-1, -1, type, polygon.closed().transformed(lift));
    contact->name = CONTACT_NAMES[static_cast<size_t>(kind)] + "_" + place;

    return contact;
}

std::shared_ptr<wood_session::InteractionContactFace> ContactFaces::seam_wedge(size_t q) const {

    const Polyline a = guide.quarter(q).inner_beams()[0][1];
    const Polyline b = guide.quarter((q + 1) % 4).inner_beams()[2][1];

    return face(ContactKind::seam_wedge, std::to_string(q), wood_session::ContactType::side_side, a.overlap(b, guide.seams[q].plane_into(q)));
}

std::shared_ptr<wood_session::InteractionContactFace> ContactFaces::oculus_wedge(size_t q) const {
    return face(ContactKind::oculus_wedge, std::to_string(q), wood_session::ContactType::side_side, guide.quarter(q).inner_beams()[1][1]);
}

std::shared_ptr<wood_session::InteractionContactFace> ContactFaces::column_plate(size_t q, size_t k) const {

    const size_t fan = k == 0 ? 0 : 2;
    const Loops rib = guide.quarter(q).outer_ribs()[k];
    const std::vector<Point> top = rib[0].get_points();
    const std::vector<Point> bottom = rib[1].get_points();
    const Polyline end_face = Polyline({top[1], top[2], bottom[2], bottom[1]}).closed();
    const Polyline carved = Polyline(guide.quarter(q).column_face(fan)).closed();

    return face(ContactKind::column_plate, fmt::format("{}_{}", q, k), wood_session::ContactType::unknown, end_face.overlap(carved, guide.columns[q].wedge_fan[fan][0]));
}

std::shared_ptr<wood_session::InteractionContactFace> ContactFaces::seam_tie(size_t q) const {

    const Loops rib = guide.quarter(q).outer_ribs()[0];
    const std::vector<Point> top = rib[0].get_points();
    const std::vector<Point> bottom = rib[1].get_points();
    const size_t n = top.size();

    return face(ContactKind::seam_tie, std::to_string(q), wood_session::ContactType::end_end, Polyline({top[0], top[n - 2], bottom[n - 2], bottom[0]}));
}

std::shared_ptr<wood_session::InteractionContactFace> ContactFaces::block_dowels(size_t q, size_t k, size_t side) const {

    const Loops block = guide.quarter(q).wedges()[k];
    const std::vector<Point> top = block[0].get_points();
    const std::vector<Point> bottom = block[1].get_points();
    const Polyline polygon = side == 0 ? Polyline({bottom[3], bottom[0], top[0], top[3]}) : Polyline({bottom[1], bottom[2], top[2], top[1]});

    return face(ContactKind::block_dowels, fmt::format("{}_{}_{}", q, k, side), wood_session::ContactType::unknown, polygon);
}

// ═══════════════════════════════════════════════════════════════════════════
// Screws
// ═══════════════════════════════════════════════════════════════════════════

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

ScrewLines::ScrewLines(const FloorGuide& floor_guide) : guide(floor_guide), rings(floor_guide.oculus()), lift(Xform::translation(0.0, 0.0, floor_guide.bay_height)) {
}

std::vector<Line> ScrewLines::rib_beam(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const Quarter quarter = guide.quarter(q);
    const size_t beam = k == 0 ? 0 : 2;
    std::vector<Line> screws;

    if (guide.seam_through_ribs) {
        const Loops rib = quarter.outer_ribs()[k];

        for (double level : {-RIB_END_MARGIN, Quarter::end_level(rib, quarter.rib_seam_ends()[k]) + RIB_END_MARGIN})
            screws.push_back(from_seam_face(cp.outer_ribs[k], cp.inner_beams[beam], level, k == 0 ? -SEAM_SCREW_OFFSET : SEAM_SCREW_OFFSET));

        return lifted(screws);
    }

    const double depth = std::min(guide.static_h(), 2.0 * (TIE_TOP - TIE_CLEARANCE));

    for (double fraction : RIB_BEAM_LEVELS)
        screws.push_back(along_axis(cp.inner_beams[beam], cp.outer_ribs[k][0], Quarter::body(quarter.inner_beams()[beam]), -depth * fraction));

    return lifted(screws);
}

std::vector<Line> ScrewLines::beam_mitre(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const size_t seam = k == 0 ? 0 : 2;
    const Point body = Quarter::body(guide.quarter(q).inner_beams()[1]);
    std::vector<Line> screws;

    for (double levels : MITRE_LEVELS[k])
        screws.push_back(along_axis(cp.inner_beams[1], cp.inner_beams[seam][0], body, corner_level(levels)));

    return lifted(screws);
}

std::vector<Line> ScrewLines::rib_corner(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const Point body = Quarter::body(guide.quarter(q).inner_ribs()[k]);
    const size_t seam = k == 0 ? 0 : 2;
    std::vector<Line> screws;

    for (double levels : RIB_CORNER_LEVELS) {
        screws.push_back(along_axis(cp.inner_ribs[k], cp.inner_beams[1][0], body, corner_level(levels)));
        const double from_seam = cp.inner_beams[seam][0].signed_distance(screws.back().start());

        if (from_seam < 0.5 * SCREW_SPACING)
            throw std::runtime_error(fmt::format("quarter {}'s inner rib {} screw starts {:.3f} mm from the seam plane, less than half the screw spacing, where the next quarter's meets it: the bay is too narrow for the corner screws", q, k, from_seam));
    }

    return lifted(screws);
}

bool ScrewLines::passes_seam_beam(size_t q, size_t k, const std::vector<Line>& screws) const {

    const Plane beam_end = guide.geometry[q].planes.inner_beams[k == 0 ? 0 : 2][1].transformed(lift);
    const Point beam_body = Quarter::body(guide.quarter(q).inner_beams()[1]).transformed(lift);
    const double beam_side = beam_end.signed_distance(beam_body) < 0.0 ? -1.0 : 1.0;

    for (const Line& screw : screws)
        if (beam_side * beam_end.signed_distance(screw.start()) < 0.0)
            return true;

    return false;
}

std::vector<Line> ScrewLines::ring(size_t q) const {

    const size_t next = (q + 1) % 4;
    const std::array<Plane, 2> faces = {guide.oculus_edges[next].tilted, guide.oculus_edges[next].ring_inner};
    std::vector<Line> screws;

    for (double levels : RING_LEVELS)
        screws.push_back(along_axis(faces, guide.oculus_edges[q].tilted, Quarter::body(rings[next]), corner_level(levels)));

    return lifted(screws);
}

std::vector<Line> ScrewLines::oculus(size_t q, size_t k) const {

    const OculusScrew aimed(guide, rings, q, k);
    std::vector<Line> screws;

    for (double levels : OCULUS_LEVELS[k])
        screws.push_back(aimed.at(corner_level(levels)));

    return lifted(screws);
}

std::vector<Line> ScrewLines::lifted(const std::vector<Line>& lines) const {

    std::vector<Line> result;

    for (const Line& line : lines)
        result.push_back(line.transformed(lift));

    return result;
}

Line ScrewLines::along_axis(const std::array<Plane, 2>& butting, const Plane& far_face, const Point& butting_body, double z) {

    const Line line = axis(butting, z);
    const Point head = Intersection::line_plane(line, far_face, false).value();
    Vector d = line.to_direction();

    if (d.dot(butting_body - head) < 0.0)
        d = -d;

    return Line::from_points(head, head + d * SCREW_LENGTH);
}

Line ScrewLines::from_seam_face(const std::array<Plane, 2>& rib, const std::array<Plane, 2>& beam, double z, double offset) {

    const Line line = axis(rib, z);
    const Vector across = rib[0].z_axis() * offset;
    const Vector along = (Intersection::line_plane(line, beam[1], false).value() - Intersection::line_plane(line, beam[0], false).value()).normalized();
    const Point head = Intersection::line_plane(Line::from_points(line.start() + across, line.end() + across), beam[0], false).value();

    return Line::from_points(head, head + along * SCREW_LENGTH);
}

Line ScrewLines::axis(const std::array<Plane, 2>& faces, double z) {

    const Line line0 = Intersection::plane_plane(Plane::xy_plane_at(z), faces[0]).value();
    const Line line1 = Intersection::plane_plane(Plane::xy_plane_at(z), faces[1]).value();
    const Vector d = line1.to_direction();
    const Point p0 = line0.start();
    const Point p1 = line1.start() + d * (p0 - line1.start()).dot(d);
    const Point middle = p0 + (p1 - p0) * 0.5;

    return Line::from_points(middle, middle + line0.to_direction());
}

double ScrewLines::corner_level(double levels) const {
    return -guide.static_h() * levels / CORNER_LEVELS;
}

// ═══════════════════════════════════════════════════════════════════════════
// The oculus screws
// ═══════════════════════════════════════════════════════════════════════════

const double WEDGE_MARGIN = 1.5; // the wedge leaves this many beam thicknesses free at both ends of its contact, add_connectors' margin
const double COARSE_STEP = 5.0; // mm, the head positions an oculus screw first tries along the ring's inner face
const double COARSE_ANGLE = 2.0; // degrees, the directions it first tries
const double SEARCH_STEP = 0.25; // mm, the head positions it then tries around the best
const double ANGLE_STEP = 0.1; // degrees, the directions it then tries

OculusScrew::OculusScrew(const FloorGuide& guide, const std::vector<Loops>& rings, size_t q, size_t k) {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const Loops beam_loops = guide.quarter(q).inner_beams()[1];
    const std::vector<Point> loop = beam_loops[1].get_points();
    const Point end = k == 0 ? loop[0] : loop[1];

    beam = cp.inner_beams[1];
    beam_end = cp.inner_beams[k == 0 ? 0 : 2][1];
    beam_body = Quarter::body(beam_loops);
    inner = guide.oculus_edges[q].ring_inner;
    ring_end = k == 0 ? guide.oculus_edges[(q + 1) % 4].tilted : guide.oculus_edges[(q + 3) % 4].ring_inner;
    ring_body = Quarter::body(rings[q]);
    along = ((k == 0 ? loop[1] : loop[0]) - end).normalized();
    wedge_start = end + along * (WEDGE_MARGIN * std::max(Quarter::thickness(beam_loops), Quarter::thickness(rings[q])));
    band = 0.5 * guide.inner_beams;
}

Line OculusScrew::at(double z) const {

    const Line trace = Intersection::plane_plane(Plane::xy_plane_at(z), inner).value();
    const Point start = Intersection::line_plane(trace, beam_end, false).value();
    Vector across = (beam_body - ring_body).flattened();
    across = (across - along * across.dot(along)).normalized();

    const Aim coarse = best_aim(start, across, Aim{150.0, 40.0, -1e300}, 150.0, 40.0, COARSE_STEP, COARSE_ANGLE);
    const Aim fine = best_aim(start, across, Aim{coarse.offset, coarse.angle, -1e300}, COARSE_STEP, COARSE_ANGLE, SEARCH_STEP, ANGLE_STEP);
    const Point head = start + along * fine.offset;
    const Vector u = across * std::cos(fine.angle * M_PI / 180.0) - along * std::sin(fine.angle * M_PI / 180.0);

    return Line::from_points(head, head + u * SCREW_LENGTH);
}

OculusScrew::Aim OculusScrew::best_aim(const Point& start, const Vector& across, const Aim& centre, double offset_span, double angle_span, double offset_step, double angle_step) const {

    Aim best = centre;

    for (double offset = std::max(centre.offset - offset_span, 0.0); offset <= centre.offset + offset_span; offset += offset_step)
        for (double angle = std::max(centre.angle - angle_span, 0.0); angle <= std::min(centre.angle + angle_span, 80.0); angle += angle_step) {
            const Point head = start + along * offset;
            const Vector u = across * std::cos(angle * M_PI / 180.0) - along * std::sin(angle * M_PI / 180.0);
            const double distance = clearance(head, u);

            if (distance > best.clearance + 1e-9)
                best = {offset, angle, distance};
        }

    return best;
}

double OculusScrew::clearance(const Point& head, const Vector& u) const {

    // the distance of a point from a plane, positive on the side of inside
    const auto depth = [](const Point& point, const Plane& plane, const Point& inside) {
        return plane.signed_distance(inside) < 0.0 ? -plane.signed_distance(point) : plane.signed_distance(point);
    };

    const Plane& contact = beam[0];
    const Vector n = (ring_body - contact.origin()).dot(contact.z_axis()) < 0.0 ? -contact.z_axis() : contact.z_axis();
    const double s_head = (head - contact.origin()).dot(n);
    const double s_rate = u.dot(n);

    if (s_rate >= 0.0)
        return -1e300;

    const Point band_point = head + u * std::max((s_head - band) / -s_rate, 0.0);
    const Point crossing = head + u * (s_head / -s_rate);
    const Point tip = head + u * SCREW_LENGTH;
    const double wedge = (wedge_start - band_point).dot(along);
    const double ring_part = std::min(depth(head, ring_end, ring_body), depth(crossing, ring_end, ring_body));
    const double in_beam = std::min({depth(tip, beam[1], beam_body), depth(tip, beam[0], beam_body), depth(tip, beam_end, beam_body)});

    return std::min({wedge, ring_part, in_beam});
}

}
