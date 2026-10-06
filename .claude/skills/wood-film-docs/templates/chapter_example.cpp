#include "docs/floor/movie.h"

namespace movie {

namespace {

const std::string CHAPTER = "01_bay";
const size_t ARC_STEPS = 24; // Segments of a drawn angle arc.
const double ORBIT_RADIANS_PER_PIXEL = 0.005; // session_viewer's camera turn per mouse pixel.
const Vector X = Vector::x_axis();
const Vector Y = Vector::y_axis();
const Vector Z = Vector::z_axis();
const Box WIDE = {-3700.0, -3700.0, H - 10.0, 3700.0, 3700.0, H + 10.0}; // The bay in plan with room around its corners.
const Box CORNER = {-3060.0, -3060.0, H - 10.0, -2660.0, -2660.0, H + 10.0}; // Column corner 0 in plan.

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The arc of radius about centre from one direction to another, turned about their common normal.
Polyline arc(const Point& centre, const Vector& from, const Vector& to, double radius) {

    const Vector axis = from.cross(to).normalized();
    const double degrees = from.angle(to, false);
    const Vector start = from.normalized() * radius;
    std::vector<Point> points;

    for (size_t i = 0; i <= ARC_STEPS; i++)
        points.push_back(centre + start.transformed(Xform::rotation(axis, degrees * static_cast<double>(i) / static_cast<double>(ARC_STEPS), true)));

    return Polyline(points);
}

/// The middle point of an arc.
Point arc_middle(const Polyline& arc) {
    return arc.get_point(ARC_STEPS / 2);
}

/// An arc drawn as a turning arrow, headed at its end.
void turn(Frame& frame, const Polyline& arc, const Color& color) {
    frame.polyline(arc, color, 3.0);
    frame.line(Line::from_points(arc.get_point(ARC_STEPS - 2), arc.get_point(ARC_STEPS)), color, 3.0, false, true);
}

/// The plane at origin with its drawn square turned so one side runs along `along`, a direction in the plane.
Plane facing(const Plane& plane, const Point& origin, const Vector& along) {
    const Vector x = along.normalized();
    return Plane::from_frame(origin, x, plane.z_axis().cross(x), plane.z_axis());
}

/// The datum trace of a vertical plane over a datum segment: both ends projected onto the plane.
Line trace(const Plane& plane, const Point& a, const Point& b) {
    return Line::from_points(plane.project(a), plane.project(b));
}

/// The oculus diamond at the datum.
Polyline diamond(const FloorGuide& guide) {
    return Polyline({guide.oculus_corners[0], guide.oculus_corners[1], guide.oculus_corners[2], guide.oculus_corners[3]}).closed();
}

// ═══════════════════════════════════════════════════════════════════════════
// Corners, centre and oculus
// ═══════════════════════════════════════════════════════════════════════════

/// Frame 1: the four corners of the rectangle, counter-clockwise, and the two spans.
void bay_corners(const Context& context) {

    const FloorGuide& guide = context.guide;
    const double half_x = guide.corners[1][0];
    const double half_y = guide.corners[2][1];
    Frame frame(CHAPTER, 1, "bay_corners", fmt::format("FloorGuide::rectangle({:.0f}, {:.0f}): four corners counter-clockwise at z 0, half spans about the origin", half_x, half_y), "top", WIDE);
    frame.polyline(up(Polyline({guide.corners[0], guide.corners[1], guide.corners[2], guide.corners[3]}).closed()), BUILT, 4.0);

    for (size_t k = 0; k < 4; k++) {
        frame.point(up(guide.corners[k]), BUILT);
        frame.label(fmt::format("corners[{}] = ({:.0f}, {:.0f}, {:.0f})", k, guide.corners[k][0], guide.corners[k][1], guide.corners[k][2]), up(guide.corners[k]));
    }

    const Polyline last = arc(guide.centre, -X, -Y, 900.0);
    frame.polyline(up(arc(guide.centre, X, Y, 900.0)), BUILT, 3.0);
    frame.polyline(up(arc(guide.centre, Y, -X, 900.0)), BUILT, 3.0);
    turn(frame, up(last), BUILT);
    frame.label("counter-clockwise", up(last.get_point(ARC_STEPS)));
    frame.dimension(up(guide.corners[0]), up(guide.corners[1]), Vector(0.0, -250.0, 0.0), fmt::format("{:.0f}", (guide.corners[1] - guide.corners[0]).magnitude()));
    frame.dimension(up(guide.corners[1]), up(guide.corners[2]), Vector(250.0, 0.0, 0.0), fmt::format("{:.0f}", (guide.corners[2] - guide.corners[1]).magnitude()));
    frame.write(context.dir);
}

/// Frame 2: the vertex centroid and the four edge midpoints, with the bimedians through the centre.
void centre_midpoints(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 2, "centre_midpoints", "centre = Point::centroid(corners): vertex mean, where the bimedians cross; midpoint(k) = middle of edge k", "top", BAY);
    plan_context(frame, guide, false);
    frame.line(up(Line::from_points(guide.corners[0], guide.corners[2])), GREY, 1.0, true);
    frame.line(up(Line::from_points(guide.corners[1], guide.corners[3])), GREY, 1.0, true);
    frame.line(up(Line::from_points(guide.midpoint(0), guide.midpoint(2))), INPUT, 2.0, true);
    frame.line(up(Line::from_points(guide.midpoint(1), guide.midpoint(3))), INPUT, 2.0, true);

    for (size_t k = 0; k < 4; k++) {
        frame.point(up(guide.midpoint(k)), RESULT);
        frame.label(fmt::format("midpoint({})", k), up(guide.midpoint(k)));
    }

    frame.point(up(guide.centre), BUILT, 16.0);
    frame.label(fmt::format("centre = ({:.0f}, {:.0f}, {:.0f})", guide.centre[0], guide.centre[1], guide.centre[2]), up(guide.centre));
    frame.write(context.dir);
}

/// Frame 3: each oculus corner at distance oculus from the centre on the ray to its edge midpoint.
void oculus_corners(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 3, "oculus_corners", "oculus_corners[q] = centre + unit(midpoint(q) - centre) * oculus: a square diamond on a rectangle", "top", BAY);
    frame.key = true;
    plan_context(frame, guide, false);
    frame.point(up(guide.centre), INPUT);

    for (size_t q = 0; q < 4; q++) {
        frame.point(up(guide.midpoint(q)), INPUT);
        frame.line(up(Line::from_points(guide.centre, guide.midpoint(q))), INPUT, 2.0, true);
        frame.point(up(guide.oculus_corners[q]), BUILT);
        frame.label(fmt::format("oculus_corners[{}]", q), up(guide.oculus_corners[q]));
    }

    frame.polyline(up(diamond(guide)), BUILT, 3.0);
    frame.dimension(up(guide.centre), up(guide.oculus_corners[0]), Vector(180.0, 0.0, 0.0), fmt::format("oculus_radius = {:.0f}", guide.oculus_radius));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// invalid()
// ═══════════════════════════════════════════════════════════════════════════

/// Frame 4: check A, every edge vector and the left turn to the next one.
void validity_turns(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 4, "validity_turns", "invalid(), check A: corners at z 0, turn = after x next edge > 0 at every corner, no zero-length edge", "top", WIDE);
    plan_context(frame, guide, false);

    for (size_t k = 0; k < 4; k++) {
        const Point& corner = guide.corners[(k + 1) % 4];
        const Vector after = corner - guide.corners[k];
        const Vector next = guide.corners[(k + 2) % 4] - corner;
        frame.line(up(Line::from_points(guide.corners[k], corner)), VARIABLE, 3.0, false, true);
        const Polyline bend = arc(corner, after, next, 500.0);
        turn(frame, up(bend), BUILT);

        if (k == 0)
            frame.label(fmt::format("turn = after.cross(corners[2] - corners[1])[2] = {:.3g}", after.cross(next)[2]), up(arc_middle(bend)));
        else
            frame.label("turn > 0", up(arc_middle(bend)));
    }

    const Vector before = guide.corners[3] - guide.corners[0];
    const Point before_start = guide.corners[0] + (X + Y) * 250.0;
    const Line before_arrow = Line::from_points(before_start, before_start + before * 0.4);
    frame.line(up(before_arrow), VARIABLE, 2.0, false, true);
    frame.label("after = corners[1] - corners[0]", up(Line::from_points(guide.corners[0], guide.corners[1]).point_at(0.3)));
    frame.label("corners[2] - corners[1]", up(Line::from_points(guide.corners[1], guide.corners[2]).point_at(0.3)));
    frame.label("before = corners[3] - corners[0]", up(before_arrow.center()));
    frame.write(context.dir);
}

/// Frame 5: check B, the oculus corner strictly inside its seam.
void validity_seam(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Point& centre = guide.centre;
    const Point midpoint = guide.midpoint(0);
    const Point& corner = guide.oculus_corners[0];
    const double along = (corner - centre).dot((midpoint - centre).normalized());
    const double length = (midpoint - centre).magnitude();
    Frame frame(CHAPTER, 5, "validity_seam", fmt::format("invalid(), check B: along = {:.0f} must lie in the open interval (0, {:.0f}), |midpoint(0) - centre|", along, length), "top", {-1700.0, -3300.0, H - 10.0, 1700.0, 300.0, H + 10.0});
    frame.line(up(Line::from_points(centre, midpoint)), GREY, 3.0);

    for (const Point& end : {centre, midpoint})
        frame.line(up(Line::from_points(end - X * 150.0, end + X * 150.0)), INPUT, 2.0);

    const Vector side = X * 300.0;
    frame.line(up(Line::from_points(centre + side, midpoint + side)), INPUT, 6.0, true);
    frame.line(up(Line::from_points(centre - side, corner - side)), VARIABLE, 4.0);
    frame.line(up(Line::from_points(corner, corner - side * 1.3)), VARIABLE, 1.0);
    frame.point(up(corner), INPUT);
    frame.label("centre: along = 0", up(centre));
    frame.label(fmt::format("midpoint(0): along = {:.0f}", length), up(midpoint));
    frame.label("oculus_corners[0]", up(corner));
    frame.label(fmt::format("along = {:.0f}", along), up(Line::from_points(centre - side, corner - side).center()));
    frame.label(fmt::format("0 < along < {:.0f}", length), up(Line::from_points(centre + side, midpoint + side).center()));
    frame.write(context.dir);
}

/// Frame 6: check C, the oculus corner angle against the seam angle at oculus corner 0.
void validity_ring(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Point& corner = guide.oculus_corners[0];
    const Vector to_next = guide.oculus_corners[1] - corner;
    const Vector to_previous = guide.oculus_corners[3] - corner;
    const Vector seam = guide.midpoint(0) - guide.centre;
    const double corner_angle = guide.oculus_corner_angle(0);
    const double seam_angle = guide.oculus_seam_angle(0);
    Frame frame(CHAPTER, 6, "validity_ring", fmt::format("invalid(), check C: sin(oculus_corner_angle) = {:.3f} >= sin(oculus_seam_angle) = {:.3f}, so the ring covers", std::sin(corner_angle * M_PI / 180.0), std::sin(seam_angle * M_PI / 180.0)), "top", {-480.0, -1480.0, H - 10.0, 480.0, -620.0, H + 10.0});
    frame.polyline(up(diamond(guide)), GREY, 2.0);
    frame.line(up(guide.seams[0].line), GREY, 2.0);

    const Point next_tip = corner + to_next.normalized() * 400.0;
    const Point previous_tip = corner + to_previous.normalized() * 400.0;
    const Point seam_tip = corner + seam.normalized() * 400.0;
    frame.line(up(Line::from_points(corner, next_tip)), INPUT, 3.0, false, true);
    frame.line(up(Line::from_points(corner, previous_tip)), INPUT, 3.0, false, true);
    frame.line(up(Line::from_points(corner, seam_tip)), INPUT, 3.0, false, true);

    const Polyline corner_arc = arc(corner, to_next, to_previous, 150.0);
    const Polyline seam_arc = arc(corner, seam, to_next, 250.0);
    frame.polyline(up(corner_arc), BUILT, 3.0);
    frame.polyline(up(seam_arc), RESULT, 3.0);
    frame.point(up(corner), INPUT);
    frame.label("oculus_corners[0]", up(corner));
    frame.label("oculus_corners[1] - oculus_corners[0]", up(next_tip));
    frame.label("oculus_corners[3] - oculus_corners[0]", up(previous_tip));
    frame.label("midpoint(0) - centre", up(seam_tip));
    frame.label(fmt::format("oculus_corner_angle(0) = {:.1f}", corner_angle), up(arc_middle(corner_arc)));
    frame.label(fmt::format("oculus_seam_angle(0) = {:.1f}", seam_angle), up(arc_middle(seam_arc)));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry primitives
// ═══════════════════════════════════════════════════════════════════════════

/// Frame 7: level, edge_plane, plane_plane_plane and line_plane on the guide's own planes.
void primitives(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Plane datum = level(0.0);
    const Plane edge0 = edge_plane(guide.edges[0].line, -Z);
    const Plane edge3 = edge_plane(guide.edges[3].line, -Z);
    const Point corner = plane_plane_plane(datum, edge0, edge3).value();
    const OculusEdge& oculus = guide.oculus_edges[0];
    const Point hit = line_plane(guide.seams[0].line, oculus.tilted).value();
    Frame frame(CHAPTER, 7, "primitives", "The helpers: level(z), edge_plane(edge, -Z) with normal direction x (-Z), plane_plane_plane, line_plane", "iso", PLANES);
    frame.plane_size = 300.0;
    frame.distance = 0.68;

    const Point datum_origin = corner + (X + Y) * 300.0;
    const Point edge0_origin = corner + X * 300.0;
    const Point edge3_origin = corner + Y * 300.0;
    frame.plane(up(facing(datum, datum_origin, X)), BUILT);
    frame.plane(up(facing(edge0, edge0_origin, X)), BUILT);
    frame.plane(up(facing(edge3, edge3_origin, Y)), BUILT);
    frame.line(up(Line::from_point_direction_length(corner, guide.edges[0].line.to_direction(), 650.0)), VARIABLE, 3.0, false, true);
    frame.point(up(corner), RESULT, 16.0);

    const Vector along = oculus.line.to_direction();
    frame.line(up(guide.seams[0].line), INPUT, 3.0);
    frame.plane(up(facing(oculus.tilted, hit + along * 300.0, along)), INPUT);
    frame.point(up(hit), RESULT, 16.0);

    frame.label("level(0.0)", up(datum_origin));
    frame.label("edge_plane(edges[0].line, -Z)", up(edge0_origin));
    frame.label("edge_plane(edges[3].line, -Z)", up(edge3_origin));
    frame.label("direction", up(corner + guide.edges[0].line.to_direction() * 650.0));
    frame.label("plane_plane_plane(...) = corners[0]", up(corner));
    frame.label("seams[0].line", up(guide.seams[0].line.point_at(0.25)));
    frame.label("oculus_edges[0].tilted", up(hit + along * 300.0));
    frame.label("line_plane(seams[0].line, tilted) = oculus_corners[0]", up(hit));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Bay edges
// ═══════════════════════════════════════════════════════════════════════════

/// Frame 8: bay edge 0, its midpoint and band[0], the vertical plane standing on it with the normal into the bay.
void bay_edge(const Context& context) {

    const FloorGuide& guide = context.guide;
    const BayEdge& edge = guide.edges[0];
    const Plane& band = edge.band[0];
    const double depth = guide.height;
    Frame frame(CHAPTER, 8, "bay_edge_band0", "edges[0]: line and midpoint; band[0] = the vertical plane on it, normal direction x (-Z) into the bay", "iso", {-3300.0, -3400.0, H - 750.0, 3300.0, -1800.0, H + 200.0});
    frame.plane_size = 300.0;
    plan_context(frame, guide, false);
    // the band's outline without its top side, which is the bay edge drawn in blue over it
    frame.polyline(up(Polyline({edge.line.end(), edge.line.end() - Z * depth, edge.line.start() - Z * depth, edge.line.start()})), RESULT, 2.0);
    frame.plane(up(facing(band, band.origin(), edge.line.to_direction())), RESULT);
    frame.line(up(edge.line), BUILT, 5.0, false, true);
    frame.point(up(edge.midpoint), BUILT);
    frame.label("corners[0]", up(edge.line.start()));
    frame.label("corners[1]", up(edge.line.end()));
    frame.label("edges[0].line", up(edge.line.point_at(0.25)));
    frame.label("edges[0].midpoint", up(edge.midpoint));
    frame.label(fmt::format("edges[0].band[0], drawn {:.0f} deep", depth), up(edge.line.point_at(0.75) - Z * (depth * 0.6)));
    frame.label(fmt::format("band[0].z_axis() = ({:.0f}, {:.0f}, {:.0f})", band.z_axis()[0] + 0.0, band.z_axis()[1] + 0.0, band.z_axis()[2] + 0.0), up(edge.midpoint + band.z_axis() * 300.0));
    frame.write(context.dir);
}

/// Frame 9: the four bands, band[0] on the edge and band[1] offset by outer_ribs into the bay, filled between.
void bands(const Context& context) {

    const FloorGuide& guide = context.guide;
    const double thickness = guide.outer_ribs;
    Frame frame(CHAPTER, 9, "bands", fmt::format("band[1] = band[0].translate_by_normal(outer_ribs): a {:.0f} mm outer rib band along every bay edge", thickness), "top", BAY);
    std::array<Point, 4> inner;

    for (size_t k = 0; k < 4; k++)
        inner[k] = plane_plane_plane(level(0.0), guide.edges[k].band[1], guide.edges[(k + 3) % 4].band[1]).value();

    for (size_t k = 0; k < 4; k++) {
        const BayEdge& edge = guide.edges[k];
        frame.line(up(edge.line), INPUT, 3.0);
        frame.line(up(Line::from_points(inner[k], inner[(k + 1) % 4])), BUILT, 3.0);

        const Point corner = plane_plane_plane(level(0.0), edge.band[0], guide.edges[(k + 1) % 4].band[0]).value();
        const Point before = plane_plane_plane(level(0.0), edge.band[0], guide.edges[(k + 3) % 4].band[0]).value();
        frame.fill(up(Polyline({before, corner, inner[(k + 1) % 4], inner[k]}).closed()), VARIABLE);
    }

    frame.label("edges[0].band[0]", up(guide.edges[0].line.point_at(0.25)));
    frame.label("edges[0].band[1]", up(Line::from_points(inner[0], inner[1]).point_at(0.75)));
    frame.label(fmt::format("outer_ribs = {:.0f}", thickness), up(guide.edges[0].midpoint + guide.edges[0].band[0].z_axis() * (thickness * 0.5)));

    for (size_t k = 1; k < 4; k++)
        frame.label(fmt::format("edges[{}].band", k), up(guide.edges[k].midpoint + guide.edges[k].band[0].z_axis() * (thickness * 0.5)));

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Seams
// ═══════════════════════════════════════════════════════════════════════════

/// Frame 10: the four seams, midpoint to oculus corner where the seam beams run, oculus corner to centre dashed.
void seams(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 10, "seams", "seams[k]: line midpoint(k) to centre, oculus_corner where both seam beams end, thickness = inner_beams", "top", BAY);
    plan_context(frame, guide, false);
    frame.polyline(up(diamond(guide)), GREY, 2.0);

    for (size_t k = 0; k < 4; k++) {
        const Seam& seam = guide.seams[k];
        frame.line(up(Line::from_points(seam.line.start(), seam.oculus_corner)), BUILT, 5.0);
        frame.line(up(Line::from_points(seam.oculus_corner, seam.line.end())), BUILT, 2.0, true);
        frame.point(up(seam.oculus_corner), INPUT);

        if (k > 0)
            frame.label(fmt::format("seams[{}]", k), up(Line::from_points(seam.line.start(), seam.oculus_corner).center()));
    }

    const Seam& seam = guide.seams[0];
    frame.label("seams[0].line: midpoint(0) to centre", up(Line::from_points(seam.line.start(), seam.oculus_corner).center()));
    frame.label(fmt::format("seams[0].thickness = {:.0f}", seam.thickness), up(seam.line.point_at(0.15)));
    frame.label("seams[0].oculus_corner", up(seam.oculus_corner));
    frame.label("seams[0].line.end() = centre", up(seam.line.end()));
    frame.write(context.dir);
}

/// Frame 11: seam 0's plane, vertical on the half seam, origin at its middle and normal into quarter 0.
void seam_plane(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Seam& seam = guide.seams[0];
    const Plane& plane = seam.plane;
    const Vector normal = plane.z_axis();
    const Polyline quarter = Polyline(guide.geometry[0].polygon).closed();
    Frame frame(CHAPTER, 11, "seam_plane", "seams[0].plane = plane_into(0): edge_plane(midpoint to oculus corner, -Z), normal into quarter 0", "iso", {-3100.0, -3300.0, H - 1000.0, 700.0, 200.0, H + 1000.0});
    frame.distance = 0.65;
    frame.plane_size = (seam.oculus_corner - seam.line.start()).magnitude() * 0.5;
    frame.polyline(up(quarter), GREY, 3.0);
    frame.polyline(up(diamond(guide)), GREY, 1.0);
    frame.line(up(Line::from_points(seam.line.start(), seam.oculus_corner)), INPUT, 5.0);
    frame.plane(up(facing(plane, plane.origin(), seam.line.to_direction())), BUILT);
    frame.point(up(plane.origin()), BUILT, 16.0);
    frame.label("seams[0].plane", up(plane.origin() + Z * (frame.plane_size * 0.6)));
    frame.label(fmt::format("origin() = ({:.0f}, {:.0f}, {:.0f})", plane.origin()[0], plane.origin()[1], plane.origin()[2]), up(plane.origin()));
    frame.label(fmt::format("z_axis() = ({:.0f}, {:.0f}, {:.0f})", normal[0] + 0.0, normal[1] + 0.0, normal[2] + 0.0), up(plane.origin() + normal * 400.0));
    frame.label("quarter 0", up(area_centroid(quarter)), true);
    frame.label("seams[0].line.start() = midpoint(0)", up(seam.line.start()));
    frame.label("seams[0].oculus_corner", up(seam.oculus_corner));
    frame.write(context.dir);
}

/// Frame 12: seam 0 close up, the beam faces each quarter reads, back to back on the seam plane.
void seam_faces(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Seam& seam = guide.seams[0];
    const Point& a = seam.line.start();
    const Point& b = seam.oculus_corner;
    const std::array<std::array<Plane, 2>, 2> faces = {seam.faces_into(0), seam.faces_into(1)};
    const std::array<Color, 2> colors = {BUILT, RESULT};
    const std::array<double, 2> heights = {-2150.0, -1850.0};
    Frame frame(CHAPTER, 12, "seam_faces", "faces_into(q) = pair(plane_into(q), thickness): quarter 0's and quarter 1's seam beams back to back", "top", {-450.0, -2350.0, H - 10.0, 450.0, -1650.0, H + 10.0});
    frame.line(up(trace(faces[0][0], a, b)), INPUT, 3.0);

    for (size_t q = 0; q < 2; q++) {
        const Line far = trace(faces[q][1], a, b);
        const Vector normal = faces[q][0].z_axis();
        frame.line(up(far), colors[q], 4.0);

        frame.fill(up(Polyline({faces[q][0].project(Point(0.0, -2340.0, 0.0)), faces[q][0].project(Point(0.0, -1650.0, 0.0)), faces[q][1].project(Point(0.0, -1650.0, 0.0)), faces[q][1].project(Point(0.0, -2340.0, 0.0))}).closed()), colors[q]);

        const Point base = faces[q][0].project(Point(0.0, heights[q], 0.0));
        frame.line(up(Line::from_points(base, base + normal * 45.0)), colors[q], 4.0, false, true);
        frame.label(fmt::format("seams[0].plane_into({})", q), up(base + normal * 45.0));
        frame.label(fmt::format("faces_into({})[1]: x = {:.0f}", q, faces[q][1].origin()[0]), up(far.point_at(q == 0 ? 0.35 : 0.65)));
        frame.label(fmt::format("quarter {}", q), up(base + normal * 250.0), true);
    }

    frame.label("faces_into(0)[0] = faces_into(1)[0]: x = 0", up(trace(faces[0][0], a, b).point_at(0.4)));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus edge
// ═══════════════════════════════════════════════════════════════════════════

/// Frame 13: oculus edge 0 from oculus corner 0 to oculus corner 3 and its vertical plane, normal away from the centre.
void oculus_edge(const Context& context) {

    const FloorGuide& guide = context.guide;
    const OculusEdge& edge = guide.oculus_edges[0];
    const Plane plane = edge_plane(edge.line, -Z);
    const Vector normal = plane.z_axis();
    Frame frame(CHAPTER, 13, "oculus_edge", "oculus_edges[0].line: oculus corner 0 to 3; plane = edge_plane(line, -Z), normal away from the centre", "top", {-1250.0, -1250.0, H - 10.0, 250.0, 250.0, H + 10.0});
    frame.plane_size = edge.line.length() * 0.5;
    frame.polyline(up(diamond(guide)), GREY, 2.0);
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 1.0);
    frame.point(up(guide.centre), GREY);
    frame.plane(up(facing(plane, plane.origin(), edge.line.to_direction())), RESULT);
    frame.line(up(edge.line), BUILT, 4.0, false, true);
    frame.point(up(plane.origin()), RESULT);
    frame.label("oculus_edges[0].line", up(edge.line.point_at(0.25)));
    frame.label("oculus_corners[0]", up(edge.line.start()));
    frame.label("oculus_corners[3]", up(edge.line.end()));
    frame.label(fmt::format("plane.origin() = ({:.0f}, {:.0f}, 0)", plane.origin()[0], plane.origin()[1]), up(plane.origin()));
    frame.label(fmt::format("plane.z_axis() = ({:.3f}, {:.3f}, 0)", normal[0], normal[1]), up(plane.origin() + normal * 250.0));
    frame.label("centre", up(guide.centre));
    frame.write(context.dir);
}

/// Frame 14: looking along oculus edge 0, the vertical plane and tilted, rotated by -oculus_plane_angle about the edge.
void tilted(const Context& context) {

    const FloorGuide& guide = context.guide;
    const OculusEdge& edge = guide.oculus_edges[0];
    const Plane plane = edge_plane(edge.line, -Z);
    const Point centre = edge.line.center();
    const Vector direction = edge.line.to_direction();
    const Vector inward = -plane.z_axis();
    const double depth = guide.height;
    const Plane section = Plane::from_point_normal(centre, direction);
    const Point vertical_bottom = plane_plane_plane(level(-depth), plane, section).value();
    const Point tilted_bottom = plane_plane_plane(level(-depth), edge.tilted, section).value();
    const double offset = (tilted_bottom - vertical_bottom).magnitude();
    Frame frame(CHAPTER, 14, "tilted", fmt::format("tilted = rotate(plane, -{:.0f} deg, edge direction, edge centre): its top trace stays on the edge", guide.oculus_plane_angle), "front", {-900.0, -900.0, H - depth - 100.0, -100.0, -100.0, H + 120.0});
    frame.orbit = fmt::format("{:.2f},0", -Y.angle(direction, true, false) / ORBIT_RADIANS_PER_PIXEL);
    frame.distance = 0.65;

    const Polyline lean = arc(centre, vertical_bottom - centre, tilted_bottom - centre, depth * 0.7);
    const Point foot = Line::from_points(centre, tilted_bottom).center();
    frame.line(up(Line::from_points(centre - inward * 350.0, centre + inward * 350.0)), INPUT, 2.0, true);
    frame.line(up(Line::from_points(centre, vertical_bottom)), INPUT, 3.0);
    frame.line(up(Line::from_points(centre, tilted_bottom)), BUILT, 4.0);
    frame.polyline(up(lean), VARIABLE, 3.0);
    frame.line(up(Line::from_points(vertical_bottom, tilted_bottom)), VARIABLE, 4.0);
    frame.line(up(Line::from_points(foot, foot + edge.tilted.z_axis() * 150.0)), BUILT, 3.0, false, true);
    frame.label("oculus_edges[0].line, seen end-on", up(centre));
    frame.label("plane, vertical", up(Line::from_points(centre, vertical_bottom).point_at(0.85)));
    frame.label("oculus_edges[0].tilted", up(Line::from_points(centre, tilted_bottom).point_at(0.3)));
    frame.label(fmt::format("oculus_plane_angle = {:.0f} deg", guide.oculus_plane_angle), up(arc_middle(lean)));
    frame.label(fmt::format("height tan(5 deg) = {:.1f} at height = {:.0f}", offset, depth), up(Line::from_points(vertical_bottom, tilted_bottom).center()));
    frame.label("tilted.z_axis()", up(foot + edge.tilted.z_axis() * 150.0));
    frame.label("toward the centre", up(centre + inward * 350.0));
    frame.write(context.dir);
}

/// Frame 15: back inner_beams into the quarter and ring_inner inner_beams toward the centre, either side of the edge.
void back_ring_inner(const Context& context) {

    const FloorGuide& guide = context.guide;
    const OculusEdge& edge = guide.oculus_edges[0];
    const Point centre = edge.line.center();
    const Vector direction = edge.line.to_direction();
    const Point a = centre - direction * 650.0;
    const Point b = centre + direction * 650.0;
    const Line back = trace(edge.back, a, b);
    const Line ring = trace(edge.ring_inner, a, b);
    const double thickness = guide.inner_beams;
    Frame frame(CHAPTER, 15, "back_ring_inner", "back = plane + inner_beams into the quarter; ring_inner = back - 2 inner_beams, inside the edge", "top", {-900.0, -900.0, H - 10.0, -100.0, -100.0, H + 10.0});
    frame.line(up(Line::from_points(a, b)), INPUT, 3.0);
    frame.line(up(back), BUILT, 4.0);
    frame.line(up(ring), RESULT, 4.0);

    const Point back_foot = centre - direction * 150.0;
    const Point ring_foot = centre + direction * 150.0;
    frame.line(up(Line::from_points(back_foot, edge.back.project(back_foot))), VARIABLE, 2.0);
    frame.line(up(Line::from_points(ring_foot, edge.ring_inner.project(ring_foot))), VARIABLE, 2.0);
    frame.line(up(Line::from_points(back.point_at(0.7), back.point_at(0.7) + edge.back.z_axis() * 80.0)), BUILT, 3.0, false, true);
    frame.label("oculus_edges[0].back", up(back.point_at(0.25)));
    frame.label("oculus_edges[0].line = tilted's datum trace", up(Line::from_points(a, b).point_at(0.5)));
    frame.label("oculus_edges[0].ring_inner", up(ring.point_at(0.75)));
    frame.label(fmt::format("inner_beams = {:.0f}", thickness), up(Line::from_points(back_foot, edge.back.project(back_foot)).center()));
    frame.label(fmt::format("back - 2 x {:.0f}: {:.0f} inside the edge", thickness, thickness), up(Line::from_points(ring_foot, edge.ring_inner.project(ring_foot)).center()));
    frame.label("back.z_axis(), away from the centre", up(back.point_at(0.7) + edge.back.z_axis() * 80.0));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Column corner
// ═══════════════════════════════════════════════════════════════════════════

/// Frame 16: column 0's frame, x along the edge after the corner and y along the edge before it, with the bisector the skew rule turns.
void corner_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const Point& corner = column.corner;
    const Vector bisector = (column.x_axis + column.y_axis).normalized();
    Frame frame(CHAPTER, 16, "corner_frame", fmt::format("corner_frame: corner_angle(0) = {:.0f}, so x_axis = after and y_axis = before; else bisector -/+ 45 deg", guide.corner_angle(0)), "top", CORNER);
    frame.line(up(guide.edges[0].line), GREY, 2.0);
    frame.line(up(guide.edges[3].line), GREY, 2.0);
    frame.line(up(Line::from_points(corner, corner + column.x_axis * 300.0)), BUILT, 5.0, false, true);
    frame.line(up(Line::from_points(corner, corner + column.y_axis * 300.0)), RESULT, 5.0, false, true);
    frame.line(up(Line::from_points(corner, corner + bisector * 340.0)), INPUT, 2.0, true);

    const Polyline right = arc(corner, column.x_axis, column.y_axis, 110.0);
    frame.polyline(up(right), VARIABLE, 3.0);
    frame.point(up(corner), INPUT);
    frame.label("corners[0]", up(corner));
    frame.label(fmt::format("columns[0].x_axis = ({:.0f}, {:.0f}, 0)", column.x_axis[0], column.x_axis[1]), up(corner + column.x_axis * 300.0));
    frame.label(fmt::format("columns[0].y_axis = ({:.0f}, {:.0f}, 0)", column.y_axis[0], column.y_axis[1]), up(corner + column.y_axis * 300.0));
    frame.label(fmt::format("corner_angle(0) = {:.0f}", guide.corner_angle(0)), up(arc_middle(right)));
    frame.label("bisector (skew corners only)", up(corner + bisector * 340.0));
    frame.write(context.dir);
}

/// Frame 17: the head pentagon in the corner frame and the chamfer direction.
void head(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const FloorGuide& parameters = guide;
    const Line chamfer = Line::from_points(column.head[2], column.head[3]);
    Frame frame(CHAPTER, 17, "head", "columns[0].head: the column_head square at the corner, its inner corner chamfered at column_head_chamfer", "top", CORNER);
    frame.key = true;
    frame.line(up(guide.edges[0].line), GREY, 2.0);
    frame.line(up(guide.edges[3].line), GREY, 2.0);
    frame.polyline(up(Polyline(column.head).closed()), BUILT, 4.0);

    for (size_t i = 0; i < column.head.size(); i++) {
        frame.point(up(column.head[i]), BUILT);
        frame.label(fmt::format("head[{}]", i), up(column.head[i]));
    }

    frame.dimension(up(column.head[0]), up(column.head[1]), column.y_axis * -30.0, fmt::format("column_head = {:.0f}", parameters.column_head));
    frame.dimension(up(column.head[1]), up(column.head[2]), column.x_axis * 30.0, fmt::format("column_head_chamfer = {:.0f}", parameters.column_head_chamfer));
    frame.line(up(chamfer), RESULT, 4.0, false, true);
    frame.label(fmt::format("chamfer_direction, |head[3] - head[2]| = {:.2f}", chamfer.length()), up(chamfer.center()));
    frame.write(context.dir);
}

/// Frame 18: the head's two side planes on the bay boundary, normals into the bay.
void head_sides(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    Frame frame(CHAPTER, 18, "head_sides", "columns[0].sides: edge_plane(edge(head, 0), -Z) and edge_plane(edge(head, 4), -Z), normals into the bay", "iso", {-3080.0, -3080.0, H - 160.0, -2720.0, -2720.0, H + 160.0});
    frame.plane_size = (column.head[1] - column.head[0]).magnitude() * 0.5;
    frame.distance = 0.75;
    frame.line(up(guide.edges[0].line), GREY, 2.0);
    frame.line(up(guide.edges[3].line), GREY, 2.0);
    frame.polyline(up(Polyline(column.head).closed()), INPUT, 3.0);
    frame.plane(up(facing(column.sides[0], column.sides[0].origin(), edge(column.head, 0).to_direction())), BUILT);
    frame.plane(up(facing(column.sides[1], column.sides[1].origin(), edge(column.head, 4).to_direction())), BUILT);
    frame.label("columns[0].sides[0]", up(column.sides[0].origin()));
    frame.label("columns[0].sides[1]", up(column.sides[1].origin()));
    frame.label("head[0] = corners[0]", up(column.head[0]));
    frame.label("head[1]", up(column.head[1]));
    frame.label("head[4]", up(column.head[4]));
    frame.write(context.dir);
}

/// Frame 19: the column's cutter levels at the head, levels[1] a placeholder until rib_bottom_level.
void levels(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const double bottom = -guide.column_head_depth;
    Frame frame(CHAPTER, 19, "levels", "columns[0].levels = {0, 0, -column_head_depth}; levels[1] is overwritten after the four quarters", "front", {-3200.0, -3050.0, H + bottom - 90.0, -2580.0, -2750.0, H + 60.0});
    const Polyline outline = Polyline(column.head).closed();
    frame.polyline(up(outline), INPUT, 2.0);
    frame.polyline(up(outline + Z * bottom), GREY, 2.0);

    for (const Point& point : column.head)
        frame.line(up(Line::from_points(point, point + Z * bottom)), GREY, 1.0);

    const std::array<double, 3> shown = {column.levels[0], column.levels[1], column.levels[2]};
    const std::array<double, 3> labelled_at = {0.95, 0.05, 0.95};
    const std::array<std::string, 3> texts = {
        fmt::format("levels[0] = {:.0f}: the datum", shown[0]),
        fmt::format("levels[1] = 0 here; rib_bottom_level sets {:.2f}", shown[1]),
        fmt::format("levels[2] = -column_head_depth = {:.0f}", shown[2]),
    };

    for (size_t i = 0; i < 3; i++) {
        const Line level_line = Line::from_points(column.axis_point - X * 280.0 + Z * shown[i], column.axis_point + X * 280.0 + Z * shown[i]);
        frame.line(up(level_line), BUILT, 3.0, i == 1);
        frame.label(texts[i], up(level_line.point_at(labelled_at[i])));
    }

    frame.label("columns[0].head", up(column.head[2]));
    frame.write(context.dir);
}

/// Frame 20: the axis point in the middle of the column_head square and the support frame on it, at the slab.
void support_plane(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const Plane& support = column.support_plane;
    const double side = guide.column_head;
    const Point& corner = column.corner;
    Frame frame(CHAPTER, 20, "support_plane", "axis_point = corner + (x_axis + y_axis) * column_head / 2; support_plane: that frame at z 0, the slab", "iso", {-3060.0, -3060.0, -60.0, -2720.0, -2720.0, 160.0});
    frame.plane_size = side * 0.25;
    frame.distance = 0.7;
    const std::array<Point, 4> square = {corner, corner + column.x_axis * side, corner + (column.x_axis + column.y_axis) * side, corner + column.y_axis * side};

    for (size_t i = 0; i < 4; i++)
        frame.line(Line::from_points(square[i], square[(i + 1) % 4]), INPUT, 2.0, true);

    frame.plane(support, BUILT);
    frame.point(column.axis_point, RESULT, 16.0);

    const std::array<Vector, 3> axes = {support.x_axis(), support.y_axis(), support.z_axis()};
    const std::array<std::string, 3> names = {"x_axis()", "y_axis()", "z_axis()"};

    for (size_t i = 0; i < 3; i++) {
        frame.line(Line::from_point_direction_length(column.axis_point, axes[i], 130.0), BUILT, 4.0, false, true);
        frame.label(fmt::format("support_plane.{}", names[i]), column.axis_point + axes[i] * 130.0);
    }

    const Point foot_x = column.axis_point - column.y_axis * (side * 0.5);
    const Point foot_y = column.axis_point - column.x_axis * (side * 0.5);
    frame.line(Line::from_points(column.axis_point, foot_x), VARIABLE, 2.0);
    frame.line(Line::from_points(column.axis_point, foot_y), VARIABLE, 2.0);
    frame.label(fmt::format("column_head / 2 = {:.0f}", side * 0.5), Line::from_points(column.axis_point, foot_x).point_at(0.6));
    frame.label(fmt::format("axis_point = ({:.0f}, {:.0f}, 0)", column.axis_point[0], column.axis_point[1]), column.axis_point);
    frame.write(context.dir);
}

/// Frame 21: the four column axes from the slab up a storey to the floor datum.
void column_axes(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 21, "column_axes", fmt::format("columns[k].axis: from axis_point up bay_height = {:.0f}, from the slab to the floor datum", guide.bay_height), "iso", FLOOR);
    frame.distance = 0.9;
    plan_context(frame, guide, false);

    for (size_t k = 0; k < 4; k++) {
        const ColumnCorner& column = guide.columns[k];
        frame.polyline(up(Polyline(column.head).closed()), GREY, 2.0);
        frame.line(column.axis, BUILT, 4.0, false, true);
        frame.point(column.axis.start(), INPUT);
        frame.label(fmt::format("columns[{}].axis", k), column.axis.point_at(k == 0 ? 0.5 : 0.6));
    }

    const Line& axis = guide.columns[0].axis;
    frame.label("axis.start() = axis_point, z 0", axis.start());
    frame.label(fmt::format("axis.end(): z = bay_height = {:.0f}", axis.end()[2]), axis.end());
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Quarters
// ═══════════════════════════════════════════════════════════════════════════

/// Frame 22: the four quarter pentagons compute_quarter starts from, quarter 0's vertices named.
void quarter_polygons(const Context& context) {

    const FloorGuide& guide = context.guide;
    const std::vector<Point>& polygon = guide.geometry[0].polygon;
    Frame frame(CHAPTER, 22, "quarter_polygons", "geometry[q].polygon: corner, midpoint, two oculus corners, midpoint; compute_quarter(q) starts from it", "top", BAY);
    frame.key = true;
    plan_context(frame, guide, false);

    for (size_t q = 0; q < 4; q++) {
        const Polyline outline = Polyline(guide.geometry[q].polygon).closed();
        frame.polyline(up(outline), q == 0 ? BUILT : GREY, q == 0 ? 4.0 : 2.0);
        frame.polyline(up(Polyline(guide.columns[q].head).closed()), GREY, 2.0);

        if (q > 0)
            frame.label(fmt::format("geometry[{}].polygon", q), up(area_centroid(outline)), true);
    }

    const std::array<std::string, 5> names = {"corners[0]", "edges[0].midpoint", "oculus_corners[0]", "oculus_corners[3]", "edges[3].midpoint"};

    for (size_t i = 0; i < polygon.size(); i++) {
        frame.point(up(polygon[i]), INPUT);
        frame.label(fmt::format("polygon[{}] = {}", i, names[i]), up(polygon[i]));
    }

    frame.write(context.dir);
}

}

/// The bay chapter: corners, centre, oculus, the validity checks, the bay edges, seams, oculus edges and column corners, and the quarter polygons.
void chapter_01_bay(const Context& context) {
    bay_corners(context);
    centre_midpoints(context);
    oculus_corners(context);
    validity_turns(context);
    validity_seam(context);
    validity_ring(context);
    primitives(context);
    bay_edge(context);
    bands(context);
    seams(context);
    seam_plane(context);
    seam_faces(context);
    oculus_edge(context);
    tilted(context);
    back_ring_inner(context);
    corner_frame(context);
    head(context);
    head_sides(context);
    levels(context);
    support_plane(context);
    column_axes(context);
    quarter_polygons(context);
}

}
