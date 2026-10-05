#include "docs/floor/movie.h"

namespace movie {

namespace {

const std::string CHAPTER = "02_quarter_planes";

// ═══════════════════════════════════════════════════════════════════════════
// Drawing helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The point of a plane's datum trace nearest a plan point.
Point foot(const Plane& plane, const Point& near) {
    const Vector along = plane.z_axis().cross(Vector::z_axis());
    return plane_plane_plane(level(0.0), plane, Plane::from_point_normal(near, along)).value();
}

/// A plane's datum trace between the feet of two plan points.
Line span(const Plane& plane, const Point& from, const Point& to) {
    return Line::from_points(foot(plane, from), foot(plane, to));
}

/// A plane's line of steepest descent from its datum trace near a point down to depth below the datum.
Line fall(const Plane& plane, const Point& near, double depth) {
    const Vector along = plane.z_axis().cross(Vector::z_axis());
    const Vector slope = plane.z_axis().cross(along).normalized();
    const Vector down = slope[2] > 0.0 ? -slope : slope;
    const Point top = foot(plane, near);
    return Line::from_points(top, top + down * (depth / -down[2]));
}

/// A plane drawn as the rectangle that hangs from its datum trace between two plan points down to depth.
Polyline sheet(const Plane& plane, const Point& from, const Point& to, double depth) {
    const Line top = span(plane, from, to);
    const Vector drop = fall(plane, top.center(), depth).to_vector();
    return Polyline({top.start(), top.end(), top.end() + drop, top.start() + drop}).closed();
}

/// The closed strip between two parallel planes' datum traces over the feet of two plan points.
Polyline strip(const Plane& plane0, const Plane& plane1, const Point& from, const Point& to) {
    const Line line0 = span(plane0, from, to);
    const Line line1 = span(plane1, from, to);
    return Polyline({line0.start(), line0.end(), line1.end(), line1.start()}).closed();
}

/// The arc a point at radius below the pivot sweeps when turned by radians about the axis through the pivot, as rotate() turns a plane.
Polyline arc(const Point& pivot, const Vector& axis, double radians, double radius) {
    const Line line = Line::from_points(pivot, pivot + axis);
    const Point start = pivot + Vector(0.0, 0.0, -radius);
    std::vector<Point> points;
    for (size_t i = 0; i <= 12; i++)
        points.push_back(start.transformed(Xform::rotation_around_line(line, radians * static_cast<double>(i) / 12.0)));
    return Polyline(points);
}

/// p0 (k 0) or p1 (k 1): the far faces of a seam beam and of the oculus beam meeting at the datum, as construction_planes finds them.
Point far_corner(const ConstructionPlanes& cp, size_t k) {
    return k == 0 ? plane_plane_plane(level(0.0), cp.inner_beams[0][1], cp.inner_beams[1][1]).value() : plane_plane_plane(level(0.0), cp.inner_beams[1][1], cp.inner_beams[2][1]).value();
}

/// The column-end point of rib k's planes, head[2] for rib 0 and head[3] for rib 1.
Point head_end(const ColumnCorner& column, size_t k) {
    return column.head[2 + k];
}

/// The outer rib band k of the corner over the column head: the head side on bay edge k and its offset by outer_ribs.
Polyline band(const ColumnCorner& column, const FloorParameters& parameters, size_t k) {
    const Vector along = k == 0 ? column.x_axis : column.y_axis;
    const Vector across = k == 0 ? column.y_axis : column.x_axis;
    const Point& corner = column.corner;
    return Polyline({corner, corner + along * parameters.column_head, corner + along * parameters.column_head + across * parameters.outer_ribs, corner + across * parameters.outer_ribs}).closed();
}

// ═══════════════════════════════════════════════════════════════════════════
// Frames
// ═══════════════════════════════════════════════════════════════════════════

/// The outer rib faces of quarter 0 hanging from the bay edges: each band re-origined from the edge midpoint to the half-edge centre.
void outer_rib_planes(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const double depth = 300.0;
    const std::array<size_t, 2> edges = {0, 3};
    Frame frame(CHAPTER, 23, "outer_rib_planes", "cp.outer_ribs: the bands of bay edges 0 and 3, re-origined at the centres of the quarter's half edges", "iso", {-3150.0, -3150.0, H - 420.0, 150.0, 150.0, H + 60.0});
    frame.key = true;
    frame.distance = 0.85;
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 1.5);

    for (size_t k = 0; k < 2; k++) {
        const Plane& outer = cp.outer_ribs[k][0];
        const Plane& inner = cp.outer_ribs[k][1];
        const Point shared = guide.edges[edges[k]].band[0].origin();
        const Point corner = outer.origin() + (outer.origin() - shared);
        frame.polyline(up(sheet(outer, corner, shared, depth)), FAMILY_COLORS[0], 3.0);
        frame.polyline(up(sheet(inner, corner, shared, depth)), FAMILY_COLORS[0], 1.5);
        const Vector lift(0.0, 0.0, 120.0);
        frame.line(up(Line::from_points(shared + lift, outer.origin() + lift)), INK, 2.0, true, true);
        frame.point(up(shared), INK);
        frame.point(up(outer.origin()), MARK);
        frame.line(up(Line::from_points(outer.origin(), outer.origin() + outer.z_axis() * 450.0)), MARK, 3.0, false, true);
        frame.label(fmt::format("cp.outer_ribs[{}][0]", k), up(outer.origin()));
        frame.label(fmt::format("cp.outer_ribs[{}][1]", k), up(fall(inner, inner.origin(), depth).end()));
        frame.label(fmt::format("edges[{}].band[0].origin()", edges[k]), up(shared));
    }

    const Point station = cp.outer_ribs[0][0].origin() + (cp.outer_ribs[0][0].origin() - guide.edges[0].band[0].origin()) * 0.5;
    const Line thickness = Line::from_points(station, cp.outer_ribs[0][1].project(station));
    frame.line(up(thickness), MARK, 4.0);
    frame.label(fmt::format("outer_ribs = {:.0f}", guide.parameters.outer_ribs), up(thickness.center()));
    frame.write(context.dir);
}

/// The two seam beams read into quarter 0: each seam plane solid with its normal into the quarter, its far face dashed.
void seam_beam_planes(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const ConstructionPlanes& cp = quarter.geometry().planes;
    const std::array<size_t, 2> beams = {0, 2};
    Frame frame(CHAPTER, 24, "seam_beam_planes", "cp.inner_beams[0] and [2]: seams[0] and seams[3] faces_into(0), the seam plane and its far face 60 inside", "top", {-3150.0, -3150.0, H - 20.0, 250.0, 250.0, H + 20.0});
    frame.polyline(up(Polyline(quarter.geometry().polygon).closed()), GREY, 1.5);

    for (size_t k = 0; k < 2; k++) {
        const Seam& seam = quarter.seam(k);
        const Point midpoint = seam.line.start();
        const Point& oculus_corner = seam.oculus_corner;
        const std::array<Plane, 2>& faces = cp.inner_beams[beams[k]];
        frame.polyline(up(strip(cp.outer_ribs[k][0], cp.outer_ribs[k][1], guide.corners[0], midpoint)), GREY, 1.0);
        frame.line(up(span(faces[0], midpoint, oculus_corner)), FAMILY_COLORS[2], 3.0);
        frame.line(up(span(faces[1], midpoint, oculus_corner)), FAMILY_COLORS[2], 2.0, true);
        frame.line(up(Line::from_points(faces[0].origin(), faces[0].origin() + faces[0].z_axis() * 320.0)), MARK, 3.0, false, true);
        frame.point(up(faces[0].origin()), MARK);
        frame.label(fmt::format("cp.inner_beams[{}][0]", beams[k]), up(foot(faces[0], midpoint + (oculus_corner - midpoint) * 0.25)));
        frame.label(fmt::format("cp.inner_beams[{}][1]", beams[k]), up(foot(faces[1], midpoint + (oculus_corner - midpoint) * 0.8)));
        frame.label(fmt::format("seams[{}].oculus_corner", seam.index), up(oculus_corner));
    }

    const Seam& seam = quarter.seam(0);
    const Point station = foot(cp.inner_beams[0][0], seam.line.start() + (seam.oculus_corner - seam.line.start()) * 0.55);
    const Line thickness = Line::from_points(station, cp.inner_beams[0][1].project(station));
    frame.line(up(thickness), MARK, 4.0);
    frame.label(fmt::format("inner_beams = {:.0f}", guide.parameters.inner_beams), up(thickness.center()));
    frame.write(context.dir);
}

/// A section along oculus edge 0: the vertical edge plane, the tilted bearing plane leaning by oculus_plane_angle, the back face and the unused ring_inner.
void oculus_beam_planes(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const OculusEdge& oculus = guide.oculus_edges[0];
    const Point centre = oculus.line.center();
    const Plane vertical = edge_plane(oculus.line, -Vector::z_axis());
    const double depth = 220.0;
    const double angle = guide.parameters.oculus_plane_angle;
    Frame frame(CHAPTER, 25, "oculus_beam_planes", "cp.inner_beams[1] = {tilted, back}: tilted leans 5 deg toward the centre, back stands 60 behind the edge", "iso", {-650.0, -650.0, H - 250.0, -350.0, -350.0, H + 40.0});
    frame.orbit = "-262,-105";
    frame.distance = 0.8;

    frame.line(up(Line::from_points(centre - vertical.z_axis() * 160.0, centre + vertical.z_axis() * 160.0)), GREY, 2.0);
    frame.line(up(fall(vertical, centre, depth)), GREY, 2.0);
    frame.line(up(fall(oculus.ring_inner, centre, depth)), GREY, 1.5, true);
    frame.line(up(fall(cp.inner_beams[1][1], centre, depth)), FAMILY_COLORS[2], 3.0, true);
    frame.line(up(fall(cp.inner_beams[1][0], centre, depth)), FAMILY_COLORS[2], 4.0);
    const Polyline sweep = arc(centre, oculus.line.to_direction(), -angle * M_PI / 180.0, 120.0);
    frame.polyline(up(sweep), MARK, 3.0);

    const Point station = foot(vertical, centre) + Vector(0.0, 0.0, -170.0);
    const Line thickness = Line::from_points(station, cp.inner_beams[1][1].project(station));
    frame.line(up(thickness), MARK, 3.0);

    frame.label("oculus_edges[0].line", up(centre));
    frame.label("cp.inner_beams[1][0] = oculus.tilted", up(fall(cp.inner_beams[1][0], centre, depth).end()));
    frame.label("cp.inner_beams[1][1] = oculus.back", up(foot(cp.inner_beams[1][1], centre)));
    frame.label("oculus.ring_inner: not read by the quarter", up(fall(oculus.ring_inner, centre, depth).end()));
    frame.label(fmt::format("oculus_plane_angle = {:.0f} deg", angle), up(sweep.get_point(sweep.point_count() - 1)));
    frame.label(fmt::format("inner_beams = {:.0f}", guide.parameters.inner_beams), up(thickness.center()));
    frame.write(context.dir);
}

/// p0 and p1: the datum points where the oculus back face meets the far faces of the two seam beams.
void p0_p1(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const ConstructionPlanes& cp = quarter.geometry().planes;
    const std::array<size_t, 2> beams = {0, 2};
    const Point& centre = guide.centre;
    Frame frame(CHAPTER, 26, "p0_p1", "p0 and p1: plane_plane_plane of level(0.0), a seam beam's far face and the oculus back face", "top", {-1300.0, -1300.0, H - 10.0, 120.0, 120.0, H + 10.0});

    frame.line(up(quarter.oculus_edge().line), GREY, 2.0);
    const Point oculus_start = quarter.oculus_edge().line.start();
    const Point oculus_end = quarter.oculus_edge().line.end();
    frame.line(up(span(cp.inner_beams[1][1], oculus_start + (oculus_start - oculus_end) * 0.15, oculus_end + (oculus_end - oculus_start) * 0.15)), INK, 2.0, true);
    frame.label("cp.inner_beams[1][1]", up(foot(cp.inner_beams[1][1], centre)));

    for (size_t k = 0; k < 2; k++) {
        const Seam& seam = quarter.seam(k);
        const Point& oculus_corner = seam.oculus_corner;
        const std::array<Plane, 2>& faces = cp.inner_beams[beams[k]];
        frame.line(up(span(faces[0], oculus_corner + (oculus_corner - centre) * 0.25, centre)), GREY, 2.0);
        frame.line(up(span(faces[1], oculus_corner + (oculus_corner - centre) * 0.25, centre)), INK, 2.0, true);
        frame.label(fmt::format("cp.inner_beams[{}][1]", beams[k]), up(foot(faces[1], oculus_corner + (centre - oculus_corner) * 0.6)));

        const Point corner = far_corner(cp, k);
        frame.point(up(corner), MARK, 16.0);
        frame.label(fmt::format("p{} = ({:.1f}, {:.1f})", k, corner[0], corner[1]), up(corner));
    }

    frame.write(context.dir);
}

/// The column head of corner 0 in its corner frame, the chamfer thick: p2 and p3 are its two chamfer vertices.
void head_chamfer(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const std::array<std::string, 5> names = {"head[0]", "head[1]", "p2 = head[2]", "p3 = head[3]", "head[4]"};
    Frame frame(CHAPTER, 27, "head_chamfer", "column.head in the corner frame; the inner ribs start at its chamfer ends p2 = head[2] and p3 = head[3]", "top", {-3040.0, -3040.0, H - 10.0, -2740.0, -2740.0, H + 10.0});

    frame.line(up(guide.edges[0].line), GREY, 2.0);
    frame.line(up(guide.edges[3].line), GREY, 2.0);
    frame.polyline(up(Polyline(column.head).closed()), INK, 3.0);
    frame.line(up(Line::from_points(column.head[2], column.head[3])), MARK, 7.0);
    frame.line(up(Line::from_points(column.corner, column.corner + column.x_axis * 90.0)), INK, 3.0, false, true);
    frame.line(up(Line::from_points(column.corner, column.corner + column.y_axis * 90.0)), INK, 3.0, false, true);
    frame.label("x_axis", up(column.corner + column.x_axis * 90.0));
    frame.label("y_axis", up(column.corner + column.y_axis * 90.0));

    for (size_t i = 0; i < column.head.size(); i++) {
        frame.point(up(column.head[i]), i == 2 || i == 3 ? MARK : INK);
        frame.label(names[i], up(column.head[i]));
    }

    frame.write(context.dir);
}

/// The inner rib planes: p2 to p0 and p3 to p1 solid, the central faces dashed, each normal toward the quarter diagonal.
void inner_rib_planes(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const ColumnCorner& column = guide.columns[0];
    Frame frame(CHAPTER, 28, "inner_rib_planes", "cp.inner_ribs: rib0 through p2 and p0, rib1 through p3 and p1; [k][1] = faces[k], 60 toward the diagonal", "top", {-3150.0, -3150.0, H - 20.0, 150.0, 150.0, H + 20.0});
    frame.key = true;
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 1.5);
    frame.polyline(up(Polyline(column.head).closed()), GREY, 1.5);

    for (size_t k = 0; k < 2; k++) {
        const Point from = head_end(column, k);
        const Point to = far_corner(cp, k);
        const Plane& outer = cp.inner_ribs[k][0];
        const Plane& central = cp.inner_ribs[k][1];
        const Line face = span(outer, from, to);
        const Line other = span(central, from, to);
        frame.line(up(span(cp.inner_beams[2 * k][1], to, guide.centre)), GREY, 1.5);
        frame.line(up(face), FAMILY_COLORS[1], 3.0);
        frame.line(up(other), FAMILY_COLORS[1], 2.0, true);
        frame.line(up(Line::from_points(outer.origin(), outer.origin() + outer.z_axis() * 250.0)), MARK, 3.0, false, true);
        frame.point(up(to), MARK);
        frame.label(fmt::format("cp.inner_ribs[{}][0]", k), up(face.point_at(1.0 / 3.0)));
        frame.label(fmt::format("faces[{0}] = cp.inner_ribs[{0}][1]", k), up(other.point_at(2.0 / 3.0)));
        frame.label(fmt::format("normals[{}]", k), up(outer.origin() + outer.z_axis() * 250.0));
        frame.label(fmt::format("p{}", k), up(to));
    }

    frame.line(up(span(cp.inner_beams[1][1], far_corner(cp, 0), far_corner(cp, 1))), GREY, 1.5);
    frame.write(context.dir);
}

/// The middle fan plane: the vertical plane on the chamfer turned by wedge_plane_angle about the chamfer, seen along the chamfer.
void tilted_chamfer_plane(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const Plane& tilted = guide.geometry[0].planes.wedges[1][0];
    const Line side1 = edge(column.head, 2);
    const Plane vertical = edge_plane(side1, Vector::z_axis());
    const double depth = guide.parameters.column_head_depth;
    const double angle = guide.parameters.wedge_plane_angle;
    Frame frame(CHAPTER, 29, "tilted_chamfer_plane", "wedge_fan: tilted = edge_plane(side1, +z) turned by wedge_plane_angle about the chamfer, seen along it", "iso", {-3080.0, -3080.0, H - 800.0, -2450.0, -2450.0, H + 40.0});
    frame.orbit = "-262,-50";
    frame.distance = 0.8;

    frame.polyline(up(Polyline(column.head).closed()), GREY, 2.0);
    frame.polyline(up(sheet(vertical, side1.start(), side1.end(), depth)), GREY, 2.0);
    frame.polyline(up(sheet(tilted, side1.start(), side1.end(), depth)), FAMILY_COLORS[3], 3.0);
    frame.line(up(side1), MARK, 6.0);
    const Polyline sweep = arc(side1.center(), side1.to_direction(), angle * M_PI / 180.0, 320.0);
    frame.polyline(up(sweep), MARK, 3.0);

    frame.label("side1 = edge(head, 2)", up(side1.start()));
    frame.label("edge_plane(side1, +z)", up(fall(vertical, side1.center(), depth).end()));
    frame.label("tilted = cp.wedges[1][0]", up(fall(tilted, side1.center(), depth).end()));
    frame.label(fmt::format("wedge_plane_angle = {:.0f} deg", angle), up(sweep.get_point(sweep.point_count() - 1)));
    frame.write(context.dir);
}

/// The crease lines line0 and line1 where the tilted plane meets the two inner ribs' central faces, headed along cross(n0, n1).
void crease_lines(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const ColumnCorner& column = guide.columns[0];
    const Plane& tilted = cp.wedges[1][0];
    const double depth = guide.parameters.column_head_depth;
    Frame frame(CHAPTER, 30, "crease_lines", "wedge_fan: line0 and line1 = plane_plane(cp.inner_ribs[k][1], tilted), only their directions are kept", "iso", {-3080.0, -3080.0, H - 800.0, -2450.0, -2450.0, H + 40.0});
    frame.distance = 0.9;

    frame.polyline(up(Polyline(column.head).closed()), GREY, 2.0);
    frame.polyline(up(sheet(tilted, column.head[2], column.head[3], depth)), GREY, 1.5);

    for (size_t k = 0; k < 2; k++) {
        const Plane& central = cp.inner_ribs[k][1];
        const Line crease = plane_plane(central, tilted).value();
        const Point top = line_plane(crease, level(0.0)).value();
        const Point bottom = line_plane(crease, level(-depth)).value();
        const Line line = crease.to_direction().dot(bottom - top) > 0.0 ? Line::from_points(top, bottom) : Line::from_points(bottom, top);
        const Point inward = top + (far_corner(cp, k) - head_end(column, k)).normalized() * 300.0;
        frame.polyline(up(sheet(central, top, inward, depth)), GREY, 1.5);
        frame.line(up(line), MARK, 5.0, false, true);
        frame.label(fmt::format("line{}", k), up(k == 0 ? bottom : top));
        frame.label(fmt::format("cp.inner_ribs[{}][1]", k), up(fall(central, inward, depth).end()));
    }

    frame.label("tilted", up(fall(tilted, column.head[3], depth).end()));
    frame.write(context.dir);
}

/// The fan's datum traces: wedge0, tilted and wedge2 solid on the head sides, their provisional far faces dashed.
void fan_planes(const Context& context) {

    const FloorGuide& guide = context.guide;
    const FloorParameters& parameters = guide.parameters;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const ColumnCorner& column = guide.columns[0];
    const std::array<double, 3> thickness = {parameters.wedge, parameters.wedge * parameters.middle_wedge_factor, parameters.wedge};
    const std::array<std::string, 3> names = {"wedge0", "tilted", "wedge2"};
    Frame frame(CHAPTER, 31, "fan_planes", "wedge_fan: pair(wedge0, wedge), pair(tilted, 1.25 wedge), pair(wedge2, wedge), the far faces provisional", "top", {-3060.0, -3060.0, H - 20.0, -2400.0, -2400.0, H + 20.0});
    frame.key = true;

    frame.polyline(up(Polyline({column.head[1], column.head[0], column.head[4]})), GREY, 2.0);

    for (size_t k = 0; k < 2; k++)
        for (const Plane& face : cp.inner_ribs[k])
            frame.line(up(span(face, head_end(column, k), head_end(column, k) + (far_corner(cp, k) - head_end(column, k)).normalized() * 750.0)), GREY, 1.5);

    for (size_t i = 0; i < 3; i++) {
        const Plane& fan = cp.wedges[i][0];
        const Plane provisional = fan.translate_by_normal(thickness[i]);
        const Line side = edge(column.head, i + 1);
        frame.line(up(span(fan, side.start(), side.end())), FAMILY_COLORS[3], 4.0);
        frame.line(up(span(provisional, side.start(), side.end())), FAMILY_COLORS[3], 2.0, true);
        frame.line(up(Line::from_points(fan.origin(), fan.origin() + fan.z_axis() * 120.0)), MARK, 3.0, false, true);
        frame.label(fmt::format("cp.wedges[{}][0] = {}", i, names[i]), up(side.point_at(0.75)));
        frame.label(fmt::format("cp.wedges[{}][1]: + {:.0f}", i, thickness[i]), up(foot(provisional, side.center())));
    }

    frame.write(context.dir);
}

/// The six flange plane pairs, each a tsections-thick strip on the bed-panel side of a rib face.
void tsection_planes(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const ColumnCorner& column = guide.columns[0];
    const std::array<size_t, 6> rib = {0, 0, 0, 1, 1, 1};
    const std::array<double, 6> station = {0.2, 0.35, 0.5, 0.5, 0.35, 0.2};
    Frame frame(CHAPTER, 32, "tsection_planes", "cp.tsections: a flange pair on each rib face toward a bed panel, moved tsections = 27 into that panel", "top", {-3150.0, -3150.0, H - 20.0, 150.0, 150.0, H + 20.0});
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 1.5);

    for (size_t k = 0; k < 2; k++) {
        const Point from = head_end(column, k);
        const Point to = far_corner(cp, k);
        frame.polyline(up(strip(cp.outer_ribs[k][0], cp.outer_ribs[k][1], from, to)), GREY, 1.5);
        frame.polyline(up(strip(cp.inner_ribs[k][0], cp.inner_ribs[k][1], from, to)), GREY, 1.5);
    }

    for (size_t i = 0; i < cp.tsections.size(); i++) {
        const Point from = head_end(column, rib[i]);
        const Point to = far_corner(cp, rib[i]);
        frame.polyline(up(strip(cp.tsections[i][0], cp.tsections[i][1], from, to)), FAMILY_COLORS[4], 2.5);
        const Line flange = span(cp.tsections[i][1], from, to);
        const std::string text = i == 0 ? fmt::format("cp.tsections[0], tsections = {:.0f}", guide.parameters.tsections) : fmt::format("cp.tsections[{}]", i);
        frame.label(text, up(flange.point_at(station[i])));
    }

    frame.write(context.dir);
}

/// column_seats, first half: the outer rib bands over the head and the column faces flush with the bay edges, offset 0.
void column_offset(const Context& context) {

    const FloorGuide& guide = context.guide;
    const FloorParameters& parameters = guide.parameters;
    const ColumnCorner& column = guide.columns[0];
    const double phi = (guide.corner_angle(0) - 90.0) * 0.5 * M_PI / 180.0;
    const double width = (parameters.outer_ribs - column.column_offset[0]) / std::cos(phi);
    Frame frame(CHAPTER, 33, "column_offset", "column_seats: offset = column_head sin(phi) is 0 at a right corner, so each band is outer_ribs wide", "top", {-3040.0, -3040.0, H - 10.0, -2740.0, -2740.0, H + 10.0});

    frame.line(up(guide.edges[0].line), GREY, 2.0);
    frame.line(up(guide.edges[3].line), GREY, 2.0);
    frame.polyline(up(Polyline(column.head).closed()), GREY, 2.0);

    for (size_t k = 0; k < 2; k++) {
        const Vector along = k == 0 ? column.x_axis : column.y_axis;
        const Vector across = k == 0 ? column.y_axis : column.x_axis;
        frame.polyline(up(band(column, parameters, k)), FAMILY_COLORS[0], 3.0);

        for (double s = 20.0; s + parameters.outer_ribs * 0.5 < parameters.column_head; s += 25.0)
            frame.line(up(Line::from_points(column.corner + along * s, column.corner + along * (s + parameters.outer_ribs * 0.5) + across * parameters.outer_ribs)), FAMILY_COLORS[0], 1.0);

        frame.label(fmt::format("column_offset[{}] = {:.0f}", k, column.column_offset[k]), up(column.corner + along * (parameters.column_head * 0.6)));
        frame.label(fmt::format("cp.outer_ribs[{}] band", k), up(column.corner + along * parameters.column_head + across * (parameters.outer_ribs * 0.5)));
    }

    const Point station = column.corner + column.x_axis * (parameters.column_head * 0.3);
    const Line dimension = Line::from_points(station, station + column.y_axis * width);
    frame.line(up(dimension), MARK, 4.0);
    frame.label(fmt::format("band = {:.0f}", width), up(dimension.point_at(0.5)));
    frame.label(fmt::format("corner_angle(0) = {:.0f} deg", guide.corner_angle(0)), up(column.corner));
    frame.write(context.dir);
}

/// column_seats, second half: the three wedge seats left on the head beside the outer rib bands and between the inner rib footprints on the chamfer.
void wedge_seat(const Context& context) {

    const FloorGuide& guide = context.guide;
    const FloorParameters& parameters = guide.parameters;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const ColumnCorner& column = guide.columns[0];
    const Plane chamfer = edge_plane(edge(column.head, 2), Vector::z_axis());
    Frame frame(CHAPTER, 34, "wedge_seat", "column_seats: wedge_seat, the head left for the wedges beside the bands and between the inner ribs", "top", {-3020.0, -3020.0, H - 10.0, -2720.0, -2720.0, H + 10.0});

    frame.polyline(up(Polyline(column.head).closed()), GREY, 2.0);

    for (size_t k = 0; k < 2; k++) {
        const Point from = head_end(column, k);
        frame.polyline(up(band(column, parameters, k)), GREY, 1.5);
        frame.polyline(up(strip(cp.inner_ribs[k][0], cp.inner_ribs[k][1], from + (from - far_corner(cp, k)).normalized() * 40.0, from + (far_corner(cp, k) - from).normalized() * 160.0)), FAMILY_COLORS[1], 2.5);
    }

    const std::array<Point, 2> footprint = {
        plane_plane_plane(level(0.0), cp.inner_ribs[0][1], chamfer).value(),
        plane_plane_plane(level(0.0), cp.inner_ribs[1][1], chamfer).value(),
    };
    const std::array<Line, 3> seats = {
        Line::from_points(column.head[2] + (column.head[1] - column.head[2]).normalized() * column.wedge_seat[0], column.head[2]),
        Line::from_points(footprint[0], footprint[1]),
        Line::from_points(column.head[3], column.head[3] + (column.head[4] - column.head[3]).normalized() * column.wedge_seat[2]),
    };

    for (size_t i = 0; i < 3; i++) {
        frame.line(up(seats[i]), MARK, 6.0);
        frame.label(fmt::format("wedge_seat[{}] = {:.3f}", i, column.wedge_seat[i]), up(seats[i].center()));
    }

    for (size_t k = 0; k < 2; k++) {
        const Line taken = Line::from_points(head_end(column, k), footprint[k]);
        frame.label(fmt::format("inner_ribs / sin{} = {:.2f}", k, taken.length()), up(taken.center()));
    }

    frame.write(context.dir);
}

}

void chapter_02_quarter_planes(const Context& context) {
    outer_rib_planes(context);
    seam_beam_planes(context);
    oculus_beam_planes(context);
    p0_p1(context);
    head_chamfer(context);
    inner_rib_planes(context);
    tilted_chamfer_plane(context);
    crease_lines(context);
    fan_planes(context);
    tsection_planes(context);
    column_offset(context);
    wedge_seat(context);
}

}
