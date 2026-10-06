#include "pch.h"
#include "src/templates/floor/floor.h"

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
        const std::vector<Outline> outer = view.outer_ribs();
        const std::vector<Outline> inner = view.inner_ribs();

        for (size_t k = 0; k < 2; k++)
            soffit = std::min({soffit, outer[k].end_level(view.rib_seam_ends()[k]), inner[k].end_level(geometry[q].planes.inner_beams[1][1])});
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

    for (const Outline& rib : quarter(q).outer_ribs())
        level = std::min({level, rib.top.get_point(2)[2], rib.bottom.get_point(2)[2]});

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

std::vector<Outline> FloorGuide::oculus() const {

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

    std::vector<Outline> plates;

    for (size_t i = 0; i < 4; i++)
        plates.push_back(Outline::loft({side2, tilted[(i + 1) % 4], side0, inner[(i + 3) % 4]}, tilted[i], inner[i], true));

    for (size_t i = 0; i < 4; i++) {
        const std::vector<Plane> sides = {inner[i], inner[(i + 1) % 4], inner[i].translate_by_normal(-tsections), inner[(i + 3) % 4].translate_by_normal(-tsections)};
        plates.push_back(Outline::loft(sides, side2, side1));
    }

    plates.push_back(Outline::loft(inner, side1, side3));

    return plates;
}

std::vector<Relationship> FloorGuide::relationships() const {

    std::vector<Relationship> rows = Contacts(*this).rows();
    const std::vector<Relationship> screws = Screws(*this).rows();
    rows.insert(rows.end(), screws.begin(), screws.end());

    return rows;
}

std::vector<Relationship> FloorGuide::relationships(Relation kind) const {

    std::vector<Relationship> rows;

    for (const Relationship& row : relationships())
        if (row.kind == kind)
            rows.push_back(row);

    return rows;
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
// OUTLINE
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

Outline Outline::loft(const std::vector<Plane>& sides, const Plane& bottom, const Plane& top, bool flip) {

    const Polyline at_bottom = Polyline::from_planes(sides, bottom);
    const Polyline at_top = Polyline::from_planes(sides, top);

    return flip ? Outline{at_bottom, at_top} : Outline{at_top, at_bottom};
}

double Outline::thickness() const {
    return (top.area_centroid() - bottom.area_centroid()).magnitude();
}

Point Outline::body() const {
    return top.area_centroid() + (bottom.area_centroid() - top.area_centroid()) * 0.5;
}

double Outline::end_level(const Plane& end) const {

    double level = 0.0;

    for (const Polyline& loop : {top, bottom})
        for (const Point& point : loop.get_points())
            if (std::abs(end.signed_distance(point)) <= 1e-6)
                level = std::min(level, point[2]);

    return level;
}

std::shared_ptr<wood_session::BeamVariable> Outline::to_rib(const std::string& name) const {

    const std::vector<Point> near = top.get_points();
    const std::vector<Point> far = bottom.get_points();
    const size_t stations = near.size() - 3;
    std::vector<Polyline> sections;

    for (size_t i = 0; i < stations; i++) {
        const Point& low = near[2 + i];
        const Point& far_low = far[2 + i];
        Point high(low[0], low[1], 0.0);
        Point far_high(far_low[0], far_low[1], 0.0);

        if (i == 0) {
            high = near[1];
            far_high = far[1];
        } else if (i + 1 == stations) {
            high = near[0];
            far_high = far[0];
        }

        sections.push_back(Polyline({low, high, far_high, far_low}).closed());
    }

    const Line axis = Line::from_points(Line::from_points(near[1], far[1]).center(), Line::from_points(near[0], far[0]).center());

    return std::make_shared<wood_session::BeamVariable>(axis, sections, name);
}

std::shared_ptr<wood_session::BeamVariable> Outline::to_beam(const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name) const {

    const std::vector<Point> near = top.get_points();
    const std::vector<Point> far = bottom.get_points();
    const Polyline first = Polyline({near[start[0]], near[start[1]], far[start[1]], far[start[0]]}).closed();
    const Polyline last = Polyline({near[end[0]], near[end[1]], far[end[1]], far[end[0]]}).closed();

    return wood_session::BeamVariable::between(first, last, name);
}

std::shared_ptr<wood_session::Plate> Outline::to_plate(const std::string& name) const {
    return std::make_shared<wood_session::Plate>(bottom, top, name);
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

std::vector<Outline> Quarter::outer_ribs() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& parabolas = geometry().parabolas;
    const std::array<Plane, 2> ends = rib_seam_ends();

    return {
        Rib(parabolas[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][1].z_axis(), cp.wedges[0][0], ends[0], false).outline(),
        Rib(parabolas[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][1].z_axis(), cp.wedges[2][0], ends[1], false).outline(),
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
        Rib(parabolas[2][0], cp.inner_ribs[0][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true).outline(),
        Rib(parabolas[3][0], cp.inner_ribs[1][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true).outline(),
    };
}

std::vector<Outline> Quarter::inner_beams() const {

    const ConstructionPlanes& cp = geometry().planes;
    const Plane side0 = Plane::xy_plane_at(0.0);
    const Plane side1 = Plane::xy_plane_at(guide.soffit);
    const size_t face = guide.seam_through_ribs ? 0 : 1;

    return {
        Outline::loft({cp.outer_ribs[0][face], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[0][0], cp.inner_beams[0][1]),
        Outline::loft({cp.inner_beams[0][1], side0, cp.inner_beams[2][1], side1}, cp.inner_beams[1][0], cp.inner_beams[1][1]),
        Outline::loft({cp.outer_ribs[1][face], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[2][0], cp.inner_beams[2][1]),
    };
}

std::vector<Outline> Quarter::wedges() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<Plane>& beds = geometry().bed_top_planes;
    const Plane top = Plane::xy_plane_at(0.0);
    const std::array<std::array<Plane, 2>, 3> ribs = {{
        {cp.outer_ribs[0][1], cp.inner_ribs[0][0]},
        {cp.inner_ribs[0][1], cp.inner_ribs[1][1]},
        {cp.inner_ribs[1][0], cp.outer_ribs[1][1]},
    }};

    std::vector<Outline> blocks;

    for (size_t i = 0; i < 3; i++)
        blocks.push_back(Outline::loft({ribs[i][0], beds[i], ribs[i][1], top}, cp.wedges[i][0], cp.wedges[i][1]));

    return blocks;
}

std::vector<Outline> Quarter::tsections() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& pb = geometry().parabolas;
    const CentralPanel& panel = geometry().central_panel;
    const Vector outer0 = cp.outer_ribs[0][0].z_axis();
    const Vector outer1 = cp.outer_ribs[1][0].z_axis();
    const std::vector<std::array<Plane, 2>>& ts = cp.tsections;

    return {
        TSection::beside_outer(pb[0], ts[0], outer0, outer0, cp.inner_beams[0][1], cp.wedges[0][0]).outline,
        TSection::beside_outer(pb[0], ts[1], outer0, panel.rib_sweep, cp.inner_beams[0][1], cp.wedges[0][0]).outline,
        TSection(panel.traces[0][0], panel.traces[0][1], cp.inner_beams[1][1], cp.wedges[1][0], Xform::project_to_plane_by_axis(ts[2][1], panel.rib_sweep), Xform::project_to_plane_by_axis(ts[2][1], panel.ruling)).outline,
        TSection(panel.traces[1][0], panel.traces[1][1], cp.inner_beams[1][1], cp.wedges[1][0], Xform::project_to_plane_by_axis(ts[3][1], panel.rib_sweep), Xform::project_to_plane_by_axis(ts[3][1], panel.ruling)).outline,
        TSection::beside_outer(pb[1], ts[4], outer1, panel.rib_sweep, cp.inner_beams[2][1], cp.wedges[2][0]).outline,
        TSection::beside_outer(pb[1], ts[5], outer1, outer1, cp.inner_beams[2][1], cp.wedges[2][0]).outline,
    };
}

std::vector<std::vector<Outline>> Quarter::beds() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& pb = geometry().parabolas;
    const CentralPanel& panel = geometry().central_panel;

    return {
        BedRow::outer(pb[0], cp.inner_ribs[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis(), cp.inner_beams[0][1], cp.wedges[0][0]).plates,
        BedRow({panel.traces[0][1], panel.traces[1][1]}, {panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]).plates,
        BedRow::outer(pb[1], cp.inner_ribs[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis(), cp.inner_beams[2][1], cp.wedges[2][0]).plates,
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
// Ribs
// ═══════════════════════════════════════════════════════════════════════════

Rib::Rib(const Polyline& trace, const Plane& face1, const Vector& sweep, const Plane& plane0, const Plane& plane1, bool inner_rib) : cut_plane0(plane0), cut_plane1(plane1), inner(inner_rib) {

    near = trace.trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();

    if (std::abs(cut_plane0.signed_distance(near.front())) > std::abs(cut_plane0.signed_distance(near.back())))
        std::reverse(near.begin(), near.end());

    const Xform projection = Xform::project_to_plane_by_axis(face1, sweep);

    for (const Point& point : near)
        far.push_back(point.transformed(projection));

    // the first and last facet extended to the end planes
    const size_t n = far.size();
    far[0] = Intersection::line_plane(Line::from_points(far[0], far[1]), cut_plane0, false).value();
    far[n - 1] = Intersection::line_plane(Line::from_points(far[n - 2], far[n - 1]), cut_plane1, false).value();
}

Outline Rib::outline() const {
    return {loop(near), loop(far)};
}

Polyline Rib::loop(const std::vector<Point>& pts) const {

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

TSection::TSection(const Polyline& soffit, const Polyline& layer, const Plane& cut_plane0, const Plane& cut_plane1, const Xform& projection10, const Xform& projection11) {

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

    outline = {Polyline(top), Polyline(bottom)};
}

TSection TSection::beside_outer(const std::array<Polyline, 3>& parabola, const std::array<Plane, 2>& faces, const Vector& outer, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1) {

    const Xform projection = Xform::project_to_plane_by_axis(faces[0], outer);

    return TSection(
        parabola[0].transformed(projection), parabola[1].transformed(projection), cut_plane0, cut_plane1,
        Xform::project_to_plane_by_axis(faces[1], sweep),
        Xform::project_to_plane_by_axis(faces[1], outer)
    );
}

BedRow::BedRow(const std::array<Polyline, 2>& lower, const std::array<Polyline, 2>& upper, const Plane& cut_plane0, const Plane& cut_plane1) {

    const std::vector<Polyline> layers = Polyline::trimmed_alike({lower[0], lower[1], upper[0], upper[1]}, cut_plane0, cut_plane1, EXTENSION);
    const std::array<std::vector<Point>, 2> under = {layers[0].get_points(), layers[1].get_points()};
    const std::array<std::vector<Point>, 2> over = {layers[2].get_points(), layers[3].get_points()};

    for (size_t i = 0; i + 1 < under[0].size(); i++) {
        const Polyline bottom({under[0][i], under[0][i + 1], under[1][i + 1], under[1][i], under[0][i]});
        const Polyline top({over[0][i], over[0][i + 1], over[1][i + 1], over[1][i], over[0][i]});
        plates.push_back({top, bottom});
    }
}

BedRow BedRow::outer(const std::array<Polyline, 3>& parabola, const Plane& side0, const Plane& side1, const Vector& normal, const Plane& cut_plane0, const Plane& cut_plane1) {

    const Xform projection0 = Xform::project_to_plane_by_axis(side0, normal);
    const Xform projection1 = Xform::project_to_plane_by_axis(side1, normal);

    return BedRow({parabola[1].transformed(projection0), parabola[1].transformed(projection1)}, {parabola[2].transformed(projection0), parabola[2].transformed(projection1)}, cut_plane0, cut_plane1);
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

    for (const Outline& plate : plates)
        result.push_back(wood_session::SolidCut::difference(plate.to_plate("column_cutter")->element_geometry_mesh().transformed(lift)));

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
// RELATIONSHIPS
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

MemberRef MemberRef::of_quarter(size_t quarter, Family family, size_t index) {
    return MemberRef{static_cast<int>(quarter), family, index, -1};
}

MemberRef MemberRef::shared(Family family, size_t index) {
    return MemberRef{-1, family, index, -1};
}

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

std::string Relationship::kind_name(Relation kind) {

    const std::array<std::string, 12> kinds = {"support", "column_plate", "cross_lap", "seam_tie", "seam_wedge", "oculus_wedge", "block_dowels", "screw_rib_beam", "screw_beam_mitre", "screw_rib_corner", "screw_ring", "screw_oculus"};

    return kinds[static_cast<size_t>(kind)];
}

double Relationship::area() const {
    return contact.area();
}

std::string Relationship::text() const {
    return fmt::format("{} {} - {}", kind_name(kind), a.name(), b.name());
}

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

Contacts::Contacts(const FloorGuide& floor_guide) : guide(floor_guide), lift(Xform::translation(0.0, 0.0, floor_guide.bay_height)) {
}

std::vector<Relationship> Contacts::rows() const {

    std::vector<Relationship> rows;

    for (size_t q = 0; q < 4; q++)
        rows.push_back(seam_wedge(q));

    for (size_t q = 0; q < 4; q++)
        rows.push_back(oculus_wedge(q));

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            rows.push_back(column_plate(q, k));

    for (size_t q = 0; q < 4; q++)
        rows.push_back(cross_lap(q));

    for (size_t q = 0; q < 4 && !guide.seam_through_ribs; q++)
        rows.push_back(seam_tie(q));

    for (size_t q = 0; q < 4; q++) {
        rows.push_back(block_dowels(q, 0, 0, MemberRef::of_quarter(q, Family::outer_ribs, 0)));
        rows.push_back(block_dowels(q, 2, 1, MemberRef::of_quarter(q, Family::outer_ribs, 1)));
        rows.push_back(block_dowels(q, 0, 1, MemberRef::of_quarter(q, Family::inner_ribs, 0)));
        rows.push_back(block_dowels(q, 1, 0, MemberRef::of_quarter(q, Family::inner_ribs, 0)));
        rows.push_back(block_dowels(q, 1, 1, MemberRef::of_quarter(q, Family::inner_ribs, 1)));
        rows.push_back(block_dowels(q, 2, 0, MemberRef::of_quarter(q, Family::inner_ribs, 1)));
    }

    for (size_t q = 0; q < 4; q++)
        rows.push_back(support(q));

    return rows;
}

Relationship Contacts::seam_wedge(size_t q) const {

    const Plane seam = guide.seams[q].plane_into(q);
    Relationship row;
    row.kind = Relation::seam_wedge;
    row.a = MemberRef::of_quarter(q, Family::inner_beams, 0);
    row.b = MemberRef::of_quarter((q + 1) % 4, Family::inner_beams, 2);
    row.plane = seam.transformed(lift);
    row.contact = guide.quarter(q).inner_beams()[0].bottom.overlap(guide.quarter((q + 1) % 4).inner_beams()[2].bottom, seam).transformed(lift);
    row.type = wood_session::ContactType::side_side;
    row.seam_or_corner = q;

    if (guide.seam_through_ribs)
        row.end = guide.edges[q].band[0].transformed(lift);

    return row;
}

Relationship Contacts::oculus_wedge(size_t q) const {

    Relationship row;
    row.kind = Relation::oculus_wedge;
    row.a = MemberRef::of_quarter(q, Family::inner_beams, 1);
    row.b = MemberRef::shared(Family::ring, q);
    row.plane = guide.oculus_edges[q].tilted.transformed(lift);
    row.contact = guide.quarter(q).inner_beams()[1].bottom.closed().transformed(lift);
    row.type = wood_session::ContactType::side_side;
    row.seam_or_corner = q;

    return row;
}

Relationship Contacts::column_plate(size_t q, size_t k) const {

    const size_t fan = k == 0 ? 0 : 2;
    const Plane& side = guide.columns[q].wedge_fan[fan][0];
    const Outline rib = guide.quarter(q).outer_ribs()[k];
    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    const Polyline end_face = Polyline({top[1], top[2], bottom[2], bottom[1]}).closed();
    Relationship row;
    row.kind = Relation::column_plate;
    row.a = MemberRef::shared(Family::column, q);
    row.b = MemberRef::of_quarter(q, Family::outer_ribs, k);
    row.plane = side.transformed(lift);
    row.contact = end_face.overlap(Polyline(guide.quarter(q).column_face(fan)).closed(), side).transformed(lift);
    row.seam_or_corner = q;

    return row;
}

Relationship Contacts::cross_lap(size_t q) const {

    Relationship row;
    row.kind = Relation::cross_lap;
    row.a = MemberRef::of_quarter(q, Family::outer_ribs, 0);
    row.b = MemberRef::of_quarter(q, Family::outer_ribs, 1);
    row.seam_or_corner = q;

    return row;
}

Relationship Contacts::seam_tie(size_t q) const {

    const Outline rib = guide.quarter(q).outer_ribs()[0];
    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    const size_t n = top.size();
    Relationship row;
    row.kind = Relation::seam_tie;
    row.a = MemberRef::of_quarter(q, Family::outer_ribs, 0);
    row.b = MemberRef::of_quarter((q + 1) % 4, Family::outer_ribs, 1);
    row.plane = guide.seams[q].plane_into(q).transformed(lift);
    row.contact = Polyline({top[0], top[n - 2], bottom[n - 2], bottom[0]}).closed().transformed(lift);
    row.type = wood_session::ContactType::end_end;
    row.seam_or_corner = q;

    return row;
}

Relationship Contacts::block_dowels(size_t q, size_t k, size_t side, const MemberRef& rib) const {

    const Outline block = guide.quarter(q).wedges()[k];
    const std::vector<Point> top = block.top.get_points();
    const std::vector<Point> bottom = block.bottom.get_points();
    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const std::array<std::array<Plane, 2>, 3> ribs = {{{cp.outer_ribs[0][1], cp.inner_ribs[0][0]}, {cp.inner_ribs[0][1], cp.inner_ribs[1][1]}, {cp.inner_ribs[1][0], cp.outer_ribs[1][1]}}};
    const Polyline face = side == 0 ? Polyline({bottom[3], bottom[0], top[0], top[3]}) : Polyline({bottom[1], bottom[2], top[2], top[1]});
    Relationship row;
    row.kind = Relation::block_dowels;
    row.a = rib;
    row.b = MemberRef::of_quarter(q, Family::wedges, k);
    row.plane = ribs[k][side].transformed(lift);
    row.contact = face.closed().transformed(lift);
    row.seam_or_corner = q;

    return row;
}

Relationship Contacts::support(size_t q) const {

    Relationship row;
    row.kind = Relation::support;
    row.a = MemberRef::shared(Family::support, q);
    row.b = MemberRef::shared(Family::column, q);
    row.plane = guide.columns[q].support_plane;
    row.seam_or_corner = q;

    return row;
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

Screws::Screws(const FloorGuide& floor_guide) : guide(floor_guide), rings(floor_guide.oculus()), lift(Xform::translation(0.0, 0.0, floor_guide.bay_height)) {
}

std::vector<Relationship> Screws::rows() const {

    std::vector<Relationship> rows;

    for (size_t q = 0; q < 4; q++) {
        for (size_t k = 0; k < 2; k++)
            rows.push_back(rib_beam(q, k));

        for (size_t k = 0; k < 2; k++)
            rows.push_back(beam_mitre(q, k));

        for (size_t k = 0; k < 2; k++)
            rows.push_back(rib_corner(q, k));
    }

    for (size_t q = 0; q < 4; q++)
        rows.push_back(ring(q));

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            rows.push_back(oculus(q, k));

    return rows;
}

Relationship Screws::rib_beam(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const Quarter quarter = guide.quarter(q);
    const size_t beam = k == 0 ? 0 : 2;
    const Outline outline = quarter.inner_beams()[beam];
    const Outline rib = quarter.outer_ribs()[k];
    const MemberRef a = MemberRef::of_quarter(q, Family::outer_ribs, k);
    const MemberRef b = MemberRef::of_quarter(q, Family::inner_beams, beam);
    std::vector<Line> screws;

    if (guide.seam_through_ribs) {
        const std::vector<Point> rib_top = rib.top.get_points();
        const std::vector<Point> rib_bottom = rib.bottom.get_points();
        const size_t n = rib_top.size();

        for (double level : {-RIB_END_MARGIN, rib.end_level(quarter.rib_seam_ends()[k]) + RIB_END_MARGIN})
            screws.push_back(from_seam_face(cp.outer_ribs[k], cp.inner_beams[beam], level, k == 0 ? -SEAM_SCREW_OFFSET : SEAM_SCREW_OFFSET));

        return row(Relation::screw_rib_beam, a, b, cp.inner_beams[beam][1], {rib_top[0], rib_top[n - 2], rib_bottom[n - 2], rib_bottom[0]}, screws, q);
    }

    const double depth = std::min(guide.static_h(), 2.0 * (TIE_TOP - TIE_CLEARANCE));

    for (double fraction : RIB_BEAM_LEVELS)
        screws.push_back(along_axis(cp.inner_beams[beam], cp.outer_ribs[k][0], outline.body(), -depth * fraction));

    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const Polyline end = Polyline({top[3], top[0], bottom[0], bottom[3]}).closed();

    return row(Relation::screw_rib_beam, a, b, cp.outer_ribs[k][1], end.overlap(rib.bottom, cp.outer_ribs[k][1]).open_points(), screws, q);
}

Relationship Screws::beam_mitre(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const size_t seam = k == 0 ? 0 : 2;
    const Outline outline = guide.quarter(q).inner_beams()[1];
    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const std::vector<Point> contact = k == 0 ? std::vector<Point>{top[3], top[0], bottom[0], bottom[3]} : std::vector<Point>{top[1], top[2], bottom[2], bottom[1]};
    std::vector<Line> screws;

    for (double levels : MITRE_LEVELS[k])
        screws.push_back(along_axis(cp.inner_beams[1], cp.inner_beams[seam][0], outline.body(), corner_level(levels)));

    return row(Relation::screw_beam_mitre, MemberRef::of_quarter(q, Family::inner_beams, seam), MemberRef::of_quarter(q, Family::inner_beams, 1), cp.inner_beams[seam][1], contact, screws, q);
}

Relationship Screws::rib_corner(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const Quarter quarter = guide.quarter(q);
    const Outline outline = quarter.inner_ribs()[k];
    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const size_t seam = k == 0 ? 0 : 2;
    const Plane& beam_end = cp.inner_beams[seam][1];
    const double beam_side = beam_end.signed_distance(quarter.inner_beams()[1].body()) < 0.0 ? -1.0 : 1.0;
    std::vector<Line> screws;
    bool through_seam = false;

    for (double levels : RIB_CORNER_LEVELS) {
        screws.push_back(along_axis(cp.inner_ribs[k], cp.inner_beams[1][0], outline.body(), corner_level(levels)));
        through_seam = through_seam || beam_side * beam_end.signed_distance(screws.back().start()) < 0.0;
        const double from_seam = cp.inner_beams[seam][0].signed_distance(screws.back().start());

        if (from_seam < 0.5 * SCREW_SPACING)
            throw std::runtime_error(fmt::format("quarter {}'s inner rib {} screw starts {:.3f} mm from the seam plane, less than half the screw spacing, where the next quarter's meets it: the bay is too narrow for the corner screws", q, k, from_seam));
    }

    const std::vector<Point> end = Polyline({top[0], top[top.size() - 2], bottom[bottom.size() - 2], bottom[0]}).clip_by_plane(Plane::xy_plane_at(guide.soffit)).open_points();
    Relationship result = row(Relation::screw_rib_corner, MemberRef::of_quarter(q, Family::inner_beams, 1), MemberRef::of_quarter(q, Family::inner_ribs, k), cp.inner_beams[1][1], end, screws, q);

    if (through_seam)
        result.through.push_back(MemberRef::of_quarter(q, Family::inner_beams, seam));

    return result;
}

Relationship Screws::ring(size_t q) const {

    const size_t next = (q + 1) % 4;
    const std::vector<Point> top = rings[next].top.get_points();
    const std::vector<Point> bottom = rings[next].bottom.get_points();
    const std::array<Plane, 2> faces = {guide.oculus_edges[next].tilted, guide.oculus_edges[next].ring_inner};
    std::vector<Line> screws;

    for (double levels : RING_LEVELS)
        screws.push_back(along_axis(faces, guide.oculus_edges[q].tilted, rings[next].body(), corner_level(levels)));

    return row(Relation::screw_ring, MemberRef::shared(Family::ring, q), MemberRef::shared(Family::ring, next), guide.oculus_edges[q].ring_inner, {top[2], top[3], bottom[3], bottom[2]}, screws, q);
}

Relationship Screws::oculus(size_t q, size_t k) const {

    const std::vector<Point> loop = guide.quarter(q).inner_beams()[1].bottom.get_points();
    const OculusScrew aimed(guide, rings, q, k);
    std::vector<Line> screws;

    for (double levels : OCULUS_LEVELS[k])
        screws.push_back(aimed.at(corner_level(levels)));

    return row(Relation::screw_oculus, MemberRef::shared(Family::ring, q), MemberRef::of_quarter(q, Family::inner_beams, 1), guide.oculus_edges[q].tilted, {loop.begin(), loop.end() - 1}, screws, q);
}

Relationship Screws::row(Relation kind, const MemberRef& a, const MemberRef& b, const Plane& plane, const std::vector<Point>& contact, const std::vector<Line>& screws, size_t corner) const {

    Relationship result;
    result.kind = kind;
    result.a = a;
    result.b = b;
    result.plane = plane.transformed(lift);
    result.contact = Polyline(contact).closed().transformed(lift);
    result.seam_or_corner = corner;

    for (const Line& screw : screws)
        result.screws.push_back(screw.transformed(lift));

    return result;
}

Line Screws::along_axis(const std::array<Plane, 2>& butting, const Plane& far_face, const Point& butting_body, double z) {

    const Line line = axis(butting, z);
    const Point head = Intersection::line_plane(line, far_face, false).value();
    Vector d = line.to_direction();

    if (d.dot(butting_body - head) < 0.0)
        d = -d;

    return Line::from_points(head, head + d * SCREW_LENGTH);
}

Line Screws::from_seam_face(const std::array<Plane, 2>& rib, const std::array<Plane, 2>& beam, double z, double offset) {

    const Line line = axis(rib, z);
    const Vector across = rib[0].z_axis() * offset;
    const Vector along = (Intersection::line_plane(line, beam[1], false).value() - Intersection::line_plane(line, beam[0], false).value()).normalized();
    const Point head = Intersection::line_plane(Line::from_points(line.start() + across, line.end() + across), beam[0], false).value();

    return Line::from_points(head, head + along * SCREW_LENGTH);
}

Line Screws::axis(const std::array<Plane, 2>& faces, double z) {

    const Line line0 = Intersection::plane_plane(Plane::xy_plane_at(z), faces[0]).value();
    const Line line1 = Intersection::plane_plane(Plane::xy_plane_at(z), faces[1]).value();
    const Vector d = line1.to_direction();
    const Point p0 = line0.start();
    const Point p1 = line1.start() + d * (p0 - line1.start()).dot(d);
    const Point middle = p0 + (p1 - p0) * 0.5;

    return Line::from_points(middle, middle + line0.to_direction());
}

double Screws::corner_level(double levels) const {
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

OculusScrew::OculusScrew(const FloorGuide& guide, const std::vector<Outline>& rings, size_t q, size_t k) {

    const ConstructionPlanes& cp = guide.geometry[q].planes;
    const Outline outline = guide.quarter(q).inner_beams()[1];
    const std::vector<Point> loop = outline.bottom.get_points();
    const Point end = k == 0 ? loop[0] : loop[1];

    beam = cp.inner_beams[1];
    beam_end = cp.inner_beams[k == 0 ? 0 : 2][1];
    beam_body = outline.body();
    inner = guide.oculus_edges[q].ring_inner;
    ring_end = k == 0 ? guide.oculus_edges[(q + 1) % 4].tilted : guide.oculus_edges[(q + 3) % 4].ring_inner;
    ring_body = rings[q].body();
    along = ((k == 0 ? loop[1] : loop[0]) - end).normalized();
    wedge_start = end + along * (WEDGE_MARGIN * std::max(outline.thickness(), rings[q].thickness()));
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

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// FLOOR
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Member> Member::of_family(Family family, const std::vector<Outline>& outlines) {

    const std::string& name = FAMILY_NAMES[static_cast<size_t>(family)];
    std::vector<Member> result;

    for (const Outline& outline : outlines) {
        Member member;
        member.thickness = outline.thickness();

        if (family == Family::outer_ribs || family == Family::inner_ribs)
            member.element = outline.to_rib(name);
        else if (family == Family::inner_beams)
            member.element = outline.to_beam({0, 3}, {1, 2}, name);
        else
            member.element = outline.to_plate(name);

        result.push_back(member);
    }

    return result;
}

const Member* QuarterMembers::get(const MemberRef& ref) const {

    const std::vector<Member>* family = nullptr;

    if (ref.family == Family::outer_ribs)
        family = &outer_ribs;
    else if (ref.family == Family::inner_ribs)
        family = &inner_ribs;
    else if (ref.family == Family::inner_beams)
        family = &inner_beams;
    else if (ref.family == Family::wedges)
        family = &wedges;
    else if (ref.family == Family::tsections)
        family = &tsections;
    else if (ref.family == Family::beds && ref.row >= 0 && static_cast<size_t>(ref.row) < beds.size())
        family = &beds[static_cast<size_t>(ref.row)];

    return family && ref.index < family->size() ? &(*family)[ref.index] : nullptr;
}

Floor::Floor(const FloorGuide& floor_guide, const std::string& name) : wood_session::WoodSession(name), guide(floor_guide) {
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

void Floor::add_members() {

    add_quarters();
    add_oculus();
    add_columns();
}

void Floor::add_quarters() {

    for (size_t q = 0; q < 4; q++) {
        const Quarter view = guide.quarter(q);
        const std::string suffix = fmt::format("_{}", q);
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("quarter_{}", q));
        const std::shared_ptr<TreeNode> bed_group = add_group("beds" + suffix, group);
        const std::vector<std::vector<Outline>> beds = view.beds();
        QuarterMembers& quarter = quarters[q];
        quarter = QuarterMembers();

        for (size_t row = 0; row < beds.size(); row++) {
            quarter.beds.push_back(Member::of_family(Family::beds, beds[row]));
            add_family(quarter.beds.back(), fmt::format("beds_{}", row), suffix, bed_group);
        }

        quarter.tsections = Member::of_family(Family::tsections, view.tsections());
        add_family(quarter.tsections, "tsections", suffix, group);
        quarter.outer_ribs = Member::of_family(Family::outer_ribs, view.outer_ribs());
        add_family(quarter.outer_ribs, "outer_ribs", suffix, group);
        quarter.inner_ribs = Member::of_family(Family::inner_ribs, view.inner_ribs());
        add_family(quarter.inner_ribs, "inner_ribs", suffix, group);
        quarter.wedges = Member::of_family(Family::wedges, view.wedges());
        add_family(quarter.wedges, "wedges", suffix, group);
        quarter.inner_beams = Member::of_family(Family::inner_beams, view.inner_beams());
        add_family(quarter.inner_beams, "inner_beams", suffix, group);
    }
}

void Floor::add_oculus() {

    const std::vector<Outline> outlines = guide.oculus();
    ring.clear();

    for (size_t i = 0; i < outlines.size(); i++) {
        const std::shared_ptr<Element> member = i < 4 ? std::static_pointer_cast<Element>(outlines[i].to_beam({1, 0}, {2, 3}, "oculus")) : std::static_pointer_cast<Element>(outlines[i].to_plate("oculus"));
        const std::shared_ptr<TreeNode> group = i < 8 ? group_named(fmt::format("oculus_{}", i % 4), group_named(fmt::format("quarter_{}", i % 4))) : group_named("oculus");
        add_placed(member, fmt::format("oculus_{}", i), group);

        if (i < 4)
            ring.push_back({member, outlines[i].thickness()});
    }
}

void Floor::add_columns() {

    for (size_t q = 0; q < 4; q++)
        add_column(q);
}

void Floor::add_column(size_t corner) {

    const size_t k = corner % 4;
    const std::shared_ptr<TreeNode> group = group_named(fmt::format("column_{}", k), group_named(fmt::format("quarter_{}", k)));

    if (columns.size() < 4)
        columns.resize(4);

    ColumnModel& model = columns[k];
    model.support = guide.columns[k].to_support();
    model.support->name = fmt::format("support_{}", k);
    model.column = guide.columns[k].to_column(guide, *model.support);
    model.column->name = fmt::format("column_{}", k);
    add(model.support, group);
    add(model.column, group);

    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(*model.support, *model.column);
    add(joint, group);
    add_joint(joint);

    for (const wood_session::SolidCut& cut : ColumnCutters(guide.quarter(k)).cuts(guide.bay_height))
        model.column->solid_cuts.push_back(cut);

    model.column->invalidate_geometry();
}

void Floor::add_family(const std::vector<Member>& family, const std::string& prefix, const std::string& suffix, const std::shared_ptr<TreeNode>& group) {

    const std::shared_ptr<TreeNode> node = add_group(prefix + suffix, group);

    for (size_t i = 0; i < family.size(); i++)
        add_placed(family[i].element, fmt::format("{}_{}{}", prefix, i, suffix), node);
}

void Floor::add_placed(const std::shared_ptr<Element>& element, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->place(Xform::translation(0.0, 0.0, guide.bay_height));
    element->name = name;
    add(element, group);
}

std::shared_ptr<Element> Floor::get(const MemberRef& ref) const {

    if (ref.family == Family::ring)
        return ref.index < ring.size() ? ring[ref.index].element : nullptr;

    if (ref.family == Family::column)
        return ref.index < columns.size() ? columns[ref.index].column : nullptr;

    if (ref.family == Family::support)
        return ref.index < columns.size() ? columns[ref.index].support : nullptr;

    if (ref.quarter < 0 || ref.quarter > 3)
        return nullptr;

    const Member* member = quarters[static_cast<size_t>(ref.quarter)].get(ref);

    return member ? member->element : nullptr;
}

double Floor::thickness(const MemberRef& ref) const {

    if (ref.family == Family::ring)
        return ref.index < ring.size() ? ring[ref.index].thickness : 0.0;

    if (ref.quarter < 0 || ref.quarter > 3)
        return 0.0;

    const Member* member = quarters[static_cast<size_t>(ref.quarter)].get(ref);

    return member ? member->thickness : 0.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Connectors
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::shared_ptr<wood_session::JointBeam>> Floor::add_connectors(const std::vector<Relation>& kinds) {

    std::map<size_t, std::vector<std::shared_ptr<wood_session::JointBeam>>> plates_of_corner;
    std::vector<std::pair<Relationship, std::shared_ptr<wood_session::JointBeam>>> built;

    // every connector first, so a missing member throws before anything is added or cut
    for (const Relationship& row : guide.relationships()) {
        if (row.kind == Relation::support || std::find(kinds.begin(), kinds.end(), row.kind) == kinds.end())
            continue;

        std::shared_ptr<wood_session::JointBeam> connector;

        if (row.kind == Relation::cross_lap) {
            const std::vector<std::shared_ptr<wood_session::JointBeam>>& plates = plates_of_corner[row.seam_or_corner];

            if (plates.size() != 2)
                throw std::runtime_error("the cross lap of corner " + std::to_string(row.seam_or_corner) + " needs its two column plates in the same call");

            connector = wood_session::JointBeam::cross_lap(*plates[0], *plates[1]);
        } else
            connector = connector_of(row);

        built.push_back({row, connector});

        if (row.kind == Relation::column_plate)
            plates_of_corner[row.seam_or_corner].push_back(connector);
    }

    std::map<std::string, size_t> numbers;
    std::vector<std::shared_ptr<wood_session::JointBeam>> added;

    for (const auto& [row, connector] : built) {
        const std::string prefix = connector_prefix(row.kind);

        if (!numbers.count(prefix))
            numbers[prefix] = next_number(prefix);

        connector->name = fmt::format("{}_{}", prefix, numbers[prefix]++);
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("connectors_{}", row.seam_or_corner), group_named(fmt::format("quarter_{}", row.seam_or_corner)));
        set_node_color(add_connector(connector, group), CONNECTOR_COLOR, true);
        (row.screws.empty() ? connectors : screws).push_back(connector);
        added.push_back(connector);
    }

    return added;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> Floor::add_screws() {
    return add_connectors({SCREW_RELATIONS.begin(), SCREW_RELATIONS.end()});
}

std::shared_ptr<wood_session::JointBeam> Floor::connector_of(const Relationship& row) const {

    const std::shared_ptr<Element> a = get(row.a);
    const std::shared_ptr<Element> b = get(row.b);

    if (!a || !b)
        throw std::runtime_error("the floor does not hold both members of " + row.text());

    const wood_session::InteractionContactFace contact(-1, -1, row.type, row.contact);

    if (row.kind == Relation::seam_wedge || row.kind == Relation::oculus_wedge) {
        const double size = std::max(thickness(row.a), thickness(row.b));
        return wood_session::JointBeam::wedge(*a, *b, contact, 1.5 * size, 2.0 * size / 3.0, row.end);
    }

    if (row.kind == Relation::column_plate)
        return wood_session::JointBeam::rectangle_plate(*a, *b, contact, thickness(row.b));

    if (row.kind == Relation::seam_tie)
        return wood_session::JointBeam::tie(*a, *b, contact, TIE_TOP);

    if (!row.screws.empty()) {
        std::vector<const Element*> passed = {a.get(), b.get()};

        for (const MemberRef& ref : row.through)
            passed.push_back(get(ref).get());

        return wood_session::JointBeam::screws(passed, row.screws);
    }

    const std::shared_ptr<wood_session::JointBeam> dowels = wood_session::JointBeam::dowels(*a, *b, contact);

    if (!dowels)
        throw std::runtime_error("the inset leaves no room for the dowels of " + row.text());

    return dowels;
}

std::string Floor::connector_prefix(Relation kind) {

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

}
