#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

const double INSIDE = 1e-9; // mm a point may sit outside a clipping edge and still count as on it

/// A point of a plane's own coordinates.
struct Point2 {
    double x = 0.0; // Along the plane's x axis.
    double y = 0.0; // Along the plane's y axis.
};

/// The points of a closed outline loop without its closing point.
static std::vector<Point> loop_points(const Polyline& polyline) {

    std::vector<Point> points = polyline.get_points();

    if (polyline.is_closed())
        points.pop_back();

    return points;
}

/// The signed distance of a point from a plane.
static double distance(const Point& point, const Plane& plane) {
    return (point - plane.origin()).dot(plane.z_axis());
}

/// The distance of a point from a polyline, over its segments.
static double distance(const Point& point, const Polyline& polyline) {

    double best = 1e300;

    for (size_t i = 0; i + 1 < polyline.point_count(); i++) {
        const Point a = polyline.get_point(i);
        const Vector ab = polyline.get_point(i + 1) - a;
        const double t = std::clamp((point - a).dot(ab) / std::max(ab.dot(ab), 1e-300), 0.0, 1.0);
        best = std::min(best, (point - (a + ab * t)).magnitude());
    }

    return best;
}

/// The signed plan angle in degrees from the line along a to the vector b, the line's sense picked to make it the smaller turn.
static double plan_angle(const Vector& a, const Vector& b) {

    const Vector x = a.dot(b) < 0.0 ? -a : a;
    const double cross = x[0] * b[1] - x[1] * b[0];
    const double dot = x[0] * b[0] + x[1] * b[1];

    return std::atan2(cross, dot) * 180.0 / M_PI;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plane polygons
// ═══════════════════════════════════════════════════════════════════════════

/// The points in the coordinates of the plane.
static std::vector<Point2> in_plane(const std::vector<Point>& points, const Plane& plane) {

    std::vector<Point2> result;

    for (const Point& point : points)
        result.push_back({(point - plane.origin()).dot(plane.x_axis()), (point - plane.origin()).dot(plane.y_axis())});

    return result;
}

/// The signed area of a polygon, positive counter-clockwise.
static double signed_area(const std::vector<Point2>& polygon) {

    double area = 0.0;

    for (size_t i = 0; i < polygon.size(); i++) {
        const Point2& a = polygon[i];
        const Point2& b = polygon[(i + 1) % polygon.size()];
        area += a.x * b.y - b.x * a.y;
    }

    return 0.5 * area;
}

/// The polygon counter-clockwise.
static std::vector<Point2> counter_clockwise(std::vector<Point2> polygon) {

    if (signed_area(polygon) < 0.0)
        std::reverse(polygon.begin(), polygon.end());

    return polygon;
}

/// How far p lies left of the edge a b, in mm.
static double left_of(const Point2& a, const Point2& b, const Point2& p) {

    const double length = std::hypot(b.x - a.x, b.y - a.y);

    return ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x)) / std::max(length, 1e-300);
}

/// The polygon clipped by a convex one, both counter-clockwise (Sutherland-Hodgman).
static std::vector<Point2> clipped(std::vector<Point2> polygon, const std::vector<Point2>& convex) {

    for (size_t i = 0; i < convex.size() && !polygon.empty(); i++) {
        const Point2& a = convex[i];
        const Point2& b = convex[(i + 1) % convex.size()];
        std::vector<Point2> result;

        for (size_t j = 0; j < polygon.size(); j++) {
            const Point2& p = polygon[j];
            const Point2& q = polygon[(j + 1) % polygon.size()];
            const double dp = left_of(a, b, p);
            const double dq = left_of(a, b, q);

            if (dp >= -INSIDE)
                result.push_back(p);

            if ((dp >= -INSIDE) != (dq >= -INSIDE)) {
                const double t = dp / (dp - dq);
                result.push_back({p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t});
            }
        }

        polygon = result;
    }

    return polygon;
}

/// Whether a sorts before b, by x and then y.
static bool lexicographic(const Point2& a, const Point2& b) {
    return a.x < b.x || (a.x == b.x && a.y < b.y);
}

/// The convex hull of points, counter-clockwise (monotone chain).
static std::vector<Point2> hull(std::vector<Point2> points) {

    std::sort(points.begin(), points.end(), lexicographic);
    std::vector<Point2> result(2 * points.size());
    size_t k = 0;

    for (size_t i = 0; i < points.size(); i++) {
        while (k >= 2 && left_of(result[k - 2], result[k - 1], points[i]) <= 0.0)
            k--;
        result[k++] = points[i];
    }

    for (size_t i = points.size() - 1, t = k + 1; i > 0; i--) {
        while (k >= t && left_of(result[k - 2], result[k - 1], points[i - 1]) <= 0.0)
            k--;
        result[k++] = points[i - 1];
    }

    result.resize(k > 0 ? k - 1 : 0);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Measures
// ═══════════════════════════════════════════════════════════════════════════

/// The farthest corner of a rib's two end faces from its end planes.
static double end_face_offset(const Outline& rib, const Plane& cut_plane0, const Plane& cut_plane1) {

    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    const size_t n = top.size();
    double worst = 0.0;

    for (const Point& point : {top[1], top[2], bottom[2], bottom[1]})
        worst = std::max(worst, std::abs(distance(point, cut_plane0)));

    for (const Point& point : {top[0], top[n - 2], bottom[n - 2], bottom[0]})
        worst = std::max(worst, std::abs(distance(point, cut_plane1)));

    return worst;
}

/// The farthest corner of the four ribs' end faces from their end planes.
static double end_face_planarity(const Quarter& quarter) {

    const ConstructionPlanes& cp = quarter.geometry().planes;
    const std::vector<Outline> outer = quarter.outer_ribs();
    const std::vector<Outline> inner = quarter.inner_ribs();

    return std::max({
        end_face_offset(outer[0], cp.wedges[0][0], cp.inner_beams[0][0]),
        end_face_offset(outer[1], cp.wedges[2][0], cp.inner_beams[2][0]),
        end_face_offset(inner[0], cp.wedges[1][0], cp.inner_beams[1][1]),
        end_face_offset(inner[1], cp.wedges[1][0], cp.inner_beams[1][1]),
    });
}

/// The farthest bed underside corner from the top loop of the flange beside it: per row, side 0 and side 1 against their two t-sections.
static double bed_flange_coincidence(const Quarter& quarter) {

    const std::vector<std::vector<Outline>> beds = quarter.beds();
    const std::vector<Outline> flanges = quarter.tsections();
    const std::array<std::array<size_t, 2>, 3> beside = {{{1, 0}, {2, 3}, {4, 5}}};
    double worst = 0.0;

    for (size_t row = 0; row < 3; row++)
        for (const Outline& bed : beds[row]) {
            const std::vector<Point> under = bed.bottom.get_points();
            worst = std::max({worst, distance(under[0], flanges[beside[row][0]].top), distance(under[1], flanges[beside[row][0]].top)});
            worst = std::max({worst, distance(under[2], flanges[beside[row][1]].top), distance(under[3], flanges[beside[row][1]].top)});
        }

    return worst;
}

/// The highest less the lowest of the eight rib face bottoms at the column head: each outer and inner rib's soffit corner on its column end plane, on both faces.
static double rib_level_spread(const Quarter& quarter) {

    std::vector<double> levels;

    for (const std::vector<Outline>& family : {quarter.outer_ribs(), quarter.inner_ribs()})
        for (const Outline& rib : family)
            levels.insert(levels.end(), {rib.top.get_point(2)[2], rib.bottom.get_point(2)[2]});

    return *std::max_element(levels.begin(), levels.end()) - *std::min_element(levels.begin(), levels.end());
}

/// The plan overlap of the four ring beams, pair by pair.
static double ring_overlap(const std::vector<Outline>& ring) {

    const Plane plan = level(0.0);
    std::vector<std::vector<Point2>> footprints;

    for (size_t i = 0; i < 4; i++) {
        std::vector<Point> points = loop_points(ring[i].top);
        const std::vector<Point> bottom = loop_points(ring[i].bottom);
        points.insert(points.end(), bottom.begin(), bottom.end());
        footprints.push_back(hull(in_plane(points, plan)));
    }

    double overlap = 0.0;

    for (size_t i = 0; i < 4; i++)
        for (size_t j = i + 1; j < 4; j++)
            overlap += std::abs(signed_area(clipped(footprints[i], footprints[j])));

    return overlap;
}

/// The area of quarter q's oculus beam face outside ring beam q's outer face, on their shared tilted plane.
static double ring_uncovered(const Floor& floor, size_t q, const Outline& ring_beam) {

    const Plane& tilted = floor.oculus_edges[q].tilted;
    const std::vector<Point2> beam = counter_clockwise(in_plane(loop_points(floor.quarter(q).inner_beams()[1].bottom), tilted));
    const std::vector<Point2> ring = counter_clockwise(in_plane(loop_points(ring_beam.top), tilted));

    return std::max(0.0, signed_area(beam) - signed_area(clipped(beam, ring)));
}

/// The relations of quarter q and its corner written into the report.
static void measure_quarter(const Floor& floor, size_t q, FloorReport& report) {

    const Quarter quarter = floor.quarter(q);
    const QuarterGeometry& geometry = quarter.geometry();
    const CentralPanel& panel = geometry.central_panel;
    const ColumnCorner& column = floor.columns[q];
    const size_t next = (q + 1) % 4;

    for (const Point& point : loop_points(floor.quarter(next).inner_beams()[2].bottom))
        report.seam_plane_gap[q] = std::max(report.seam_plane_gap[q], std::abs(distance(point, geometry.planes.inner_beams[0][0])));

    report.oculus_corner_gap[q] = (geometry.polygon[2] - floor.geometry[next].polygon[3]).magnitude();
    report.ruling_off_chamfer_deg[q] = plan_angle(column.chamfer_direction, panel.ruling);
    report.ruling_off_oculus_edge_deg[q] = plan_angle(direction(floor.oculus_edges[q].line), panel.ruling);
    report.closure_residual_mm[q] = panel.residual;
    report.end_face_planarity_mm[q] = end_face_planarity(quarter);
    report.bed_flange_coincidence_mm[q] = bed_flange_coincidence(quarter);
    report.central_layer_shift_vs_compas_mm[q] = panel.layer_shift;
    report.wedge_seat_mm[q] = column.wedge_seat;
    report.column_offset_mm[q] = column.column_offset;
    report.rib_level_spread_mm[q] = rib_level_spread(quarter);

    const std::vector<Outline> outer = quarter.outer_ribs();

    for (size_t k = 0; k < 2; k++) {
        report.rib_sweep_obliqueness_deg[q][k] = panel.obliqueness[k];
        report.rib_shear_mm[q][k] = floor.sizes.inner_ribs * std::tan(panel.obliqueness[k] * M_PI / 180.0);
        report.rib_bottom_clearance_mm[q][k] = std::min(outer[k].top.get_point(2)[2], outer[k].bottom.get_point(2)[2]) - column.levels[1];
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Report
// ═══════════════════════════════════════════════════════════════════════════

FloorReport Floor::check() const {

    FloorReport report;
    const std::vector<Outline> ring = oculus();

    for (size_t q = 0; q < 4; q++) {
        measure_quarter(*this, q, report);
        report.ring_uncovered_mm2 += ring_uncovered(*this, q, ring[q]);
    }

    report.ring_overlap_mm2 = ring_overlap(ring);

    return report;
}

bool FloorReport::ok(double tolerance) const {

    for (size_t q = 0; q < 4; q++)
        if (seam_plane_gap[q] > tolerance || oculus_corner_gap[q] > tolerance || closure_residual_mm[q] > tolerance || end_face_planarity_mm[q] > tolerance || bed_flange_coincidence_mm[q] > tolerance)
            return false;

    return ring_overlap_mm2 <= tolerance && ring_uncovered_mm2 <= tolerance;
}

std::string FloorReport::str() const {

    std::string text = fmt::format("floor report: {}\n", ok() ? "ok" : "FAILING");

    for (size_t q = 0; q < 4; q++) {
        text += fmt::format("  quarter {}: seam gap {:.3e}, oculus corner gap {:.3e}, closure {:.3e}, end faces {:.3e}, beds on flanges {:.3e} mm\n", q, seam_plane_gap[q], oculus_corner_gap[q], closure_residual_mm[q], end_face_planarity_mm[q], bed_flange_coincidence_mm[q]);
        text += fmt::format("    ruling {:.3f} deg off the chamfer, {:.3f} off the oculus edge; rib sweep {:.3f} / {:.3f} deg oblique, shear {:.3f} / {:.3f} mm\n", ruling_off_chamfer_deg[q], ruling_off_oculus_edge_deg[q], rib_sweep_obliqueness_deg[q][0], rib_sweep_obliqueness_deg[q][1], rib_shear_mm[q][0], rib_shear_mm[q][1]);
        text += fmt::format("    section layers {:.3f} / {:.3f} mm from compas_tf's; rib bottoms {:.3f} / {:.3f} mm above the cutter level, the eight rib bottoms at the head span {:.3f} mm; seats {:.3f} / {:.3f} / {:.3f} mm; column offset {:.3f} / {:.3f} mm\n", central_layer_shift_vs_compas_mm[q][0], central_layer_shift_vs_compas_mm[q][1], rib_bottom_clearance_mm[q][0], rib_bottom_clearance_mm[q][1], rib_level_spread_mm[q], wedge_seat_mm[q][0], wedge_seat_mm[q][1], wedge_seat_mm[q][2], column_offset_mm[q][0], column_offset_mm[q][1]);
    }

    text += fmt::format("  ring: beams overlap {:.3e} mm2, quarter beam faces uncovered {:.3e} mm2", ring_overlap_mm2, ring_uncovered_mm2);

    return text;
}

}
