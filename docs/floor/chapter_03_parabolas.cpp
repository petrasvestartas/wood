#include "docs/floor/movie.h"

namespace movie {

namespace {

const std::string CHAPTER = "03_parabolas";
const double EXTENSION = 1000.0; // geometry::trim's push-out of both end chords, floor_geometry.cpp:9, not exported.
const Box CORNER = {-3050.0, -3050.0, H - 10.0, -2450.0, -2450.0, H + 10.0}; // Column head 0 and its three blocks in plan.
const Box RIB_RUN = {-3000.0, -3050.0, H - 750.0, 100.0, -2950.0, H + 50.0}; // Outer rib 0 in elevation, datum to soffit.
const Box FAN_END = {-2850.0, -3050.0, H - 760.0, -2050.0, -2950.0, H - 450.0}; // Outer rib 0's column end in elevation.
const Box LAYERS = {-2620.0, -3050.0, H - 680.0, -1600.0, -2950.0, H - 330.0}; // The first two soffit segments of outer rib 0.
const std::array<double, 2> GRAPH_RUN_IN = {180.0, 245.0}; // The run-ins frame 43 plots f(x) over, mm.
const std::array<double, 2> GRAPH_SCALE = {6.0, 4.0}; // Drawing mm per mm of run-in and per mm of f in frame 43's graph.

// ═══════════════════════════════════════════════════════════════════════════
// Construction, as the static functions of floor.cpp compute it
// ═══════════════════════════════════════════════════════════════════════════

/// The plan quad of four planes at the datum by floor.cpp quad(): corners on planes 3-0, 0-1, 1-2 and 2-3.
Polyline plan_quad(const std::array<Plane, 4>& planes) {

    const Plane xy = level(0.0);

    return Polyline({
        plane_plane_plane(xy, planes[0], planes[3]).value(),
        plane_plane_plane(xy, planes[0], planes[1]).value(),
        plane_plane_plane(xy, planes[1], planes[2]).value(),
        plane_plane_plane(xy, planes[2], planes[3]).value(),
    });
}

/// The block far faces wedge_fan leaves before block_planes: each fan plane moved by wedge, the middle one by wedge times middle_wedge_factor.
std::array<Plane, 3> provisional_far_faces(const FloorGuide& guide) {

    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const FloorParameters& parameters = guide.parameters;

    return {
        cp.wedges[0][0].translate_by_normal(parameters.wedge),
        cp.wedges[1][0].translate_by_normal(parameters.wedge * parameters.middle_wedge_factor),
        cp.wedges[2][0].translate_by_normal(parameters.wedge),
    };
}

/// The first-pass block quads of quarter 0, from their fan planes to the provisional far faces.
std::vector<Polyline> first_pass_wedges(const FloorGuide& guide) {

    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const std::array<Plane, 3> far = provisional_far_faces(guide);

    return {
        plan_quad({cp.wedges[0][0], cp.outer_ribs[0][1], far[0], cp.inner_ribs[0][0]}),
        plan_quad({cp.wedges[1][0], cp.inner_ribs[0][1], far[1], cp.inner_ribs[1][1]}),
        plan_quad({cp.wedges[2][0], cp.inner_ribs[1][0], far[2], cp.outer_ribs[1][1]}),
    };
}

/// The first-pass flange quads of quarter 0, from their beam's far face to their block's provisional far face.
std::vector<Polyline> first_pass_tsections(const FloorGuide& guide) {

    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const std::array<Plane, 3> far = provisional_far_faces(guide);
    std::vector<Polyline> result;

    for (size_t i = 0; i < cp.tsections.size(); i++)
        result.push_back(plan_quad({cp.tsections[i][0], cp.inner_beams[i / 2][1], cp.tsections[i][1], far[i / 2]}));

    return result;
}

/// The outer parabola of floor.cpp outer_parabola(): from -height at the run-in along the quad axis, controlled halfway on at -static_h, to the seam at -static_h.
Polyline outer_parabola(const Polyline& quad, double run_in, const FloorParameters& parameters) {

    const Point start = quad.get_point(0);
    const Point end = quad.get_point(1);
    const Point trimmed = start + (end - start).normalized() * run_in;
    const Point middle = trimmed + (end - trimmed) * 0.5;

    return Polyline::quadratic_points(trimmed + Vector(0.0, 0.0, -parameters.height), middle + Vector(0.0, 0.0, -parameters.static_h()), end + Vector(0.0, 0.0, -parameters.static_h()));
}

/// The soffit end on the fan plane as fan_end finds it: the trimmed parabola's end nearer the fan plane.
Point fan_hit(const Polyline& quad, double run_in, const Plane& fan, const Plane& seam, const FloorParameters& parameters) {

    const std::vector<Point> pts = trim(outer_parabola(quad, run_in, parameters), fan, seam).get_points();
    const double d0 = std::abs(signed_distance(pts.front(), fan));
    const double d1 = std::abs(signed_distance(pts.back(), fan));

    return d0 > d1 ? pts.back() : pts.front();
}

/// The fan plane and the seam plane outer rib k is trimmed by, as run_ins pairs them.
std::array<Plane, 2> rib_ends(const ConstructionPlanes& cp, size_t k) {
    return k == 0 ? std::array<Plane, 2>{cp.wedges[0][0], cp.inner_beams[0][0]} : std::array<Plane, 2>{cp.wedges[2][0], cp.inner_beams[2][0]};
}

// ═══════════════════════════════════════════════════════════════════════════
// Drawing helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The point a fraction t of the way from a to b.
Point between(const Point& a, const Point& b, double t) {
    return a + (b - a) * t;
}

/// A point lowered from the datum by depth.
Point below(const Point& point, double depth) {
    return Point(point[0], point[1], point[2] - depth);
}

/// The line where a plane crosses a rib face, from the datum down to z.
Line face_trace(const Plane& plane, const Plane& face, double z) {
    return Line::from_points(plane_plane_plane(level(0.0), plane, face).value(), plane_plane_plane(level(z), plane, face).value());
}

/// A quad's side from corner i to corner i + 1, extended both ways by margin and dashed: the datum trace of the plane the side lies on.
void side_trace(Frame& frame, const Polyline& quad, size_t i, double margin, const Color& color = INK) {

    const Point a = quad.get_point(i);
    const Point b = quad.get_point((i + 1) % 4);
    const Vector along = (b - a).normalized() * margin;
    frame.line(up(Line::from_points(a + (-along), b + along)), color, 1.5, true);
}

/// Every quad of the list, closed, in one colour.
void draw_quads(Frame& frame, const std::vector<Polyline>& quads, const Color& color, double width) {

    for (const Polyline& quad : quads)
        frame.polyline(up(quad.closed()), color, width);
}

/// The datum quad of every member of quarter 0 in grey: what the quad steps leave the drawing.
void all_quads_grey(Frame& frame, const ConstructionQuads& quads) {

    for (const std::vector<Polyline>* list : {&quads.outer_ribs, &quads.inner_beams, &quads.inner_ribs, &quads.wedges, &quads.tsections})
        draw_quads(frame, *list, GREY, 1.0);
}

/// The polyline in a colour with a dot on every point.
void dotted(Frame& frame, const Polyline& polyline, const Color& color, double width) {

    frame.polyline(up(polyline), color, width);

    for (const Point& point : polyline.get_points())
        frame.point(up(point), INK, 8.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Frames
// ═══════════════════════════════════════════════════════════════════════════

/// 35: the quad() corner rule on outer rib 0, both outer rib quads and their axes get_point(0) to get_point(1).
void outer_rib_quads(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const std::vector<Polyline>& ribs = geometry.quads.outer_ribs;
    Frame frame(CHAPTER, 35, "outer_rib_quads", "quad(): corner i where two of four planes cross the datum; the outer rib quads and their axes", "top", QUARTER);
    frame.polyline(up(Polyline(geometry.polygon).closed()), GREY, 1.0);

    for (size_t i = 0; i < 4; i++)
        side_trace(frame, ribs[0], i, 250.0);

    for (const Polyline& quad : ribs) {
        frame.polyline(up(quad.closed()), FAMILY_COLORS[0], 3.0);
        frame.line(up(Line::from_points(quad.get_point(0), quad.get_point(1))), MARK, 3.0, false, true);
    }

    const std::array<std::string, 4> corners = {
        "corner 0: outer_ribs[0][0] x wedges[0][0]",
        "corner 1: outer_ribs[0][0] x inner_beams[0][0]",
        "corner 2: inner_beams[0][0] x outer_ribs[0][1]",
        "corner 3: outer_ribs[0][1] x wedges[0][0]",
    };

    for (size_t i = 0; i < 4; i++) {
        frame.point(up(ribs[0].get_point(i)), INK, 10.0);
        frame.label(corners[i], up(ribs[0].get_point(i)));
    }

    frame.label("quads.outer_ribs[0]", up(between(ribs[0].get_point(3), ribs[0].get_point(2), 0.4)));
    frame.label("quads.outer_ribs[1]", up(between(ribs[1].get_point(3), ribs[1].get_point(2), 0.3)));
    frame.label("axis: get_point(0) -> get_point(1)", up(between(ribs[1].get_point(0), ribs[1].get_point(1), 0.65)));
    frame.write(context.dir);
}

/// 36: the seam beam 0, oculus beam and seam beam 2 quads in their colour over the outer rib quads.
void inner_beam_quads(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const std::vector<Polyline>& beams = geometry.quads.inner_beams;
    Frame frame(CHAPTER, 36, "inner_beam_quads", "Inner beam quads: seam beam 0, the oculus beam and seam beam 2, each between two faces and two ends", "top", QUARTER);
    frame.polyline(up(Polyline(geometry.polygon).closed()), GREY, 1.0);
    draw_quads(frame, geometry.quads.outer_ribs, GREY, 2.0);
    draw_quads(frame, beams, FAMILY_COLORS[2], 3.0);

    const std::array<std::string, 3> names = {"quads.inner_beams[0]: seam beam 0", "quads.inner_beams[1]: oculus beam", "quads.inner_beams[2]: seam beam 2"};

    for (size_t i = 0; i < beams.size(); i++)
        frame.label(names[i], up(area_centroid(beams[i].closed())));

    frame.write(context.dir);
}

/// 37: the two inner rib quads, their corner 2 on p0 or p1 and corner 3 on head[2] or head[3].
void inner_rib_quads(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const std::vector<Polyline>& ribs = geometry.quads.inner_ribs;
    Frame frame(CHAPTER, 37, "inner_rib_quads", "Inner rib quads, chamfer plane to oculus back face: corner 2 = p0 / p1, corner 3 = head[2] / head[3]", "top", QUARTER);
    frame.polyline(up(Polyline(geometry.polygon).closed()), GREY, 1.0);
    draw_quads(frame, geometry.quads.outer_ribs, GREY, 2.0);
    draw_quads(frame, geometry.quads.inner_beams, GREY, 2.0);
    frame.polyline(up(Polyline(context.guide.columns[0].head).closed()), GREY, 2.0);
    draw_quads(frame, ribs, FAMILY_COLORS[1], 3.0);

    const std::array<std::array<std::string, 2>, 2> corners = {{{"corner 2 = p0", "corner 3 = head[2]"}, {"corner 2 = p1", "corner 3 = head[3]"}}};

    for (size_t k = 0; k < ribs.size(); k++)
        for (size_t c = 0; c < 2; c++) {
            frame.point(up(ribs[k].get_point(2 + c)), MARK, 12.0);
            frame.label(corners[k][c], up(ribs[k].get_point(2 + c)));
        }

    frame.label("quads.inner_ribs[0]", up(between(ribs[0].get_point(0), ribs[0].get_point(1), 0.4)));
    frame.label("quads.inner_ribs[1]", up(between(ribs[1].get_point(0), ribs[1].get_point(1), 0.6)));
    frame.write(context.dir);
}

/// 38: the three block quads of the first pass, ending on the provisional far faces.
void wedge_quads(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const FloorParameters& parameters = context.guide.parameters;
    const std::vector<Polyline> blocks = first_pass_wedges(context.guide);
    Frame frame(CHAPTER, 38, "wedge_quads", "Wedge quads, first pass: each block from its fan plane to a provisional far face, wedge thick", "top", CORNER);
    draw_quads(frame, geometry.quads.outer_ribs, GREY, 2.0);
    draw_quads(frame, geometry.quads.inner_ribs, GREY, 2.0);
    frame.polyline(up(Polyline(context.guide.columns[0].head).closed()), GREY, 2.0);
    draw_quads(frame, blocks, FAMILY_COLORS[3], 3.0);

    for (const Polyline& block : blocks)
        side_trace(frame, block, 2, 60.0);

    for (size_t i = 0; i < blocks.size(); i++)
        frame.label(fmt::format("quads.wedges[{}]", i), up(area_centroid(blocks[i].closed())));

    frame.label(fmt::format("provisional wedges[0][1]: wedges[0][0] + wedge = {:.0f}", parameters.wedge), up(between(blocks[0].get_point(2), blocks[0].get_point(3), 0.5)));
    frame.label(fmt::format("provisional wedges[1][1]: + wedge x middle_wedge_factor = {:.0f}", parameters.wedge * parameters.middle_wedge_factor), up(between(blocks[1].get_point(2), blocks[1].get_point(3), 0.5)));
    frame.write(context.dir);
}

/// 39: the six first-pass flange quads, the block-end side of each, on the block far face, bold.
void tsection_quads(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const std::vector<Polyline> flanges = first_pass_tsections(context.guide);
    const std::array<double, 6> stations = {0.3, 0.5, 0.7, 0.7, 0.5, 0.3};
    Frame frame(CHAPTER, 39, "tsection_quads", "T-section quads: six flanges from a beam's far face to a block's far face, the block end bold", "top", QUARTER);
    draw_quads(frame, geometry.quads.outer_ribs, GREY, 1.5);
    draw_quads(frame, geometry.quads.inner_beams, GREY, 1.5);
    draw_quads(frame, geometry.quads.inner_ribs, GREY, 1.5);
    draw_quads(frame, first_pass_wedges(context.guide), GREY, 1.5);
    draw_quads(frame, flanges, FAMILY_COLORS[4], 2.5);

    for (size_t i = 0; i < flanges.size(); i++) {
        frame.line(up(Line::from_points(flanges[i].get_point(3), flanges[i].get_point(0))), MARK, 6.0);
        frame.label(fmt::format("quads.tsections[{}]", i), up(between(flanges[i].get_point(0), flanges[i].get_point(1), stations[i])));
    }

    frame.label("block end on wedges[0][1]", up(between(flanges[0].get_point(3), flanges[0].get_point(0), 0.5)));
    frame.write(context.dir);
}

/// 40: outer rib 0's soffit at the trial run-in wedge, its control polygon and stations, in elevation.
void outer_parabola_trial(const Context& context) {

    const FloorParameters& parameters = context.guide.parameters;
    const ConstructionPlanes& cp = context.guide.geometry[0].planes;
    const Polyline& quad = context.guide.geometry[0].quads.outer_ribs[0];
    const Point start = quad.get_point(0);
    const Point end = quad.get_point(1);
    const Point trimmed = start + (end - start).normalized() * parameters.wedge;
    const Point middle = trimmed + (end - trimmed) * 0.5;
    const Polyline soffit = outer_parabola(quad, parameters.wedge, parameters);
    const std::array<Point, 3> control = {below(trimmed, parameters.height), below(middle, parameters.static_h()), below(end, parameters.static_h())};
    Frame frame(CHAPTER, 40, "outer_parabola", "outer_parabola at run_in = wedge: a 7-point quadratic Bezier, its control point as deep as its end", "front", RIB_RUN);
    frame.key = true;

    frame.line(up(Line::from_points(start, end)), GREY, 2.0);
    frame.line(up(face_trace(cp.wedges[0][0], cp.outer_ribs[0][0], -parameters.height - 60.0)), GREY, 2.0);
    frame.line(up(face_trace(cp.inner_beams[0][0], cp.outer_ribs[0][0], -parameters.height - 60.0)), GREY, 2.0);
    frame.line(up(Line::from_points(control[0], control[1])), INK, 1.5, true);
    frame.line(up(Line::from_points(control[1], control[2])), INK, 1.5, true);

    for (const std::array<Point, 2>& drop : std::array<std::array<Point, 2>, 3>{{{trimmed, control[0]}, {middle, control[1]}, {end, control[2]}}})
        frame.line(up(Line::from_points(drop[0], drop[1])), INK, 1.0, true);

    dotted(frame, soffit, FAMILY_COLORS[0], 4.0);
    frame.line(up(Line::from_points(start, trimmed)), MARK, 5.0);

    frame.label("start = quad.get_point(0)", up(start));
    frame.label("trimmed", up(trimmed));
    frame.label("middle", up(middle));
    frame.label("end = quad.get_point(1)", up(end));
    frame.label(fmt::format("run_in = wedge = {:.0f}", parameters.wedge), up(between(start, trimmed, 0.5)));
    frame.label(fmt::format("trimmed - height: z {:.0f}", -parameters.height), up(control[0]));
    frame.label("control: middle - static_h()", up(control[1]));
    frame.label(fmt::format("end - static_h(): z {:.0f}", -parameters.static_h()), up(control[2]));
    frame.write(context.dir);
}

/// 41: fan_end on outer rib 0: the first chord extended past the fan plane, cut there, the Bezier start dropped.
void fan_end_cut(const Context& context) {

    const FloorParameters& parameters = context.guide.parameters;
    const ConstructionPlanes& cp = context.guide.geometry[0].planes;
    const Polyline& quad = context.guide.geometry[0].quads.outer_ribs[0];
    const std::array<Plane, 2> ends = rib_ends(cp, 0);
    const Polyline soffit = outer_parabola(quad, parameters.wedge, parameters);
    const Point first = soffit.get_point(0);
    const Point second = soffit.get_point(1);
    const Point pushed = first + (first - second).normalized() * EXTENSION;
    const Point hit = fan_hit(quad, parameters.wedge, ends[0], ends[1], parameters);
    Frame frame(CHAPTER, 41, "fan_end", "fan_end: trim extends the first chord by EXTENSION and cuts it on the fan plane; the Bezier start drops", "front", FAN_END);
    frame.key = true;

    frame.polyline(up(soffit), GREY, 3.0);
    frame.point(up(first), GREY, 16.0);
    frame.line(up(Line::from_points(second, pushed)), INK, 1.5, true);
    frame.line(up(face_trace(ends[0], cp.outer_ribs[0][0], -760.0)), FAMILY_COLORS[3], 3.0);
    frame.polyline(up(trim(soffit, ends[0], ends[1])), FAMILY_COLORS[0], 4.0);
    frame.point(up(hit), MARK, 16.0);

    frame.label(fmt::format("dropped: Bezier start ({:.0f}, {:.0f})", first[0], first[2]), up(first));
    frame.label("outer_parabola(quads.outer_ribs[0], wedge)", up(second));
    frame.label(fmt::format("first chord extended, EXTENSION = {:.0f}", EXTENSION), up(between(first, pushed, 0.06)));
    frame.label("fan plane wedges[0][0]", up(plane_plane_plane(level(-480.0), ends[0], cp.outer_ribs[0][0]).value()));
    frame.label(fmt::format("fan_end = {:.2f} at x {:.1f}", hit[2], hit[0]), up(hit));
    frame.write(context.dir);
}

/// 42: both outer ribs' fan_end at the trial run-in, their end faces on the fan planes and the corner's shared level.
void shared_level(const Context& context) {

    const FloorParameters& parameters = context.guide.parameters;
    const QuarterGeometry& geometry = context.guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const Point corner = context.guide.columns[0].corner;
    std::array<double, 2> ends_z = {0.0, 0.0};
    Frame frame(CHAPTER, 42, "shared_level", "run_ins: level = max of both outer ribs' fan_end at run_in = wedge, the shallower end of the corner", "iso", FAN);

    for (size_t k = 0; k < 2; k++) {
        const std::array<Plane, 2> ends = rib_ends(cp, k);
        const Line outer = face_trace(ends[0], cp.outer_ribs[k][0], -760.0);
        const Line inner = face_trace(ends[0], cp.outer_ribs[k][1], -760.0);
        const Point hit = fan_hit(geometry.quads.outer_ribs[k], parameters.wedge, ends[0], ends[1], parameters);
        ends_z[k] = hit[2];
        frame.polyline(up(Polyline({outer.start(), outer.end(), inner.end(), inner.start()}).closed()), GREY, 2.0);
        frame.polyline(up(trim(outer_parabola(geometry.quads.outer_ribs[k], parameters.wedge, parameters), ends[0], ends[1])), GREY, 3.0);
        frame.point(up(hit), MARK, 16.0);
        frame.label(fmt::format("fan_end(quads.outer_ribs[{}]) = {:.2f}", k, hit[2]), up(hit));
        frame.label(fmt::format("fan plane wedges[{}][0]", k == 0 ? 0 : 2), up(outer.start()));
    }

    const double level_z = std::max(ends_z[0], ends_z[1]);
    const std::vector<Point> slice = {
        Point(corner[0] - 50.0, corner[1] - 50.0, level_z),
        Point(corner[0] + 600.0, corner[1] - 50.0, level_z),
        Point(corner[0] + 600.0, corner[1] + 600.0, level_z),
        Point(corner[0] - 50.0, corner[1] + 600.0, level_z),
    };
    frame.polyline(up(Polyline(slice).closed()), MARK, 1.5);
    frame.label(fmt::format("level = max(fan_end 0, fan_end 1) = {:.2f}", level_z), up(slice[2]));
    frame.write(context.dir);
}

/// A point of the f(x) graph drawn in a rib's plane: run-in x along the rib axis from x = GRAPH_RUN_IN[0], f = fan_end - level up from the level.
Point graph_point(const Point& origin, const Vector& along, double x, double f) {
    return origin + along * ((x - GRAPH_RUN_IN[0]) * GRAPH_SCALE[0]) + Vector(0.0, 0.0, f * GRAPH_SCALE[1]);
}

/// 43: run_in_to_level's secant on the 3000 x 2400 bay's deeper outer rib: soffits at x0, x1 and the root, each cut on its fan plane, and f(x) with the first secant line beside them.
void secant(const Context& context) {

    const FloorGuide& guide = context.tied;
    const FloorParameters& parameters = guide.parameters;
    const QuarterGeometry& geometry = guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const size_t k = geometry.run_in[0] < geometry.run_in[1] ? 0 : 1;
    const Polyline& quad = geometry.quads.outer_ribs[k];
    const std::array<Plane, 2> ends = rib_ends(cp, k);
    const std::array<Plane, 2> other = rib_ends(cp, 1 - k);
    const double level_z = std::max(fan_hit(quad, parameters.wedge, ends[0], ends[1], parameters)[2], fan_hit(geometry.quads.outer_ribs[1 - k], parameters.wedge, other[0], other[1], parameters)[2]);
    const std::array<double, 3> runs = {parameters.wedge, parameters.wedge + 1.0, geometry.run_in[k]};

    const Point start = quad.get_point(0);
    const Vector along = (quad.get_point(1) - start).normalized();
    const Point near = Point(start[0], start[1], level_z) + along * -150.0;
    const Point far = Point(start[0], start[1], level_z) + along * 650.0;
    const Point graph = Point(start[0], start[1], level_z) + along * 800.0;
    const Point graph_end = graph_point(graph, along, GRAPH_RUN_IN[1], 0.0);
    const Box box = {std::min(near[0], graph_end[0]) - 60.0, std::min(near[1], graph_end[1]) - 60.0, H - 820.0, std::max(near[0], graph_end[0]) + 60.0, std::max(near[1], graph_end[1]) + 60.0, H - 450.0};
    Frame frame(CHAPTER, 43, "secant", "run_in_to_level, 3000 x 2400: soffits at x0 = wedge, x1 = wedge + 1 and x*; right, f(x) and the secant", k == 0 ? "front" : "right", box);

    frame.line(up(face_trace(ends[0], cp.outer_ribs[k][0], -760.0)), GREY, 3.0);
    frame.line(up(Line::from_points(near, far)), MARK, 1.5, true);
    std::array<Point, 3> hits;
    std::array<Point, 3> starts;

    for (size_t i = 0; i < 3; i++) {
        const Polyline soffit = outer_parabola(quad, runs[i], parameters);
        hits[i] = fan_hit(quad, runs[i], ends[0], ends[1], parameters);
        starts[i] = soffit.get_point(0);
        frame.polyline(up(trim(soffit, ends[0], ends[1])), i == 2 ? FAMILY_COLORS[0] : GREY, i == 2 ? 4.0 : 2.0);
        frame.point(up(starts[i]), INK, 8.0);
        frame.point(up(hits[i]), i == 2 ? MARK : INK, 14.0);
    }

    frame.label(fmt::format("x0 = {:.0f}, x1 = {:.0f}: f0 = {:.3f}, f1 = {:.3f}", runs[0], runs[1], hits[0][2] - level_z, hits[1][2] - level_z), up(hits[0]));
    frame.label(fmt::format("x* = run_in[{}] = {:.3f}: f = 0", k, runs[2]), up(hits[2]));
    frame.label(fmt::format("level = {:.3f}", level_z), up(far));
    frame.label(fmt::format("fan plane wedges[{}][0]", k == 0 ? 0 : 2), up(plane_plane_plane(level(-480.0), ends[0], cp.outer_ribs[k][0]).value()));
    frame.label("Bezier start at x0", up(starts[0]));
    frame.label("Bezier start at x*", up(starts[2]));

    std::vector<Point> curve;

    for (double x = GRAPH_RUN_IN[0]; x <= GRAPH_RUN_IN[1] + 1e-9; x += 2.5)
        curve.push_back(graph_point(graph, along, x, fan_hit(quad, x, ends[0], ends[1], parameters)[2] - level_z));

    const double f0 = hits[0][2] - level_z;
    const double f1 = hits[1][2] - level_z;
    const double x2 = runs[1] - f1 * (runs[1] - runs[0]) / (f1 - f0);
    frame.line(up(Line::from_points(graph, graph_end)), GREY, 1.5);
    frame.polyline(up(Polyline(curve)), INK, 2.5);
    frame.line(up(Line::from_points(graph_point(graph, along, runs[0], f0), graph_point(graph, along, x2, 0.0))), MARK, 1.5, true);
    frame.point(up(graph_point(graph, along, runs[0], f0)), INK, 10.0);
    frame.point(up(graph_point(graph, along, runs[1], f1)), INK, 10.0);
    frame.point(up(graph_point(graph, along, x2, 0.0)), MARK, 12.0);
    frame.point(up(graph_point(graph, along, runs[2], 0.0)), FAMILY_COLORS[0], 12.0);
    frame.label("f(x) = fan_end(x) - level", up(curve.back()));
    frame.label(fmt::format("secant through x0, x1: x2 = {:.2f}", x2), up(graph_point(graph, along, x2, 0.0)));
    frame.write(context.dir);
}

/// 44: block_planes on the 3000 x 2400 bay: the provisional far faces dashed, the final ones over the run-ins, each thickness along its fan normal.
void block_far_faces(const Context& context) {

    const FloorGuide& guide = context.tied;
    const FloorParameters& parameters = guide.parameters;
    const QuarterGeometry& geometry = guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const std::array<double, 2>& run_in = geometry.run_in;
    const std::array<double, 3> thickness = {run_in[0], parameters.middle_wedge_factor * (0.5 * (run_in[0] + run_in[1])), run_in[1]};
    const std::array<std::string, 3> names = {"thickness[0] = run_in[0]", "thickness[1] = middle_wedge_factor x mean", "thickness[2] = run_in[1]"};
    const std::vector<Polyline> provisional = first_pass_wedges(guide);
    const std::vector<Polyline>& blocks = geometry.quads.wedges;
    const Point corner = guide.columns[0].corner;
    const Box box = {corner[0] - 50.0, corner[1] - 50.0, H - 10.0, corner[0] + 550.0, corner[1] + 550.0, H + 10.0};
    Frame frame(CHAPTER, 44, "block_planes", "block_planes, 3000 x 2400 bay: far faces moved to the solved run-ins; the provisional ones dashed grey", "top", box);

    draw_quads(frame, geometry.quads.outer_ribs, GREY, 2.0);
    draw_quads(frame, geometry.quads.inner_ribs, GREY, 2.0);
    frame.polyline(up(Polyline(guide.columns[0].head).closed()), GREY, 2.0);
    draw_quads(frame, blocks, FAMILY_COLORS[3], 2.0);

    for (size_t i = 0; i < 3; i++) {
        side_trace(frame, provisional[i], 2, 60.0, GREY);
        frame.line(up(Line::from_points(blocks[i].get_point(2), blocks[i].get_point(3))), FAMILY_COLORS[3], 5.0);
        const Point origin = cp.wedges[i][0].origin();
        const Vector normal = cp.wedges[i][0].z_axis();
        const Vector plan = Vector(normal[0], normal[1], 0.0).normalized();
        const Point end = line_plane(Line::from_points(origin, origin + plan), cp.wedges[i][1]).value();
        frame.line(up(Line::from_points(origin, end)), MARK, 3.0, false, true);
        frame.label(fmt::format("{} = {:.3f}", names[i], thickness[i]), up(between(origin, end, 0.5)));
    }

    frame.label(fmt::format("provisional: wedge x middle_wedge_factor = {:.0f}", parameters.wedge * parameters.middle_wedge_factor), up(between(provisional[1].get_point(2), provisional[1].get_point(3), 0.2)));
    frame.write(context.dir);
}

/// 45: construction_quads run again: the wedges and tsections quads on the final far faces, every other quad unchanged.
void second_pass(const Context& context) {

    const ConstructionQuads& quads = context.guide.geometry[0].quads;
    Frame frame(CHAPTER, 45, "second_pass", "construction_quads again after block_planes: only the wedges and tsections quads read the far faces", "top", QUARTER);

    draw_quads(frame, quads.outer_ribs, GREY, 1.5);
    draw_quads(frame, quads.inner_beams, GREY, 1.5);
    draw_quads(frame, quads.inner_ribs, GREY, 1.5);
    draw_quads(frame, quads.wedges, FAMILY_COLORS[3], 3.5);
    draw_quads(frame, quads.tsections, FAMILY_COLORS[4], 3.5);

    frame.label("rebuilt: quads.wedges", up(area_centroid(quads.wedges[1].closed())));
    frame.label("rebuilt: quads.tsections", up(between(quads.tsections[0].get_point(0), quads.tsections[0].get_point(1), 0.4)));
    frame.label("unchanged: outer_ribs, inner_beams, inner_ribs", up(area_centroid(quads.inner_beams[1].closed())));
    frame.write(context.dir);
}

/// 46: boundary_parabolas: both outer soffits at the solved run-ins, hung under their plan quads.
void boundary_soffits(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const FloorParameters& parameters = context.guide.parameters;
    Frame frame(CHAPTER, 46, "boundary_parabolas", "boundary_parabolas: outer_parabola on the final quads at the solved run_in: parabolas[0][0], [1][0]", "iso", QUARTER);
    all_quads_grey(frame, geometry.quads);

    for (size_t k = 0; k < 2; k++) {
        const Polyline& soffit = geometry.parabolas[k][0];
        const Point first = soffit.get_point(0);
        const Point last = soffit.get_point(soffit.point_count() - 1);
        dotted(frame, soffit, FAMILY_COLORS[0], 4.0);
        frame.line(up(Line::from_points(first, Point(first[0], first[1], 0.0))), INK, 1.5, true);
        frame.line(up(Line::from_points(last, Point(last[0], last[1], 0.0))), INK, 1.5, true);
        frame.label(fmt::format("parabolas[{}][0]", k), up(soffit.get_point(3)));
        frame.label(fmt::format("run_in[{}] = {:.0f}: z {:.0f}", k, geometry.run_in[k], -parameters.height), up(first));
    }

    frame.label(fmt::format("seam: z {:.0f} = -static_h()", -parameters.static_h()), up(geometry.parabolas[0][0].get_point(geometry.parabolas[0][0].point_count() - 1)));
    frame.write(context.dir);
}

/// 47: offset_polyline on outer rib 0: the segment planes moved by tsections, the start cap and the mitre points of the layers.
void layers(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const double t = context.guide.parameters.tsections;
    const std::array<Polyline, 3>& layer = geometry.parabolas[0];
    const std::vector<Point> pts = layer[0].get_points();
    const Vector z = Vector::z_axis();
    std::array<Vector, 2> normals;
    Frame frame(CHAPTER, 47, "layers", "offset_polyline: each segment's plane moved up by t, cut with the next and the base plane; caps square", "front", LAYERS);

    frame.polyline(up(layer[0]), GREY, 4.0);
    frame.polyline(up(layer[1]), FAMILY_COLORS[4], 3.0);
    frame.polyline(up(layer[2]), FAMILY_COLORS[5], 3.0);

    for (size_t i = 0; i < 2; i++) {
        const Vector x = (pts[i + 1] - pts[i]).normalized();
        normals[i] = x.cross(z.cross(x)).normalized();
        frame.line(up(Line::from_points(pts[i] + normals[i] * t + x * -150.0, pts[i + 1] + normals[i] * t + x * 150.0)), INK, 1.5, true);
    }

    const Point cap = pts[0] + normals[0] * (2.0 * t + 25.0);
    const Point centre = between(pts[1], pts[2], 0.5);
    frame.line(up(Line::from_points(pts[0] + normals[0] * -20.0, cap)), INK, 1.5, true);
    frame.line(up(Line::from_points(centre, centre + normals[1] * t)), MARK, 3.0, false, true);
    frame.point(up(layer[1].get_point(1)), MARK, 14.0);
    frame.point(up(layer[1].get_point(0)), INK, 10.0);

    frame.label("parabolas[0][0]: soffit", up(pts[2]));
    frame.label("parabolas[0][1]: offset by tsections", up(between(layer[1].get_point(0), layer[1].get_point(1), 0.5)));
    frame.label("parabolas[0][2]: offset by 2 x tsections", up(layer[2].get_point(2)));
    frame.label("mitre: ppp(planes[1], planes[2], base)", up(layer[1].get_point(1)));
    frame.label("start cap: normal pts[1] - pts[0]", up(cap));
    frame.label(fmt::format("tsections = {:.0f}", t), up(centre + normals[1] * (0.5 * t)));
    frame.write(context.dir);
}

/// 48: the shadow parabolas: each outer rib's layers slid along its normal onto its inner rib's outer face.
void shadows(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    Frame frame(CHAPTER, 48, "shadows", "project_to_plane_by_axis: outer layers slid along outer_ribs[i][0]'s normal onto inner_ribs[i][0]", "iso", QUARTER);
    frame.key = true;
    draw_quads(frame, geometry.quads.outer_ribs, GREY, 1.5);
    draw_quads(frame, geometry.quads.inner_ribs, GREY, 1.5);

    for (size_t i = 0; i < 2; i++) {
        frame.polyline(up(geometry.parabolas[i][0]), GREY, 3.0);
        frame.polyline(up(geometry.parabolas[2 + i][0]), FAMILY_COLORS[1], 5.0);
        frame.polyline(up(geometry.parabolas[2 + i][1]), FAMILY_COLORS[1], 1.5);
        frame.polyline(up(geometry.parabolas[2 + i][2]), FAMILY_COLORS[1], 1.5);

        for (const size_t j : {size_t(0), size_t(3), size_t(6)})
            frame.line(up(Line::from_points(geometry.parabolas[i][0].get_point(j), geometry.parabolas[2 + i][0].get_point(j))), INK, 1.5, true, true);

        frame.label(fmt::format("parabolas[{}][0]", i), up(geometry.parabolas[i][0].get_point(3)));
        frame.label(fmt::format("parabolas[{}]: shadow on inner_ribs[{}][0]", 2 + i, i), up(geometry.parabolas[2 + i][0].get_point(6)));
    }

    frame.label("along outer_ribs[0][0].z_axis()", up(between(geometry.parabolas[0][0].get_point(6), geometry.parabolas[2][0].get_point(6), 0.5)));
    frame.write(context.dir);
}

}

void chapter_03_parabolas(const Context& context) {
    outer_rib_quads(context);
    inner_beam_quads(context);
    inner_rib_quads(context);
    wedge_quads(context);
    tsection_quads(context);
    outer_parabola_trial(context);
    fan_end_cut(context);
    shared_level(context);
    secant(context);
    block_far_faces(context);
    second_pass(context);
    boundary_soffits(context);
    layers(context);
    shadows(context);
}

}
