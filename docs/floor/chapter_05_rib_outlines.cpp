#include "docs/floor/movie.h"

namespace movie {

namespace {

const std::string CHAPTER = "05_rib_outlines";

// ═══════════════════════════════════════════════════════════════════════════
// Rib steps
// ═══════════════════════════════════════════════════════════════════════════

/// The arguments Quarter::outer_ribs or Quarter::inner_ribs passes to rib() for rib k.
struct RibCall {
    Polyline trace; // The soffit trace on the rib's base face.
    Plane face1; // The second face the trace is swept onto.
    Vector sweep; // The direction of that sweep.
    Plane cut_plane0; // The fan plane at the column.
    Plane cut_plane1; // The beam face at the other end.
};

/// The rib() arguments of outer rib k, or of inner rib k when inner.
RibCall rib_call(const Quarter& quarter, size_t k, bool inner) {

    const ConstructionPlanes& cp = quarter.geometry().planes;
    const std::vector<std::array<Polyline, 3>>& parabolas = quarter.geometry().parabolas;

    if (inner)
        return {parabolas[2 + k][0], cp.inner_ribs[k][1], quarter.geometry().central_panel.rib_sweep, cp.wedges[1][0], cp.inner_beams[1][1]};

    return {parabolas[k][0], cp.outer_ribs[k][1], cp.outer_ribs[k][1].z_axis(), cp.wedges[k == 0 ? 0 : 2][0], quarter.rib_seam_ends()[k]};
}

/// The distance of a point from a plane, unsigned, as rib() measures d0 and d1.
double distance_to(const Point& point, const Plane& plane) {
    return std::abs((point - plane.origin()).dot(plane.z_axis()));
}

/// pts as rib() builds them: the trace trimmed by both cut planes, reversed when its last point is nearer the fan.
std::vector<Point> trimmed(const RibCall& call) {

    std::vector<Point> pts = trim(call.trace, call.cut_plane0, call.cut_plane1).get_points();

    if (distance_to(pts.front(), call.cut_plane0) > distance_to(pts.back(), call.cut_plane0))
        std::reverse(pts.begin(), pts.end());

    return pts;
}

/// far before the re-cut: every trimmed point moved along the sweep onto face1.
std::vector<Point> swept(const RibCall& call, const std::vector<Point>& pts) {

    const Xform projection = Xform::project_to_plane_by_axis(call.face1, call.sweep);
    std::vector<Point> far;

    for (const Point& point : pts)
        far.push_back(point.transformed(projection));

    return far;
}

/// rib_plane as rib_loop() builds it: the vertical plane through pts[0] normal to the plan span.
Plane rib_plane(const std::vector<Point>& pts) {

    const Vector span(pts.back()[0] - pts.front()[0], pts.back()[1] - pts.front()[1], 0.0);

    return Plane::from_point_normal(pts.front(), span.cross(Vector(0.0, 0.0, 1.0)));
}

/// The point a plane meets the rib face's vertical plane at level z.
Point meet(const Plane& plane, const Plane& face, double z) {
    return plane_plane_plane(plane, level(z), face).value();
}

/// A point at the datum above or below a point.
Point datum(const Point& point) {
    return Point(point[0], point[1], 0.0);
}

/// "(x, z)" of a point seen in elevation along y.
std::string xz(const Point& point) {
    return fmt::format("({:.1f}, {:.1f})", point[0], point[2]);
}

/// The plan footprint of a rib outline at the datum: both loops' z-0 corners p1, p0.
Polyline footprint(const Outline& rib) {
    return Polyline({rib.top.get_point(0), rib.top.get_point(1), rib.bottom.get_point(1), rib.bottom.get_point(0)}).closed();
}

/// Both loops of a rib outline and the edges joining their facing vertices.
void rib_outline(Frame& frame, const Outline& rib, const Color& color, double width) {

    frame.polyline(up(rib.top), color, width);
    frame.polyline(up(rib.bottom), color, width);

    for (size_t i = 0; i + 1 < rib.top.point_count(); i++)
        frame.line(up(Line::from_points(rib.top.get_point(i), rib.bottom.get_point(i))), color, 1.0);
}

/// The end face of a rib on its beam: the last trace point and p1 on both loops.
Polyline end_face(const Outline& rib) {

    const size_t last = rib.top.point_count() - 2;

    return Polyline({rib.top.get_point(last), rib.top.get_point(0), rib.bottom.get_point(0), rib.bottom.get_point(last)}).closed();
}

/// A square level slice in the column frame at z, side mm from the corner.
Polyline slice(const ColumnCorner& column, double z, double side) {

    const Point corner(column.corner[0], column.corner[1], z);

    return Polyline({corner, corner + column.x_axis * side, corner + column.x_axis * side + column.y_axis * side, corner + column.y_axis * side}).closed();
}

// ═══════════════════════════════════════════════════════════════════════════
// Frames
// ═══════════════════════════════════════════════════════════════════════════

/// The seam planes grey, the two planes rib_seam_ends() picks green and the outer rib bands that end on them.
void rib_seam_ends_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const std::array<Plane, 2> ends = quarter.rib_seam_ends();
    const std::vector<Outline> outer = quarter.outer_ribs();
    Frame frame(CHAPTER, 69, "rib_seam_ends", fmt::format("rib_seam_ends(): seam_through_ribs = {}, so the outer ribs end on face 1 of the seam beams", guide.parameters.seam_through_ribs), "top", QUARTER);
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 1.0);

    for (size_t k = 0; k < 2; k++) {
        const Seam& seam = quarter.seam(k);
        const Point on_edge = line_plane(quarter.edge(k).line, ends[k]).value();
        const Point on_oculus = line_plane(guide.oculus_edges[0].line, ends[k]).value();
        frame.line(up(Line::from_points(seam.line.start(), seam.oculus_corner)), GREY, 3.0);
        frame.line(up(Line::from_points(on_edge, on_oculus)), FAMILY_COLORS[2], 4.0);
        frame.line(up(Line::from_points(on_oculus, on_oculus + ends[k].z_axis() * 250.0)), FAMILY_COLORS[2], 3.0, false, true);
        frame.polyline(up(footprint(outer[k])), FAMILY_COLORS[0], 3.0);
        frame.label(fmt::format("rib_seam_ends()[{}] = inner_beams[{}][1]", k, k == 0 ? 0 : 2), up(Line::from_points(on_edge, on_oculus).point_at(0.6)));
        frame.label(fmt::format("seams[{}]: inner_beams[{}][0]", seam.index, k == 0 ? 0 : 2), up(Line::from_points(seam.line.start(), seam.oculus_corner).point_at(0.35)));
        frame.label(fmt::format("outer_ribs()[{}]", k), up(area_centroid(footprint(outer[k]))));
    }

    frame.write(context.dir);
}

/// Outer rib 0 in elevation: the Bezier samples grey, the trace trimmed between the fan and the end plane, d1.
void trim_frame(const Context& context) {

    const Quarter quarter = context.guide.quarter(0);
    const RibCall call = rib_call(quarter, 0, false);
    const std::vector<Point> pts = trimmed(call);
    const Plane face = rib_plane(pts);
    const Point top = meet(call.cut_plane0, face, 0.0);
    const Point foot = call.cut_plane0.project(pts.back());
    const double d0 = distance_to(pts.front(), call.cut_plane0);
    const double d1 = distance_to(pts.back(), call.cut_plane0);
    Frame frame(CHAPTER, 70, "rib_trim", "rib(): trim() extends both end chords and cuts them on cut_plane0 and cut_plane1; pts runs fan to beam", "front", {-3100.0, -3050.0, H - 780.0, 100.0, -2850.0, H + 60.0});

    frame.polyline(up(call.trace), GREY, 2.0);

    for (const Point& point : call.trace.get_points())
        frame.point(up(point), GREY, 9.0);

    frame.line(up(Line::from_points(top, meet(call.cut_plane0, face, -780.0))), INK, 2.0, true);
    frame.line(up(Line::from_points(datum(pts.back()), meet(call.cut_plane1, face, -300.0))), INK, 2.0, true);
    frame.line(up(Line::from_points(call.trace.get_point(0), pts.front())), MARK, 3.0, true);

    for (size_t i = 0; i + 1 < pts.size(); i++)
        frame.line(up(Line::from_points(pts[i], pts[i + 1])), FAMILY_COLORS[0], 4.0, false, true);

    frame.line(up(Line::from_points(pts.back(), foot)), MARK, 2.0, true);
    frame.point(up(pts.front()), MARK);
    frame.point(up(pts.back()), MARK);

    frame.label(fmt::format("parabolas[0][0] start {}", xz(call.trace.get_point(0))), up(call.trace.get_point(0)));
    frame.label(fmt::format("pts[0] {}, d0 = {:.1e}", xz(pts.front()), d0), up(pts.front()));
    frame.label(fmt::format("pts[{}] {}", pts.size() - 1, xz(pts.back())), up(pts.back()));
    frame.label("cut_plane0 = wedges[0][0]", up(top));
    frame.label("cut_plane1 = rib_seam_ends()[0]", up(datum(pts.back())));
    frame.label(fmt::format("d1 = {:.2f}", d1), up(Line::from_points(pts.back(), foot).point_at(0.3)));
    frame.write(context.dir);
}

/// Outer rib 0 and inner rib 0 in plan at the column end: every trace point moved along the sweep onto face1.
void sweep_frame(const Context& context) {

    const Quarter quarter = context.guide.quarter(0);
    const QuarterGeometry& geometry = context.guide.geometry[0];
    const RibCall outer = rib_call(quarter, 0, false);
    const RibCall inner = rib_call(quarter, 0, true);
    const std::vector<Point> outer_pts = trimmed(outer);
    const std::vector<Point> inner_pts = trimmed(inner);
    const std::vector<Point> outer_far = swept(outer, outer_pts);
    const std::vector<Point> inner_far = swept(inner, inner_pts);
    const Outline outer_rib = quarter.outer_ribs()[0];
    const Outline inner_rib = quarter.inner_ribs()[0];
    Frame frame(CHAPTER, 71, "rib_sweep", "rib(): project_to_plane_by_axis(face1, sweep) moves pts onto face1: square for outer, along r for inner", "top", {-2850.0, -3050.0, H - 800.0, -2000.0, -2200.0, H + 50.0});

    frame.line(up(Line::from_points(outer_rib.top.get_point(1), outer_rib.top.get_point(0))), GREY, 3.0);
    frame.line(up(Line::from_points(inner_rib.top.get_point(1), inner_rib.top.get_point(0))), GREY, 3.0);
    frame.line(up(Line::from_points(outer_rib.bottom.get_point(1), outer_rib.bottom.get_point(0))), FAMILY_COLORS[0], 3.0);
    frame.line(up(Line::from_points(inner_rib.bottom.get_point(1), inner_rib.bottom.get_point(0))), FAMILY_COLORS[1], 3.0);

    for (size_t i = 0; i < 2; i++) {
        frame.point(up(outer_pts[i]), GREY);
        frame.point(up(inner_pts[i]), GREY);
        frame.line(up(Line::from_points(outer_pts[i], outer_far[i])), MARK, 3.0, false, true);
        frame.line(up(Line::from_points(inner_pts[i], inner_far[i])), MARK, 3.0, false, true);
    }

    const Point base = inner_rib.top.get_point(1) + (inner_rib.top.get_point(0) - inner_rib.top.get_point(1)).normalized() * 450.0;
    const Vector normal = geometry.planes.inner_ribs[0][1].z_axis();
    const Vector n = normal.dot(inner.sweep) < 0.0 ? -normal : normal;
    frame.line(up(Line::from_points(base, base + n * 180.0)), INK, 3.0, false, true);
    frame.line(up(Line::from_points(base, base + inner.sweep * 180.0)), MARK, 3.0, false, true);

    frame.label(fmt::format("outer: {:.2f} along outer_ribs[0][1].z_axis()", (outer_far[1] - outer_pts[1]).magnitude()), up(Line::from_points(outer_pts[1], outer_far[1]).center()));
    frame.label(fmt::format("inner: {:.2f} along rib_sweep", (inner_far[1] - inner_pts[1]).magnitude()), up(Line::from_points(inner_pts[1], inner_far[1]).center()));
    frame.label(fmt::format("n: normal of inner_ribs[0][1], {:.2f} deg from r", geometry.central_panel.obliqueness[0]), up(base + n * 180.0));
    frame.label("r = central_panel.rib_sweep", up(base + inner.sweep * 180.0));
    frame.label("face1 = outer_ribs[0][1]", up(Line::from_points(outer_rib.bottom.get_point(1), outer_rib.bottom.get_point(0)).point_at(0.05)));
    frame.label("face1 = inner_ribs[0][1]", up(Line::from_points(inner_rib.bottom.get_point(1), inner_rib.bottom.get_point(0)).point_at(0.06)));
    frame.write(context.dir);
}

/// Inner rib 0 at the column: the far trace's first facet run on to the fan plane wedges[1][0].
void recut_frame(const Context& context) {

    const Quarter quarter = context.guide.quarter(0);
    const RibCall call = rib_call(quarter, 0, true);
    const std::vector<Point> pts = trimmed(call);
    const std::vector<Point> far = swept(call, pts);
    const Point recut = line_plane(Line::from_points(far[0], far[1]), call.cut_plane0).value();
    const Point beyond = far[0] + (far[0] - far[1]).normalized() * 120.0;
    const Box box = {up(recut)[0] - 260.0, up(recut)[1] - 260.0, up(recut)[2] - 140.0, up(recut)[0] + 260.0, up(recut)[1] + 260.0, up(recut)[2] + 220.0};
    Frame frame(CHAPTER, 72, "rib_recut", fmt::format("rib(): far[0] = line_plane(far[0]far[1], cut_plane0); here it moves {:.1g} mm", (recut - far[0]).magnitude()), "iso", box);
    frame.plane_size = 140.0;
    frame.distance = 0.72;

    frame.plane(up(Plane::from_frame(recut, call.cut_plane0.x_axis(), call.cut_plane0.y_axis(), call.cut_plane0.z_axis())), GREY);
    frame.line(up(Line::from_points(pts[0], pts[1])), GREY, 3.0);
    frame.line(up(Line::from_points(pts[0], far[0])), GREY, 2.0, false, true);
    frame.line(up(Line::from_points(far[0], far[1])), FAMILY_COLORS[1], 4.0);
    frame.line(up(Line::from_points(far[0], beyond)), INK, 2.0, true);
    frame.point(up(pts[0]), GREY);
    frame.point(up(recut), MARK);

    frame.label("pts[0]", up(pts[0]));
    frame.label("far[0] = projected point = re-cut point", up(recut));
    frame.label("far[0] -> far[1]: first facet", up(Line::from_points(far[0], far[1]).point_at(0.25)));
    frame.label("cut_plane0 = wedges[1][0]", up(recut + call.cut_plane0.y_axis() * 110.0));
    frame.write(context.dir);
}

/// Outer rib 0's base face: span, rib_plane and p0 where the fan plane meets the datum on that face.
void p0_frame(const Context& context) {

    const Quarter quarter = context.guide.quarter(0);
    const RibCall call = rib_call(quarter, 0, false);
    const std::vector<Point> pts = trimmed(call);
    const Plane face = rib_plane(pts);
    const Point p0 = plane_plane_plane(call.cut_plane0, level(0.0), face).value();
    const Vector span(pts.back()[0] - pts.front()[0], pts.back()[1] - pts.front()[1], 0.0);
    const Point datum_end(pts.front()[0] - 300.0, pts.front()[1], 0.0);
    Frame frame(CHAPTER, 73, "rib_plane_p0", "rib_loop(): rib_plane holds the face; p0 = plane_plane_plane(cut_plane0, level(0.0), rib_plane)", "front", {-3100.0, -3050.0, H - 780.0, 100.0, -2850.0, H + 60.0});

    frame.polyline(up(Polyline(pts)), GREY, 3.0);
    frame.line(up(Line::from_points(datum_end, datum(pts.back()))), INK, 2.0, true);
    frame.line(up(Line::from_points(p0, meet(call.cut_plane0, face, -780.0))), GREY, 3.0);
    frame.line(up(Line::from_points(pts.front(), pts.front() + span)), INK, 3.0, false, true);
    frame.point(up(p0), MARK);
    frame.point(up(pts.front()), GREY);

    frame.label(fmt::format("p0 {}", xz(p0)), up(p0));
    frame.label(fmt::format("pts[0] {}", xz(pts.front())), up(pts.front()));
    frame.label(fmt::format("span = ({:.1f}, {:.1f}, 0)", span[0], span[1]), up(pts.front() + span));
    frame.label(fmt::format("rib_plane: normal span x z = ({:.0f}, {:.0f}, {:.0f})", face.z_axis()[0], face.z_axis()[1], face.z_axis()[2]), up(pts.front() + span * 0.5));
    frame.label("cut_plane0 = wedges[0][0]", up(Line::from_points(p0, pts.front()).center()));
    frame.label("level(0.0)", up(datum_end));
    frame.write(context.dir);
}

/// Outer rib 0's base face loop: p1, p0, the seven trace points and p1 again, every edge headed.
void loop_frame(const Context& context) {

    const Polyline loop = context.guide.quarter(0).outer_ribs()[0].top;
    const size_t n = loop.point_count();
    Frame frame(CHAPTER, 74, "rib_loop", fmt::format("rib_loop(): {} points {{p1, p0, pts[0..{}], p1}}; outer_ribs()[0].top, bottom the same on face1", n, n - 4), "front", {-3100.0, -3050.0, H - 780.0, 100.0, -2850.0, H + 60.0});
    frame.key = true;

    for (size_t i = 0; i + 1 < n; i++)
        frame.line(up(Line::from_points(loop.get_point(i), loop.get_point(i + 1))), FAMILY_COLORS[0], 4.0, false, true);

    frame.point(up(loop.get_point(0)), MARK);
    frame.point(up(loop.get_point(1)), MARK);

    frame.label(fmt::format("[0] = [{}] = p1 {}", n - 1, xz(loop.get_point(0))), up(loop.get_point(0)));
    frame.label(fmt::format("[1] = p0 {}", xz(loop.get_point(1))), up(loop.get_point(1)));
    frame.label(fmt::format("[2] = pts[0] {}: get_point(2)", xz(loop.get_point(2))), up(loop.get_point(2)));
    frame.label(fmt::format("[3] .. [{}] = pts[1] .. pts[{}]", n - 3, n - 5), up(loop.get_point(5)));
    frame.label(fmt::format("[{}] = pts[{}] {}", n - 2, n - 4, xz(loop.get_point(n - 2))), up(loop.get_point(n - 2)));
    frame.label("datum edge p1 -> p0", up(Line::from_points(loop.get_point(0), loop.get_point(1)).point_at(0.5)));
    frame.label("up the end plane to p1", up(Line::from_points(loop.get_point(n - 2), loop.get_point(n - 1)).center()));
    frame.write(context.dir);
}

/// Both outer rib outlines of quarter 0 with their depths at the fan and at the seam beam.
void outer_ribs_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    const std::vector<Outline> outer = guide.quarter(0).outer_ribs();
    const size_t last = outer[0].top.point_count() - 2;
    Frame frame(CHAPTER, 75, "outer_rib_outlines", fmt::format("outer_ribs(): two {:.0f} mm ribs along the bay edges, side fan planes to the seam beams' far faces", guide.parameters.outer_ribs), "iso", QUARTER);
    frame.key = true;
    frame.distance = 0.72;
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 1.0);
    frame.polyline(up(Polyline(guide.columns[0].head).closed()), INK, 3.0);

    for (const Outline& rib : outer)
        rib_outline(frame, rib, FAMILY_COLORS[0], 3.0);

    frame.label("outer_ribs()[0]", up(middle(outer[0])));
    frame.label("outer_ribs()[1]", up(middle(outer[1])));
    frame.label(fmt::format("{:.1f} on wedges[0][0]", outer[0].top.get_point(2)[2]), up(outer[0].top.get_point(2)));
    frame.label(fmt::format("{:.1f} on rib_seam_ends()[0]", outer[0].top.get_point(last)[2]), up(outer[0].top.get_point(last)));
    frame.label(fmt::format("{:.1f} on rib_seam_ends()[1]", outer[1].bottom.get_point(last)[2]), up(outer[1].bottom.get_point(last)));
    frame.label("columns[0].head", up(guide.columns[0].head[2]));
    frame.write(context.dir);
}

/// The inner ribs in plan from the chamfer fan plane to the oculus beam's back face, swept along r.
void inner_ribs_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const std::vector<Outline> inner = quarter.inner_ribs();
    const Plane& back = guide.geometry[0].planes.inner_beams[1][1];
    const Point back_a = line_plane(quarter.seam(0).line, back).value();
    const Point back_b = line_plane(quarter.seam(1).line, back).value();
    Frame frame(CHAPTER, 76, "inner_rib_outlines", "inner_ribs(): the shadow parabolas trimmed by wedges[1][0] and inner_beams[1][1], swept along rib_sweep", "top", QUARTER);
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 1.0);
    frame.line(up(Line::from_points(back_a, back_b)), FAMILY_COLORS[2], 3.0);

    for (const Outline& rib : quarter.outer_ribs())
        frame.polyline(up(footprint(rib)), GREY, 2.0);

    for (const Outline& rib : inner) {
        frame.polyline(up(footprint(rib)), FAMILY_COLORS[1], 3.0);
        frame.line(up(Line::from_points(rib.top.get_point(1), rib.bottom.get_point(1))), MARK, 3.0, false, true);
        frame.line(up(Line::from_points(rib.top.get_point(0), rib.bottom.get_point(0))), MARK, 3.0, false, true);
    }

    frame.label("inner_ribs()[0]", up(middle(inner[0])));
    frame.label("inner_ribs()[1]", up(middle(inner[1])));
    frame.label(fmt::format("p0 {:.0f}, {:.0f} on wedges[1][0]", inner[0].top.get_point(1)[0], inner[0].top.get_point(1)[1]), up(inner[0].top.get_point(1)));
    frame.label(fmt::format("p1 {:.1f}, {:.1f}", inner[0].top.get_point(0)[0], inner[0].top.get_point(0)[1]), up(inner[0].top.get_point(0)));
    frame.label(fmt::format("inner_beams[1][1]: x + y = {:.1f}", back.origin()[0] + back.origin()[1]), up(Line::from_points(back_a, back_b).center()));
    frame.label(fmt::format("outline_thickness(inner_ribs()[1]) = {:.2f} along rib_sweep", outline_thickness(inner[1])), up(inner[1].bottom.get_point(0)));
    frame.write(context.dir);
}

/// The spread of the eight rib face bottoms at the column head, as rib_level_spread reads them.
double bottom_spread(const std::vector<Outline>& outer, const std::vector<Outline>& inner) {

    double highest = -1e300;
    double lowest = 1e300;

    for (const std::vector<Outline>* family : {&outer, &inner})
        for (const Outline& rib : *family)
            for (const double z : {rib.top.get_point(2)[2], rib.bottom.get_point(2)[2]}) {
                highest = std::max(highest, z);
                lowest = std::min(lowest, z);
            }

    return highest - lowest;
}

/// Column 0's three cutter levels with the four outer rib face bottoms on levels[1].
void levels_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const Quarter quarter = guide.quarter(0);
    const std::vector<Outline> outer = quarter.outer_ribs();
    const double side = 400.0;
    const std::array<Polyline, 3> slices = {slice(column, column.levels[0], side), slice(column, column.levels[1], side), slice(column, column.levels[2], side)};
    const double clearance = std::min(outer[0].top.get_point(2)[2], outer[0].bottom.get_point(2)[2]) - column.levels[1];
    Frame frame(CHAPTER, 77, "rib_bottom_level", "rib_bottom_level(): the deepest get_point(2) of both outer ribs becomes columns[q].levels[1]", "iso", FAN);
    frame.polyline(up(Polyline(column.head).closed()), INK, 3.0);

    for (const Outline& rib : outer) {
        frame.polyline(up(rib.top), GREY, 1.5);
        frame.polyline(up(rib.bottom), GREY, 1.5);
        frame.point(up(rib.top.get_point(2)), MARK);
        frame.point(up(rib.bottom.get_point(2)), MARK);
    }

    for (size_t i = 0; i < 3; i++)
        frame.polyline(up(slices[i]), i == 1 ? MARK : INK, i == 1 ? 3.0 : 1.5);

    frame.label(fmt::format("levels[0] = {:.2f}", column.levels[0]), up(slices[0].get_point(2)));
    frame.label(fmt::format("levels[1] = rib_bottom_level = {:.2f}", column.levels[1]), up(slices[1].get_point(3)));
    frame.label(fmt::format("levels[2] = -column_head_depth = {:.2f}", column.levels[2]), up(slices[2].get_point(1)));

    for (size_t k = 0; k < 2; k++)
        frame.label(fmt::format("outer_ribs()[{}]: top and bottom get_point(2)", k), up(Line::from_points(outer[k].top.get_point(2), outer[k].bottom.get_point(2)).center()));

    frame.label(fmt::format("spread {:.3f}, clearance {:.3f}", bottom_spread(outer, quarter.inner_ribs()), clearance), up(Point(column.axis_point[0], column.axis_point[1], column.levels[1])));
    frame.write(context.dir);
}

/// The seam end of outer rib 0 close up: -static_h, the end plane x = -60 and the hit on the last chord.
void end_level_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const RibCall call = rib_call(quarter, 0, false);
    const Outline rib = quarter.outer_ribs()[0];
    const size_t last = rib.top.point_count() - 2;
    const Point end = rib.top.get_point(last);
    const Point vertex = call.trace.get_point(call.trace.point_count() - 1);
    const Point before = call.trace.get_point(call.trace.point_count() - 2);
    const double static_h = guide.parameters.static_h();
    const double level_value = end_level(rib, quarter.rib_seam_ends()[0]);
    const Point left(-500.0, vertex[1], -static_h);
    Frame frame(CHAPTER, 78, "end_level_outer", "soffit starts at -static_h; end_level() finds outer rib 0's lowest corner on rib_seam_ends()[0]", "front", {-500.0, -3050.0, H - 260.0, 100.0, -2850.0, H + 20.0});

    frame.line(up(Line::from_points(before, vertex)), GREY, 3.0);
    frame.point(up(vertex), GREY);
    frame.line(up(Line::from_points(left, Point(100.0, vertex[1], -static_h))), INK, 2.0, true);
    frame.line(up(Line::from_points(datum(end), Point(end[0], end[1], -260.0))), FAMILY_COLORS[2], 3.0);
    frame.line(up(Line::from_points(rib.top.get_point(last - 1), end)), FAMILY_COLORS[0], 4.0);
    frame.point(up(end), MARK, 14.0);

    frame.label(fmt::format("{} parabola end vertex", xz(vertex)), up(vertex));
    frame.label(fmt::format("{} pts[5]", xz(before)), up(before));
    frame.label(fmt::format("end_level = {:.4f}", level_value), up(end));
    frame.label(fmt::format("soffit = -static_h = {:.0f}", -static_h), up(left));
    frame.label(fmt::format("x = {:.0f}: rib_seam_ends()[0] = inner_beams[0][1]", end[0]), up(datum(end)));
    frame.write(context.dir);
}

/// The four rib end faces on their beams with their lowest corners, and the soffit level they set.
void soffit_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const std::vector<Outline> outer = quarter.outer_ribs();
    const std::vector<Outline> inner = quarter.inner_ribs();
    const std::array<Plane, 2> ends = quarter.rib_seam_ends();
    const Plane& back = guide.geometry[0].planes.inner_beams[1][1];
    const Polyline level_loop = Polyline(guide.geometry[0].polygon).closed().transformed(Xform::translation(0.0, 0.0, guide.soffit));
    const size_t last = outer[0].top.point_count() - 2;
    Frame frame(CHAPTER, 79, "soffit", fmt::format("soffit = min(-static_h, end_level of all 16 rib ends) = {:.4f}: inner and ring beams go this deep", guide.soffit), "iso", {-3100.0, -3100.0, H - 300.0, 100.0, 100.0, H + 20.0});
    frame.key = true;
    frame.distance = 0.75;
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 1.0);
    frame.polyline(up(level_loop), MARK, 2.0);

    for (size_t k = 0; k < 2; k++) {
        frame.polyline(up(outer[k].top), GREY, 1.0);
        frame.polyline(up(inner[k].top), GREY, 1.0);
        frame.polyline(up(end_face(outer[k])), FAMILY_COLORS[0], 4.0);
        frame.polyline(up(end_face(inner[k])), FAMILY_COLORS[1], 4.0);
        frame.point(up(outer[k].top.get_point(last)), MARK);
        frame.point(up(inner[k].bottom.get_point(last)), MARK);
        frame.label(fmt::format("outer_ribs()[{}]: end_level {:.4f}", k, end_level(outer[k], ends[k])), up(outer[k].top.get_point(last)));
        frame.label(fmt::format("inner_ribs()[{}]: end_level {:.4f}", k, end_level(inner[k], back)), up(inner[k].bottom.get_point(last)));
    }

    frame.label(fmt::format("soffit = {:.2f}", guide.soffit), up(Line::from_points(level_loop.get_point(2), level_loop.get_point(3)).center()));
    frame.write(context.dir);
}

/// The plan group the guide draws per quarter: polygon_q, column_head_q and oculus_corner_q in black.
void plan_groups_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 80, "draw_plan", "draw(): quarter_q > plan_q holds polygon_q, column_head_q (black, width 3) and oculus_corner_q", "top", BAY);

    for (size_t q = 0; q < 4; q++) {
        const Polyline polygon = Polyline(guide.geometry[q].polygon).closed();
        frame.polyline(up(polygon), INK, 3.0);
        frame.polyline(up(Polyline(guide.columns[q].head).closed()), INK, 3.0);
        frame.point(up(guide.oculus_corners[q]), q == 0 ? MARK : INK, 10.0);
        frame.label(fmt::format("quarter_{} > plan_{} > polygon_{}", q, q, q), up(area_centroid(polygon)), true);
    }

    frame.label("column_head_0", up(guide.columns[0].head[2]));
    frame.label("oculus_corner_0", up(guide.oculus_corners[0]));
    frame.write(context.dir);
}

/// One drawn family of quarter 0: its quads and face planes in the family colour, a rib's three parabolas too, from index parabola on as DrawnFamily::parabola gives it, -1 for none.
void drawn_family(Frame& frame, Family family, const std::vector<Polyline>& quads, const std::vector<std::array<Plane, 2>>& planes, const std::vector<std::array<Polyline, 3>>& parabolas, int parabola) {

    const Color& color = FAMILY_COLORS[static_cast<size_t>(family)];

    for (size_t i = 0; i < quads.size(); i++) {
        frame.polyline(up(quads[i].closed()), color, 2.0);
        frame.plane(up(planes[i][0]), color);
        frame.plane(up(planes[i][1]), color);

        if (parabola < 0)
            continue;

        for (size_t layer = 0; layer < 3; layer++)
            frame.polyline(up(parabolas[static_cast<size_t>(parabola) + i][layer]), color, layer == 0 ? 2.0 : 1.0);
    }
}

/// Everything draw() puts under quarter_0, as example 1 takes it out with get_branch("quarter_0").
void families_frame(const Context& context) {

    const FloorGuide& guide = context.guide;
    const QuarterGeometry& geometry = guide.geometry[0];
    const WoodSession drawn = guide.get_branch("quarter_0");
    Frame frame(CHAPTER, 81, "draw_families", fmt::format("Example 1: get_branch(\"quarter_0\") holds {} objects in {} groups, no member built", drawn.lookup.size(), drawn.tree.root()->children().size()), "iso", QUARTER);
    frame.distance = 0.72;
    frame.plane_size = 60.0;
    frame.polyline(up(Polyline(geometry.polygon).closed()), INK, 3.0);
    frame.polyline(up(Polyline(guide.columns[0].head).closed()), INK, 3.0);

    drawn_family(frame, Family::outer_ribs, geometry.quads.outer_ribs, geometry.planes.outer_ribs, geometry.parabolas, 0);
    drawn_family(frame, Family::inner_ribs, geometry.quads.inner_ribs, geometry.planes.inner_ribs, geometry.parabolas, 2);
    drawn_family(frame, Family::inner_beams, geometry.quads.inner_beams, geometry.planes.inner_beams, geometry.parabolas, -1);
    drawn_family(frame, Family::wedges, geometry.quads.wedges, geometry.planes.wedges, geometry.parabolas, -1);
    drawn_family(frame, Family::tsections, geometry.quads.tsections, geometry.planes.tsections, geometry.parabolas, -1);

    frame.label("outer_ribs_0_0: quad, face_0, face_1, soffit, ...", up(area_centroid(geometry.quads.outer_ribs[0].closed())));
    frame.label("inner_ribs_0_0: soffit", up(geometry.parabolas[2][0].get_point(3)));
    frame.label("inner_beams_1_0", up(area_centroid(geometry.quads.inner_beams[1].closed())));
    frame.label("wedges_1_0", up(area_centroid(geometry.quads.wedges[1].closed())));
    frame.label("tsections_2_0", up(area_centroid(geometry.quads.tsections[2].closed())));
    frame.label("beds_top of outer_ribs_1_0", up(geometry.parabolas[1][2].get_point(3)));
    frame.label("plan_0", up(geometry.polygon[2]));
    frame.write(context.dir);
}

}

void chapter_05_rib_outlines(const Context& context) {
    rib_seam_ends_frame(context);
    trim_frame(context);
    sweep_frame(context);
    recut_frame(context);
    p0_frame(context);
    loop_frame(context);
    outer_ribs_frame(context);
    inner_ribs_frame(context);
    levels_frame(context);
    end_level_frame(context);
    soffit_frame(context);
    plan_groups_frame(context);
    families_frame(context);
}

}
