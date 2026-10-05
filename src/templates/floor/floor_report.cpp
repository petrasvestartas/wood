#include "pch.h"
#include "src/templates/floor/floor_geometry.h"
#include "closest.h"
#include "convex_hull.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

/// The distance of a point from a polyline, over its segments.
static double distance(const Point& point, const Polyline& polyline) {
    return std::get<2>(Closest::polyline_point(polyline, point));
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

/// The total area of the polygons a boolean of two loops on a plane leaves: clip_type 0 their overlap, 2 a outside b.
static double boolean_area(const Polyline& a, const Polyline& b, const Plane& plane, int clip_type) {

    double area = 0.0;

    for (const Polyline& polygon : Polyline::boolean_op(a, b, plane, clip_type))
        area += polygon_area(polygon);

    return area;
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
        worst = std::max(worst, std::abs(signed_distance(point, cut_plane0)));

    for (const Point& point : {top[0], top[n - 2], bottom[n - 2], bottom[0]})
        worst = std::max(worst, std::abs(signed_distance(point, cut_plane1)));

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

    std::vector<Polyline> footprints;

    for (size_t i = 0; i < 4; i++) {
        std::vector<Point> points = open_points(ring[i].top);
        const std::vector<Point> bottom = open_points(ring[i].bottom);
        points.insert(points.end(), bottom.begin(), bottom.end());

        for (Point& point : points)
            point[2] = 0.0;

        footprints.push_back(Polyline(ConvexHull::hull_2d(points)).closed());
    }

    double overlap = 0.0;

    for (size_t i = 0; i < 4; i++)
        for (size_t j = i + 1; j < 4; j++)
            overlap += boolean_area(footprints[i], footprints[j], level(0.0), 0);

    return overlap;
}

/// The area of quarter q's oculus beam face outside ring beam q's outer face, on their shared tilted plane.
static double ring_uncovered(const Floor& floor, size_t q, const Outline& ring_beam) {
    return boolean_area(floor.quarter(q).inner_beams()[1].bottom, ring_beam.top, floor.oculus_edges[q].tilted, 2);
}

/// The relations of quarter q and its corner written into the report.
static void measure_quarter(const Floor& floor, size_t q, FloorReport& report) {

    const Quarter quarter = floor.quarter(q);
    const QuarterGeometry& geometry = quarter.geometry();
    const CentralPanel& panel = geometry.central_panel;
    const ColumnCorner& column = floor.columns[q];
    const size_t next = (q + 1) % 4;

    for (const Point& point : open_points(floor.quarter(next).inner_beams()[2].bottom))
        report.seam_plane_gap[q] = std::max(report.seam_plane_gap[q], std::abs(signed_distance(point, geometry.planes.inner_beams[0][0])));

    report.oculus_corner_gap[q] = (geometry.polygon[2] - floor.geometry[next].polygon[3]).magnitude();
    report.ruling_off_chamfer_deg[q] = plan_angle(column.chamfer_direction, panel.ruling);
    report.ruling_off_oculus_edge_deg[q] = plan_angle(floor.oculus_edges[q].line.to_direction(), panel.ruling);
    report.closure_residual_mm[q] = panel.residual;
    report.end_face_planarity_mm[q] = end_face_planarity(quarter);
    report.bed_flange_coincidence_mm[q] = bed_flange_coincidence(quarter);
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
        text += fmt::format("    rib bottoms {:.3f} / {:.3f} mm above the cutter level, the eight rib bottoms at the head span {:.3f} mm; seats {:.3f} / {:.3f} / {:.3f} mm; column offset {:.3f} / {:.3f} mm\n", rib_bottom_clearance_mm[q][0], rib_bottom_clearance_mm[q][1], rib_level_spread_mm[q], wedge_seat_mm[q][0], wedge_seat_mm[q][1], wedge_seat_mm[q][2], column_offset_mm[q][0], column_offset_mm[q][1]);
    }

    text += fmt::format("  ring: beams overlap {:.3e} mm2, quarter beam faces uncovered {:.3e} mm2", ring_overlap_mm2, ring_uncovered_mm2);

    return text;
}

}
