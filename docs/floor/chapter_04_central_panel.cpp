#include "docs/floor/movie.h"

namespace movie {

namespace {

const std::string CHAPTER = "04_central_panel";
const double SCAN_STEP = 0.5; // As floor_panel.cpp's SCAN_STEP, degrees between scanned sweeps.
const double SCAN_RANGE = 85.0; // As floor_panel.cpp's SCAN_RANGE, degrees either side of the reference.
const double GRAZING = 1e-3; // As floor_panel.cpp's GRAZING, the smallest |n . r| scanned.
const double TRIAL = 10.0; // Degrees of the wrong trial sweep the closure frame shows.
const Box PANEL_PLAN = {-3050.0, -3050.0, H - 700.0, 50.0, 50.0, H + 20.0}; // Quarter 0 in plan.
const Box PANEL_3D = {-2850.0, -2850.0, H - 680.0, -20.0, -20.0, H + 20.0}; // The central panel in 3D.
const std::array<Color, 3> SIDE_TINTS = {
    Color(0.55f, 0.75f, 0.95f, 1.0f, "sides_true_true"),
    Color(0.72f, 0.86f, 0.52f, 1.0f, "sides_true_false"),
    Color(0.98f, 0.76f, 0.48f, 1.0f, "sides_false_false"),
};

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The vector in plan, z dropped, as floor_panel.cpp's flat.
Vector flat(const Vector& vector) {
    return Vector(vector[0], vector[1], 0.0);
}

/// The plan unit vector turned by degrees about z from the reference, as floor_panel.cpp's turned.
Vector turned(const Vector& reference, double degrees) {
    return flat(reference).normalized().transformed(Xform::rotation_z(degrees, true));
}

/// Quarter 0's central faces, cp.inner_ribs[k][1].
std::array<Plane, 2> central_faces(const QuarterGeometry& geometry) {
    return {geometry.planes.inner_ribs[0][1], geometry.planes.inner_ribs[1][1]};
}

/// The central faces' normals, as central_panel reads them.
std::array<Vector, 2> face_normals(const QuarterGeometry& geometry) {
    const std::array<Plane, 2> faces = central_faces(geometry);
    return {faces[0].z_axis(), faces[1].z_axis()};
}

/// The shadow of outer rib k on inner rib k's outer face, parabolas[2 + k][0].
const Polyline& shadow(const QuarterGeometry& geometry, size_t k) {
    return geometry.parabolas[2 + k][0];
}

/// The scan's 0 degrees, flat(normals[0] - normals[1]).normalized().
Vector reference(const QuarterGeometry& geometry) {
    const std::array<Vector, 2> normals = face_normals(geometry);
    return flat(normals[0] - normals[1]).normalized();
}

/// The two shifts along r, thickness / (n . r), as floor_panel.cpp's shifts.
std::array<double, 2> shifts(const std::array<Vector, 2>& normals, double thickness, const Vector& r) {
    return {thickness / normals[0].dot(r), thickness / normals[1].dot(r)};
}

/// Whether the sweep crosses both faces clear of GRAZING, and the side of each, as floor_panel.cpp's sweep_sides.
bool sweep_sides(const std::array<Vector, 2>& normals, const Vector& r, std::array<bool, 2>& sides) {
    sides = {normals[0].dot(r) > 0.0, normals[1].dot(r) > 0.0};
    return std::abs(normals[0].dot(r)) > GRAZING && std::abs(normals[1].dot(r)) > GRAZING;
}

/// The point dropped to the datum, z 0.
Point plan(const Point& point) {
    return Point(point[0], point[1], 0.0);
}

/// The plan arc about a centre from one direction to another, radius mm, in steps of about two degrees.
Polyline arc(const Point& centre, const Vector& from, const Vector& to, double radius) {

    const double angle = flat(from).angle(flat(to), true, true);
    const size_t steps = std::max<size_t>(2, static_cast<size_t>(std::ceil(std::abs(angle) / 2.0)));
    std::vector<Point> points;

    for (size_t i = 0; i <= steps; i++)
        points.push_back(centre + turned(from, angle * static_cast<double>(i) / static_cast<double>(steps)) * radius);

    return Polyline(points);
}

/// The middle of the arc's sweep at the radius.
Point arc_middle(const Point& centre, const Vector& from, const Vector& to, double radius) {
    return centre + turned(from, 0.5 * flat(from).angle(flat(to), true, true)) * radius;
}

/// The camera box around guide points, padded by pad mm and lifted to the floor.
Box around(const std::vector<Point>& points, double pad) {

    const double inf = std::numeric_limits<double>::infinity();
    Box box = {inf, inf, inf, -inf, -inf, -inf};

    for (const Point& point : points)
        for (int i = 0; i < 3; i++) {
            box[i] = std::min(box[i], point[i] - pad);
            box[3 + i] = std::max(box[3 + i], point[i] + pad);
        }

    box[2] += H;
    box[5] += H;

    return box;
}

/// The plan line of a vertical face through a point, half mm either side.
Line face_line(const Point& through, const Vector& normal, double half) {
    const Vector along = Vector::z_axis().cross(normal).normalized();
    return Line::from_points(through - along * half, through + along * half);
}

/// The same-index chord from rib 0's polyline to rib 1's.
Line chord(const Polyline& rib0, const Polyline& rib1, size_t i) {
    return Line::from_points(rib0.get_point(i), rib1.get_point(i));
}

/// Quarter 0's inner rib quads at the datum and the column head, grey.
void rib_bands(Frame& frame, const FloorGuide& guide) {

    for (const Polyline& quad : guide.geometry[0].quads.inner_ribs)
        frame.polyline(up(quad.closed()), GREY, 2.0);

    frame.polyline(up(Polyline(guide.columns[0].head).closed()), GREY, 2.0);
}

/// The panel's normal cross-section and its two offsets, as section_layers builds them.
std::array<Polyline, 3> section_curves(const CentralPanel& panel, double tsections) {

    const Polyline& soffit = panel.traces[0][0];
    const Plane plane = Plane::from_point_normal(soffit.get_point(0), panel.ruling);
    const Polyline section = soffit.transformed(Xform::project_to_plane_by_axis(plane, panel.ruling));

    return {section, offset_polyline(section, tsections), offset_polyline(section, 2.0 * tsections)};
}

/// A bed panel's two face curves trimmed by its planes, reversed so index 0 is the deepest, as panel_top_plane reads them.
std::array<std::vector<Point>, 2> deepest_first(const std::array<Polyline, 2>& faces, const Plane& cut_plane0, const Plane& cut_plane1) {

    std::array<std::vector<Point>, 2> pts = {trim(faces[0], cut_plane0, cut_plane1).get_points(), trim(faces[1], cut_plane0, cut_plane1).get_points()};

    if (pts[0].front()[2] > pts[0].back()[2]) {
        std::reverse(pts[0].begin(), pts[0].end());
        std::reverse(pts[1].begin(), pts[1].end());
    }

    return pts;
}

/// The deepest quad of a bed panel: pts[0][0], pts[0][1], pts[1][1], pts[1][0], closed.
Polyline deepest_quad(const std::array<std::vector<Point>, 2>& pts) {
    return Polyline({pts[0][0], pts[0][1], pts[1][1], pts[1][0], pts[0][0]});
}

/// Degrees between a plane's normal and the world z.
double tilt(const Plane& plane) {
    return std::acos(std::clamp(plane.z_axis()[2], -1.0, 1.0)) * 180.0 / M_PI;
}

/// Bed panel 0's +2t layer projected along outer rib 0's normal onto its two side faces: inner_ribs[0][0] (side00) and outer_ribs[0][1] (side01).
std::array<Polyline, 2> side_faces(const QuarterGeometry& geometry) {

    const ConstructionPlanes& cp = geometry.planes;
    const Vector axis = cp.outer_ribs[0][0].z_axis();
    const Polyline& top = geometry.parabolas[0][2];

    return {top.transformed(Xform::project_to_plane_by_axis(cp.inner_ribs[0][0], axis)), top.transformed(Xform::project_to_plane_by_axis(cp.outer_ribs[0][1], axis))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Frames
// ═══════════════════════════════════════════════════════════════════════════

/// 49: the shadows in, the central soffit traces, three rulings and the sweep out.
void panel_inputs_outputs(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    Frame frame(CHAPTER, 49, "panel_inputs_outputs", "central_panel(): the shadows on the outer faces in; one rib_sweep, one ruling and the central traces out", "iso", PANEL_3D);
    frame.key = true;
    frame.distance = 0.8;
    rib_bands(frame, context.guide);

    for (size_t k = 0; k < 2; k++) {
        frame.polyline(up(shadow(geometry, k)), GREY, 3.0);
        frame.polyline(up(panel.traces[k][0]), FAMILY_COLORS[1], 5.0);
        frame.polyline(up(panel.traces[k][1]), FAMILY_COLORS[4], 2.0);
        frame.polyline(up(panel.traces[k][2]), FAMILY_COLORS[5], 2.0);
    }

    const size_t last = panel.traces[0][0].point_count() - 1;

    for (const size_t i : {size_t(0), last / 2, last})
        frame.line(up(chord(panel.traces[0][0], panel.traces[1][0], i)), MARK, 3.0);

    frame.line(up(Line::from_points(shadow(geometry, 0).get_point(0), panel.traces[0][0].get_point(0))), MARK, 5.0, false, true);
    frame.label("parabolas[2][0]", up(shadow(geometry, 0).get_point(2)));
    frame.label("parabolas[3][0]", up(shadow(geometry, 1).get_point(2)));
    frame.label("traces[0][0]", up(panel.traces[0][0].get_point(5)));
    frame.label("traces[1][0]", up(panel.traces[1][0].get_point(5)));
    frame.label("ruling", up(chord(panel.traces[0][0], panel.traces[1][0], last / 2).center()));
    frame.label("rib_sweep", up(shadow(geometry, 0).get_point(0)));
    frame.write(context.dir);
}

/// 50: the two face normals and the reference halfway between normals[0] and -normals[1].
void reference_direction(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const std::array<Vector, 2> normals = face_normals(geometry);
    const Vector ref = reference(geometry);
    const Point centre = plan(Point::centroid({shadow(geometry, 0).get_point(0), shadow(geometry, 1).get_point(0)}));
    const double length = 220.0;
    const double radius = 140.0;
    Frame frame(CHAPTER, 50, "reference", "reference = flat(normals[0] - normals[1]).normalized(): equally oblique to both central faces", "top", around({centre, centre + normals[0] * length, centre + normals[1] * length, centre - normals[1] * length, centre + ref * length * 1.2}, 70.0));
    rib_bands(frame, context.guide);

    frame.line(up(Line::from_points(centre, centre + normals[0] * length)), FAMILY_COLORS[1], 3.0, false, true);
    frame.line(up(Line::from_points(centre, centre + normals[1] * length)), FAMILY_COLORS[1], 3.0, false, true);
    frame.line(up(Line::from_points(centre, centre - normals[1] * length * 0.8)), INK, 2.0, true, true);
    frame.line(up(Line::from_points(centre, centre + ref * length * 1.2)), MARK, 5.0, false, true);
    frame.polyline(up(arc(centre, ref, normals[0], radius)), INK, 2.0);
    frame.polyline(up(arc(centre, ref, -normals[1], radius)), INK, 2.0);

    frame.label("normals[0]", up(centre + normals[0] * length));
    frame.label("normals[1]", up(centre + normals[1] * length));
    frame.label("-normals[1]", up(centre - normals[1] * length * 0.8));
    frame.label("reference", up(centre + ref * length * 1.2));
    frame.label(fmt::format("{:.2f} deg", std::abs(ref.angle(normals[0], true, true))), up(arc_middle(centre, ref, normals[0], radius)));
    frame.label(fmt::format("{:.2f} deg", std::abs(ref.angle(-normals[1], true, true))), up(arc_middle(centre, ref, -normals[1], radius)));
    frame.write(context.dir);
}

/// 51: the fan of trial sweeps, every tenth of the 0.5 degree steps drawn, the unscanned 85..90 degree wedges grey.
void scan_fan(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const ColumnCorner& column = context.guide.columns[0];
    const Vector ref = reference(geometry);
    const Point centre = Point::centroid({column.head[2], column.head[3]});
    const double radius = 150.0;
    size_t intervals = 0;

    for (double lo = -SCAN_RANGE; lo < SCAN_RANGE; lo += SCAN_STEP)
        intervals++;

    Frame frame(CHAPTER, 51, "scan_fan", fmt::format("rib_sweep scans r = turned(reference, lo) for lo = -85 .. 84.5 in steps of 0.5: {} intervals", intervals), "top", around({centre}, 175.0));
    frame.polyline(up(Polyline(column.head).closed()), GREY, 2.0);
    frame.polyline(up(Polyline::from_sides(72, radius, true).translated(centre - Point(0.0, 0.0, 0.0))), INK, 1.0);

    for (double degrees = -SCAN_RANGE; degrees <= SCAN_RANGE + 1e-9; degrees += 10.0 * SCAN_STEP)
        frame.line(up(Line::from_points(centre, centre + turned(ref, degrees) * radius)), INK, 1.0);

    for (double degrees = SCAN_RANGE + 1.0; degrees <= 90.0 + 1e-9; degrees += 1.0) {
        frame.line(up(Line::from_points(centre, centre + turned(ref, degrees) * radius)), GREY, 2.0);
        frame.line(up(Line::from_points(centre, centre + turned(ref, -degrees) * radius)), GREY, 2.0);
    }

    frame.line(up(Line::from_points(centre, centre + ref * radius * 1.15)), MARK, 4.0, false, true);
    frame.label("reference: 0 deg", up(centre + ref * radius * 1.15));
    frame.label(fmt::format("first lo = -SCAN_RANGE = {:.0f}", -SCAN_RANGE), up(centre + turned(ref, -SCAN_RANGE) * radius));
    frame.label(fmt::format("last hi = SCAN_RANGE = {:.0f}", SCAN_RANGE), up(centre + turned(ref, SCAN_RANGE) * radius));
    frame.label(fmt::format("SCAN_STEP = {:.1f}, one ray in 10 drawn", SCAN_STEP), up(centre + turned(ref, 45.0) * radius));
    frame.label("85..90 deg not scanned", up(centre + turned(ref, -88.0) * radius * 0.8));
    frame.write(context.dir);
}

/// 52: every scanned interval as a ray tinted by its sides, the two that change side red, the face directions dashed.
void skip_intervals(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const ColumnCorner& column = context.guide.columns[0];
    const std::array<Vector, 2> normals = face_normals(geometry);
    const Vector ref = reference(geometry);
    const Point centre = Point::centroid({column.head[2], column.head[3]});
    const double radius = 150.0;
    Frame frame(CHAPTER, 52, "skip_intervals", "sweep_sides: skip an interval where r grazes a face or changes side; red where r is parallel to a face", "top", around({centre}, 185.0));
    frame.polyline(up(Polyline(column.head).closed()), GREY, 2.0);
    std::vector<double> skipped;

    for (double lo = -SCAN_RANGE; lo < SCAN_RANGE; lo += SCAN_STEP) {
        std::array<bool, 2> sides_lo;
        std::array<bool, 2> sides_hi;
        const bool clear = sweep_sides(normals, turned(ref, lo), sides_lo) && sweep_sides(normals, turned(ref, lo + SCAN_STEP), sides_hi);
        const Vector r = turned(ref, lo + 0.5 * SCAN_STEP);

        if (!clear || sides_lo != sides_hi) {
            skipped.push_back(lo);
            continue;
        }

        const size_t tint = sides_lo[0] && sides_lo[1] ? 0 : sides_lo[0] ? 1 : 2;
        frame.line(up(Line::from_points(centre, centre + r * radius)), SIDE_TINTS[tint], 1.0);
    }

    for (size_t k = 0; k < 2; k++) {
        const Vector along = Vector::z_axis().cross(normals[k]).normalized();
        const Vector scanned = along.dot(ref) > 0.0 ? along : -along;
        frame.line(up(Line::from_points(centre - scanned * radius * 1.2, centre + scanned * radius * 1.2)), FAMILY_COLORS[1], 2.0, true);
        frame.label(fmt::format("r parallel to faces[{}]", k), up(centre + scanned * radius * 1.2));
    }

    // The skipped rays lie within 0.05 degrees of the face directions: drawn last and wider, so the dashed face lines do not hide them.
    for (const double lo : skipped) {
        const Vector r = turned(ref, lo + 0.5 * SCAN_STEP);
        frame.line(up(Line::from_points(centre, centre + r * radius * 1.1)), MARK, 5.0);
        frame.label(fmt::format("skipped [{:.1f}, {:.1f}]", lo, lo + SCAN_STEP), up(centre + r * radius * 0.55));
    }

    frame.label("sides = {true, true}", up(centre + turned(ref, -82.5) * radius * 0.85));
    frame.label("sides = {true, false}", up(centre + turned(ref, 20.0) * radius * 0.75));
    frame.label("sides = {false, false}", up(centre + turned(ref, 82.5) * radius * 0.85));
    frame.write(context.dir);
}

/// 53: one shadow point moved along r by a[0] until it reaches the central face 60 further along normals[0].
void shift_frame(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const std::array<Vector, 2> normals = face_normals(geometry);
    const double thickness = context.guide.parameters.inner_ribs;
    const std::array<double, 2> a = shifts(normals, thickness, panel.rib_sweep);
    const Point start = shadow(geometry, 0).get_point(0);
    const Point hit = panel.traces[0][0].get_point(0);
    const Point foot = central_faces(geometry)[0].project(start);
    const Vector along = Vector::z_axis().cross(normals[0]).normalized();
    Frame frame(CHAPTER, 53, "shifts", fmt::format("shifts: a = thickness / (n . r); a[0] = {:.2f}, a[1] = {:.2f} at the final r", a[0], a[1]), "top", around({start, hit}, 80.0));
    frame.distance = 0.75;

    frame.line(up(face_line(start, normals[0], 100.0)), GREY, 3.0);
    frame.line(up(face_line(foot, normals[0], 100.0)), GREY, 3.0);
    frame.line(up(Line::from_points(start, foot)), INK, 2.0, true);
    frame.line(up(Line::from_points(start, hit)), MARK, 3.0, false, true);
    frame.polyline(up(arc(start, normals[0], panel.rib_sweep, 40.0)), INK, 2.0);
    frame.point(up(start), INK);
    frame.point(up(hit), MARK);

    frame.label("shadows[0] pt 0", up(start));
    frame.label("soffits[0] pt 0", up(hit));
    frame.label("cp.inner_ribs[0][0]", up(start - along * 55.0));
    frame.label("faces[0] = cp.inner_ribs[0][1]", up(foot + along * 55.0));
    frame.label(fmt::format("thickness = {:.0f}", thickness), up(start + (foot - start) * 0.3));
    frame.label(fmt::format("a[0] = {:.2f}", a[0]), up(start + (hit - start) * 0.7));
    frame.label(fmt::format("{:.2f} deg", panel.obliqueness[0]), up(arc_middle(start, normals[0], panel.rib_sweep, 40.0)));
    frame.write(context.dir);
}

/// 54: a wrong trial sweep at +10 degrees, its start and vertex chords not parallel, and an inset at the root where they are.
void closure_frame(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const std::array<Vector, 2> normals = face_normals(geometry);
    const Vector r = turned(reference(geometry), TRIAL);
    const std::array<double, 2> a = shifts(normals, context.guide.parameters.inner_ribs, r);
    const std::array<Polyline, 2> shifted = {shadow(geometry, 0).translated(r * a[0]), shadow(geometry, 1).translated(r * a[1])};
    const size_t n = shifted[0].point_count() - 1;
    const Line start = chord(shifted[0], shifted[1], 0);
    const Line vertex = chord(shifted[0], shifted[1], n);
    const Vector s = flat(start.to_direction());
    const Vector v = flat(vertex.to_direction());
    const double closure = s.cross(v)[2] / (s.magnitude() * v.magnitude());
    const Point extension = vertex.start() + s.normalized() * vertex.length(); // to_direction() is unit: the length comes from the chord itself.
    Frame frame(CHAPTER, 54, "closure", fmt::format("closure at a wrong trial r = turned(reference, {:.0f}): sine from start chord to vertex chord = {:.4f}", TRIAL, closure), "top", PANEL_PLAN);

    for (size_t k = 0; k < 2; k++) {
        frame.polyline(up(shadow(geometry, k)), GREY, 2.0);
        frame.polyline(up(shifted[k]), FAMILY_COLORS[1], 3.0);
    }

    frame.line(up(start), MARK, 5.0);
    frame.line(up(vertex), MARK, 5.0);
    frame.line(up(Line::from_points(vertex.start(), extension)), INK, 2.0, true);
    frame.line(up(Line::from_points(shadow(geometry, 0).get_point(0), shadow(geometry, 0).get_point(0) + r * 250.0)), MARK, 3.0, false, true);

    const Point origin = panel.traces[0][0].get_point(0);
    const Xform inset = Xform::translation(-1300.0 - origin[0], -2850.0 - origin[1], 0.0) * Xform::scale_uniform(origin, 0.35);

    for (size_t k = 0; k < 2; k++)
        frame.polyline(up(panel.traces[k][0].transformed(inset)), FAMILY_COLORS[1], 2.0);

    frame.line(up(chord(panel.traces[0][0], panel.traces[1][0], 0).transformed(inset)), MARK, 3.0);
    frame.line(up(chord(panel.traces[0][0], panel.traces[1][0], n).transformed(inset)), MARK, 3.0);

    frame.label("start", up(start.center()));
    frame.label("vertex", up(vertex.center()));
    frame.label(fmt::format("start direction: closure = {:.4f}", closure), up(extension));
    frame.label(fmt::format("trial r, {:.0f} deg", TRIAL), up(shadow(geometry, 0).get_point(0) + r * 250.0));
    frame.label("shadows[0] + r a[0]", up(shifted[0].get_point(3)));
    frame.label("shadows[1] + r a[1]", up(shifted[1].get_point(3)));
    frame.label("inset at rib_sweep: chords parallel, closure 0", up(chord(panel.traces[0][0], panel.traces[1][0], n).transformed(inset).center()));
    frame.write(context.dir);
}

/// 57: the chosen sweep r through the middle wedge, with +r on rib 0 and -r on rib 1 at the index-0 shadow points.
void rib_sweep_frame(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const Point middle_wedge = area_centroid(geometry.quads.wedges[1].closed());
    const double signed_best = reference(geometry).angle(panel.rib_sweep, true, true);
    const double best = std::abs(signed_best) < 5e-4 ? 0.0 : signed_best; // No "-0.000" for a root on the reference.
    Frame frame(CHAPTER, 57, "rib_sweep", fmt::format("rib_sweep = turned(reference, best): the root nearest the reference, best = {:.3f} deg", best), "top", around({middle_wedge, shadow(geometry, 0).get_point(0), shadow(geometry, 1).get_point(0)}, 150.0));
    frame.distance = 0.8;
    rib_bands(frame, context.guide);
    frame.polyline(up(geometry.quads.wedges[1].closed()), GREY, 2.0);

    frame.line(up(Line::from_points(middle_wedge - panel.rib_sweep * 120.0, middle_wedge + panel.rib_sweep * 120.0)), MARK, 5.0, false, true);

    for (size_t k = 0; k < 2; k++)
        frame.line(up(Line::from_points(plan(shadow(geometry, k).get_point(0)), plan(panel.traces[k][0].get_point(0)))), MARK, 3.0, false, true);

    frame.label("panel.rib_sweep r", up(middle_wedge + panel.rib_sweep * 120.0));
    frame.label("parabolas[2][0] pt 0", up(plan(shadow(geometry, 0).get_point(0))));
    frame.label("parabolas[3][0] pt 0", up(plan(shadow(geometry, 1).get_point(0))));
    frame.label("rib 0: + r a[0]", up(plan(panel.traces[0][0].get_point(0))));
    frame.label("rib 1: - r |a[1]|", up(plan(panel.traces[1][0].get_point(0))));
    frame.write(context.dir);
}

/// 58: seven horizontal moves per rib along r from the shadows onto the central faces, the soffits thick.
void soffits_frame(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const std::array<double, 2> a = shifts(face_normals(geometry), context.guide.parameters.inner_ribs, panel.rib_sweep);
    Frame frame(CHAPTER, 58, "soffits", "along(shadows[k], faces[k], rib_sweep): each shadow point slides along r onto its central face, z kept", "iso", PANEL_3D);
    frame.distance = 0.8;
    rib_bands(frame, context.guide);

    for (size_t k = 0; k < 2; k++) {
        frame.polyline(up(shadow(geometry, k)), GREY, 3.0);
        frame.polyline(up(panel.traces[k][0]), FAMILY_COLORS[1], 5.0);

        for (size_t i = 0; i < shadow(geometry, k).point_count(); i++)
            frame.line(up(Line::from_points(shadow(geometry, k).get_point(i), panel.traces[k][0].get_point(i))), MARK, 2.0, false, true);
    }

    const size_t last = panel.traces[0][0].point_count() - 1;
    frame.label("soffits[0]", up(panel.traces[0][0].get_point(last)));
    frame.label("soffits[1]", up(panel.traces[1][0].get_point(last)));
    frame.label("shadows[0]", up(shadow(geometry, 0).get_point(2)));
    frame.label("shadows[1]", up(shadow(geometry, 1).get_point(2)));
    frame.label(fmt::format("a[0] = {:.2f} along r", a[0]), up(Line::from_points(shadow(geometry, 0).get_point(4), panel.traces[0][0].get_point(4)).center()));
    frame.label(fmt::format("a[1] = {:.2f} along r", a[1]), up(Line::from_points(shadow(geometry, 1).get_point(4), panel.traces[1][0].get_point(4)).center()));
    frame.write(context.dir);
}

/// 59: the seven same-index chords between the soffits, all parallel to the ruling, with the chamfer and the oculus edge, its label off the chord-6 midpoint it nearly covers in plan.
void ruling_frame(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const ColumnCorner& column = context.guide.columns[0];
    const FloorReport report = context.guide.check();
    const Line& oculus_edge = context.guide.oculus_edges[0].line;
    const size_t last = panel.traces[0][0].point_count() - 1;
    Frame frame(CHAPTER, 59, "ruling", "ruling u = flat(soffits[1] pt 0 - soffits[0] pt 0): all seven same-index chords run along it", "top", PANEL_PLAN);
    frame.key = true;
    rib_bands(frame, context.guide);
    frame.line(up(oculus_edge), GREY, 4.0);
    frame.line(up(Line::from_points(column.head[2], column.head[3])), GREY, 4.0);

    for (size_t k = 0; k < 2; k++)
        frame.polyline(up(panel.traces[k][0]), FAMILY_COLORS[1], 4.0);

    for (size_t i = 0; i <= last; i++)
        frame.line(up(chord(panel.traces[0][0], panel.traces[1][0], i)), MARK, i == 0 ? 5.0 : 2.0);

    frame.label("ruling u", up(chord(panel.traces[0][0], panel.traces[1][0], 0).center()));
    frame.label(fmt::format("chord {}: {:.2f}", last, chord(panel.traces[0][0], panel.traces[1][0], last).length()), up(chord(panel.traces[0][0], panel.traces[1][0], last).center()));
    frame.label(fmt::format("chamfer: ruling_off_chamfer_deg = {:.3f}", report.ruling_off_chamfer_deg[0]), up(Point::centroid({column.head[2], column.head[3]})));
    frame.label(fmt::format("oculus_edges[0]: ruling_off_oculus_edge_deg = {:.3f}", report.ruling_off_oculus_edge_deg[0]), up(oculus_edge.start() + (oculus_edge.end() - oculus_edge.start()) * 0.8));
    frame.label("soffits[0]", up(panel.traces[0][0].get_point(2)));
    frame.label("soffits[1]", up(panel.traces[1][0].get_point(4)));
    frame.write(context.dir);
}

/// 60: the normal foot against the r hit on the central face, the obliqueness arc and the shear between them.
void obliqueness_shear(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const std::array<Vector, 2> normals = face_normals(geometry);
    const double shear = context.guide.parameters.inner_ribs * std::tan(panel.obliqueness[0] * M_PI / 180.0);
    const Point start = shadow(geometry, 0).get_point(0);
    const Point hit = panel.traces[0][0].get_point(0);
    const Point foot = central_faces(geometry)[0].project(start);
    Frame frame(CHAPTER, 60, "obliqueness_shear", fmt::format("obliqueness = {:.3f} / {:.3f} deg; rib_shear_mm = inner_ribs tan(obliqueness) = {:.3f}", panel.obliqueness[0], panel.obliqueness[1], shear), "top", around({start, hit, foot}, 45.0));
    frame.distance = 0.75;

    frame.line(up(face_line(start, normals[0], 90.0)), GREY, 3.0);
    frame.line(up(face_line(foot, normals[0], 90.0)), GREY, 3.0);
    frame.line(up(Line::from_points(start, foot)), INK, 2.0, true);
    frame.line(up(Line::from_points(start, hit)), MARK, 3.0, false, true);
    frame.line(up(Line::from_points(foot, hit)), MARK, 5.0);
    frame.polyline(up(arc(start, normals[0], panel.rib_sweep, 35.0)), INK, 2.0);
    frame.point(up(foot), INK);
    frame.point(up(hit), MARK);

    frame.label("shadows[0] pt 0", up(start));
    frame.label("normal foot", up(foot));
    frame.label("r hit = soffits[0] pt 0", up(hit));
    frame.label(fmt::format("obliqueness[0] = {:.3f} deg", panel.obliqueness[0]), up(arc_middle(start, normals[0], panel.rib_sweep, 35.0)));
    frame.label(fmt::format("rib_shear_mm = {:.3f}", (hit - foot).magnitude()), up(Point::centroid({foot, hit})));
    frame.write(context.dir);
}

/// 61: rib 0's soffit points moved along the ruling onto faces[1], landing on rib 1's soffit points.
void residual_frame(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const Polyline landed = panel.traces[0][0].transformed(Xform::project_to_plane_by_axis(central_faces(geometry)[1], panel.ruling));
    const size_t last = landed.point_count() - 1;
    Frame frame(CHAPTER, 61, "residual", fmt::format("residual = largest_shift(along(soffits[0], faces[1], ruling), soffits[1]) = {:.1e} mm", panel.residual), "iso", PANEL_3D);
    frame.distance = 0.8;
    rib_bands(frame, context.guide);

    for (size_t k = 0; k < 2; k++)
        frame.polyline(up(panel.traces[k][0]), FAMILY_COLORS[1], 4.0);

    for (size_t i = 0; i <= last; i++) {
        frame.line(up(Line::from_points(panel.traces[0][0].get_point(i), landed.get_point(i))), INK, 2.0, true);
        frame.point(up(landed.get_point(i)), MARK);
    }

    frame.label("soffits[0]", up(panel.traces[0][0].get_point(2)));
    frame.label("soffits[1]", up(panel.traces[1][0].get_point(4)));
    frame.label("i = 0", up(panel.traces[0][0].get_point(0)));
    frame.label(fmt::format("i = {}", last), up(panel.traces[0][0].get_point(last)));
    frame.label("along ruling u", up(Line::from_points(panel.traces[0][0].get_point(5), landed.get_point(5)).center()));
    frame.write(context.dir);
}

/// 62: the vertical plane through soffits[0] pt 0 normal to u, and rib 0's soffit moved onto it along u.
void section_frame(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const Polyline& soffit = panel.traces[0][0];
    const Polyline section = section_curves(panel, context.guide.parameters.tsections)[0];
    const size_t last = section.point_count() - 1;
    Frame frame(CHAPTER, 62, "section", "section = along(soffits[0], plane at soffits[0] pt 0 normal to u, u): the panel's normal cross-section", "iso", PANEL_3D);
    frame.distance = 0.8;
    frame.plane_size = 300.0;
    rib_bands(frame, context.guide);
    frame.polyline(up(soffit), GREY, 3.0);
    frame.polyline(up(panel.traces[1][0]), GREY, 3.0);
    frame.plane(up(Plane::from_point_normal(soffit.get_point(0), panel.ruling)), MARK);

    for (size_t i = 1; i <= last; i++)
        frame.line(up(Line::from_points(soffit.get_point(i), section.get_point(i))), INK, 2.0, true, true);

    frame.polyline(up(section), MARK, 5.0);
    frame.label("section", up(section.get_point(last)));
    frame.label("soffits[0]", up(soffit.get_point(4)));
    frame.label("Plane::from_point_normal(soffits[0] pt 0, ruling)", up(soffit.get_point(0)));
    frame.label("along u", up(Line::from_points(soffit.get_point(3), section.get_point(3)).center()));
    frame.write(context.dir);
}

/// 63: looking along u, the section and its offsets by t and 2t, a mitre, the end normal and the 27 square to a segment.
void section_offsets(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const double t = context.guide.parameters.tsections;
    const std::array<Polyline, 3> curves = section_curves(geometry.central_panel, t);
    const Polyline& section = curves[0];
    const Polyline& offset2 = curves[2];
    const Line segment = Line::from_points(section.get_point(0), section.get_point(1));
    const Vector x = segment.to_direction().normalized();
    const Vector normal = x.cross(Vector::z_axis().cross(x)).normalized();
    const Vector in0 = (offset2.get_point(1) - offset2.get_point(0)).normalized();
    const Vector in1 = (offset2.get_point(2) - offset2.get_point(1)).normalized();
    Frame frame(CHAPTER, 63, "section_offsets", fmt::format("offset1 = offset_polyline(section, {:.0f}), offset2 by {:.0f}: mitred corners, square ends, seen along u", t, 2.0 * t), "iso", around({section.get_point(0), section.get_point(1), offset2.get_point(0), offset2.get_point(1)}, 50.0));
    frame.orbit = "-262,-105";

    frame.polyline(up(section), FAMILY_COLORS[1], 4.0);
    frame.polyline(up(curves[1]), FAMILY_COLORS[4], 3.0);
    frame.polyline(up(offset2), FAMILY_COLORS[5], 3.0);
    frame.line(up(Line::from_points(offset2.get_point(1), offset2.get_point(1) + in0 * 150.0)), INK, 2.0, true);
    frame.line(up(Line::from_points(offset2.get_point(1), offset2.get_point(1) - in1 * 150.0)), INK, 2.0, true);
    frame.line(up(Line::from_points(section.get_point(0), offset2.get_point(0))), INK, 2.0);
    frame.line(up(Line::from_points(segment.center(), segment.center() + normal * t)), MARK, 3.0, false, true);

    frame.label("section", up(segment.point_at(0.8)));
    frame.label("offset1", up(Line::from_points(curves[1].get_point(0), curves[1].get_point(1)).point_at(0.25)));
    frame.label("offset2", up(Line::from_points(offset2.get_point(0), offset2.get_point(1)).point_at(0.75)));
    frame.label("mitre: plane_plane_plane", up(offset2.get_point(1)));
    frame.label("end normal at pt 0", up(Point::centroid({section.get_point(0), offset2.get_point(0)})));
    frame.label(fmt::format("tsections = {:.0f}", t), up(segment.center() + normal * (0.5 * t)));
    frame.write(context.dir);
}

/// 64: the soffit, +t and +2t on both central faces with rulings, the section's curves grey with their moves along u.
void traces_frame(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const std::array<Polyline, 3> curves = section_curves(panel, context.guide.parameters.tsections);
    const std::array<Color, 3> colors = {FAMILY_COLORS[1], FAMILY_COLORS[4], FAMILY_COLORS[5]};
    const size_t last = panel.traces[0][0].point_count() - 1;
    Frame frame(CHAPTER, 64, "traces", "traces[k] = {soffits[k], along(offset1, faces[k], u), along(offset2, faces[k], u)}: nested cylinders", "iso", PANEL_3D);
    frame.key = true;
    frame.distance = 0.8;
    rib_bands(frame, context.guide);

    for (const Polyline& curve : curves)
        frame.polyline(up(curve), GREY, 2.0);

    for (size_t layer = 0; layer < 3; layer++)
        for (size_t k = 0; k < 2; k++)
            frame.polyline(up(panel.traces[k][layer]), colors[layer], layer == 0 ? 5.0 : 3.0);

    for (const size_t i : {size_t(0), last / 2, last}) {
        for (size_t layer = 0; layer < 3; layer++)
            frame.line(up(chord(panel.traces[0][layer], panel.traces[1][layer], i)), INK, 1.0);

        for (size_t k = 0; k < 2; k++)
            frame.line(up(Line::from_points(curves[2].get_point(i), panel.traces[k][2].get_point(i))), GREY, 1.5, true);
    }

    frame.label("traces[0][0]: soffit", up(panel.traces[0][0].get_point(last)));
    frame.label("traces[0][1]: +t", up(panel.traces[0][1].get_point(4)));
    frame.label("traces[0][2]: +2t", up(panel.traces[0][2].get_point(2)));
    frame.label("traces[1][0]", up(panel.traces[1][0].get_point(last)));
    frame.label("traces[1][2]", up(panel.traces[1][2].get_point(4)));
    frame.label("offset2 in the section", up(curves[2].get_point(5)));
    frame.write(context.dir);
}

/// 65: one consumer of the panel per colour: the inner ribs' far loops, t-sections 2 and 3, bed row 1 and bed_top_planes[1].
void consumers(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const Quarter quarter = context.guide.quarter(0);
    const std::vector<Outline> inner = quarter.inner_ribs();
    const std::vector<Outline> tsections = quarter.tsections();
    const std::vector<Outline> row = quarter.beds()[1];
    Frame frame(CHAPTER, 65, "consumers", "Who reads the panel: the inner ribs' far loops, t-sections 2 and 3, bed row 1 and bed_top_planes[1]", "iso", PANEL_3D);
    frame.distance = 0.8;
    frame.plane_size = 200.0;

    for (size_t k = 0; k < 2; k++)
        for (const Polyline& trace : panel.traces[k])
            frame.polyline(up(trace), GREY, 2.0);

    for (size_t k = 0; k < 2; k++) {
        frame.polyline(up(inner[k].bottom), FAMILY_COLORS[1], 3.0);
        frame.polyline(up(tsections[2 + k].top), FAMILY_COLORS[4], 2.0);
        frame.polyline(up(tsections[2 + k].bottom), FAMILY_COLORS[4], 2.0);
    }

    for (const Outline& plate : row) {
        frame.polyline(up(plate.top), FAMILY_COLORS[5], 1.5);
        frame.polyline(up(plate.bottom), FAMILY_COLORS[5], 1.5);
    }

    frame.plane(up(geometry.bed_top_planes[1]), MARK);
    frame.label(fmt::format("{}: far loop along rib_sweep", member_name(Family::inner_ribs, 0, 0)), up(area_centroid(inner[0].bottom)));
    frame.label(member_name(Family::inner_ribs, 1, 0), up(area_centroid(inner[1].bottom)));
    frame.label(fmt::format("{}: traces[0][0..1]", member_name(Family::tsections, 2, 0)), up(panel.traces[0][1].get_point(5)));
    frame.label(fmt::format("{}: traces[1][0..1]", member_name(Family::tsections, 3, 0)), up(panel.traces[1][1].get_point(5)));
    frame.label("beds row 1: traces[k][1], traces[k][2]", up(middle(row[row.size() / 2])));
    frame.label("bed_top_planes[1]", up(geometry.bed_top_planes[1].origin()));
    frame.write(context.dir);
}

/// 66: bed panel 0's +2t layer projected along outer rib 0's normal onto its two side faces.
void side_projections(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const Polyline& top = geometry.parabolas[0][2];
    const std::array<Polyline, 2> faces = side_faces(geometry);
    Frame frame(CHAPTER, 66, "side_projections", "bed_top_planes: parabolas[0][2] along cp.outer_ribs[0][0].z_axis() onto bed panel 0's two side faces", "iso", {-2900.0, -3050.0, H - 700.0, 100.0, -900.0, H + 20.0});
    frame.distance = 0.85;
    frame.polyline(up(geometry.quads.outer_ribs[0].closed()), GREY, 2.0);
    frame.polyline(up(geometry.quads.inner_ribs[0].closed()), GREY, 2.0);
    frame.polyline(up(top), GREY, 3.0);

    for (const Polyline& face : faces)
        frame.polyline(up(face), FAMILY_COLORS[5], 4.0);

    for (size_t i = 0; i < top.point_count(); i++)
        frame.line(up(Line::from_points(top.get_point(i), faces[0].get_point(i))), INK, 1.5, true);

    frame.label("parabolas[0][2]", up(top.get_point(2)));
    frame.label("side01: on cp.outer_ribs[0][1]", up(faces[1].get_point(1)));
    frame.label("side00: on cp.inner_ribs[0][0]", up(faces[0].get_point(5)));
    frame.label("along cp.outer_ribs[0][0].z_axis()", up(Line::from_points(faces[1].get_point(4), faces[0].get_point(4)).center()));
    frame.write(context.dir);
}

/// 67: in elevation turned about 30 degrees, the trimmed side curves of bed panel 0, their deepest quad and the plane fitted to it.
void side_bed_plane(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const std::array<Polyline, 2> faces = side_faces(geometry);
    const std::array<std::vector<Point>, 2> pts = deepest_first(faces, cp.inner_beams[0][1], cp.wedges[0][0]);
    const Plane& plane = geometry.bed_top_planes[0];
    Frame frame(CHAPTER, 67, "side_bed_plane", fmt::format("panel_top_plane: a PCA plane through pts[0][0..1], pts[1][0..1], normal turned up, {:.2f} deg from z", tilt(plane)), "front", {-2800.0, -3000.0, H - 720.0, -1550.0, -2300.0, H - 300.0});
    frame.plane_size = 250.0;
    frame.orbit = "-105,0"; // About 30 degrees of yaw, so the curve on y = -2900 separates from the one on inner rib 0 and the deepest quad opens.

    for (size_t k = 0; k < 2; k++)
        frame.polyline(up(Polyline(pts[k])), GREY, 3.0);

    frame.polyline(up(deepest_quad(pts)), MARK, 3.0);
    frame.plane(up(plane), FAMILY_COLORS[5]);

    for (const Point& corner : {pts[0][0], pts[0][1], pts[1][0], pts[1][1]})
        frame.point(up(corner), MARK);

    frame.label("pts[0][0], on cp.wedges[0][0]", up(pts[0][0]));
    frame.label("pts[0][1]", up(pts[0][1]));
    frame.label("bed_top_planes[0]", up(plane.origin()));
    frame.label("trimmed +2t curves", up(pts[0][2]));
    frame.write(context.dir);
}

/// 68: the central bed's trimmed +2t traces, their deepest quad and the fitted plane.
void central_bed_plane(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const CentralPanel& panel = geometry.central_panel;
    const std::array<std::vector<Point>, 2> pts = deepest_first({panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]);
    const Plane& plane = geometry.bed_top_planes[1];
    Frame frame(CHAPTER, 68, "central_bed_plane", fmt::format("panel_top_plane on traces[0][2], traces[1][2]: bed_top_planes[1], {:.2f} deg from z", tilt(plane)), "iso", {-2850.0, -2850.0, H - 700.0, -1900.0, -1900.0, H - 380.0});
    frame.plane_size = 200.0;
    frame.distance = 0.85;

    for (size_t k = 0; k < 2; k++)
        frame.polyline(up(Polyline(pts[k])), GREY, 3.0);

    frame.polyline(up(deepest_quad(pts)), MARK, 3.0);
    frame.line(up(Line::from_points(pts[0][0], pts[1][0])), INK, 2.0, true);
    frame.plane(up(plane), FAMILY_COLORS[5]);

    for (const Point& corner : {pts[0][0], pts[0][1], pts[1][0], pts[1][1]})
        frame.point(up(corner), MARK);

    frame.label("pts[0][0], on cp.wedges[1][0]", up(pts[0][0]));
    frame.label("pts[1][1]", up(pts[1][1]));
    frame.label("traces[0][2] trimmed", up(Point::centroid({pts[0][1], pts[0][2]})));
    frame.label("traces[1][2] trimmed", up(Point::centroid({pts[1][1], pts[1][2]})));
    frame.label("bed_top_planes[1]", up(plane.origin()));
    frame.write(context.dir);
}

}

void chapter_04_central_panel(const Context& context) {
    panel_inputs_outputs(context);
    reference_direction(context);
    scan_fan(context);
    skip_intervals(context);
    shift_frame(context);
    closure_frame(context);
    rib_sweep_frame(context);
    soffits_frame(context);
    ruling_frame(context);
    obliqueness_shear(context);
    residual_frame(context);
    section_frame(context);
    section_offsets(context);
    traces_frame(context);
    consumers(context);
    side_projections(context);
    side_bed_plane(context);
    central_bed_plane(context);
}

}
