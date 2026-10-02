#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

const double CUTTER_MARGIN = 100.0; // how far column cutter quads overshoot and how thick they are

// ═══════════════════════════════════════════════════════════════════════════
// Ribs
// ═══════════════════════════════════════════════════════════════════════════

/// A rib: the parabola trimmed by its two end planes, closed down to z 0, and that outline projected through the rib thickness; an inner rib also ends its base on the second plane.
static Outline rib(const Polyline& parabola, const Xform& projection, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

    std::vector<Point> pts = trim(parabola, cut_plane0, cut_plane1).get_points();
    const double d0 = std::abs((pts.front() - cut_plane0.origin()).dot(cut_plane0.z_axis()));
    const double d1 = std::abs((pts.back() - cut_plane0.origin()).dot(cut_plane0.z_axis()));

    if (d0 > d1)
        std::reverse(pts.begin(), pts.end());

    const Vector span(pts.back()[0] - pts.front()[0], pts.back()[1] - pts.front()[1], 0.0);
    const Plane rib_plane = Plane::from_point_normal(pts.front(), span.cross(Vector(0.0, 0.0, 1.0)));
    const Point p0 = plane_plane_plane(cut_plane0, level(0.0), rib_plane).value();
    Point p1(pts.back()[0], pts.back()[1], 0.0);

    if (inner)
        p1 = line_plane(Line::from_points(p0, p1), cut_plane1).value();

    std::vector<Point> loop = {p1, p0};
    loop.insert(loop.end(), pts.begin(), pts.end());
    loop.push_back(p1);
    const Polyline top(loop);

    return {top, top.transformed(projection)};
}

std::vector<Outline> FloorGuide::outer_ribs() const {

    const ConstructionPlanes cp = construction_planes();
    const std::vector<std::array<Polyline, 3>> parabolas = boundary_parabolas();
    const Xform projection0 = Xform::project_to_plane_by_axis(cp.outer_ribs[0][1], cp.outer_ribs[0][1].z_axis());
    const Xform projection1 = Xform::project_to_plane_by_axis(cp.outer_ribs[1][1], cp.outer_ribs[1][1].z_axis());

    return {
        rib(parabolas[0][0], projection0, cp.wedges[0][0], cp.inner_beams[0][0], false),
        rib(parabolas[1][0], projection1, cp.wedges[2][0], cp.inner_beams[2][0], false),
    };
}

std::vector<Outline> FloorGuide::inner_ribs() const {

    const ConstructionPlanes cp = construction_planes();
    const std::vector<std::array<Polyline, 3>> parabolas = boundary_parabolas();
    const Vector across = cp.inner_ribs[0][1].z_axis() - cp.inner_ribs[1][1].z_axis();
    const Xform projection0 = Xform::project_to_plane_by_axis(cp.inner_ribs[0][1], across);
    const Xform projection1 = Xform::project_to_plane_by_axis(cp.inner_ribs[1][1], across);

    return {
        rib(parabolas[2][0], projection0, cp.wedges[1][0], cp.inner_beams[1][1], true),
        rib(parabolas[3][0], projection1, cp.wedges[1][0], cp.inner_beams[1][1], true),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Beams and wedges
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Outline> FloorGuide::inner_beams() const {

    const ConstructionPlanes cp = construction_planes();
    const Plane side0 = level(0.0);
    const Plane side1 = level(-static_h());

    return {
        loft_planes({cp.outer_ribs[0][1], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[0][0], cp.inner_beams[0][1]),
        loft_planes({cp.inner_beams[0][1], side0, cp.inner_beams[2][1], side1}, cp.inner_beams[1][0], cp.inner_beams[1][1]),
        loft_planes({cp.outer_ribs[1][1], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[2][0], cp.inner_beams[2][1]),
    };
}

std::vector<Outline> FloorGuide::wedges_inner_beams() const {

    const ConstructionPlanes cp = construction_planes();
    const std::vector<Plane> beds = bed_top_planes();
    const Plane top = level(0.0);
    const std::array<std::array<Plane, 2>, 3> ribs = {{
        {cp.outer_ribs[0][1], cp.inner_ribs[0][0]},
        {cp.inner_ribs[0][1], cp.inner_ribs[1][1]},
        {cp.inner_ribs[1][0], cp.outer_ribs[1][1]},
    }};

    std::vector<Outline> wedges;

    for (size_t i = 0; i < 3; i++)
        wedges.push_back(loft_planes({ribs[i][0], beds[i], ribs[i][1], top}, cp.wedges[i][0], cp.wedges[i][1]));

    return wedges;
}

// ═══════════════════════════════════════════════════════════════════════════
// T-sections and beds
// ═══════════════════════════════════════════════════════════════════════════

/// A t-section: two parabolas projected onto its first face and trimmed, closed into one outline, and the same on its second face.
static Outline tsection(const Polyline& parabola0, const Polyline& parabola1, const Plane& cut_plane0, const Plane& cut_plane1, const Xform& projection0, const Xform& projection10, const Xform& projection11) {

    const Polyline face00 = parabola0.transformed(projection0);
    const Polyline face10 = parabola1.transformed(projection0);
    const std::vector<Point> cut00 = trim(face00, cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut01 = trim(face00.transformed(projection10), cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut10 = trim(face10, cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut11 = trim(face10.transformed(projection11), cut_plane0, cut_plane1).get_points();

    std::vector<Point> top = cut00;
    top.insert(top.end(), cut10.rbegin(), cut10.rend());
    top.push_back(cut00.front());

    std::vector<Point> bottom = cut01;
    bottom.insert(bottom.end(), cut11.rbegin(), cut11.rend());
    bottom.push_back(cut01.front());

    return {Polyline(top), Polyline(bottom)};
}

std::vector<Outline> FloorGuide::tsections() const {

    const ConstructionPlanes cp = construction_planes();
    const std::vector<std::array<Polyline, 3>> pb = boundary_parabolas();
    const Vector outer0 = cp.outer_ribs[0][0].z_axis();
    const Vector outer1 = cp.outer_ribs[1][0].z_axis();
    const Vector across = cp.inner_ribs[0][0].z_axis() - cp.inner_ribs[1][0].z_axis();
    const std::vector<std::array<Plane, 2>>& ts = cp.t_sections;

    return {
        tsection(
            pb[0][0], pb[0][1], cp.inner_beams[0][1], cp.wedges[0][0],
            Xform::project_to_plane_by_axis(ts[0][0], outer0),
            Xform::project_to_plane_by_axis(ts[0][1], outer0),
            Xform::project_to_plane_by_axis(ts[0][1], outer0)
        ),
        tsection(
            pb[0][0], pb[0][1], cp.inner_beams[0][1], cp.wedges[0][0],
            Xform::project_to_plane_by_axis(ts[1][0], outer0),
            Xform::project_to_plane_by_axis(ts[1][1], across),
            Xform::project_to_plane_by_axis(ts[1][1], outer0)
        ),
        tsection(
            pb[2][0], pb[2][1], cp.inner_beams[1][1], cp.wedges[1][0],
            Xform::project_to_plane_by_axis(ts[2][0], across),
            Xform::project_to_plane_by_axis(ts[2][1], across),
            Xform::project_to_plane_by_axis(ts[2][1], across)
        ),
        tsection(
            pb[3][0], pb[3][1], cp.inner_beams[1][1], cp.wedges[1][0],
            Xform::project_to_plane_by_axis(ts[4][0], across),
            Xform::project_to_plane_by_axis(ts[4][1], across),
            Xform::project_to_plane_by_axis(ts[4][1], across)
        ),
        tsection(
            pb[1][0], pb[1][1], cp.inner_beams[2][1], cp.wedges[2][0],
            Xform::project_to_plane_by_axis(ts[3][0], outer1),
            Xform::project_to_plane_by_axis(ts[3][1], across),
            Xform::project_to_plane_by_axis(ts[3][1], outer1)
        ),
        tsection(
            pb[1][0], pb[1][1], cp.inner_beams[2][1], cp.wedges[2][0],
            Xform::project_to_plane_by_axis(ts[5][0], outer1),
            Xform::project_to_plane_by_axis(ts[5][1], outer1),
            Xform::project_to_plane_by_axis(ts[5][1], outer1)
        ),
    };
}

/// One bed row: the lower and upper parabola trimmed, each projected onto the panel's two side planes, one quad pair per segment.
static std::vector<Outline> bed_row(const Polyline& parabola0, const Polyline& parabola1, const Plane& cut_plane0, const Plane& cut_plane1, const Xform& projection0, const Xform& projection1) {

    const std::array<std::vector<Point>, 2> lower = projected(trim(parabola0, cut_plane0, cut_plane1), projection0, projection1);
    const std::array<std::vector<Point>, 2> upper = projected(trim(parabola1, cut_plane0, cut_plane1), projection0, projection1);

    std::vector<Outline> plates;

    for (size_t i = 0; i + 1 < lower[0].size(); i++) {
        const Polyline bottom({lower[0][i], lower[0][i + 1], lower[1][i + 1], lower[1][i], lower[0][i]});
        const Polyline top({upper[0][i], upper[0][i + 1], upper[1][i + 1], upper[1][i], upper[0][i]});
        plates.push_back({top, bottom});
    }

    return plates;
}

std::vector<std::vector<Outline>> FloorGuide::beds() const {

    const ConstructionPlanes cp = construction_planes();
    const std::vector<std::array<Polyline, 3>> pb = boundary_parabolas();
    const Vector outer0 = cp.outer_ribs[0][0].z_axis();
    const Vector outer1 = cp.outer_ribs[1][0].z_axis();
    const Vector across = cp.inner_ribs[0][0].z_axis() - cp.inner_ribs[1][0].z_axis();

    return {
        bed_row(
            pb[0][1], pb[0][2], cp.inner_beams[0][1], cp.wedges[0][0],
            Xform::project_to_plane_by_axis(cp.inner_ribs[0][0], outer0),
            Xform::project_to_plane_by_axis(cp.outer_ribs[0][1], outer0)
        ),
        bed_row(
            pb[2][1], pb[2][2], cp.inner_beams[1][1], cp.wedges[1][0],
            Xform::project_to_plane_by_axis(cp.inner_ribs[0][1], across),
            Xform::project_to_plane_by_axis(cp.inner_ribs[1][1], across)
        ),
        bed_row(
            pb[1][1], pb[1][2], cp.inner_beams[2][1], cp.wedges[2][0],
            Xform::project_to_plane_by_axis(cp.inner_ribs[1][0], outer1),
            Xform::project_to_plane_by_axis(cp.outer_ribs[1][1], outer1)
        ),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Outline> FloorGuide::oculus(const std::vector<FloorGuide>& quarters) {

    const FloorGuide& guide = quarters.front();
    const size_t n = quarters.size();
    const Plane side0 = level(0.0);
    const Plane side1 = level(-guide.static_h() + guide.size_tsections);
    const Plane side2 = level(-guide.static_h());
    const Plane side3 = level(-guide.static_h() + guide.size_tsections * 2.0);

    std::vector<Plane> outer;
    std::vector<Plane> inner;

    for (const FloorGuide& quarter : quarters) {
        const ConstructionPlanes cp = quarter.construction_planes();
        outer.push_back(cp.inner_beams[1][0]);
        inner.push_back(offset(cp.inner_beams[1][1], -guide.size_inner_beams * 2.0));
    }

    std::vector<Outline> plates;

    for (size_t i = 0; i < n; i++)
        plates.push_back(loft_planes({side2, outer[(i + 1) % n], side0, inner[(i + n - 1) % n]}, outer[i], inner[i], true));

    for (size_t i = 0; i < n; i++) {
        const std::vector<Plane> sides = {inner[i], inner[(i + 1) % n], offset(inner[i], -guide.size_tsections), offset(inner[(i + n - 1) % n], -guide.size_tsections)};
        plates.push_back(loft_planes(sides, side2, side1));
    }

    plates.push_back(loft_planes(inner, side1, side3));

    return plates;
}

// ═══════════════════════════════════════════════════════════════════════════
// Column cutters
// ═══════════════════════════════════════════════════════════════════════════

/// The cutter quad stretched in its own plane: its long sides by the margin at both ends, then its short sides inwards, both short sides for a top quad and only the first for a bottom one.
static std::vector<Point> stretch(std::vector<Point> quad, bool top) {

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

std::vector<Outline> FloorGuide::column_cutters() const {

    const ConstructionPlanes cp = construction_planes();
    const std::vector<Point> column = quarter_column_polygon();
    const Vector down(0.0, 0.0, -1.0);
    const Plane xy0 = level(0.0);
    const Plane xy1 = level(-height - size_tsections * 1.65);
    const Plane xy2 = level(-column_head_depth);
    const Plane side0 = edge_plane(edge(column, 0), down);
    const Plane side1 = edge_plane(edge(column, 4), down);
    const std::vector<Plane> fan_top = {side0, cp.wedges[0][0], cp.wedges[1][0], cp.wedges[2][0], side1};
    const std::vector<Plane> fan_bottom = {side0, edge_plane(edge(column, 1), down), edge_plane(edge(column, 3), down), side1};

    std::vector<Point> p0;
    std::vector<Point> p1;
    std::vector<Point> p2;

    for (size_t i = 0; i + 1 < fan_top.size(); i++) {
        p0.push_back(plane_plane_plane(xy0, fan_top[i], fan_top[i + 1]).value());
        p1.push_back(plane_plane_plane(xy1, fan_top[i], fan_top[i + 1]).value());
    }

    for (size_t i = 0; i + 1 < fan_bottom.size(); i++)
        p2.push_back(plane_plane_plane(xy2, fan_bottom[i], fan_bottom[i + 1]).value());

    const Vector quarter = (p2[2] - p2[0]) * 0.25;
    const std::vector<std::vector<Point>> quads = {
        {p0[0], p0[1], p1[1], p1[0]},
        {p0[1], p0[2], p1[2], p1[1]},
        {p0[2], p0[3], p1[3], p1[2]},
        {p1[0], p1[1], p2[1], p2[0]},
        {p1[1], p1[2], p2[1] + quarter, p2[1] - quarter},
        {p1[2], p1[3], p2[2], p2[1]},
    };

    std::vector<Outline> plates;

    for (size_t i = 0; i < quads.size(); i++) {
        const std::vector<Point> quad = stretch(quads[i], i < 3);
        const Vector normal = (quad[2] - quad[1]).cross(quad[1] - quad[0]).normalized() * CUTTER_MARGIN;
        const Polyline top = Polyline(quad).closed();
        plates.push_back({top, top.transformed(Xform::translation(normal[0], normal[1], normal[2]))});
    }

    return plates;
}

}
