#include "docs/floor/movie.h"

namespace movie {

namespace {

const std::string CHAPTER = "06_outlines";

// ═══════════════════════════════════════════════════════════════════════════
// Drawing helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The colour of a quarter family.
const Color& colour(Family family) {
    return FAMILY_COLORS[static_cast<size_t>(family)];
}

/// The plane re-origined at the foot of a point on it, its axes kept, so its square is drawn there.
Plane at(const Plane& plane, const Point& near) {
    return Plane::from_frame(plane.project(near), plane.x_axis(), plane.y_axis(), plane.z_axis());
}

/// A face's datum trace from where it meets one plane to where it meets another.
Line trace(const Plane& face, const Plane& from, const Plane& to) {
    const Plane datum = level(0.0);
    return Line::from_points(plane_plane_plane(datum, face, from).value(), plane_plane_plane(datum, face, to).value());
}

/// Where a face crosses a section plane, from level z0 to level z1.
Line cross_trace(const Plane& face, const Plane& section, double z0, double z1) {
    return Line::from_points(plane_plane_plane(level(z0), section, face).value(), plane_plane_plane(level(z1), section, face).value());
}

/// A plane's trace at level z under a plan edge: where the horizontals square to the edge from its two ends meet the plane.
Line trace_under(const Plane& plane, const Line& edge, double z) {
    const Vector across = edge.to_direction().cross(Vector::z_axis());
    const Point a(edge.start()[0], edge.start()[1], z);
    const Point b(edge.end()[0], edge.end()[1], z);
    return Line::from_points(line_plane(Line::from_points(a, a + across), plane).value(), line_plane(Line::from_points(b, b + across), plane).value());
}

/// A polyline drawn dashed, segment by segment.
void dashed(Frame& frame, const Polyline& polyline, const Color& color) {
    for (const Line& line : polyline.get_lines())
        frame.line(line, color, 1.5, true);
}

/// An outline's two loops, lifted, and the thin lines joining their facing corners.
void loops(Frame& frame, const Outline& outline, const Color& top, const Color& bottom, double width) {
    frame.polyline(up(outline.top), top, width);
    frame.polyline(up(outline.bottom), bottom, width);
    const std::vector<Point> a = open_points(outline.top);
    const std::vector<Point> b = open_points(outline.bottom);

    for (size_t i = 0; i < std::min(a.size(), b.size()); i++)
        frame.line(up(Line::from_points(a[i], b[i])), INK, 1.0);
}

/// Quarter 0's placed members of a family, in one colour.
void family_members(Frame& frame, const Context& context, Family family, const Color& color) {
    for (const Member& member : members(context.members.members.quarters[0], family))
        frame.element(member.element, color);
}

/// One placed member of quarter 0's family.
void one_member(Frame& frame, const Context& context, Family family, size_t index, const Color& color) {
    frame.element(members(context.members.members.quarters[0], family)[index].element, color);
}

/// An outline as a plate lifted to the floor, as add_oculus_model and column_cuts place one.
std::shared_ptr<Element> placed_plate(const Outline& outline, const std::string& name) {
    const std::shared_ptr<Element> element = to_plate(outline, name);
    element->place(LIFT);
    return element;
}

/// The scene name of bed plate index of row in quarter 0.
std::string bed_name(size_t row, size_t index) {
    MemberRef ref = quarter_member(0, Family::beds, index);
    ref.row = static_cast<int>(row);
    return ref.name();
}

/// The column cutter construction of quarter 0 up to its six quads, as Quarter::column_cutters builds it.
struct CutterCorners {
    std::vector<Plane> fan_top; // side0, the three fan planes, side1.
    std::vector<Plane> fan_bottom; // side0, the two shaft faces, side1.
    std::vector<Point> p0; // fan_top creases at levels[0].
    std::vector<Point> p1; // fan_top creases at levels[1].
    std::vector<Point> p2; // fan_bottom creases at levels[2].
    Vector quarter; // (p2[2] - p2[0]) / 4.
    std::vector<std::vector<Point>> quads; // The six quads before stretch.
};

/// The planes, crease points and quads of quarter 0's column cutters before they are stretched.
CutterCorners cutter_corners(const FloorGuide& guide) {
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const ColumnCorner& corner = guide.columns[0];
    const Vector down(0.0, 0.0, -1.0);
    const std::array<Plane, 3> xy = {level(corner.levels[0]), level(corner.levels[1]), level(corner.levels[2])};
    CutterCorners result;
    result.fan_top = {corner.sides[0], cp.wedges[0][0], cp.wedges[1][0], cp.wedges[2][0], corner.sides[1]};
    result.fan_bottom = {corner.sides[0], edge_plane(wood_floor::geometry::edge(corner.head, 1), down), edge_plane(wood_floor::geometry::edge(corner.head, 3), down), corner.sides[1]};

    for (size_t i = 0; i + 1 < result.fan_top.size(); i++) {
        result.p0.push_back(plane_plane_plane(xy[0], result.fan_top[i], result.fan_top[i + 1]).value());
        result.p1.push_back(plane_plane_plane(xy[1], result.fan_top[i], result.fan_top[i + 1]).value());
    }

    for (size_t i = 0; i + 1 < result.fan_bottom.size(); i++)
        result.p2.push_back(plane_plane_plane(xy[2], result.fan_bottom[i], result.fan_bottom[i + 1]).value());

    const std::vector<Point>& p0 = result.p0;
    const std::vector<Point>& p1 = result.p1;
    const std::vector<Point>& p2 = result.p2;
    result.quarter = (p2[2] - p2[0]) * 0.25;
    result.quads = {
        {p0[0], p0[1], p1[1], p1[0]},
        {p0[1], p0[2], p1[2], p1[1]},
        {p0[2], p0[3], p1[3], p1[2]},
        {p1[0], p1[1], p2[1], p2[0]},
        {p1[1], p1[2], p2[1] + result.quarter, p2[1] - result.quarter},
        {p1[2], p1[3], p2[2], p2[1]},
    };

    return result;
}

/// A cutter quad after the first half of stretch: edges 0-1 and 2-3 lengthened by margin at both ends.
std::vector<Point> lengthened(std::vector<Point> quad, double margin) {
    const Vector d0 = (quad[1] - quad[0]).normalized() * margin;
    const Vector d1 = (quad[3] - quad[2]).normalized() * margin;
    quad[0] = quad[0] - d0;
    quad[1] = quad[1] + d0;
    quad[2] = quad[2] - d1;
    quad[3] = quad[3] + d1;
    return quad;
}

/// The overshoot and thickness of the cutters, read off a finished cutter: the distance between its two loops.
double cutter_margin(const FloorGuide& guide) {
    return outline_thickness(guide.quarter(0).column_cutters()[0]);
}

// ═══════════════════════════════════════════════════════════════════════════
// Inner beams
// ═══════════════════════════════════════════════════════════════════════════

/// loft_planes on seam beam 0: the four side planes in ring order, the bottom and top planes, the two loops and their corners.
void show_loft_planes(const Context& context) {
    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const Outline beam = guide.quarter(0).inner_beams()[0];
    const std::vector<Plane> planes = {cp.outer_ribs[0][0], level(0.0), cp.inner_beams[1][0], level(guide.soffit)};
    const std::array<std::string, 4> names = {"outer_ribs[0][0]", "side0", "inner_beams[1][0]", "side1"};
    const std::vector<Point> top = open_points(beam.top);
    const std::vector<Point> bottom = open_points(beam.bottom);
    Frame frame(CHAPTER, 82, "loft_planes", "loft_planes: corner i of both loops is planes[i] x planes[i + 1], on the bottom plane and on the top", "iso", {-500.0, -3150.0, H - 350.0, 400.0, -850.0, H + 150.0});
    frame.plane_size = 150.0;

    for (size_t i = 0; i < 4; i++) {
        const size_t before = (i + 3) % 4;
        const Point face = Point::centroid({top[before], top[i], bottom[before], bottom[i]});
        frame.plane(up(at(planes[i], face)), INK);
        frame.label(fmt::format("planes[{}] = {}", i, names[i]), up(face));
    }

    const Point near_bottom = Point::lerp(bottom[0], bottom[1], 0.75) + Vector(0.0, 0.0, guide.soffit * 0.5);
    const Point near_top = Point::lerp(top[0], top[1], 0.25) + Vector(0.0, 0.0, guide.soffit * 0.5);
    frame.plane(up(at(cp.inner_beams[0][0], near_bottom)), colour(Family::inner_beams));
    frame.plane(up(at(cp.inner_beams[0][1], near_top)), colour(Family::inner_beams));
    loops(frame, beam, colour(Family::inner_beams), colour(Family::inner_beams), 3.0);

    for (size_t i = 0; i < 4; i++) {
        frame.point(up(top[i]), MARK, 10.0);
        frame.point(up(bottom[i]), MARK, 10.0);
    }

    frame.label("bottom = inner_beams[0][0]", up(cp.inner_beams[0][0].project(near_bottom)));
    frame.label("top = inner_beams[0][1]", up(cp.inner_beams[0][1].project(near_top)));
    frame.label("bottom loop corner 0: planes[0] x planes[1] x bottom", up(bottom[0]));
    frame.label("top loop corner 2: planes[2] x planes[3] x top", up(top[2]));
    frame.write(context.dir);
}

/// The section along seam 0 at x 0: the beam top and soffit levels, and the two outer rib faces that face picks between.
void show_beam_levels(const Context& context) {
    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const Plane& seam = cp.inner_beams[0][0];
    const Plane outside = cp.outer_ribs[0][0].translate_by_normal(-100.0);
    const Line side0 = Line::from_points(plane_plane_plane(level(0.0), seam, outside).value(), plane_plane_plane(level(0.0), seam, cp.inner_beams[1][0]).value());
    const Line side1 = Line::from_points(plane_plane_plane(level(guide.soffit), seam, outside).value(), plane_plane_plane(level(guide.soffit), seam, cp.inner_beams[1][0]).value());
    const Line face0 = cross_trace(cp.outer_ribs[0][0], seam, 60.0, guide.soffit - 60.0);
    const Line face1 = cross_trace(cp.outer_ribs[0][1], seam, 60.0, guide.soffit - 60.0);
    Frame frame(CHAPTER, 83, "beam_levels", "inner_beams: side0 = level(0.0), side1 = level(soffit); face = 0 runs the seam beams to the bay edge", "right", {-100.0, -3200.0, H - 300.0, 100.0, -800.0, H + 100.0});

    frame.line(up(side0), colour(Family::inner_beams), 4.0);
    frame.line(up(side1), colour(Family::inner_beams), 4.0);
    frame.line(up(face0), MARK, 4.0);
    frame.line(up(face1), INK, 2.0, true);
    frame.line(up(cross_trace(cp.inner_beams[1][0], seam, 0.0, guide.soffit)), GREY, 2.0);

    frame.label("side0 = level(0.0)", up(side0.point_at(0.6)));
    frame.label(fmt::format("side1 = level(guide.soffit), z {:.2f}", guide.soffit), up(side1.point_at(0.4)));
    frame.label(fmt::format("face 0 = outer_ribs[0][0], y {:.0f}: seam_through_ribs", cp.outer_ribs[0][0].origin()[1]), up(face0.start()));
    frame.label(fmt::format("face 1 = outer_ribs[0][1], y {:.0f}: tied variant", cp.outer_ribs[0][1].origin()[1]), up(face1.end()));
    frame.write(context.dir);
}

/// Seam beam 0 seen on the seam plane: its bottom loop's four corners named by the planes that make them.
void show_seam_beam_0(const Context& context) {
    const FloorGuide& guide = context.guide;
    const Outline beam = guide.quarter(0).inner_beams()[0];
    const std::vector<Point> bottom = open_points(beam.bottom);
    const std::array<std::string, 4> pairs = {"outer_ribs[0][face] x side0", "side0 x inner_beams[1][0]", "inner_beams[1][0] x side1", "side1 x outer_ribs[0][face]"};
    Frame frame(CHAPTER, 84, "seam_beam_0", "inner_beams()[0]: seam beam 0, its bottom loop on the seam plane x 0, its oculus end on the tilted plane", "right", {-100.0, -3150.0, H - 260.0, 100.0, -850.0, H + 60.0});

    frame.polyline(up(beam.top), GREY, 1.5);
    frame.polyline(up(beam.bottom), colour(Family::inner_beams), 4.0);

    for (size_t i = 0; i < bottom.size(); i++) {
        frame.point(up(bottom[i]), MARK);
        frame.label(fmt::format("bottom[{}] = {}", i, pairs[i]), up(bottom[i]));
    }

    frame.label(fmt::format("{:.1f} at the datum", Point::distance(bottom[0], bottom[1])), up(Point::mid_point(bottom[0], bottom[1])));
    frame.label(fmt::format("{:.1f} at the soffit", Point::distance(bottom[2], bottom[3])), up(Point::mid_point(bottom[2], bottom[3])));
    frame.write(context.dir);
}

/// The oculus beam between the two seam beams' far faces: its bottom loop on the tilted plane, its top loop on the back face, and the lean of tilted.
void show_oculus_beam(const Context& context) {
    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const std::vector<Outline> beams = quarter.inner_beams();
    const OculusEdge& edge = guide.oculus_edges[0];
    const Point centre = edge.line.center();
    const Point drop(centre[0], centre[1], guide.soffit);
    const Point foot = line_plane(Line::from_points(drop, drop + edge.back.z_axis()), edge.tilted).value();
    const std::vector<Point> top = open_points(beams[1].top);
    const std::vector<Point> bottom = open_points(beams[1].bottom);
    Frame frame(CHAPTER, 85, "oculus_beam", "inner_beams()[1]: the oculus beam between the seam beams' far faces, from tilted to back", "iso", {-1250.0, -1250.0, H - 260.0, 100.0, 100.0, H + 60.0});

    one_member(frame, context, Family::inner_beams, 0, GREY);
    one_member(frame, context, Family::inner_beams, 2, GREY);
    loops(frame, beams[1], INK, colour(Family::inner_beams), 3.0);
    frame.line(up(Line::from_points(centre, drop)), INK, 1.5, true);
    frame.line(up(Line::from_points(drop, foot)), MARK, 4.0);

    frame.label("bottom loop on oculus_edges[0].tilted", up(bottom[2]));
    frame.label("top loop on oculus_edges[0].back", up(top[1]));
    frame.label(fmt::format("lean {:.1f} = -soffit tan(oculus_plane_angle)", Point::distance(drop, foot)), up(Point::mid_point(drop, foot)));
    frame.label("inner_beams_0_0: ring planes[0] = its far face", up(Point::lerp(open_points(beams[0].top)[0], open_points(beams[0].top)[1], 0.9)));
    frame.label("inner_beams_2_0: ring planes[2] = its far face", up(Point::lerp(open_points(beams[2].top)[0], open_points(beams[2].top)[1], 0.9)));
    frame.write(context.dir);
}

/// The three inner beams of quarter 0 in plan, the outer ribs ending on the seam beams' far faces.
void show_seam_beam_2(const Context& context) {
    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const ConstructionPlanes& cp = quarter.geometry().planes;
    const std::vector<Outline> beams = quarter.inner_beams();
    const std::vector<Outline> ribs = quarter.outer_ribs();
    Frame frame(CHAPTER, 86, "seam_beam_2", "inner_beams()[2] mirrors seam beam 0 on seam 3; the three inner beams of quarter 0", "top", QUARTER);
    frame.key = true;

    family_members(frame, context, Family::outer_ribs, GREY);
    family_members(frame, context, Family::inner_beams, colour(Family::inner_beams));

    for (size_t i = 0; i < beams.size(); i++)
        frame.label(member_name(Family::inner_beams, i, 0), up(middle(beams[i])));

    frame.label(fmt::format("outer_ribs_0_0 ends on inner_beams[0][1], x {:.0f}", cp.inner_beams[0][1].origin()[0]), up(ribs[0].top.get_point(0)));
    frame.label(fmt::format("outer_ribs_1_0 ends on inner_beams[2][1], y {:.0f}", cp.inner_beams[2][1].origin()[1]), up(ribs[1].top.get_point(0)));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Wedges
// ═══════════════════════════════════════════════════════════════════════════

/// The two rib faces that bound each wedge block, as datum traces past the far face, and the bed top plane each block stands on.
void show_wedge_bounds(const Context& context) {
    const FloorGuide& guide = context.guide;
    const QuarterGeometry& geometry = guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const std::array<std::array<Plane, 2>, 3> ribs = {{
        {cp.outer_ribs[0][1], cp.inner_ribs[0][0]},
        {cp.inner_ribs[0][1], cp.inner_ribs[1][1]},
        {cp.inner_ribs[1][0], cp.outer_ribs[1][1]},
    }};
    const std::array<std::string, 3> names = {"outer_ribs[0][1], inner_ribs[0][0]", "inner_ribs[0][1], inner_ribs[1][1]", "inner_ribs[1][0], outer_ribs[1][1]"};
    Frame frame(CHAPTER, 87, "wedge_bounds", "wedges: ribs[i] holds the two rib faces of block i, beds[i] = bed_top_planes[i] its underside", "top", {-2980.0, -2980.0, H - 700.0, -2280.0, -2280.0, H + 20.0});
    frame.plane_size = 80.0;

    family_members(frame, context, Family::outer_ribs, GREY);
    family_members(frame, context, Family::inner_ribs, GREY);

    for (size_t i = 0; i < 3; i++) {
        const Plane beyond = cp.wedges[i][1].translate_by_normal(150.0);
        const Line face0 = trace(ribs[i][0], cp.wedges[i][0], beyond);
        frame.line(up(face0), colour(Family::wedges), 4.0);
        frame.line(up(trace(ribs[i][1], cp.wedges[i][0], beyond)), colour(Family::wedges), 4.0);
        frame.plane(up(geometry.bed_top_planes[i]), colour(Family::beds));
        frame.label(fmt::format("ribs[{}]: {}", i, names[i]), up(face0.point_at(0.5)));
        frame.label(fmt::format("beds[{}] = bed_top_planes[{}]", i, i), up(geometry.bed_top_planes[i].origin()));
    }

    frame.write(context.dir);
}

/// The three wedge blocks lofted from the fan plane to the far face, each on its bed plane.
void show_wedge_blocks(const Context& context) {
    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const ConstructionPlanes& cp = quarter.geometry().planes;
    const std::vector<Outline> wedges = quarter.wedges();
    Frame frame(CHAPTER, 88, "wedge_blocks", "wedges(): loft_planes({ribs[i][0], beds[i], ribs[i][1], top}) from the fan plane to the far face", "iso", {-3100.0, -3100.0, H - 760.0, -2250.0, -2250.0, H + 40.0});
    frame.distance = 0.75;

    family_members(frame, context, Family::outer_ribs, GREY);
    family_members(frame, context, Family::inner_ribs, GREY);
    family_members(frame, context, Family::wedges, colour(Family::wedges));

    for (size_t i = 0; i < wedges.size(); i++) {
        const double thickness = signed_distance(cp.wedges[i][1].origin(), cp.wedges[i][0]);
        frame.label(fmt::format("{}: {:.0f} along the fan normal", member_name(Family::wedges, i, 0), thickness), up(middle(wedges[i])));
    }

    frame.label("bottom loop on cp.wedges[0][0], the fan plane", up(open_points(wedges[0].bottom)[2]));
    frame.label("top loop on cp.wedges[0][1], the far face", up(open_points(wedges[0].top)[3]));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// T-sections
// ═══════════════════════════════════════════════════════════════════════════

/// T-section 0 on its first face y -2900: the soffit and +t traces, untrimmed dashed, trimmed between the beam face and the fan plane.
void show_tsection_trim(const Context& context) {
    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const QuarterGeometry& geometry = quarter.geometry();
    const ConstructionPlanes& cp = geometry.planes;
    const Plane& face = cp.tsections[0][0];
    const Xform projection = Xform::project_to_plane_by_axis(face, cp.outer_ribs[0][0].z_axis());
    const Polyline soffit = geometry.parabolas[0][0].transformed(projection);
    const Polyline layer = geometry.parabolas[0][1].transformed(projection);
    const Polyline cut00 = trim(soffit, cp.inner_beams[0][1], cp.wedges[0][0]);
    const Polyline cut10 = trim(layer, cp.inner_beams[0][1], cp.wedges[0][0]);
    const Line beam = cross_trace(cp.inner_beams[0][1], face, 60.0, guide.soffit - 80.0);
    const Line fan = cross_trace(cp.wedges[0][0], face, 60.0, -780.0);
    const Line gap = Line::from_points(cut00.get_point(cut00.point_count() - 1), cut10.get_point(cut10.point_count() - 1));
    Frame frame(CHAPTER, 89, "tsection_trim", "tsection: the soffit and + t traces on face 0, each trimmed between cut_plane0 and cut_plane1", "front", {-2850.0, -2960.0, H - 780.0, 60.0, -2840.0, H + 80.0});

    frame.polyline(up(quarter.outer_ribs()[0].bottom), GREY, 1.5);
    dashed(frame, up(soffit), INK);
    dashed(frame, up(layer), INK);
    frame.polyline(up(cut00), colour(Family::tsections), 4.0);
    frame.polyline(up(cut10), colour(Family::tsections), 4.0);
    frame.line(up(beam), INK, 2.0);
    frame.line(up(fan), INK, 2.0);
    frame.line(up(gap), MARK, 4.0);

    frame.label("soffit = parabolas[0][0] projected along outer0 onto ts[0][0]", up(soffit.get_point(0)));
    frame.label("cut00 = trim(soffit, cut_plane0, cut_plane1)", up(cut00.get_point(3)));
    frame.label("cut10 = trim(layer, cut_plane0, cut_plane1)", up(cut10.get_point(5)));
    frame.label("cut_plane0 = inner_beams[0][1]", up(beam.start()));
    frame.label("cut_plane1 = wedges[0][0], the fan", up(fan.start()));
    frame.label(fmt::format("tsections = {:.0f}", guide.parameters.tsections), up(gap.center()));
    frame.write(context.dir);
}

/// T-section 0's top loop: cut00, then cut10 reversed, then cut00's first point again, numbered.
void show_tsection_loop(const Context& context) {
    const Quarter quarter = context.guide.quarter(0);
    const Polyline loop = quarter.tsections()[0].top;
    const std::vector<Point> points = loop.get_points();
    const size_t n = points.size();
    const size_t half = (n - 1) / 2;
    Frame frame(CHAPTER, 90, "tsection_loop", "tsection: top = cut00 + cut10 reversed + cut00.front(), one closed strip on ts[0][0]", "front", {-2850.0, -2960.0, H - 780.0, 60.0, -2840.0, H + 80.0});

    frame.polyline(up(quarter.outer_ribs()[0].bottom), GREY, 1.5);
    frame.polyline(up(loop), colour(Family::tsections), 3.0);

    for (size_t i = 0; i + 1 < n; i++)
        frame.point(up(points[i]), MARK, 8.0);

    frame.line(up(Line::from_points(points[0], points[1])), MARK, 3.0, false, true);
    frame.line(up(Line::from_points(points[half], points[half + 1])), MARK, 3.0, false, true);

    frame.label(fmt::format("0 = {}: cut00.front(); {}: cut10.front()", n - 1, n - 2), up(points[0]));
    frame.label(fmt::format("1 .. {}: cut00, the soffit", half - 1), up(points[2]));
    frame.label(fmt::format("{}: cut00.back(); {}: cut10.back()", half - 1, half), up(points[half - 1]));
    frame.label(fmt::format("{} .. {}: cut10 reversed, + tsections", half, n - 2), up(points[n - 5]));
    frame.label(fmt::format("tsections()[0].top: {} points", n), up(points[4]));
    frame.write(context.dir);
}

/// The outer parabola of outer rib 0 in plan, carried along the outer rib normal onto the first faces of t-sections 0 and 1.
void show_outer_tsection(const Context& context) {
    const QuarterGeometry& geometry = context.guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const Vector outer = cp.outer_ribs[0][0].z_axis();
    const Polyline& parabola = geometry.parabolas[0][0];
    const Polyline on0 = parabola.transformed(Xform::project_to_plane_by_axis(cp.tsections[0][0], outer));
    const Polyline on1 = parabola.transformed(Xform::project_to_plane_by_axis(cp.tsections[1][0], outer));
    Frame frame(CHAPTER, 91, "outer_tsection", "outer_tsection: project_to_plane_by_axis(faces[0], outer) copies the outer parabola onto the flange face", "top", {-3050.0, -3100.0, H - 700.0, 50.0, -900.0, H + 20.0});

    frame.line(up(trace(cp.tsections[0][0], cp.wedges[0][0], cp.inner_beams[0][1])), GREY, 2.0);
    frame.line(up(trace(cp.tsections[1][0], cp.wedges[0][0], cp.inner_beams[0][1])), GREY, 2.0);
    frame.polyline(up(parabola), INK, 3.0);
    frame.polyline(up(on0), colour(Family::tsections), 3.0);
    frame.polyline(up(on1), colour(Family::tsections), 3.0);

    for (size_t i = 0; i < parabola.point_count(); i++)
        frame.line(up(Line::from_points(parabola.get_point(i), on1.get_point(i))), MARK, 1.5, false, true);

    frame.label(fmt::format("parabolas[0][0]: in plan on y {:.0f}", parabola.get_point(0)[1]), up(parabola.get_point(1)));
    frame.label("along outer = outer_ribs[0][0].z_axis()", up(Point::mid_point(parabola.get_point(3), on1.get_point(3))));
    frame.label("t-section 0: faces[0] = ts[0][0] = outer_ribs[0][1]", up(on0.get_point(5)));
    frame.label("t-section 1: faces[0] = ts[1][0] = inner_ribs[0][0]", up(on1.get_point(4)));
    frame.write(context.dir);
}

/// T-sections 0 and 5 on the panel sides of the two outer ribs, seen from the bay centre.
void show_flanges_0_5(const Context& context) {
    const Quarter quarter = context.guide.quarter(0);
    const std::vector<Outline> flanges = quarter.tsections();
    const std::vector<Point> top = flanges[0].top.get_points();
    const size_t half = (top.size() - 1) / 2;
    Frame frame(CHAPTER, 92, "flanges_0_5", "tsections 0 and 5: outer_tsection(pb[k], ts[0 or 5], outer, outer, ...), both projections along outer", "iso", QUARTER);
    frame.orbit = "-628,0";
    frame.distance = 0.72;

    family_members(frame, context, Family::outer_ribs, GREY);
    one_member(frame, context, Family::tsections, 0, colour(Family::tsections));
    one_member(frame, context, Family::tsections, 5, colour(Family::tsections));

    frame.label(member_name(Family::tsections, 0, 0), up(middle(flanges[0])));
    frame.label(member_name(Family::tsections, 5, 0), up(middle(flanges[5])));
    frame.label(fmt::format("cut_plane1 = wedges[0][0]: x {:.0f}", top[0][0]), up(top[0]));
    frame.label(fmt::format("cut_plane0 = inner_beams[0][1]: x {:.0f}", top[half - 1][0]), up(top[half - 1]));
    frame.write(context.dir);
}

/// T-section 1 in plan near the fan: the soffit carried to its second face along rib_sweep, the + t along outer0.
void show_flanges_1_4(const Context& context) {
    const Quarter quarter = context.guide.quarter(0);
    const ConstructionPlanes& cp = quarter.geometry().planes;
    const Outline flange = quarter.tsections()[1];
    const std::vector<Point> top = flange.top.get_points();
    const std::vector<Point> bottom = flange.bottom.get_points();
    const size_t n = top.size();
    const size_t half = (n - 1) / 2;
    const Line face0 = trace(cp.inner_ribs[0][0], cp.wedges[0][0], cp.inner_beams[0][1]);
    const Line face1 = trace(cp.tsections[1][1], cp.wedges[0][0], cp.inner_beams[0][1]);
    Frame frame(CHAPTER, 93, "flanges_1_4", "tsections 1 and 4: on face 1 the soffit arrives along rib_sweep, the + t along the outer rib normal", "top", {-2800.0, -2960.0, H - 760.0, -1150.0, -1750.0, H + 20.0});

    one_member(frame, context, Family::inner_ribs, 0, GREY);
    frame.line(up(face0), INK, 1.5, true);
    frame.line(up(face1), INK, 1.5, true);
    frame.polyline(up(flange.top), GREY, 2.0);
    frame.polyline(up(flange.bottom), colour(Family::tsections), 3.0);

    for (size_t i = 1; i + 1 < half; i++)
        frame.line(up(Line::from_points(top[i], bottom[i])), MARK, 2.5, false, true);

    for (size_t i = half + 1; i + 2 < n; i++)
        frame.line(up(Line::from_points(top[i], bottom[i])), INK, 2.5, false, true);

    frame.label("rib_sweep: the soffit onto ts[1][1]", up(Point::mid_point(top[2], bottom[2])));
    frame.label("outer0: the + t onto ts[1][1]", up(Point::mid_point(top[n - 3], bottom[n - 3])));
    frame.label(fmt::format("fan end: top z {:.1f}, bottom z {:.1f}", top[0][2], bottom[0][2]), up(bottom[0]));
    frame.label("face 0 = ts[1][0] = inner_ribs[0][0]", up(face0.point_at(0.48)));
    frame.label(fmt::format("face 1 = ts[1][1], {:.0f} into panel 0", context.guide.parameters.tsections), up(face1.point_at(0.32)));
    frame.write(context.dir);
}

/// T-sections 2 and 3 on the inner ribs' central faces, with the ruling u and the sweep r that carry their traces to the second face.
void show_flanges_2_3(const Context& context) {
    const Quarter quarter = context.guide.quarter(0);
    const QuarterGeometry& geometry = quarter.geometry();
    const CentralPanel& panel = geometry.central_panel;
    const std::vector<Outline> flanges = quarter.tsections();
    const std::vector<Point> top = flanges[2].top.get_points();
    const size_t half = (top.size() - 1) / 2;
    const Point a = panel.traces[0][0].get_point(3);
    const Point b = line_plane(Line::from_points(a, a + panel.ruling), geometry.planes.inner_ribs[1][1]).value();
    const Point r = a + panel.rib_sweep.normalized() * 200.0;
    Frame frame(CHAPTER, 94, "flanges_2_3", "tsections 2 and 3: tsection on panel.traces[k], the soffit along rib_sweep, the + t along panel.ruling", "top", {-2900.0, -2900.0, H - 700.0, 0.0, 0.0, H + 30.0});

    family_members(frame, context, Family::inner_ribs, GREY);
    one_member(frame, context, Family::tsections, 2, colour(Family::tsections));
    one_member(frame, context, Family::tsections, 3, colour(Family::tsections));
    frame.line(up(Line::from_points(a, b)), MARK, 3.0, false, true);
    frame.line(up(Line::from_points(a, r)), INK, 3.0, false, true);

    frame.label(member_name(Family::tsections, 2, 0), up(middle(flanges[2])));
    frame.label(member_name(Family::tsections, 3, 0), up(middle(flanges[3])));
    frame.label("panel.ruling u: projection11, the + t", up(Point::mid_point(a, b)));
    frame.label("panel.rib_sweep r: projection10, the soffit", up(r));
    frame.label("cut_plane0 = inner_beams[1][1]", up(top[half - 1]));
    frame.label("cut_plane1 = wedges[1][0]", up(top[0]));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Beds
// ═══════════════════════════════════════════════════════════════════════════

/// Bed row 0's four layers: the + t and + 2t of the outer parabola on both side faces, untrimmed dashed, trimmed between the beam face and the fan plane.
void show_bed_layers(const Context& context) {
    const Quarter quarter = context.guide.quarter(0);
    const QuarterGeometry& geometry = quarter.geometry();
    const ConstructionPlanes& cp = geometry.planes;
    const Vector outer = cp.outer_ribs[0][0].z_axis();
    const std::array<Plane, 2> sides = {cp.inner_ribs[0][0], cp.outer_ribs[0][1]};
    std::array<Polyline, 2> lower;
    std::array<Polyline, 2> upper;
    Frame frame(CHAPTER, 95, "bed_layers", "bed_row: lower and upper trimmed on both side faces; unequal point counts throw runtime_error", "iso", {-2900.0, -3050.0, H - 720.0, 0.0, -900.0, H + 20.0});
    frame.orbit = "0,105";

    frame.polyline(up(quarter.outer_ribs()[0].bottom), GREY, 1.5);
    frame.polyline(up(quarter.inner_ribs()[0].top), GREY, 1.5);

    for (size_t s = 0; s < 2; s++) {
        const Xform projection = Xform::project_to_plane_by_axis(sides[s], outer);
        const Polyline lower_face = geometry.parabolas[0][1].transformed(projection);
        const Polyline upper_face = geometry.parabolas[0][2].transformed(projection);
        lower[s] = trim(lower_face, cp.inner_beams[0][1], cp.wedges[0][0]);
        upper[s] = trim(upper_face, cp.inner_beams[0][1], cp.wedges[0][0]);
        dashed(frame, up(lower_face), INK);
        dashed(frame, up(upper_face), INK);
        frame.polyline(up(lower[s]), colour(Family::beds), 3.0);
        frame.polyline(up(upper[s]), colour(Family::beds), 3.0);
    }

    frame.label(fmt::format("lower[0]: + t on inner_ribs[0][0], {} points", lower[0].point_count()), up(lower[0].get_point(2)));
    frame.label(fmt::format("upper[0]: + 2t, {} points", upper[0].point_count()), up(upper[0].get_point(4)));
    frame.label(fmt::format("lower[1]: + t on outer_ribs[0][1], {} points", lower[1].point_count()), up(lower[1].get_point(3)));
    frame.label(fmt::format("upper[1]: + 2t, {} points", upper[1].point_count()), up(upper[1].get_point(5)));
    frame.label("cut_plane0 = inner_beams[0][1]", up(lower[1].get_point(lower[1].point_count() - 1)));
    frame.label("cut_plane1 = wedges[0][0]", up(lower[0].get_point(0)));
    frame.write(context.dir);
}

/// Bed row 0: one plate per facet between the side faces, on the flanges.
void show_bed_plates(const Context& context) {
    const Quarter quarter = context.guide.quarter(0);
    const std::vector<Outline> row = quarter.beds()[0];
    Frame frame(CHAPTER, 96, "bed_plates", "bed_row: plate i = the quads lower[0..1][i, i + 1] and upper[0..1][i, i + 1], one per facet", "iso", {-2900.0, -3050.0, H - 720.0, 0.0, -900.0, H + 20.0});
    frame.orbit = "0,105";

    frame.polyline(up(quarter.outer_ribs()[0].bottom), GREY, 1.5);
    frame.polyline(up(quarter.inner_ribs()[0].top), GREY, 1.5);
    one_member(frame, context, Family::tsections, 0, GREY);
    one_member(frame, context, Family::tsections, 1, GREY);

    for (size_t i = 0; i < row.size(); i++) {
        frame.element(context.members.members.quarters[0].beds[0][i].element, colour(Family::beds));
        frame.label(bed_name(0, i), up(middle(row[i])));
    }

    frame.write(context.dir);
}

/// Bed row 0's layers in plan: the outer parabola's + t and + 2t carried along the outer rib normal onto the two side faces.
void show_outer_bed_row(const Context& context) {
    const QuarterGeometry& geometry = context.guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const Vector outer = cp.outer_ribs[0][0].z_axis();
    const Polyline& lower = geometry.parabolas[0][1];
    const Polyline& upper = geometry.parabolas[0][2];
    const Polyline on0 = lower.transformed(Xform::project_to_plane_by_axis(cp.inner_ribs[0][0], outer));
    const Polyline on1 = lower.transformed(Xform::project_to_plane_by_axis(cp.outer_ribs[0][1], outer));
    Frame frame(CHAPTER, 97, "outer_bed_row", "outer_bed_row: projection0 and projection1 carry + t and + 2t along outer onto side0 and side1", "top", {-3050.0, -3100.0, H - 700.0, 50.0, -900.0, H + 20.0});

    frame.line(up(trace(cp.inner_ribs[0][0], cp.wedges[0][0], cp.inner_beams[0][1])), GREY, 2.0);
    frame.line(up(trace(cp.outer_ribs[0][1], cp.wedges[0][0], cp.inner_beams[0][1])), GREY, 2.0);
    frame.polyline(up(upper), GREY, 2.0);
    frame.polyline(up(lower), INK, 3.0);
    frame.polyline(up(on0), colour(Family::beds), 3.0);
    frame.polyline(up(on1), colour(Family::beds), 3.0);

    for (size_t i = 0; i < lower.point_count(); i++)
        frame.line(up(Line::from_points(lower.get_point(i), on0.get_point(i))), MARK, 1.5, false, true);

    frame.label("parabolas[0][1], + t: lower_faces", up(lower.get_point(1)));
    frame.label("parabolas[0][2], + 2t: upper_faces", up(upper.get_point(5)));
    frame.label("projection0 onto side0 = inner_ribs[0][0]", up(on0.get_point(4)));
    frame.label("projection1 onto side1 = outer_ribs[0][1]", up(on1.get_point(2)));
    frame.label("along outer = outer_ribs[0][0].z_axis()", up(Point::mid_point(lower.get_point(3), on0.get_point(3))));
    frame.write(context.dir);
}

/// All three bed rows of quarter 0 in plan.
void show_bed_rows(const Context& context) {
    const Quarter quarter = context.guide.quarter(0);
    const std::vector<std::vector<Outline>> rows = quarter.beds();
    Frame frame(CHAPTER, 98, "bed_rows", "beds(): row 0 on outer panel 0, row 1 across the central panel, row 2 on outer panel 1", "top", QUARTER);
    frame.key = true;

    family_members(frame, context, Family::outer_ribs, GREY);
    family_members(frame, context, Family::inner_ribs, GREY);
    family_members(frame, context, Family::beds, colour(Family::beds));

    for (size_t r = 0; r < rows.size(); r++)
        frame.label(fmt::format("beds()[{}]: {} .. {}", r, bed_name(r, 0), bed_name(r, rows[r].size() - 1)), up(middle(rows[r][rows[r].size() / 2])));

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus
// ═══════════════════════════════════════════════════════════════════════════

/// The four levels of the oculus, in front of the ring beams.
void show_oculus_levels(const Context& context) {
    const FloorGuide& guide = context.guide;
    const double t = guide.parameters.tsections;
    const std::array<double, 4> z = {0.0, guide.soffit + t, guide.soffit, guide.soffit + t * 2.0};
    const std::array<std::string, 4> names = {"side0 = level(0)", "side1 = level(soffit + tsections)", "side2 = level(soffit)", "side3 = level(soffit + 2 tsections)"};
    const std::array<double, 4> pins = {-550.0, -600.0, -250.0, -850.0};
    Frame frame(CHAPTER, 99, "oculus_levels", "oculus(): side0 = datum, side2 = soffit, side1 and side3 one and two tsections above the soffit", "front", {-1050.0, -1150.0, H - 260.0, -50.0, 100.0, H + 60.0});

    for (const Member& member : context.members.members.ring)
        frame.element(member.element, GREY);

    for (size_t i = 0; i < 4; i++) {
        frame.line(up(Line::from_points(Point(-1150.0, -1100.0, z[i]), Point(1150.0, -1100.0, z[i]))), RING, 3.0);
        frame.label(fmt::format("{}: z {:.2f}", names[i], z[i]), up(Point(pins[i], -1100.0, z[i])));
    }

    frame.write(context.dir);
}

/// The oculus in plan: per edge the tilted plane at the datum and at the soffit, and the ring inner plane.
void show_oculus_planes(const Context& context) {
    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 100, "oculus_planes", "oculus(): tilted[q] = oculus_edges[q].tilted and inner[q] = oculus_edges[q].ring_inner", "top", {-1300.0, -1300.0, H - 260.0, 1300.0, 1300.0, H + 40.0});

    for (size_t q = 0; q < 4; q++) {
        const OculusEdge& edge = guide.oculus_edges[q];
        const Line datum = trace_under(edge.tilted, edge.line, 0.0);
        const Line soffit = trace_under(edge.tilted, edge.line, guide.soffit);
        const Line inner = trace_under(edge.ring_inner, edge.line, 0.0);
        frame.line(up(datum), INK, 2.5);
        frame.line(up(soffit), colour(Family::inner_beams), 2.5);
        frame.line(up(inner), RING, 2.5);

        if (q % 2 == 1)
            continue;

        frame.label(fmt::format("tilted[{}] at side0: on oculus_edges[{}].line", q, q), up(datum.point_at(0.3)));
        frame.label(fmt::format("inner[{}] = ring_inner: {:.0f} inside", q, Point::distance(datum.center(), inner.center())), up(inner.point_at(0.7)));

        if (q == 0)
            frame.label(fmt::format("tilted[0] at side2: {:.1f} inward", Point::distance(Point(datum.center()[0], datum.center()[1], guide.soffit), soffit.center())), up(soffit.point_at(0.5)));
    }

    frame.write(context.dir);
}

/// The four ring beams in plan, each running on to the next tilted plane and stopping on the previous inner plane.
void show_ring_beams(const Context& context) {
    const FloorGuide& guide = context.guide;
    const std::vector<Outline> oculus = guide.oculus();
    const Vector lift(0.0, 0.0, 10.0);
    Frame frame(CHAPTER, 101, "ring_beams", "oculus()[0..3]: loft_planes({side2, tilted[i + 1], side0, inner[i + 3]}, tilted[i], inner[i], flip)", "top", {-1200.0, -1200.0, H - 260.0, 1200.0, 1200.0, H + 40.0});
    frame.key = true;

    for (const OculusEdge& edge : guide.oculus_edges)
        frame.line(up(edge.line), GREY, 1.5);

    for (size_t i = 0; i < 4; i++) {
        const std::vector<Point> top = open_points(oculus[i].top);
        const std::vector<Point> bottom = open_points(oculus[i].bottom);
        frame.element(context.members.members.ring[i].element, RING);
        frame.line(up(Line::from_points(Point::mid_point(top[2], bottom[2]) + lift, Point::mid_point(top[1], bottom[1]) + lift)), INK, 3.0, false, true);
        frame.label(fmt::format("oculus_{}", i), up(middle(oculus[i])));
    }

    frame.label("oculus_0 runs on to tilted[1]", up(open_points(oculus[0].bottom)[1]));
    frame.label("oculus_0 stops on inner[3]", up(open_points(oculus[0].bottom)[2]));
    frame.write(context.dir);
}

/// The four bottom wedges: ledge strips a tsections wide and thick inside the ring.
void show_bottom_wedges(const Context& context) {
    const FloorGuide& guide = context.guide;
    const std::vector<Outline> oculus = guide.oculus();
    const std::vector<Point> top = open_points(oculus[4].top);
    Frame frame(CHAPTER, 102, "bottom_wedges", "oculus()[4..7]: bottom wedges between inner[i] and inner[i] moved -tsections, from soffit to soffit + t", "top", {-1100.0, -1100.0, H - 260.0, 1100.0, 1100.0, H + 40.0});

    for (const Member& member : context.members.members.ring)
        frame.element(member.element, GREY);

    for (size_t i = 4; i < 8; i++) {
        frame.element(placed_plate(oculus[i], fmt::format("oculus_{}", i)), colour(Family::wedges));
        frame.label(fmt::format("oculus_{}", i), up(middle(oculus[i])));
    }

    frame.label("inner[0]", up(Point::lerp(top[3], top[0], 0.25)));
    frame.label("inner[0].translate_by_normal(-tsections)", up(Point::lerp(top[1], top[2], 0.25)));
    frame.write(context.dir);
}

/// The central plate bounded by the four ring inner planes, on the bottom wedges.
void show_central_plate(const Context& context) {
    const FloorGuide& guide = context.guide;
    const std::vector<Outline> oculus = guide.oculus();
    const Outline& plate = oculus[8];
    const std::vector<Point> top = open_points(plate.top);
    const std::vector<Point> bottom = open_points(plate.bottom);
    const Point centre(guide.centre[0], guide.centre[1], top[0][2]);
    Frame frame(CHAPTER, 103, "central_plate", "oculus()[8]: loft_planes(inner, side1, side3), the plate inside the ring on the bottom wedges", "top", {-1100.0, -1100.0, H - 260.0, 1100.0, 1100.0, H + 40.0});

    for (const Member& member : context.members.members.ring)
        frame.element(member.element, GREY);

    for (size_t i = 4; i < 8; i++)
        frame.element(placed_plate(oculus[i], fmt::format("oculus_{}", i)), GREY);

    frame.element(placed_plate(plate, "oculus_8"), colour(Family::beds));
    frame.line(up(Line::from_points(centre, top[0])), MARK, 3.0);

    frame.label("oculus_8", up(Point::lerp(centre, top[2], 0.5)), true);
    frame.label(fmt::format("half-diagonal {:.1f}", Point::distance(centre, top[0])), up(Point::mid_point(centre, top[0])));
    frame.label(fmt::format("top loop on side3, z {:.2f}", top[1][2]), up(top[1]));
    frame.label(fmt::format("bottom loop on side1, z {:.2f}", bottom[3][2]), up(bottom[3]));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Column cutters
// ═══════════════════════════════════════════════════════════════════════════

/// The column head in plan with the five fan_top planes, each through one head edge at the datum.
void show_fan_top(const Context& context) {
    const ColumnCorner& column = context.guide.columns[0];
    const CutterCorners cutters = cutter_corners(context.guide);
    const std::array<std::string, 5> names = {"side0 = corner.sides[0]", "cp.wedges[0][0]", "cp.wedges[1][0]", "cp.wedges[2][0]", "side1 = corner.sides[1]"};
    Frame frame(CHAPTER, 104, "fan_top", "column_cutters: fan_top = {side0, wedges[0][0], wedges[1][0], wedges[2][0], side1}, one per head edge", "top", HEAD);
    frame.plane_size = 45.0;

    for (size_t k = 0; k < cutters.fan_top.size(); k++) {
        const Line edge = wood_floor::geometry::edge(column.head, k);
        frame.line(up(edge), colour(Family::wedges), 4.0);
        frame.plane(up(at(cutters.fan_top[k], edge.center())), INK);
        frame.label(fmt::format("fan_top[{}] = {}", k, names[k]), up(edge.center()));
    }

    frame.write(context.dir);
}

/// The square shaft under the head in 3D: the four fan_bottom planes as the shaft's faces down to levels[2], the chamfer face left out.
void show_fan_bottom(const Context& context) {
    const ColumnCorner& column = context.guide.columns[0];
    const CutterCorners cutters = cutter_corners(context.guide);
    const Plane xy2 = level(column.levels[2]);
    const Point corner = plane_plane_plane(xy2, cutters.fan_bottom[3], cutters.fan_bottom[0]).value();
    const std::vector<Point> square = {corner, cutters.p2[0], cutters.p2[1], cutters.p2[2]};
    const Vector rise(0.0, 0.0, -column.levels[2]);
    const Line chamfer = wood_floor::geometry::edge(column.head, 2);
    const std::array<std::string, 4> names = {"side0", "edge_plane(edge(column, 1), down)", "edge_plane(edge(column, 3), down)", "side1"};
    Frame frame(CHAPTER, 105, "fan_bottom", "column_cutters: fan_bottom = {side0, the shaft faces under head edges 1 and 3, side1}", "iso", {-3080.0, -3080.0, H - 780.0, -2600.0, -2600.0, H + 40.0});
    frame.plane_size = 70.0;

    for (size_t k = 0; k < column.head.size(); k++)
        if (k != 2)
            frame.line(up(wood_floor::geometry::edge(column.head, k)), GREY, 2.0);

    frame.line(up(chamfer), INK, 2.0, true);
    frame.polyline(up(Polyline(square).closed()), MARK, 4.0);

    for (const Point& point : square)
        frame.line(up(Line::from_points(point, point + rise)), INK, 1.5, true);

    for (size_t k = 0; k < 4; k++) {
        const Point foot = Point::mid_point(square[k], square[(k + 1) % 4]);
        frame.plane(up(at(cutters.fan_bottom[k], foot + rise * 0.5)), INK);
        frame.label(fmt::format("fan_bottom[{}] = {}", k, names[k]), up(foot));
    }

    frame.label("chamfer edge(column, 2): no plane here", up(chamfer.center()));
    frame.write(context.dir);
}

/// The eleven crease points of the two fans at the three cutter levels.
void show_cutter_corners(const Context& context) {
    const ColumnCorner& column = context.guide.columns[0];
    const CutterCorners cutters = cutter_corners(context.guide);
    const std::vector<Point> square = {plane_plane_plane(level(column.levels[2]), cutters.fan_bottom[3], cutters.fan_bottom[0]).value(), cutters.p2[0], cutters.p2[1], cutters.p2[2]};
    Frame frame(CHAPTER, 106, "cutter_corners", "column_cutters: p0, p1 = fan_top creases at levels[0], [1]; p2 = fan_bottom creases at levels[2]", "iso", {-3080.0, -3080.0, H - 780.0, -2600.0, -2600.0, H + 40.0});

    frame.polyline(up(Polyline(column.head).closed()), GREY, 2.0);
    frame.polyline(up(Polyline(square).closed()), GREY, 2.0);

    for (size_t i = 0; i < cutters.p0.size(); i++) {
        frame.line(up(Line::from_points(cutters.p0[i], cutters.p1[i])), INK, 1.5, true);
        frame.point(up(cutters.p0[i]), MARK, 14.0);
        frame.point(up(cutters.p1[i]), colour(Family::wedges), 14.0);
    }

    for (const Point& point : cutters.p2)
        frame.point(up(point), STEEL, 14.0);

    frame.label(fmt::format("p0[0], z {:.0f}", cutters.p0[0][2]), up(cutters.p0[0]));
    frame.label("p0[2]", up(cutters.p0[2]));
    frame.label(fmt::format("p1[0], z {:.1f}", cutters.p1[0][2]), up(cutters.p1[0]));
    frame.label("p1[1]", up(cutters.p1[1]));
    frame.label("p1[3]", up(cutters.p1[3]));
    frame.label(fmt::format("p2[0], z {:.0f}", cutters.p2[0][2]), up(cutters.p2[0]));
    frame.label("p2[1]: the shaft corner", up(cutters.p2[1]));
    frame.label("p2[2]", up(cutters.p2[2]));
    frame.write(context.dir);
}

/// The six cutter quads before stretch: three on the fan, three down to the shaft.
void show_cutter_quads(const Context& context) {
    const ColumnCorner& column = context.guide.columns[0];
    const CutterCorners cutters = cutter_corners(context.guide);
    const Point& p21 = cutters.p2[1];
    Frame frame(CHAPTER, 107, "cutter_quads", "column_cutters: quads 0-2 on the fan between levels[0] and [1], quads 3-5 down to levels[2]", "iso", {-3080.0, -3080.0, H - 780.0, -2550.0, -2550.0, H + 40.0});
    frame.distance = 0.8;

    frame.polyline(up(Polyline(column.head).closed()), GREY, 1.5);

    for (size_t i = 0; i < cutters.quads.size(); i++) {
        const Polyline quad = Polyline(cutters.quads[i]).closed();
        frame.polyline(up(quad), i < 3 ? colour(Family::wedges) : STEEL, 3.0);
        frame.label(fmt::format("quads[{}]", i), up(area_centroid(quad)));
    }

    frame.line(up(Line::from_points(p21 - cutters.quarter, p21 + cutters.quarter)), MARK, 5.0);
    frame.label(fmt::format("p2[1] +- quarter: {:.1f} long", (cutters.quarter * 2.0).magnitude()), up(p21 + cutters.quarter));
    frame.write(context.dir);
}

/// Quad 0 face-on: its top and bottom edges lengthened by the margin at both ends.
void show_stretch_edges(const Context& context) {
    const CutterCorners cutters = cutter_corners(context.guide);
    const double margin = cutter_margin(context.guide);
    const std::vector<Point>& quad = cutters.quads[0];
    const std::vector<Point> longer = lengthened(quad, margin);
    const std::array<std::string, 4> names = {"quad[0] - d0", "quad[1] + d0", "quad[2] - d1", "quad[3] + d1"};
    Frame frame(CHAPTER, 108, "stretch_edges", "stretch: d0 and d1 lengthen edges 0-1 and 2-3 by CUTTER_MARGIN at both ends", "iso", {-2900.0, -3200.0, H - 830.0, -2550.0, -2600.0, H + 130.0});
    frame.orbit = "-419,-75";

    frame.polyline(up(Polyline(quad).closed()), GREY, 3.0);
    frame.polyline(up(Polyline(longer).closed()), colour(Family::wedges), 3.0);

    for (size_t k = 0; k < 4; k++) {
        frame.line(up(Line::from_points(quad[k], longer[k])), MARK, 3.0, false, true);
        frame.label(names[k], up(longer[k]));
    }

    frame.label(fmt::format("quads[0], d0 = unit(quad[1] - quad[0]) x {:.0f}", margin), up(area_centroid(Polyline(quad).closed())));
    frame.write(context.dir);
}

/// Quads 0 and 3 face-on to quad 0: the lengthened edges pushed apart by d2 and d3, a bottom quad's lower edge kept on levels[2].
void show_stretch_apart(const Context& context) {
    const FloorGuide& guide = context.guide;
    const CutterCorners cutters = cutter_corners(guide);
    const double margin = cutter_margin(guide);
    const std::vector<Outline> plates = guide.quarter(0).column_cutters();
    const std::vector<Point> longer0 = lengthened(cutters.quads[0], margin);
    const std::vector<Point> longer3 = lengthened(cutters.quads[3], margin);
    const std::vector<Point> final0 = open_points(plates[0].top);
    const std::vector<Point> final3 = open_points(plates[3].top);
    Frame frame(CHAPTER, 109, "stretch_apart", "stretch: edge 0-1 moves by -d2; edge 2-3 by -d3 only on a top quad, a bottom quad keeps it on levels[2]", "iso", {-2900.0, -3200.0, H - 830.0, -2550.0, -2600.0, H + 130.0});
    frame.orbit = "-419,-75";

    frame.polyline(up(Polyline(longer0).closed()), GREY, 2.0);
    dashed(frame, up(Polyline(longer3).closed()), GREY);
    frame.polyline(up(plates[0].top), colour(Family::wedges), 3.0);
    frame.polyline(up(plates[3].top), STEEL, 3.0);

    for (size_t k = 0; k < 4; k++)
        frame.line(up(Line::from_points(longer0[k], final0[k])), MARK, 3.0, false, true);

    for (size_t k = 0; k < 2; k++)
        frame.line(up(Line::from_points(longer3[k], final3[k])), MARK, 3.0, false, true);

    frame.label(fmt::format("-d2: quads[0] edge 0-1 to z {:.1f}", final0[0][2]), up(final0[0]));
    frame.label(fmt::format("-d3: quads[0] edge 2-3 to z {:.1f}", final0[3][2]), up(final0[3]));
    frame.label("quads[0]: a top quad, both edges move", up(final0[1]));
    frame.label(fmt::format("-d2: quads[3] edge 0-1 to z {:.1f}", final3[0][2]), up(final3[0]));
    frame.label(fmt::format("quads[3] edge 2-3 stays at z {:.0f}", final3[2][2]), up(final3[2]));
    frame.write(context.dir);
}

/// The six stretched quads thickened into cutter plates, with their normals, on the column head's capitel box.
void show_cutter_plates(const Context& context) {
    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const std::vector<Outline> plates = guide.quarter(0).column_cutters();
    const double side = guide.parameters.column_head + guide.parameters.column_head_chamfer;
    const Point& c = column.corner;
    const std::vector<Point> square = {c, c + column.x_axis * side, c + column.x_axis * side + column.y_axis * side, c + column.y_axis * side};
    const Vector depth(0.0, 0.0, column.levels[2]);
    Frame frame(CHAPTER, 110, "cutter_plates", "column_cutters(): each stretched quad and its copy moved CUTTER_MARGIN along the quad normal", "iso", {-3100.0, -3100.0, H - 850.0, -2450.0, -2450.0, H + 150.0});

    frame.polyline(up(Polyline(square).closed()), GREY, 1.5);
    frame.polyline(up(Polyline(square).closed().translated(depth)), GREY, 1.5);

    for (const Point& point : square)
        frame.line(up(Line::from_points(point, point + depth)), GREY, 1.5);

    for (size_t i = 0; i < plates.size(); i++) {
        frame.element(placed_plate(plates[i], "column_cutter"), i < 3 ? colour(Family::wedges) : STEEL);
        frame.line(up(Line::from_points(area_centroid(plates[i].top), area_centroid(plates[i].bottom))), MARK, 3.0, false, true);
        frame.label(fmt::format("column_cutters()[{}]", i), up(middle(plates[i])));
    }

    frame.label(fmt::format("normal x {:.0f}: the second loop", outline_thickness(plates[1])), up(area_centroid(plates[1].bottom)));
    frame.write(context.dir);
}

}

/// Chapter 06: the outlines of the inner beams, wedges, t-sections, beds, oculus and column cutters, in the order floor_members.cpp builds them.
void chapter_06_outlines(const Context& context) {
    show_loft_planes(context);
    show_beam_levels(context);
    show_seam_beam_0(context);
    show_oculus_beam(context);
    show_seam_beam_2(context);
    show_wedge_bounds(context);
    show_wedge_blocks(context);
    show_tsection_trim(context);
    show_tsection_loop(context);
    show_outer_tsection(context);
    show_flanges_0_5(context);
    show_flanges_1_4(context);
    show_flanges_2_3(context);
    show_bed_layers(context);
    show_bed_plates(context);
    show_outer_bed_row(context);
    show_bed_rows(context);
    show_oculus_levels(context);
    show_oculus_planes(context);
    show_ring_beams(context);
    show_bottom_wedges(context);
    show_central_plate(context);
    show_fan_top(context);
    show_fan_bottom(context);
    show_cutter_corners(context);
    show_cutter_quads(context);
    show_stretch_edges(context);
    show_stretch_apart(context);
    show_cutter_plates(context);
}

}
