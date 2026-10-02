#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

const Vector UP(0.0, 0.0, 1.0); // world z, the normal side of edge planes facing out of the quarter
const Vector DOWN(0.0, 0.0, -1.0); // minus world z, the normal side of edge planes facing into the quarter

/// A plane pair: the plane and its copy moved by distance along the normal.
static std::array<Plane, 2> pair(const Plane& plane, double distance) {
    return {plane, offset(plane, distance)};
}

/// The plan quad of four planes at z 0: corners 3-0, 0-1, 1-2 and 2-3.
static Polyline quad(const std::array<Plane, 4>& planes) {

    const Plane xy = level(0.0);

    return Polyline({
        plane_plane_plane(xy, planes[0], planes[3]).value(),
        plane_plane_plane(xy, planes[0], planes[1]).value(),
        plane_plane_plane(xy, planes[1], planes[2]).value(),
        plane_plane_plane(xy, planes[2], planes[3]).value(),
    });
}

/// The plan quads of a family of quad planes.
static std::vector<Polyline> quads(const std::vector<std::array<Plane, 4>>& family) {

    std::vector<Polyline> result;

    for (const std::array<Plane, 4>& planes : family)
        result.push_back(quad(planes));

    return result;
}

/// The plane fitted to the deepest quad of a bed panel: its parabola cut by the panel planes, projected onto the panel's two side planes.
static Plane panel_top_plane(const Polyline& parabola, const Plane& cut_plane0, const Plane& cut_plane1, const Xform& projection0, const Xform& projection1) {

    std::array<std::vector<Point>, 2> pts = projected(trim(parabola, cut_plane0, cut_plane1), projection0, projection1);

    if (pts[0].front()[2] > pts[0].back()[2]) {
        std::reverse(pts[0].begin(), pts[0].end());
        std::reverse(pts[1].begin(), pts[1].end());
    }

    const Plane plane = Plane::from_points_pca({pts[0][0], pts[0][1], pts[1][0], pts[1][1]});
    const Vector normal = plane.z_axis()[2] < 0.0 ? -plane.z_axis() : plane.z_axis();

    return Plane::from_point_normal(plane.origin(), normal);
}

/// The wedge plane pairs: three around the column head, the middle one tilted about its top edge and the outer two leaning to meet it on the inner ribs, then the three inner beam faces.
static std::vector<std::array<Plane, 2>> wedge_planes(const FloorGuide& guide, const ConstructionPlanes& cp) {

    const std::vector<Point> column = guide.quarter_column_polygon();
    const Line side0 = edge(column, 1);
    const Line side1 = edge(column, 2);
    const Line side2 = edge(column, 3);

    const Plane tilted = rotate(edge_plane(side1, UP), guide.wedge_plane_angle * M_PI / 180.0, direction(side1), side1.center());
    const Line line0 = plane_plane(cp.inner_ribs[0][1], tilted).value();
    const Line line1 = plane_plane(cp.inner_ribs[1][1], tilted).value();
    const Plane wedge0 = Plane::from_point_normal(side0.center(), direction(line0).cross(direction(side0)));
    const Plane wedge2 = Plane::from_point_normal(side2.center(), (-direction(line1)).cross(direction(side2)));

    return {
        pair(wedge0, guide.size_wedge),
        pair(tilted, guide.size_wedge * 1.25),
        pair(wedge2, guide.size_wedge),
        pair(cp.inner_beams[0][1], guide.size_inner_beams),
        pair(cp.inner_beams[1][1], guide.size_inner_beams),
        pair(cp.inner_beams[2][1], guide.size_inner_beams),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

/// The oculus corner towards the edge midpoint at index k of the four: its distance from the centre is size_oculus times its own over the geometric mean of its neighbours'.
static Point oculus_corner(const std::array<Point, 4>& midpoints, size_t k, double size_oculus) {

    const Point centre(0.0, 0.0, 0.0);
    const double own = (midpoints[k] - centre).magnitude();
    const double before = (midpoints[(k + 3) % 4] - centre).magnitude();
    const double after = (midpoints[(k + 1) % 4] - centre).magnitude();

    return centre + (midpoints[k] - centre).normalized() * (size_oculus * own / std::sqrt(before * after));
}

FloorGuide FloorGuide::trapezoid(const std::array<Point, 4>& corners, int quadrant, const FloorGuide& sizes) {

    const size_t q = static_cast<size_t>(quadrant) % 4;
    std::array<Point, 4> midpoints;

    for (size_t k = 0; k < 4; k++)
        midpoints[k] = Line::from_points(corners[k], corners[(k + 1) % 4]).center();

    FloorGuide guide = sizes;
    guide.corner = corners[q];
    guide.midpoint_x = midpoints[q];
    guide.midpoint_y = midpoints[(q + 3) % 4];
    guide.oculus_x = oculus_corner(midpoints, q, sizes.size_oculus);
    guide.oculus_y = oculus_corner(midpoints, (q + 3) % 4, sizes.size_oculus);

    return guide;
}

FloorGuide FloorGuide::rectangle(double gx, double gy, int quadrant, const FloorGuide& sizes) {
    return trapezoid({Point(-gx, -gy, 0.0), Point(gx, -gy, 0.0), Point(gx, gy, 0.0), Point(-gx, gy, 0.0)}, quadrant, sizes);
}

// ═══════════════════════════════════════════════════════════════════════════
// Plan
// ═══════════════════════════════════════════════════════════════════════════

double FloorGuide::static_h() const {
    return height - rise;
}

Vector FloorGuide::x_axis() const {
    return (midpoint_x - corner).normalized();
}

Vector FloorGuide::y_axis() const {
    return (midpoint_y - corner).normalized();
}

Point FloorGuide::corner_point_column(double column_size) const {
    return corner + x_axis() * (column_size * 0.5) + y_axis() * (column_size * 0.5);
}

std::vector<Point> FloorGuide::quarter_polygon() const {
    return {corner, midpoint_x, oculus_x, oculus_y, midpoint_y};
}

std::vector<Point> FloorGuide::quarter_column_polygon() const {

    const Vector x = x_axis();
    const Vector y = y_axis();
    const double head = size_column_head;
    const double chamfer = size_column_head_chamfer;

    return {corner, corner + x * head, corner + x * head + y * chamfer, corner + x * chamfer + y * head, corner + y * head};
}

// ═══════════════════════════════════════════════════════════════════════════
// Construction planes
// ═══════════════════════════════════════════════════════════════════════════

ConstructionPlanes FloorGuide::construction_planes() const {

    const std::vector<Point> polygon = quarter_polygon();
    const std::vector<Point> column = quarter_column_polygon();
    ConstructionPlanes cp;

    const Plane outer0 = edge_plane(edge(polygon, 0), DOWN);
    const Plane outer1 = edge_plane(edge(polygon, 4), DOWN);
    cp.outer_ribs = {pair(outer0, size_outer_ribs), pair(outer1, size_outer_ribs)};

    const Line oculus_edge = edge(polygon, 2);
    const Plane beam0 = edge_plane(edge(polygon, 1), DOWN);
    const Plane beam1 = edge_plane(oculus_edge, DOWN);
    const Plane beam1_rotated = rotate(beam1, -oculus_plane_angle * M_PI / 180.0, direction(oculus_edge), oculus_edge.center());
    const Plane beam2 = edge_plane(edge(polygon, 3), DOWN);
    cp.inner_beams = {pair(beam0, size_inner_beams), {beam1_rotated, offset(beam1, size_inner_beams)}, pair(beam2, size_inner_beams)};

    const Plane xy = level(0.0);
    const Point p0 = plane_plane_plane(xy, cp.inner_beams[0][1], cp.inner_beams[1][1]).value();
    const Point p1 = plane_plane_plane(xy, cp.inner_beams[1][1], cp.inner_beams[2][1]).value();
    const Point p2 = column[2];
    const Point p3 = column[3];
    const Plane rib0 = Plane::from_point_normal(p2 + (p0 - p2) * 0.5, (p0 - p2).cross(DOWN));
    const Plane rib1 = Plane::from_point_normal(p3 + (p1 - p3) * 0.5, (p1 - p3).cross(UP));
    cp.inner_ribs = {pair(rib0, size_inner_ribs), pair(rib1, size_inner_ribs)};

    cp.wedges = wedge_planes(*this, cp);

    cp.t_sections = {
        pair(cp.outer_ribs[0][1], size_tsections),
        pair(cp.inner_ribs[0][0], -size_tsections),
        pair(cp.inner_ribs[0][1], size_tsections),
        pair(cp.inner_ribs[1][0], -size_tsections),
        pair(cp.inner_ribs[1][1], size_tsections),
        pair(cp.outer_ribs[1][1], size_tsections),
    };

    return cp;
}

ConstructionQuads FloorGuide::construction_quads() const {

    const ConstructionPlanes cp = construction_planes();
    ConstructionQuads result;

    result.outer_ribs = quads({
        {cp.outer_ribs[0][0], cp.inner_beams[0][0], cp.outer_ribs[0][1], cp.wedges[0][0]},
        {cp.outer_ribs[1][0], cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.wedges[2][0]},
    });
    result.inner_beams = quads({
        {cp.inner_beams[0][0], cp.inner_beams[1][0], cp.inner_beams[0][1], cp.outer_ribs[0][1]},
        {cp.inner_beams[1][0], cp.inner_beams[2][1], cp.inner_beams[1][1], cp.inner_beams[0][1]},
        {cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.inner_beams[2][1], cp.inner_beams[1][0]},
    });
    result.inner_ribs = quads({
        {cp.inner_ribs[0][1], cp.inner_beams[1][1], cp.inner_ribs[0][0], cp.wedges[1][0]},
        {cp.inner_ribs[1][1], cp.inner_beams[1][1], cp.inner_ribs[1][0], cp.wedges[1][0]},
    });
    result.wedges = quads({
        {cp.wedges[0][0], cp.outer_ribs[0][1], cp.wedges[0][1], cp.inner_ribs[0][0]},
        {cp.wedges[1][0], cp.inner_ribs[0][1], cp.wedges[1][1], cp.inner_ribs[1][1]},
        {cp.wedges[2][0], cp.inner_ribs[1][0], cp.wedges[2][1], cp.outer_ribs[1][1]},
        {cp.wedges[3][0], cp.inner_ribs[0][0], cp.wedges[3][1], cp.outer_ribs[0][1]},
        {cp.wedges[4][0], cp.inner_ribs[1][1], cp.wedges[4][1], cp.inner_ribs[0][1]},
        {cp.wedges[5][0], cp.outer_ribs[1][1], cp.wedges[5][1], cp.inner_ribs[1][0]},
    });
    result.t_sections = quads({
        {cp.t_sections[0][0], cp.inner_beams[0][1], cp.t_sections[0][1], cp.wedges[0][1]},
        {cp.t_sections[1][0], cp.inner_beams[0][1], cp.t_sections[1][1], cp.wedges[0][1]},
        {cp.t_sections[2][0], cp.inner_beams[1][1], cp.t_sections[2][1], cp.wedges[1][1]},
        {cp.t_sections[3][0], cp.inner_beams[2][1], cp.t_sections[3][1], cp.wedges[2][1]},
        {cp.t_sections[4][0], cp.inner_beams[1][1], cp.t_sections[4][1], cp.wedges[1][1]},
        {cp.t_sections[5][0], cp.inner_beams[2][1], cp.t_sections[5][1], cp.wedges[2][1]},
    });

    return result;
}

std::vector<std::array<Polyline, 3>> FloorGuide::boundary_parabolas() const {

    const ConstructionPlanes cp = construction_planes();
    const ConstructionQuads quads = construction_quads();
    std::vector<std::array<Polyline, 3>> parabolas;

    for (const Polyline& quad : quads.outer_ribs) {
        const Point start = quad.get_point(0);
        const Point end = quad.get_point(1);
        const Point trimmed = start + (end - start).normalized() * size_wedge;
        const Point middle = trimmed + (end - trimmed) * 0.5;
        const Polyline parabola = Polyline::quadratic_points(trimmed + Vector(0.0, 0.0, -height), middle + Vector(0.0, 0.0, -static_h()), end + Vector(0.0, 0.0, -static_h()));
        parabolas.push_back({parabola, offset_polyline(parabola, size_tsections), offset_polyline(parabola, 2.0 * size_tsections)});
    }

    for (size_t i = 0; i < 2; i++) {
        const Xform projection = Xform::project_to_plane_by_axis(cp.inner_ribs[i][0], cp.outer_ribs[i][0].z_axis());
        const std::array<Polyline, 3>& outer = parabolas[i];
        parabolas.push_back({outer[0].transformed(projection), outer[1].transformed(projection), outer[2].transformed(projection)});
    }

    return parabolas;
}

double FloorGuide::block_level_bottom() const {

    const ConstructionPlanes cp = construction_planes();
    const Polyline middle = cut(boundary_parabolas()[2][0], cp.wedges[1][1], cp.inner_beams[1][1]);

    return middle.get_point(0)[2];
}

double FloorGuide::block_level_top() const {
    return block_level_bottom() + size_wedge;
}

std::vector<Plane> FloorGuide::bed_top_planes() const {

    const ConstructionPlanes cp = construction_planes();
    const std::vector<std::array<Polyline, 3>> parabolas = boundary_parabolas();
    const Vector rib_bisector = cp.inner_ribs[0][0].z_axis() - cp.inner_ribs[1][0].z_axis();

    return {
        panel_top_plane(
            parabolas[0][2],
            cp.inner_beams[0][1],
            cp.wedges[0][0],
            Xform::project_to_plane_by_axis(cp.inner_ribs[0][0], cp.outer_ribs[0][0].z_axis()),
            Xform::project_to_plane_by_axis(cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis())
        ),
        panel_top_plane(
            parabolas[2][2],
            cp.inner_beams[1][1],
            cp.wedges[1][0],
            Xform::project_to_plane_by_axis(cp.inner_ribs[0][1], rib_bisector),
            Xform::project_to_plane_by_axis(cp.inner_ribs[1][1], rib_bisector)
        ),
        panel_top_plane(
            parabolas[1][2],
            cp.inner_beams[2][1],
            cp.wedges[2][0],
            Xform::project_to_plane_by_axis(cp.inner_ribs[1][0], cp.outer_ribs[1][0].z_axis()),
            Xform::project_to_plane_by_axis(cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis())
        ),
    };
}

}
