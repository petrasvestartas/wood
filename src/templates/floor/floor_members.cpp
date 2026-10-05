#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

const double CUTTER_MARGIN = 100.0; // how far column cutter quads overshoot and how thick they are

// ═══════════════════════════════════════════════════════════════════════════
// Ribs
// ═══════════════════════════════════════════════════════════════════════════

/// A rib face's outline: the trimmed soffit trace closed up to z 0 over the end planes; an inner rib also ends its base on the second plane.
static Polyline rib_loop(const std::vector<Point>& pts, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

    const Vector span(pts.back()[0] - pts.front()[0], pts.back()[1] - pts.front()[1], 0.0);
    const Plane rib_plane = Plane::from_point_normal(pts.front(), span.cross(Vector(0.0, 0.0, 1.0)));
    const Point p0 = plane_plane_plane(cut_plane0, level(0.0), rib_plane).value();
    Point p1(pts.back()[0], pts.back()[1], 0.0);

    if (inner)
        p1 = line_plane(Line::from_points(p0, p1), cut_plane1).value();

    std::vector<Point> loop = {p1, p0};
    loop.insert(loop.end(), pts.begin(), pts.end());
    loop.push_back(p1);

    return Polyline(loop);
}

/// A rib: its soffit trace trimmed by the two end planes on its first face, and on its second face the trace swept along the rib with its end corners cut on the end planes, the first and last facet extended (R4).
static Outline rib(const Polyline& trace, const Plane& face1, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1, bool inner) {

    std::vector<Point> pts = trim(trace, cut_plane0, cut_plane1).get_points();
    const double d0 = std::abs((pts.front() - cut_plane0.origin()).dot(cut_plane0.z_axis()));
    const double d1 = std::abs((pts.back() - cut_plane0.origin()).dot(cut_plane0.z_axis()));

    if (d0 > d1)
        std::reverse(pts.begin(), pts.end());

    const Xform projection = Xform::project_to_plane_by_axis(face1, sweep);
    std::vector<Point> far;

    for (const Point& point : pts)
        far.push_back(point.transformed(projection));

    const size_t n = far.size();
    far[0] = line_plane(Line::from_points(far[0], far[1]), cut_plane0).value();
    far[n - 1] = line_plane(Line::from_points(far[n - 2], far[n - 1]), cut_plane1).value();

    return {rib_loop(pts, cut_plane0, cut_plane1, inner), rib_loop(far, cut_plane0, cut_plane1, inner)};
}

std::vector<Outline> Quarter::outer_ribs() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& parabolas = geometry().parabolas;

    return {
        rib(parabolas[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][1].z_axis(), cp.wedges[0][0], cp.inner_beams[0][0], false),
        rib(parabolas[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][1].z_axis(), cp.wedges[2][0], cp.inner_beams[2][0], false),
    };
}

std::vector<Outline> Quarter::inner_ribs() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& parabolas = geometry().parabolas;
    const Vector& sweep = geometry().central_panel.rib_sweep;

    return {
        rib(parabolas[2][0], cp.inner_ribs[0][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true),
        rib(parabolas[3][0], cp.inner_ribs[1][1], sweep, cp.wedges[1][0], cp.inner_beams[1][1], true),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Beams and wedges
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Outline> Quarter::inner_beams() const {

    const ConstructionPlanes& cp = geometry().planes;
    const Plane side0 = level(0.0);
    const Plane side1 = level(-sizes().static_h());

    return {
        loft_planes({cp.outer_ribs[0][1], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[0][0], cp.inner_beams[0][1]),
        loft_planes({cp.inner_beams[0][1], side0, cp.inner_beams[2][1], side1}, cp.inner_beams[1][0], cp.inner_beams[1][1]),
        loft_planes({cp.outer_ribs[1][1], side0, cp.inner_beams[1][0], side1}, cp.inner_beams[2][0], cp.inner_beams[2][1]),
    };
}

std::vector<Outline> Quarter::wedges_inner_beams() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<Plane>& beds = geometry().bed_top_planes;
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

/// A t-section: its soffit and +t traces on its first face, each trimmed there, closed into one outline, and the same traces projected onto its second face and trimmed there.
static Outline tsection(const Polyline& soffit, const Polyline& layer, const Plane& cut_plane0, const Plane& cut_plane1, const Xform& projection10, const Xform& projection11) {

    const std::vector<Point> cut00 = trim(soffit, cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut01 = trim(soffit.transformed(projection10), cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut10 = trim(layer, cut_plane0, cut_plane1).get_points();
    const std::vector<Point> cut11 = trim(layer.transformed(projection11), cut_plane0, cut_plane1).get_points();

    std::vector<Point> top = cut00;
    top.insert(top.end(), cut10.rbegin(), cut10.rend());
    top.push_back(cut00.front());

    std::vector<Point> bottom = cut01;
    bottom.insert(bottom.end(), cut11.rbegin(), cut11.rend());
    bottom.push_back(cut01.front());

    return {Polyline(top), Polyline(bottom)};
}

/// The t-section beside an outer panel rib face: the outer parabola and its +t projected along the outer rib normal onto the face, the soffit continued to the far face along the rib's sweep, the +t along the panel.
static Outline outer_tsection(const std::array<Polyline, 3>& parabola, const std::array<Plane, 2>& faces, const Vector& outer, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1) {

    const Xform projection = Xform::project_to_plane_by_axis(faces[0], outer);

    return tsection(
        parabola[0].transformed(projection), parabola[1].transformed(projection), cut_plane0, cut_plane1,
        Xform::project_to_plane_by_axis(faces[1], sweep),
        Xform::project_to_plane_by_axis(faces[1], outer)
    );
}

std::vector<Outline> Quarter::tsections() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& pb = geometry().parabolas;
    const CentralPanel& panel = geometry().central_panel;
    const Vector outer0 = cp.outer_ribs[0][0].z_axis();
    const Vector outer1 = cp.outer_ribs[1][0].z_axis();
    const std::vector<std::array<Plane, 2>>& ts = cp.t_sections;

    return {
        outer_tsection(pb[0], ts[0], outer0, outer0, cp.inner_beams[0][1], cp.wedges[0][0]),
        outer_tsection(pb[0], ts[1], outer0, panel.rib_sweep, cp.inner_beams[0][1], cp.wedges[0][0]),
        tsection(
            panel.traces[0][0], panel.traces[0][1], cp.inner_beams[1][1], cp.wedges[1][0],
            Xform::project_to_plane_by_axis(ts[2][1], panel.rib_sweep),
            Xform::project_to_plane_by_axis(ts[2][1], panel.ruling)
        ),
        tsection(
            panel.traces[1][0], panel.traces[1][1], cp.inner_beams[1][1], cp.wedges[1][0],
            Xform::project_to_plane_by_axis(ts[4][1], panel.rib_sweep),
            Xform::project_to_plane_by_axis(ts[4][1], panel.ruling)
        ),
        outer_tsection(pb[1], ts[3], outer1, panel.rib_sweep, cp.inner_beams[2][1], cp.wedges[2][0]),
        outer_tsection(pb[1], ts[5], outer1, outer1, cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

/// One bed row: the lower and upper layer on each of the panel's two side planes, each trimmed there, one quad pair per segment.
static std::vector<Outline> bed_row(const std::array<Polyline, 2>& lower_faces, const std::array<Polyline, 2>& upper_faces, const Plane& cut_plane0, const Plane& cut_plane1) {

    const std::array<std::vector<Point>, 2> lower = {trim(lower_faces[0], cut_plane0, cut_plane1).get_points(), trim(lower_faces[1], cut_plane0, cut_plane1).get_points()};
    const std::array<std::vector<Point>, 2> upper = {trim(upper_faces[0], cut_plane0, cut_plane1).get_points(), trim(upper_faces[1], cut_plane0, cut_plane1).get_points()};

    if (lower[1].size() != lower[0].size() || upper[0].size() != lower[0].size() || upper[1].size() != lower[0].size())
        throw std::runtime_error(fmt::format("a bed row's layers are cut on different facets: {} / {} lower and {} / {} upper points", lower[0].size(), lower[1].size(), upper[0].size(), upper[1].size()));

    std::vector<Outline> plates;

    for (size_t i = 0; i + 1 < lower[0].size(); i++) {
        const Polyline bottom({lower[0][i], lower[0][i + 1], lower[1][i + 1], lower[1][i], lower[0][i]});
        const Polyline top({upper[0][i], upper[0][i + 1], upper[1][i + 1], upper[1][i], upper[0][i]});
        plates.push_back({top, bottom});
    }

    return plates;
}

/// An outer bed row: the parabola's +t and +2t projected along the outer rib normal onto the panel's two side planes.
static std::vector<Outline> outer_bed_row(const std::array<Polyline, 3>& parabola, const Plane& side0, const Plane& side1, const Vector& outer, const Plane& cut_plane0, const Plane& cut_plane1) {

    const Xform projection0 = Xform::project_to_plane_by_axis(side0, outer);
    const Xform projection1 = Xform::project_to_plane_by_axis(side1, outer);

    return bed_row({parabola[1].transformed(projection0), parabola[1].transformed(projection1)}, {parabola[2].transformed(projection0), parabola[2].transformed(projection1)}, cut_plane0, cut_plane1);
}

std::vector<std::vector<Outline>> Quarter::beds() const {

    const ConstructionPlanes& cp = geometry().planes;
    const std::vector<std::array<Polyline, 3>>& pb = geometry().parabolas;
    const CentralPanel& panel = geometry().central_panel;

    return {
        outer_bed_row(pb[0], cp.inner_ribs[0][0], cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis(), cp.inner_beams[0][1], cp.wedges[0][0]),
        bed_row({panel.traces[0][1], panel.traces[1][1]}, {panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]),
        outer_bed_row(pb[1], cp.inner_ribs[1][0], cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis(), cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Outline> Floor::oculus() const {

    const Plane side0 = level(0.0);
    const Plane side1 = level(-sizes.static_h() + sizes.tsections);
    const Plane side2 = level(-sizes.static_h());
    const Plane side3 = level(-sizes.static_h() + sizes.tsections * 2.0);

    std::vector<Plane> tilted;
    std::vector<Plane> inner;

    for (const OculusEdge& edge : oculus_edges) {
        tilted.push_back(edge.tilted);
        inner.push_back(edge.ring_inner);
    }

    std::vector<Outline> plates;

    for (size_t i = 0; i < 4; i++)
        plates.push_back(loft_planes({side2, tilted[(i + 1) % 4], side0, inner[(i + 3) % 4]}, tilted[i], inner[i], true));

    for (size_t i = 0; i < 4; i++) {
        const std::vector<Plane> sides = {inner[i], inner[(i + 1) % 4], inner[i].translate_by_normal(-sizes.tsections), inner[(i + 3) % 4].translate_by_normal(-sizes.tsections)};
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

std::vector<Outline> Quarter::column_cutters() const {

    const ConstructionPlanes& cp = geometry().planes;
    const ColumnCorner& corner = column();
    const std::vector<Point>& column = corner.head;
    const Vector down(0.0, 0.0, -1.0);
    const Plane xy0 = level(corner.levels[0]);
    const Plane xy1 = level(corner.levels[1]);
    const Plane xy2 = level(corner.levels[2]);
    const Plane side0 = corner.sides[0];
    const Plane side1 = corner.sides[1];
    const std::vector<Plane> fan_top = {side0, cp.wedges[0][0], cp.wedges[1][0], cp.wedges[2][0], side1};
    const std::vector<Plane> fan_bottom = {side0, edge_plane(geometry::edge(column, 1), down), edge_plane(geometry::edge(column, 3), down), side1};

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
