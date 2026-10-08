#include "pch.h"
#include "src/templates/floor/floor_guide.h"

#include <span>

using namespace session_cpp;
using namespace wood_session;

namespace wood_floor {

const double EXTENSION = 1000.0; // how far a trace's ends are pushed out before the planes that end its member trim it
const double RIGHT_ANGLE = 1e-9; // degrees off 90 within which a corner counts as right, so a rectangle keeps its exact edge directions
const double RIB_START_TOLERANCE = 1e-11; // mm an outer rib's end may sit off its corner's shared level; within it the rib keeps the size_wedge as its start
const size_t RIB_START_STEPS = 50; // secant steps a rib start may take to land its rib's end on the shared level
const double SCAN_STEP = 0.5; // degrees between the sweep directions scanned for rule A's root
const double SCAN_RANGE = 85.0; // degrees either side of n0 - n1 the scan covers: the second root, where both ribs shift alike, lies at 90
const double GRAZING = 1e-3; // |n . r| below which a sweep runs along a rib face and is skipped
const size_t BISECTIONS = 200; // halvings of the bracket, far past the last bit of the angle
const double CUTTER_MARGIN = 100.0; // how far column cutter quads overshoot and how thick they are

// ═══════════════════════════════════════════════════════════════════════════
// The guide
// ═══════════════════════════════════════════════════════════════════════════

FloorGuide::FloorGuide(
    const std::array<Point, 4>& corners,
    double size_oculus,
    double size_column_head,
    double size_column_head_chamfer,
    double size_outer_ribs,
    double size_inner_ribs,
    double size_inner_beams,
    double size_wedge,
    double size_tsections,
    double height,
    double rise,
    double wedge_plane_angle,
    double oculus_plane_angle,
    double column_head_depth,
    double bay_height,
    double middle_wedge_factor)
    : WoodSession("floor_guide"),
      corners(corners),
      size_oculus(size_oculus),
      size_column_head(size_column_head),
      size_column_head_chamfer(size_column_head_chamfer),
      size_outer_ribs(size_outer_ribs),
      size_inner_ribs(size_inner_ribs),
      size_inner_beams(size_inner_beams),
      size_wedge(size_wedge),
      size_tsections(size_tsections),
      height(height),
      rise(rise),
      wedge_plane_angle(wedge_plane_angle),
      oculus_plane_angle(oculus_plane_angle),
      column_head_depth(column_head_depth),
      bay_height(bay_height),
      middle_wedge_factor(middle_wedge_factor) {

    // centre of the floor, the average of the four corners
    centre = Point::centroid({corners[0], corners[1], corners[2], corners[3]});

    // oculus points, size_oculus from the centre towards each edge midpoint
    for (size_t q = 0; q < 4; q++)
        oculus_points[q] = centre + (midpoint(q) - centre).normalized() * size_oculus;

    // construction planes, a pair per member; each column block as thick as its rib's start, chosen so both outer ribs of a column end at one depth
    for (size_t q = 0; q < 4; q++) {
        const ConstructionPlanes planes = compute_construction_planes(q);
        _construction_planes[q] = planes;
    }

    // construction quads, each member's footprint on the floor where four of its planes cross
    for (size_t q = 0; q < 4; q++) {
        const ConstructionQuads quads = compute_construction_quads(_construction_planes[q]);
        _construction_quads[q] = quads;
    }

    // boundary parabolas, the curved underside of each rib and its two layers 27 and 54 above
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::array<Polyline, 3>, 4> parabolas = compute_boundary_parabolas(q);
        _boundary_parabolas[q] = parabolas;
    }

    // central panel, the ruled surface between the two inner ribs and its traces on their faces
    for (size_t q = 0; q < 4; q++) {
        const CentralPanel panel = compute_central_panel(q);
        _central_panel[q] = panel;
    }

    // bed top planes, the underside of each column block where it sits on its bed row: beside rib 0, in the central panel, beside rib 1
    for (size_t q = 0; q < 4; q++) {
        const std::array<Plane, 3> bed_planes = compute_bed_top_planes(q);
        _bed_top_planes[q] = bed_planes;
    }

    // outer ribs, each its parabola trimmed by its end planes on its first face and swept to its second
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::array<Polyline, 2>, 2> ribs = compute_outer_ribs(q);
        _outer_ribs[q] = ribs;
    }

    // inner ribs, swept along the central panel's rib sweep
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::array<Polyline, 2>, 2> ribs = compute_inner_ribs(q);
        _inner_ribs[q] = ribs;
    }

    // the middle cutter level, one for every column: the deepest outer rib bottom corner on a fan plane
    for (size_t q = 0; q < 4; q++)
        for (const std::array<Polyline, 2>& rib : _outer_ribs[q])
            _rib_bottom = std::min({_rib_bottom, rib[0].get_point(2)[2], rib[1].get_point(2)[2]});

    // soffit, the one level every inner and ring beam's underside sits at: the deepest rib end on a beam, so every rib meets its beam in full
    soffit = -static_h();

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            soffit = std::min({soffit, end_level(_outer_ribs[q][k], rib_seam_ends(q)[k]), end_level(_inner_ribs[q][k], _construction_planes[q].inner_beams[1][1])});

    // t-sections, six flanges beside the rib faces
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::array<Polyline, 2>, 6> flanges = compute_tsections(q);
        _tsections[q] = flanges;
    }

    // bed rails, per bed row its lower and upper layer on its two side faces, trimmed alike
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::array<std::array<Polyline, 2>, 2>, 3> rails = compute_bed_rails(q);
        _bed_rails[q] = rails;
    }

    // beds, per row one quad plate for each segment of its rails
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::vector<std::array<Polyline, 2>>, 3> rows = compute_beds(q);
        _beds[q] = rows;
    }

    // wedges, the three column blocks between the ribs, standing on the bed top planes
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::array<Polyline, 2>, 3> blocks = compute_wedges(q);
        _wedges[q] = blocks;
    }

    // inner beams, the two seam beams and the oculus beam down to the soffit
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::array<Polyline, 2>, 3> beams = compute_inner_beams(q);
        _inner_beams[q] = beams;
    }

    // oculus, four ring beams around the hole, four bottom wedges and the inner plate
    const std::array<std::array<Polyline, 2>, 9> ring = compute_oculus();
    _oculus = ring;

    // column cutters, six plates per column that carve its head down to the cutter level and the head depth
    for (size_t q = 0; q < 4; q++) {
        const std::array<std::array<Polyline, 2>, 6> cutters = compute_column_cutters(q);
        _column_cutters[q] = cutters;
    }

    draw();
}

double FloorGuide::static_h() const {
    return height - rise;
}

Point FloorGuide::midpoint(size_t k) const {
    return Point::mid_point(corners[k % 4], corners[(k + 1) % 4]);
}

double FloorGuide::corner_angle(size_t k) const {
    return (corners[(k + 1) % 4] - corners[k % 4]).angle(corners[(k + 3) % 4] - corners[k % 4], false);
}

// ═══════════════════════════════════════════════════════════════════════════
// Floor plan geometry
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Point> FloorGuide::quarter_polygon(size_t q) const {
    return {corners[q], midpoint(q), oculus_points[q], oculus_points[(q + 3) % 4], midpoint(q + 3)};
}

std::vector<Point> FloorGuide::quarter_column_polygon(size_t q) const {

    const Plane frame = column_frame(q);
    const Point& corner = frame.origin();
    const Vector x = frame.x_axis();
    const Vector y = frame.y_axis();

    return {corner, corner + x * size_column_head, corner + x * size_column_head + y * size_column_head_chamfer, corner + x * size_column_head_chamfer + y * size_column_head, corner + y * size_column_head};
}

Plane FloorGuide::column_frame(size_t q) const {

    const Point& corner = corners[q];
    const Vector after = (corners[(q + 1) % 4] - corner).normalized();
    const Vector before = (corners[(q + 3) % 4] - corner).normalized();

    if (std::abs(corner_angle(q) - 90.0) <= RIGHT_ANGLE)
        return Plane::from_frame(corner, after, before, Vector::z_axis());

    const Vector bisector = (after + before).normalized();

    return Plane::from_frame(corner, bisector.transformed(Xform::rotation(Vector::z_axis(), -45.0, true)), bisector.transformed(Xform::rotation(Vector::z_axis(), 45.0, true)), Vector::z_axis());
}

Plane FloorGuide::support_plane(size_t q) const {

    const Plane frame = column_frame(q);

    return Plane::from_frame(frame.origin() + (frame.x_axis() + frame.y_axis()) * (size_column_head * 0.5), frame.x_axis(), frame.y_axis(), Vector::z_axis());
}

// ═══════════════════════════════════════════════════════════════════════════
// Beams: the plate edges as plane pairs, then their plan quads
// ═══════════════════════════════════════════════════════════════════════════

const ConstructionPlanes& FloorGuide::construction_planes(size_t q) const {
    return _construction_planes[q % 4];
}

const ConstructionQuads& FloorGuide::construction_quads(size_t q) const {
    return _construction_quads[q % 4];
}

std::array<Plane, 2> FloorGuide::pair(const Plane& plane, double distance) {
    return {plane, plane.translate_by_normal(distance)};
}

ConstructionPlanes FloorGuide::compute_construction_planes(size_t q) const {

    const std::vector<Point> polygon = quarter_polygon(q);
    const std::vector<Point> head = quarter_column_polygon(q);
    const Vector down = -Vector::z_axis();
    ConstructionPlanes cp;

    // 1. outer ribs: the bay edge's plane, normal into the bay, its origin at the quarter's half edge
    const Plane edge0 = Plane::from_line(Line::from_points(corners[q], corners[(q + 1) % 4]), down);
    const Plane edge1 = Plane::from_line(Line::from_points(corners[(q + 3) % 4], corners[q]), down);
    cp.outer_ribs = {
        pair(edge0.moved_to(Point::mid_point(polygon[0], polygon[1])), size_outer_ribs),
        pair(edge1.moved_to(Point::mid_point(polygon[4], polygon[0])), size_outer_ribs),
    };

    // 2. inner beams on the polygon's seam and oculus lines; the oculus one tilted by oculus_plane_angle about its line
    const Line oculus_line = Line::from_points(polygon[2], polygon[3]);
    const Plane oculus_plane = Plane::from_line(oculus_line, down);
    const Plane tilted = oculus_plane.transformed(Xform::rotation_around_line(oculus_line, -oculus_plane_angle, true));
    cp.inner_beams = {
        pair(Plane::from_line(Line::from_points(polygon[1], polygon[2]), down), size_inner_beams),
        {tilted, oculus_plane.translate_by_normal(size_inner_beams)},
        pair(Plane::from_line(Line::from_points(polygon[3], polygon[4]), down), size_inner_beams),
    };

    // 3. inner ribs from the column head's chamfer points to the inner beam corners
    const Plane xy = Plane::xy_plane_at(0.0);
    const Point p0 = Intersection::plane_plane_plane(xy, cp.inner_beams[0][1], cp.inner_beams[1][1]).value();
    const Point p1 = Intersection::plane_plane_plane(xy, cp.inner_beams[1][1], cp.inner_beams[2][1]).value();
    const Point p2 = head[2];
    const Point p3 = head[3];
    cp.inner_ribs = {
        pair(Plane::from_line(Line::from_points(p2, p0), down), size_inner_ribs),
        pair(Plane::from_line(Line::from_points(p3, p1), Vector::z_axis()), size_inner_ribs),
    };

    // 4. wedges: the chamfer plane tilted by wedge_plane_angle about its top edge, the two side planes leaning with it along the inner ribs
    const Line side0 = Line::from_points(head[1], head[2]);
    const Line side1 = Line::from_points(head[2], head[3]);
    const Line side2 = Line::from_points(head[3], head[4]);
    const Plane chamfer = Plane::from_line(side1, Vector::z_axis()).transformed(Xform::rotation_around_line(side1, wedge_plane_angle, true));
    const Line line0 = Intersection::plane_plane(chamfer, cp.inner_ribs[0][1]).value();
    const Line line1 = Intersection::plane_plane(chamfer, cp.inner_ribs[1][1]).value();
    const Plane wedge0 = Plane::from_line(side0, -line0.to_direction());
    const Plane wedge2 = Plane::from_line(side2, line1.to_direction());

    // the near faces first, as the rib starts read them; each side block then as thick as its rib's start, the middle one middle_wedge_factor times their mean
    cp.wedges = {pair(wedge0, 0.0), pair(chamfer, 0.0), pair(wedge2, 0.0)};
    const std::array<double, 2> starts = compute_rib_starts(cp);
    cp.wedges = {pair(wedge0, starts[0]), pair(chamfer, middle_wedge_factor * (0.5 * (starts[0] + starts[1]))), pair(wedge2, starts[1])};

    // 5. t-sections beside the ribs
    cp.tsections = {
        pair(cp.outer_ribs[0][1], size_tsections),
        pair(cp.inner_ribs[0][0], -size_tsections),
        pair(cp.inner_ribs[0][1], size_tsections),
        pair(cp.inner_ribs[1][1], size_tsections),
        pair(cp.inner_ribs[1][0], -size_tsections),
        pair(cp.outer_ribs[1][1], size_tsections),
    };

    return cp;
}

ConstructionQuads FloorGuide::compute_construction_quads(const ConstructionPlanes& cp) const {

    // the four planes of each member's quad, its corners 3-0, 0-1, 1-2 and 2-3 on the datum
    const auto quad_of = [](const std::array<Plane, 4>& planes) {
        return Polyline::from_planes({planes[3], planes[0], planes[1], planes[2]}, Plane::xy_plane_at(0.0));
    };

    ConstructionQuads quad;
    quad.outer_ribs = {
        quad_of({cp.outer_ribs[0][0], cp.inner_beams[0][0], cp.outer_ribs[0][1], cp.wedges[0][0]}),
        quad_of({cp.outer_ribs[1][0], cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.wedges[2][0]}),
    };
    quad.inner_beams = {
        quad_of({cp.inner_beams[0][0], cp.inner_beams[1][0], cp.inner_beams[0][1], cp.outer_ribs[0][1]}),
        quad_of({cp.inner_beams[1][0], cp.inner_beams[2][1], cp.inner_beams[1][1], cp.inner_beams[0][1]}),
        quad_of({cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.inner_beams[2][1], cp.inner_beams[1][0]}),
    };
    quad.inner_ribs = {
        quad_of({cp.inner_ribs[0][1], cp.inner_beams[1][1], cp.inner_ribs[0][0], cp.wedges[1][0]}),
        quad_of({cp.inner_ribs[1][1], cp.inner_beams[1][1], cp.inner_ribs[1][0], cp.wedges[1][0]}),
    };
    quad.wedges = {
        quad_of({cp.wedges[0][0], cp.outer_ribs[0][1], cp.wedges[0][1], cp.inner_ribs[0][0]}),
        quad_of({cp.wedges[1][0], cp.inner_ribs[0][1], cp.wedges[1][1], cp.inner_ribs[1][1]}),
        quad_of({cp.wedges[2][0], cp.inner_ribs[1][0], cp.wedges[2][1], cp.outer_ribs[1][1]}),
    };
    quad.tsections = {
        quad_of({cp.tsections[0][0], cp.inner_beams[0][1], cp.tsections[0][1], cp.wedges[0][1]}),
        quad_of({cp.tsections[1][0], cp.inner_beams[0][1], cp.tsections[1][1], cp.wedges[0][1]}),
        quad_of({cp.tsections[2][0], cp.inner_beams[1][1], cp.tsections[2][1], cp.wedges[1][1]}),
        quad_of({cp.tsections[3][0], cp.inner_beams[1][1], cp.tsections[3][1], cp.wedges[1][1]}),
        quad_of({cp.tsections[4][0], cp.inner_beams[2][1], cp.tsections[4][1], cp.wedges[2][1]}),
        quad_of({cp.tsections[5][0], cp.inner_beams[2][1], cp.tsections[5][1], cp.wedges[2][1]}),
    };

    return quad;
}

// ═══════════════════════════════════════════════════════════════════════════
// 3D geometry
// ═══════════════════════════════════════════════════════════════════════════

std::array<double, 2> FloorGuide::rib_starts(size_t q) const {
    return compute_rib_starts(_construction_planes[q % 4]);
}

const std::array<std::array<Polyline, 3>, 4>& FloorGuide::boundary_parabolas(size_t q) const {
    return _boundary_parabolas[q % 4];
}

const CentralPanel& FloorGuide::central_panel(size_t q) const {
    return _central_panel[q % 4];
}

const std::array<Plane, 3>& FloorGuide::bed_top_planes(size_t q) const {
    return _bed_top_planes[q % 4];
}

const std::array<std::array<Polyline, 2>, 2>& FloorGuide::outer_ribs(size_t q) const {
    return _outer_ribs[q % 4];
}

const std::array<std::array<Polyline, 2>, 2>& FloorGuide::inner_ribs(size_t q) const {
    return _inner_ribs[q % 4];
}

const std::array<std::array<Polyline, 2>, 6>& FloorGuide::tsections(size_t q) const {
    return _tsections[q % 4];
}

const std::array<std::array<std::array<Polyline, 2>, 2>, 3>& FloorGuide::bed_rails(size_t q) const {
    return _bed_rails[q % 4];
}

const std::array<std::vector<std::array<Polyline, 2>>, 3>& FloorGuide::beds(size_t q) const {
    return _beds[q % 4];
}

const std::array<std::array<Polyline, 2>, 3>& FloorGuide::wedges(size_t q) const {
    return _wedges[q % 4];
}

const std::array<std::array<Polyline, 2>, 3>& FloorGuide::inner_beams(size_t q) const {
    return _inner_beams[q % 4];
}

const std::array<std::array<Polyline, 2>, 6>& FloorGuide::column_cutters(size_t q) const {
    return _column_cutters[q % 4];
}

const std::array<std::array<Polyline, 2>, 9>& FloorGuide::oculus() const {
    return _oculus;
}

std::array<double, 3> FloorGuide::column_levels(size_t q) const {
    return {0.0, _rib_bottom, -column_head_depth};
}

std::array<Plane, 2> FloorGuide::rib_seam_ends(size_t q) const {

    const ConstructionPlanes& cp = construction_planes(q);

    return {cp.inner_beams[0][1], cp.inner_beams[2][1]};
}

// ═══════════════════════════════════════════════════════════════════════════
// Rib starts
// ═══════════════════════════════════════════════════════════════════════════

Line FloorGuide::outer_rib_axis(const ConstructionPlanes& cp, size_t k) {

    const Plane xy = Plane::xy_plane_at(0.0);
    const Plane& face = cp.outer_ribs[k][0];

    return Line::from_points(Intersection::plane_plane_plane(xy, cp.wedges[2 * k][0], face).value(), Intersection::plane_plane_plane(xy, face, cp.inner_beams[2 * k][0]).value());
}

std::array<double, 2> FloorGuide::compute_rib_starts(const ConstructionPlanes& cp) const {

    const std::array<Line, 2> axes = {outer_rib_axis(cp, 0), outer_rib_axis(cp, 1)};
    const std::array<Plane, 2> fans = {cp.wedges[0][0], cp.wedges[2][0]};
    const std::array<Plane, 2> seams = {cp.inner_beams[0][0], cp.inner_beams[2][0]};
    const double level = std::max(fan_end(axes[0], size_wedge, fans[0], seams[0]), fan_end(axes[1], size_wedge, fans[1], seams[1]));

    return {rib_start_at_level(axes[0], fans[0], seams[0], level), rib_start_at_level(axes[1], fans[1], seams[1], level)};
}

double FloorGuide::rib_start_at_level(const Line& axis, const Plane& fan, const Plane& seam, double level) const {

    const double length = axis.length();
    double x0 = size_wedge;
    double f0 = fan_end(axis, x0, fan, seam) - level;

    if (std::abs(f0) <= RIB_START_TOLERANCE)
        return x0;

    double x1 = x0 + 1.0;
    double f1 = fan_end(axis, x1, fan, seam) - level;

    for (size_t i = 0; i < RIB_START_STEPS; i++) {
        if (std::abs(f1) <= RIB_START_TOLERANCE)
            return x1;

        const double x2 = x1 - f1 * (x1 - x0) / (f1 - f0);

        if (x2 <= 0.0 || x2 >= length)
            throw std::runtime_error(fmt::format("an outer rib's start for the column level {:.3f} leaves its axis: {:.3f} of {:.3f} mm", level, x2, length));

        x0 = x1;
        f0 = f1;
        x1 = x2;
        f1 = fan_end(axis, x1, fan, seam) - level;
    }

    throw std::runtime_error(fmt::format("an outer rib's start for the column level {:.3f} did not converge: {:.3e} mm off", level, f1));
}

double FloorGuide::fan_end(const Line& axis, double distance, const Plane& fan, const Plane& seam) const {

    const std::vector<Point> pts = outer_parabola(axis, distance).trimmed(fan, seam, EXTENSION).get_points();
    const double d0 = std::abs(fan.signed_distance(pts.front()));
    const double d1 = std::abs(fan.signed_distance(pts.back()));

    return d0 > d1 ? pts.back()[2] : pts.front()[2];
}

// ═══════════════════════════════════════════════════════════════════════════
// Parabolas and bed planes
// ═══════════════════════════════════════════════════════════════════════════

Polyline FloorGuide::outer_parabola(const Line& axis, double distance) const {

    const Point start = axis.start();
    const Point end = axis.end();
    const Point trimmed = start + (end - start).normalized() * distance;
    const Point middle = Point::mid_point(trimmed, end);

    return Polyline::quadratic_points(trimmed + Vector(0.0, 0.0, -height), middle + Vector(0.0, 0.0, -static_h()), end + Vector(0.0, 0.0, -static_h()));
}

std::array<std::array<Polyline, 3>, 4> FloorGuide::compute_boundary_parabolas(size_t q) const {

    const ConstructionPlanes& cp = _construction_planes[q];
    const std::array<double, 2> starts = compute_rib_starts(cp);
    std::array<std::array<Polyline, 3>, 4> parabolas;

    for (size_t k = 0; k < 2; k++) {
        const Polyline parabola = outer_parabola(outer_rib_axis(cp, k), starts[k]);
        parabolas[k] = {parabola, parabola.offset_toward(size_tsections, Vector::z_axis()), parabola.offset_toward(2.0 * size_tsections, Vector::z_axis())};
    }

    // the inner parabolas are projections of the outer ones onto the inner ribs' outer faces
    for (size_t i = 0; i < 2; i++) {
        const Xform projection = Xform::project_to_plane_by_axis(cp.inner_ribs[i][0], cp.outer_ribs[i][0].z_axis());
        const std::array<Polyline, 3>& outer = parabolas[i];
        parabolas[2 + i] = {outer[0].transformed(projection), outer[1].transformed(projection), outer[2].transformed(projection)};
    }

    return parabolas;
}

std::array<Plane, 3> FloorGuide::compute_bed_top_planes(size_t q) const {

    const ConstructionPlanes& cp = _construction_planes[q];
    const std::array<std::array<Polyline, 3>, 4>& parabolas = _boundary_parabolas[q];
    const CentralPanel& panel = _central_panel[q];

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

    const Xform side00 = Xform::project_to_plane_by_axis(cp.inner_ribs[0][0], cp.outer_ribs[0][0].z_axis());
    const Xform side01 = Xform::project_to_plane_by_axis(cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis());
    const Xform side20 = Xform::project_to_plane_by_axis(cp.inner_ribs[1][0], cp.outer_ribs[1][0].z_axis());
    const Xform side21 = Xform::project_to_plane_by_axis(cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis());

    return {
        fitted({parabolas[0][2].transformed(side00), parabolas[0][2].transformed(side01)}, cp.inner_beams[0][1], cp.wedges[0][0]),
        fitted({panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]),
        fitted({parabolas[1][2].transformed(side20), parabolas[1][2].transformed(side21)}, cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// The central panel
// ═══════════════════════════════════════════════════════════════════════════

CentralPanel FloorGuide::compute_central_panel(size_t q) const {

    const ConstructionPlanes& cp = _construction_planes[q];
    const std::array<Plane, 2> faces = {cp.inner_ribs[0][1], cp.inner_ribs[1][1]};
    const std::array<Vector, 2> normals = {faces[0].z_axis(), faces[1].z_axis()};
    const std::array<Polyline, 2> shadows = {_boundary_parabolas[q][2][0], _boundary_parabolas[q][3][0]};
    const Vector reference = (normals[0] - normals[1]).flattened().normalized();

    CentralPanel panel;
    panel.rib_sweep = rib_sweep(shadows, normals, size_inner_ribs, reference);
    const std::array<Polyline, 2> soffits = {shadows[0].transformed(Xform::project_to_plane_by_axis(faces[0], panel.rib_sweep)), shadows[1].transformed(Xform::project_to_plane_by_axis(faces[1], panel.rib_sweep))};
    panel.ruling = (soffits[1].get_point(0) - soffits[0].get_point(0)).flattened().normalized();

    // the layers: the soffit's offsets by size_tsections and twice that in the panel's own cross-section, projected along the ruling onto both central faces
    const Polyline section = soffits[0].transformed(Xform::project_to_plane_by_axis(Plane::from_point_normal(soffits[0].get_point(0), panel.ruling), panel.ruling));
    const Polyline layer1 = section.offset_toward(size_tsections, Vector::z_axis());
    const Polyline layer2 = section.offset_toward(2.0 * size_tsections, Vector::z_axis());

    for (size_t k = 0; k < 2; k++)
        panel.traces[k] = {soffits[k], layer1.transformed(Xform::project_to_plane_by_axis(faces[k], panel.ruling)), layer2.transformed(Xform::project_to_plane_by_axis(faces[k], panel.ruling))};

    return panel;
}

Vector FloorGuide::rib_sweep(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& reference) {

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

double FloorGuide::closure(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& r) {

    // each rib's outer face trace moves thickness / (n . r) along r to reach its central face
    const std::array<double, 2> shift = {thickness / normals[0].dot(r), thickness / normals[1].dot(r)};
    const size_t n = shadows[0].point_count() - 1;
    const Vector start = ((shadows[1].get_point(0) + r * shift[1]) - (shadows[0].get_point(0) + r * shift[0])).flattened();
    const Vector vertex = ((shadows[1].get_point(n) + r * shift[1]) - (shadows[0].get_point(n) + r * shift[0])).flattened();

    return start.cross(vertex)[2] / (start.magnitude() * vertex.magnitude());
}

bool FloorGuide::sweep_sides(const std::array<Vector, 2>& normals, const Vector& reference, double degrees, std::array<bool, 2>& sides) {

    const Vector r = turned(reference, degrees);
    sides = {normals[0].dot(r) > 0.0, normals[1].dot(r) > 0.0};

    return std::abs(normals[0].dot(r)) > GRAZING && std::abs(normals[1].dot(r)) > GRAZING;
}

double FloorGuide::bisect(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& reference, double lo, double hi) {

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

Vector FloorGuide::turned(const Vector& reference, double degrees) {

    return reference.flattened().normalized().transformed(Xform::rotation_z(degrees, true));
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

std::array<std::array<std::array<Polyline, 2>, 2>, 3> FloorGuide::compute_bed_rails(size_t q) const {

    // one row: the lower and upper layer on the panel's two side planes trimmed alike
    const auto bed_row = [](const std::array<Polyline, 2>& lower, const std::array<Polyline, 2>& upper, const Plane& cut_plane0, const Plane& cut_plane1) {

        const std::vector<Polyline> layers = Polyline::trimmed_alike({lower[0], lower[1], upper[0], upper[1]}, cut_plane0, cut_plane1, EXTENSION);

        return std::array<std::array<Polyline, 2>, 2>{{{layers[0], layers[1]}, {layers[2], layers[3]}}};
    };

    // an outer row: the parabola's +t and +2t projected along the outer rib normal onto the panel's two side planes
    const auto outer_bed_row = [&bed_row](const std::array<Polyline, 3>& parabola, const Plane& side0, const Plane& side1, const Vector& normal, const Plane& cut_plane0, const Plane& cut_plane1) {

        const Xform projection0 = Xform::project_to_plane_by_axis(side0, normal);
        const Xform projection1 = Xform::project_to_plane_by_axis(side1, normal);

        return bed_row({parabola[1].transformed(projection0), parabola[1].transformed(projection1)}, {parabola[2].transformed(projection0), parabola[2].transformed(projection1)}, cut_plane0, cut_plane1);
    };

    const ConstructionPlanes& cp = construction_planes(q);
    const std::array<std::array<Polyline, 3>, 4>& pb = boundary_parabolas(q);
    const CentralPanel& panel = central_panel(q);

    return {
        outer_bed_row(pb[0], cp.inner_ribs[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis(), cp.inner_beams[0][1], cp.wedges[0][0]),
        bed_row({panel.traces[0][1], panel.traces[1][1]}, {panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]),
        outer_bed_row(pb[1], cp.inner_ribs[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis(), cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

std::array<std::vector<std::array<Polyline, 2>>, 3> FloorGuide::compute_beds(size_t q) const {

    std::array<std::vector<std::array<Polyline, 2>>, 3> rows;

    for (size_t r = 0; r < 3; r++) {
        const std::array<std::array<Polyline, 2>, 2>& rails = bed_rails(q)[r];
        std::vector<std::array<Polyline, 2>> plates;

        for (size_t i = 0; i + 1 < rails[0][0].point_count(); i++) {
            const Polyline bottom = Polyline({rails[0][0].get_point(i), rails[0][0].get_point(i + 1), rails[0][1].get_point(i + 1), rails[0][1].get_point(i)}).closed();
            const Polyline top = Polyline({rails[1][0].get_point(i), rails[1][0].get_point(i + 1), rails[1][1].get_point(i + 1), rails[1][1].get_point(i)}).closed();
            plates.push_back({top, bottom});
        }

        rows[r] = plates;
    }

    return rows;
}

std::array<std::array<Polyline, 2>, 6> FloorGuide::compute_tsections(size_t q) const {

    // a t-section: its soffit and +t traces trimmed on its first face and closed into one loop, the same projected onto its second face
    const auto tsection = [](const Polyline& soffit, const Polyline& layer, const Plane& cut_plane0, const Plane& cut_plane1, const Xform& projection10, const Xform& projection11) {

        const std::vector<Point> cut00 = soffit.trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();
        const std::vector<Point> cut01 = soffit.transformed(projection10).trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();
        const std::vector<Point> cut10 = layer.trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();
        const std::vector<Point> cut11 = layer.transformed(projection11).trimmed(cut_plane0, cut_plane1, EXTENSION).get_points();

        std::vector<Point> top = cut00;
        top.insert(top.end(), cut10.rbegin(), cut10.rend());

        std::vector<Point> bottom = cut01;
        bottom.insert(bottom.end(), cut11.rbegin(), cut11.rend());

        return std::array<Polyline, 2>{Polyline(top).closed(), Polyline(bottom).closed()};
    };

    // beside an outer rib face: the parabola and its +t projected along the outer rib normal onto the face, the soffit continued to the far face along the sweep, the +t along the panel
    const auto outer_tsection = [&tsection](const std::array<Polyline, 3>& parabola, const std::array<Plane, 2>& faces, const Vector& outer, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1) {

        const Xform projection = Xform::project_to_plane_by_axis(faces[0], outer);

        return tsection(
            parabola[0].transformed(projection), parabola[1].transformed(projection), cut_plane0, cut_plane1,
            Xform::project_to_plane_by_axis(faces[1], sweep),
            Xform::project_to_plane_by_axis(faces[1], outer)
        );
    };

    const ConstructionPlanes& cp = construction_planes(q);
    const std::array<std::array<Polyline, 3>, 4>& pb = boundary_parabolas(q);
    const CentralPanel& panel = central_panel(q);
    const Vector outer0 = cp.outer_ribs[0][0].z_axis();
    const Vector outer1 = cp.outer_ribs[1][0].z_axis();
    const std::array<std::array<Plane, 2>, 6>& ts = cp.tsections;

    return {
        outer_tsection(pb[0], ts[0], outer0, outer0, cp.inner_beams[0][1], cp.wedges[0][0]),
        outer_tsection(pb[0], ts[1], outer0, panel.rib_sweep, cp.inner_beams[0][1], cp.wedges[0][0]),
        tsection(panel.traces[0][0], panel.traces[0][1], cp.inner_beams[1][1], cp.wedges[1][0], Xform::project_to_plane_by_axis(ts[2][1], panel.rib_sweep), Xform::project_to_plane_by_axis(ts[2][1], panel.ruling)),
        tsection(panel.traces[1][0], panel.traces[1][1], cp.inner_beams[1][1], cp.wedges[1][0], Xform::project_to_plane_by_axis(ts[3][1], panel.rib_sweep), Xform::project_to_plane_by_axis(ts[3][1], panel.ruling)),
        outer_tsection(pb[1], ts[4], outer1, panel.rib_sweep, cp.inner_beams[2][1], cp.wedges[2][0]),
        outer_tsection(pb[1], ts[5], outer1, outer1, cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

std::array<std::array<Polyline, 2>, 2> FloorGuide::compute_outer_ribs(size_t q) const {

    const ConstructionPlanes& cp = construction_planes(q);
    const std::array<std::array<Polyline, 3>, 4>& parabolas = boundary_parabolas(q);
    const std::array<Plane, 2> ends = rib_seam_ends(q);

    return {
        rib(parabolas[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][1].z_axis(), cp.wedges[0][0], ends[0], false),
        rib(parabolas[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][1].z_axis(), cp.wedges[2][0], ends[1], false),
    };
}

std::array<std::array<Polyline, 2>, 2> FloorGuide::compute_inner_ribs(size_t q) const {

    const ConstructionPlanes& cp = construction_planes(q);
    const std::array<std::array<Polyline, 3>, 4>& parabolas = boundary_parabolas(q);
    const Vector& sweep = central_panel(q).rib_sweep;

    return {
        rib(parabolas[2][0], cp.inner_ribs[0][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true),
        rib(parabolas[3][0], cp.inner_ribs[1][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true),
    };
}

std::array<std::array<Polyline, 2>, 3> FloorGuide::compute_wedges(size_t q) const {

    const ConstructionPlanes& cp = construction_planes(q);
    const std::array<Plane, 3>& beds = bed_top_planes(q);
    const Plane top = Plane::xy_plane_at(0.0);
    const std::array<std::array<Plane, 2>, 3> ribs = {{
        {cp.outer_ribs[0][1], cp.inner_ribs[0][0]},
        {cp.inner_ribs[0][1], cp.inner_ribs[1][1]},
        {cp.inner_ribs[1][0], cp.outer_ribs[1][1]},
    }};

    std::array<std::array<Polyline, 2>, 3> blocks;

    for (size_t i = 0; i < 3; i++)
        blocks[i] = loft({ribs[i][0], beds[i], ribs[i][1], top}, cp.wedges[i][0], cp.wedges[i][1]);

    return blocks;
}

std::array<std::array<Polyline, 2>, 3> FloorGuide::compute_inner_beams(size_t q) const {

    const ConstructionPlanes& cp = construction_planes(q);
    const Plane side0 = Plane::xy_plane_at(0.0);
    const Plane side1 = Plane::xy_plane_at(soffit);

    // the seam beams run on through the outer rib band to the bay's outer face
    return {
        loft({cp.outer_ribs[0][0], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[0][0], cp.inner_beams[0][1]),
        loft({cp.inner_beams[0][1], side0, cp.inner_beams[2][1], side1}, cp.inner_beams[1][0], cp.inner_beams[1][1]),
        loft({cp.outer_ribs[1][0], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[2][0], cp.inner_beams[2][1]),
    };
}

std::array<std::array<Polyline, 2>, 9> FloorGuide::compute_oculus() const {

    const Plane side0 = Plane::xy_plane_at(0.0);
    const Plane side1 = Plane::xy_plane_at(soffit + size_tsections);
    const Plane side2 = Plane::xy_plane_at(soffit);
    const Plane side3 = Plane::xy_plane_at(soffit + size_tsections * 2.0);

    std::vector<Plane> tilted;
    std::vector<Plane> inner;

    for (size_t q = 0; q < 4; q++) {
        tilted.push_back(construction_planes(q).inner_beams[1][0]);
        inner.push_back(ring_inner(q));
    }

    // four ring beams, four bottom wedges, the inner plate
    std::array<std::array<Polyline, 2>, 9> plates;

    for (size_t i = 0; i < 4; i++)
        plates[i] = loft({side2, tilted[(i + 1) % 4], side0, inner[(i + 3) % 4]}, tilted[i], inner[i], true);

    for (size_t i = 0; i < 4; i++) {
        const std::vector<Plane> sides = {inner[i], inner[(i + 1) % 4], inner[i].translate_by_normal(-size_tsections), inner[(i + 3) % 4].translate_by_normal(-size_tsections)};
        plates[4 + i] = loft(sides, side2, side1);
    }

    plates[8] = loft(inner, side1, side3);

    return plates;
}

Plane FloorGuide::ring_inner(size_t q) const {
    return construction_planes(q).inner_beams[1][1].translate_by_normal(-size_inner_beams * 2.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Column cutters
// ═══════════════════════════════════════════════════════════════════════════

std::array<std::array<Polyline, 2>, 6> FloorGuide::compute_column_cutters(size_t q) const {

    // a cutter quad stretched in its own plane: its long sides by the margin at both ends, then its short sides inwards, both for a top quad, only the first for a bottom one
    const auto stretch = [](std::vector<Point> quad, bool top) {

        Polyline::extend_line_segment(quad[0], quad[1], CUTTER_MARGIN, CUTTER_MARGIN);
        Polyline::extend_line_segment(quad[2], quad[3], CUTTER_MARGIN, CUTTER_MARGIN);

        const Vector d2 = (quad[2] - quad[1]).normalized() * CUTTER_MARGIN;
        const Vector d3 = (quad[0] - quad[3]).normalized() * CUTTER_MARGIN;
        quad[0] = quad[0] - d2;
        quad[1] = quad[1] - d2;

        if (top) {
            quad[2] = quad[2] - d3;
            quad[3] = quad[3] - d3;
        }

        return quad;
    };

    const std::vector<Point> head = quarter_column_polygon(q);
    const Vector down(0.0, 0.0, -1.0);
    const Plane xy2 = Plane::xy_plane_at(column_levels(q)[2]);
    const std::vector<Plane> fan_bottom = {Plane::from_line(Line::from_points(head[0], head[1]), -Vector::z_axis()), Plane::from_line(Line::from_points(head[1], head[2]), down), Plane::from_line(Line::from_points(head[3], head[4]), down), Plane::from_line(Line::from_points(head[4], head[0]), -Vector::z_axis())};
    const std::array<std::vector<Point>, 3> faces = {column_face(q, 0), column_face(q, 1), column_face(q, 2)};
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

    std::array<std::array<Polyline, 2>, 6> plates;

    for (size_t i = 0; i < quads.size(); i++) {
        const std::vector<Point> quad = stretch(quads[i], i < 3);
        const Vector normal = (quad[2] - quad[1]).cross(quad[1] - quad[0]).normalized() * CUTTER_MARGIN;
        const Polyline top = Polyline(quad).closed();
        plates[i] = {top, top.translated(normal)};
    }

    return plates;
}

std::vector<Point> FloorGuide::column_face(size_t q, size_t i) const {

    const ConstructionPlanes& cp = construction_planes(q);
    const std::vector<Point> head = quarter_column_polygon(q);
    const std::array<Plane, 5> fan = {Plane::from_line(Line::from_points(head[0], head[1]), -Vector::z_axis()), cp.wedges[0][0], cp.wedges[1][0], cp.wedges[2][0], Plane::from_line(Line::from_points(head[4], head[0]), -Vector::z_axis())};
    const Plane xy0 = Plane::xy_plane_at(column_levels(q)[0]);
    const Plane xy1 = Plane::xy_plane_at(column_levels(q)[1]);

    return {
        Intersection::plane_plane_plane(xy0, fan[i], fan[i + 1]).value(),
        Intersection::plane_plane_plane(xy0, fan[i + 1], fan[i + 2]).value(),
        Intersection::plane_plane_plane(xy1, fan[i + 1], fan[i + 2]).value(),
        Intersection::plane_plane_plane(xy1, fan[i], fan[i + 1]).value(),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

std::array<Polyline, 2> FloorGuide::loft(const std::vector<Plane>& sides, const Plane& bottom, const Plane& top, bool flip) {

    const Polyline at_bottom = Polyline::from_planes(sides, bottom);
    const Polyline at_top = Polyline::from_planes(sides, top);

    return flip ? std::array<Polyline, 2>{at_bottom, at_top} : std::array<Polyline, 2>{at_top, at_bottom};
}

double FloorGuide::thickness(const std::array<Polyline, 2>& loops) {
    return Point::distance(loops[0].area_centroid(), loops[1].area_centroid());
}

Point FloorGuide::body(const std::array<Polyline, 2>& loops) {
    return Point::mid_point(loops[0].area_centroid(), loops[1].area_centroid());
}

double FloorGuide::end_level(const std::array<Polyline, 2>& loops, const Plane& end) {

    double level = 0.0;

    for (const Polyline& loop : loops)
        for (const Point& point : loop.get_points())
            if (std::abs(end.signed_distance(point)) <= 1e-6)
                level = std::min(level, point[2]);

    return level;
}

std::array<Polyline, 2> FloorGuide::rib(const Polyline& trace, const Plane& face1, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

    // a rib face's loop: the trace closed up to z 0 over the end planes; an inner rib also ends its base on the second plane
    const auto rib_loop = [](const std::vector<Point>& pts, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

        const Vector span = (pts.back() - pts.front()).flattened();
        const Plane rib_plane = Plane::from_point_normal(pts.front(), span.cross(Vector::z_axis()));
        const Point p0 = Intersection::plane_plane_plane(cut_plane0, Plane::xy_plane_at(0.0), rib_plane).value();
        Point p1(pts.back()[0], pts.back()[1], 0.0);

        if (inner)
            p1 = Intersection::line_plane(Line::from_points(p0, p1), cut_plane1, false).value();

        std::vector<Point> loop = {p1, p0};
        loop.insert(loop.end(), pts.begin(), pts.end());

        return Polyline(loop).closed();
    };

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

// ═══════════════════════════════════════════════════════════════════════════
// Drawing
// ═══════════════════════════════════════════════════════════════════════════

void FloorGuide::draw() {

    // one member family as it is drawn: its name, its colour, its plan quads and face planes in member order, and the index of its first rib parabola, -1 for none
    struct DrawnFamily {
        std::string name;
        Color color;
        std::span<const Polyline> quads; // a family's members, however many
        std::span<const std::array<Plane, 2>> planes;
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
        const ConstructionPlanes& cp = construction_planes(q);
        const ConstructionQuads& quads = construction_quads(q);
        const std::shared_ptr<TreeNode> quarter = add_group("quarter" + suffix);
        const std::shared_ptr<TreeNode> plan = add_group("plan" + suffix, quarter);

        line(Polyline(quarter_polygon(q)).closed(), "polygon" + suffix, Color::black(), 3.0, plan);
        line(Polyline(quarter_column_polygon(q)).closed(), "size_column_head" + suffix, Color::black(), 3.0, plan);
        Point corner = oculus_points[q];
        corner.name = "oculus_corner" + suffix;
        corner.width = 10.0;
        add_point(corner, plan);

        const std::array<DrawnFamily, 5> families = {{
            {"outer_ribs", Color(232.0f / 255.0f, 71.0f / 255.0f, 139.0f / 255.0f, 1.0f, "outer_ribs"), quads.outer_ribs, cp.outer_ribs, 0},
            {"inner_ribs", Color(242.0f / 255.0f, 204.0f / 255.0f, 12.0f / 255.0f, 1.0f, "inner_ribs"), quads.inner_ribs, cp.inner_ribs, 2},
            {"inner_beams", Color(124.0f / 255.0f, 124.0f / 255.0f, 124.0f / 255.0f, 1.0f, "inner_beams"), quads.inner_beams, cp.inner_beams, -1},
            {"wedges", Color(168.0f / 255.0f, 168.0f / 255.0f, 168.0f / 255.0f, 1.0f, "wedges"), quads.wedges, cp.wedges, -1},
            {"tsections", Color(245.0f / 255.0f, 216.0f / 255.0f, 144.0f / 255.0f, 1.0f, "tsections"), quads.tsections, cp.tsections, -1},
        }};

        for (const DrawnFamily& family : families) {
            const std::string& name = family.name;
            const Color& color = family.color;
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

                const std::array<Polyline, 3>& parabola = boundary_parabolas(q)[static_cast<size_t>(family.parabola) + i];
                line(parabola[0], "soffit", color, 2.0, member);
                line(parabola[1], "tsections_top", color, 1.0, member);
                line(parabola[2], "beds_top", color, 1.0, member);
            }
        }
    }
}

}
