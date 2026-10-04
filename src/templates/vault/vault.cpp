#include "pch.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Voussoirs
// ═══════════════════════════════════════════════════════════════════════════

/// The circle of an arch over a span with its intrados crown at rise.
struct Arc {
    double radius = 0.0; // Intrados radius.
    double centre = 0.0; // Height of the centre, below 0 for a segment.
    double sector = 0.0; // Angle from the crown to a springing in radians.
};

/// The arc through both springings at z 0 and the crown at rise.
Arc compute_arc(double span, double rise) {

    const double radius = rise / 2.0 + span * span / (8.0 * rise);

    return {radius, rise - radius, std::asin(std::min(1.0, span / 2.0 / radius))};
}

/// A closed polyline through points.
Polyline to_loop(const std::vector<Point>& points) {

    std::vector<Point> closed = points;
    closed.push_back(points.front());

    return Polyline(closed);
}

/// A voussoir lofted from its intrados corners to its extrados corners, both turned so the intrados faces away from the extrados.
std::shared_ptr<wood_session::Block> to_voussoir(std::vector<Point> intrados, std::vector<Point> extrados, const std::string& name) {

    const Point centre = Point::centroid(intrados);
    Vector normal(0.0, 0.0, 0.0);
    for (size_t i = 0; i < intrados.size(); i++)
        normal = normal + (intrados[i] - centre).cross(intrados[(i + 1) % intrados.size()] - centre);
    if (normal.dot(Point::centroid(extrados) - Point::centroid(intrados)) < 0.0) {
        std::reverse(intrados.begin(), intrados.end());
        std::reverse(extrados.begin(), extrados.end());
    }

    return std::make_shared<wood_session::Block>(std::vector<Polyline>{to_loop(intrados), to_loop(extrados)}, name);
}

/// A stone lofted from an intrados loop to an extrados loop of paired corners, the corners a pair shares merged, so a bent stone such as a groin keeps one solid.
std::shared_ptr<wood_session::Block> to_stone(std::vector<Point> intrados, std::vector<Point> extrados, const std::string& name) {

    const Point centre = Point::centroid(intrados);
    Vector normal(0.0, 0.0, 0.0);
    for (size_t i = 0; i < intrados.size(); i++)
        normal = normal + (intrados[i] - centre).cross(intrados[(i + 1) % intrados.size()] - centre);
    if (normal.dot(Point::centroid(extrados) - centre) < 0.0) {
        std::reverse(intrados.begin(), intrados.end());
        std::reverse(extrados.begin(), extrados.end());
    }

    std::vector<std::vector<Point>> faces = {std::vector<Point>(intrados.rbegin(), intrados.rend()), extrados};
    for (size_t i = 0; i < intrados.size(); i++) {
        const size_t j = (i + 1) % intrados.size();
        faces.push_back({intrados[i], intrados[j], extrados[j], extrados[i]});
    }

    std::vector<std::vector<Point>> kept;
    for (const std::vector<Point>& face : faces) {
        std::vector<Point> corners;
        for (size_t i = 0; i < face.size(); i++)
            if (face[i].distance(face[(i + 1) % face.size()]) > 1e-6)
                corners.push_back(face[i]);
        if (corners.size() >= 3)
            kept.push_back(corners);
    }

    return std::make_shared<wood_session::Block>(Mesh::from_polylines(kept, 1e-6), name);
}

/// The pieces of a length cut into rings, shifted by offset: a part ring first when offset is not 0, a part ring last when the rings do not fill it.
std::vector<std::pair<double, double>> compute_pieces(double length, double ring, double offset) {

    std::vector<std::pair<double, double>> pieces;
    double start = 0.0;
    if (offset > 1e-9) {
        pieces.emplace_back(0.0, offset);
        start = offset;
    }

    while (start + ring <= length + 1e-9) {
        pieces.emplace_back(start, start + ring);
        start += ring;
    }

    if (start < length - 1e-9)
        pieces.emplace_back(start, length);

    return pieces;
}

// ═══════════════════════════════════════════════════════════════════════════
// Arches and barrels
// ═══════════════════════════════════════════════════════════════════════════

/// A point of an arch section at angle from the crown, radius from the centre, along y.
Point compute_section(const Arc& arc, double radius, double angle, double along) {
    return Point(radius * std::sin(angle), along, arc.centre + radius * std::cos(angle));
}

/// The voussoir of an arch section between two angles and two stations along y.
std::shared_ptr<Element> to_ring(const Arc& arc, double thickness, double from, double to, double near, double far) {

    const double outer = arc.radius + thickness;
    const std::vector<Point> intrados = {compute_section(arc, arc.radius, from, near), compute_section(arc, arc.radius, to, near), compute_section(arc, arc.radius, to, far), compute_section(arc, arc.radius, from, far)};
    const std::vector<Point> extrados = {compute_section(arc, outer, from, near), compute_section(arc, outer, to, near), compute_section(arc, outer, to, far), compute_section(arc, outer, from, far)};

    return to_voussoir(intrados, extrados, "voussoir");
}

std::vector<std::shared_ptr<Element>> arch(double span, double rise, double thickness, double depth, int voussoirs) {

    const Arc arc = compute_arc(span, rise);
    const double step = 2.0 * arc.sector / voussoirs;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int k = 0; k < voussoirs; k++)
        blocks.push_back(to_ring(arc, thickness, -arc.sector + k * step, -arc.sector + (k + 1) * step, 0.0, depth));

    return blocks;
}

std::vector<std::shared_ptr<Element>> barrel(double span, double rise, double thickness, double length, int courses, int rings, double stagger, bool closed) {

    const Arc arc = compute_arc(span, rise);
    const double step = 2.0 * arc.sector / courses;
    const double ring = length / rings;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int k = 0; k < courses; k++) {
        std::vector<std::pair<double, double>> pieces = compute_pieces(length, ring, k % 2 == 1 ? stagger * ring : 0.0);
        if (closed) {
            pieces.insert(pieces.begin(), {-ring, 0.0});
            pieces.emplace_back(length, length + ring);
        }

        for (const std::pair<double, double>& piece : pieces)
            blocks.push_back(to_ring(arc, thickness, -arc.sector + k * step, -arc.sector + (k + 1) * step, piece.first, piece.second));
    }

    return blocks;
}

// ═══════════════════════════════════════════════════════════════════════════
// Domes and walls
// ═══════════════════════════════════════════════════════════════════════════

/// A point of a sphere about the origin at radius, theta from the zenith and phi around, lowered by drop.
Point compute_sphere(double radius, double theta, double phi, double drop) {
    return Point(radius * std::sin(theta) * std::cos(phi), radius * std::sin(theta) * std::sin(phi), radius * std::cos(theta) - drop);
}

std::vector<std::shared_ptr<Element>> dome(double radius, double bottom, double top, int meridians, int hoops, double oculus, double springing) {

    const double first = oculus * Tolerance::TO_RADIANS;
    const double last = springing * Tolerance::TO_RADIANS;
    const double drop = radius * std::cos(last);
    const double around = Tolerance::TWO_PI / meridians;
    const double down = (last - first) / hoops;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int i = 0; i < hoops; i++) {
        const double upper = first + i * down;
        const double lower = upper + down;
        const double outer_upper = radius + top + (bottom - top) * (upper - first) / (last - first);
        const double outer_lower = radius + top + (bottom - top) * (lower - first) / (last - first);
        const double shift = i % 2 == 1 ? around / 2.0 : 0.0;

        // six corners: the hoops above and below joint at the middle of this voussoir, so its edges bend there and every face is shared
        for (int j = 0; j < meridians; j++) {
            const double left = j * around + shift;
            const std::vector<double> phis = {left, left + around / 2.0, left + around};
            std::vector<Point> intrados;
            std::vector<Point> extrados;
            for (const double phi : phis) {
                intrados.push_back(compute_sphere(radius, upper, phi, drop));
                extrados.push_back(compute_sphere(outer_upper, upper, phi, drop));
            }
            for (auto phi = phis.rbegin(); phi != phis.rend(); ++phi) {
                intrados.push_back(compute_sphere(radius, lower, *phi, drop));
                extrados.push_back(compute_sphere(outer_lower, lower, *phi, drop));
            }
            blocks.push_back(to_voussoir(intrados, extrados, "voussoir"));
        }
    }

    return blocks;
}

std::vector<std::shared_ptr<Element>> wall(double length, double height, double thickness, double brick, double course, double stagger) {

    std::vector<std::shared_ptr<Element>> blocks;
    const std::vector<std::pair<double, double>> rows = compute_pieces(height, course, 0.0);
    for (size_t row = 0; row < rows.size(); row++) {
        for (const std::pair<double, double>& piece : compute_pieces(length, brick, row % 2 == 1 ? stagger * brick : 0.0)) {
            const std::vector<Point> base = {Point(piece.first, 0.0, rows[row].first), Point(piece.second, 0.0, rows[row].first), Point(piece.second, thickness, rows[row].first), Point(piece.first, thickness, rows[row].first)};
            std::vector<Point> lid;
            for (const Point& point : base)
                lid.push_back(Point(point[0], point[1], rows[row].second));
            blocks.push_back(to_voussoir(base, lid, "brick"));
        }
    }

    return blocks;
}

// ═══════════════════════════════════════════════════════════════════════════
// Vaults over a square bay
// ═══════════════════════════════════════════════════════════════════════════

/// The voussoirs of the web of a square bay towards side outward: under groin its barrel crosses the side, courses across the arch and rings out from the centre; otherwise it runs along the side, courses from the crown down to the wall and rings along it; cut on the diagonals to the quarter of the bay towards the side, slivers dropped.
std::vector<std::shared_ptr<Element>> to_web(const Arc& arc, double span, double thickness, int courses, int rings, const Vector& outward, bool groin) {

    const Vector across = Vector(0.0, 0.0, 1.0).cross(outward);
    const double half = span / 2.0;
    const double outer = arc.radius + thickness;
    const std::vector<Plane> diagonals = {Plane::from_point_normal(Point(0.0, 0.0, 0.0), (outward - across).normalized()), Plane::from_point_normal(Point(0.0, 0.0, 0.0), (outward + across).normalized())};

    // a point of the web at an arch angle and a station along its barrel axis
    const auto place = [&](double radius, double angle, double station) {
        const double section = radius * std::sin(angle);
        const double height = arc.centre + radius * std::cos(angle);
        const Vector plan = groin ? outward * station + across * section : outward * section + across * station;
        return Point(plan[0], plan[1], height);
    };

    const double from = groin ? -arc.sector : 0.0;
    const double step = (arc.sector - from) / courses;
    const double start = groin ? 0.0 : -half;
    const double ring = (half - start) / rings;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int k = 0; k < courses; k++) {
        for (int j = 0; j < rings; j++) {
            const double a = from + k * step;
            const double b = a + step;
            const double near = start + j * ring;
            const double far = near + ring;
            std::shared_ptr<wood_session::Block> block = to_voussoir({place(arc.radius, a, near), place(arc.radius, b, near), place(arc.radius, b, far), place(arc.radius, a, far)}, {place(outer, a, near), place(outer, b, near), place(outer, b, far), place(outer, a, far)}, "voussoir");
            const double whole = std::abs(block->element_geometry_mesh().volume());
            block->cuts = diagonals;
            block->invalidate_geometry();
            if (std::abs(block->model_geometry_mesh().volume()) > 1e-3 * whole)
                blocks.push_back(block);
        }
    }

    return blocks;
}

/// The four webs of a square bay, one towards each side.
std::vector<std::shared_ptr<Element>> to_bay(double span, double rise, double thickness, int courses, int rings, bool groin) {

    const Arc arc = compute_arc(span, rise);
    std::vector<std::shared_ptr<Element>> blocks;
    for (const Vector& outward : {Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(-1.0, 0.0, 0.0), Vector(0.0, -1.0, 0.0)})
        for (const std::shared_ptr<Element>& block : to_web(arc, span, thickness, courses, rings, outward, groin))
            blocks.push_back(block);

    return blocks;
}

/// A point of a cross vault web along outward at an arch angle from the crown, positive towards across, radius from the arch centre, station out from the bay centre.
Point compute_web(const Arc& arc, const Vector& outward, const Vector& across, double radius, double angle, double station) {

    const Vector plan = outward * station + across * (radius * std::sin(angle));

    return Point(plan[0], plan[1], arc.centre + radius * std::cos(angle));
}

std::vector<std::shared_ptr<Element>> cross_vault(double span, double rise, double thickness, int courses, int rings) {

    const Arc arc = compute_arc(span, rise);
    const double outer = arc.radius + thickness;
    const double end = outer * std::sin(arc.sector);
    const double ring = 2.0 * end / rings;
    const double step = arc.sector / courses;
    const std::vector<Vector> sides = {Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(-1.0, 0.0, 0.0), Vector(0.0, -1.0, 0.0)};

    std::vector<std::shared_ptr<Element>> blocks;
    for (int m = 0; m < courses; m++) {
        const double up = m * step;
        const double down = up + step;

        // the head joints of the course from the wall in, the course at the crown and every second one shifted half a ring
        std::vector<double> joints = {end};
        for (double station = end - (m % 2 == 0 ? ring / 2.0 : ring); station > 1e-9; station -= ring)
            joints.push_back(station);
        std::sort(joints.begin(), joints.end());

        // the groin stone reaches from the groin to the first joint past it
        double groin = end;
        for (const double joint : joints)
            if (joint >= outer * std::sin(down) - 1e-9) {
                groin = joint;
                break;
            }

        for (size_t i = 0; i < 4; i++) {
            const Vector& outward = sides[i];
            const Vector& across = sides[(i + 1) % 4];
            const Vector& back = sides[(i + 2) % 4];

            // the groin stone between this web and the next, bent along the diagonal
            std::vector<Point> intrados;
            std::vector<Point> extrados;
            for (const double radius : {arc.radius, outer}) {
                const double low = radius * std::sin(down);
                const double high = radius * std::sin(up);
                std::vector<Point>& loop = radius == arc.radius ? intrados : extrados;
                loop = {compute_web(arc, outward, across, radius, down, groin), compute_web(arc, outward, across, radius, down, low), compute_web(arc, across, back, radius, -down, groin), compute_web(arc, across, back, radius, -up, groin), compute_web(arc, outward, across, radius, up, high), compute_web(arc, outward, across, radius, up, groin)};
            }
            blocks.push_back(to_stone(intrados, extrados, "groin"));

            // the voussoirs of the web on both sides of its crown, from the groin stone out to the wall
            for (const double sign : {-1.0, 1.0})
                for (size_t j = 0; j + 1 < joints.size(); j++) {
                    if (joints[j] < groin - 1e-9)
                        continue;

                    std::vector<Point> inner;
                    std::vector<Point> outside;
                    for (const double radius : {arc.radius, outer}) {
                        std::vector<Point>& loop = radius == arc.radius ? inner : outside;
                        loop = {compute_web(arc, outward, across, radius, sign * down, joints[j]), compute_web(arc, outward, across, radius, sign * down, joints[j + 1]), compute_web(arc, outward, across, radius, sign * up, joints[j + 1]), compute_web(arc, outward, across, radius, sign * up, joints[j])};
                    }
                    blocks.push_back(to_voussoir(inner, outside, "voussoir"));
                }
        }
    }

    return blocks;
}

std::vector<std::shared_ptr<Element>> cloister_vault(double span, double rise, double thickness, int courses, int rings) {
    return to_bay(span, rise, thickness, courses, rings, false);
}

// ═══════════════════════════════════════════════════════════════════════════
// Star vault
// ═══════════════════════════════════════════════════════════════════════════

/// The sphere through the four corners of a square bay at z 0 with its top at rise.
struct Sail {
    double radius = 0.0; // Intrados radius.
    double centre = 0.0; // Height of the centre, below 0.
};

/// The sail over a bay of span rising to rise.
Sail compute_sail(double span, double rise) {

    const double radius = (span * span / 2.0 + rise * rise) / (2.0 * rise);

    return {radius, rise - radius};
}

/// The sail intrados over a plan point, moved out from the sphere centre by offset.
Point compute_on_sail(const Sail& sail, const Point& plan, double offset) {

    const Point centre(0.0, 0.0, sail.centre);
    const Point point(plan[0], plan[1], sail.centre + std::sqrt(std::max(0.0, sail.radius * sail.radius - plan[0] * plan[0] - plan[1] * plan[1])));

    return centre + (point - centre) * ((sail.radius + offset) / sail.radius);
}

/// The web over a counter-clockwise plan triangle: its intrados and extrados meshed at subdivisions per side, the sides radial to the sail so neighbouring webs share them.
std::shared_ptr<Element> to_shell(const Sail& sail, const std::vector<Point>& corners, double thickness, int subdivisions) {

    const int n = subdivisions;
    std::map<std::pair<int, int>, size_t> index;
    std::vector<Point> vertices;
    for (int i = 0; i <= n; i++)
        for (int j = 0; i + j <= n; j++) {
            index[{i, j}] = vertices.size();
            vertices.push_back(corners[0] + (corners[1] - corners[0]) * (static_cast<double>(i) / n) + (corners[2] - corners[0]) * (static_cast<double>(j) / n));
        }

    const size_t count = vertices.size();
    std::vector<Point> points;
    for (const Point& plan : vertices)
        points.push_back(compute_on_sail(sail, plan, 0.0));
    for (const Point& plan : vertices)
        points.push_back(compute_on_sail(sail, plan, thickness));

    // the extrados counter-clockwise from above, the intrados the other way
    std::vector<std::vector<size_t>> faces;
    const auto add_triangle = [&](size_t a, size_t b, size_t c) {
        faces.push_back({a + count, b + count, c + count});
        faces.push_back({c, b, a});
    };
    for (int i = 0; i < n; i++)
        for (int j = 0; i + j < n; j++) {
            add_triangle(index[{i, j}], index[{i + 1, j}], index[{i, j + 1}]);
            if (i + j + 1 < n)
                add_triangle(index[{i + 1, j}], index[{i + 1, j + 1}], index[{i, j + 1}]);
        }

    // the radial sides round the boundary, counter-clockwise
    std::vector<size_t> boundary;
    for (int k = 0; k < n; k++)
        boundary.push_back(index[{k, 0}]);
    for (int k = 0; k < n; k++)
        boundary.push_back(index[{n - k, k}]);
    for (int k = 0; k < n; k++)
        boundary.push_back(index[{0, n - k}]);
    for (size_t k = 0; k < boundary.size(); k++) {
        const size_t a = boundary[k];
        const size_t b = boundary[(k + 1) % boundary.size()];
        faces.push_back({a, b, b + count, a + count});
    }

    return std::make_shared<wood_session::Block>(Mesh::from_vertices_and_faces(points, faces), "web");
}

/// A rib along a plan segment under the sail: a square section of rib with its top on the intrados, sampled at subdivisions.
std::shared_ptr<Element> to_rib(const Sail& sail, const Point& start, const Point& end, double rib, int subdivisions) {

    std::vector<Point> axis;
    std::vector<Vector> ups;
    for (int k = 0; k <= subdivisions; k++)
        axis.push_back(compute_on_sail(sail, start + (end - start) * (static_cast<double>(k) / subdivisions), -rib / 2.0));
    for (int k = 0; k < subdivisions; k++)
        ups.push_back((Point::centroid({axis[k], axis[k + 1]}) - Point(0.0, 0.0, sail.centre)).normalized());

    return std::make_shared<wood_session::Beam>(Polyline(axis), wood_session::profile_rectangle(rib, rib), ups, "rib");
}

std::vector<std::shared_ptr<Element>> star_vault(double span, double rise, double thickness, double star, double rib, int subdivisions) {

    const Sail sail = compute_sail(span, rise);
    const double half = span / 2.0;
    const Point crown(0.0, 0.0, 0.0);
    const std::vector<Point> corners = {Point(half, half, 0.0), Point(-half, half, 0.0), Point(-half, -half, 0.0), Point(half, -half, 0.0)};
    const std::vector<Point> stars = {Point(0.0, star * half, 0.0), Point(-star * half, 0.0, 0.0), Point(0.0, -star * half, 0.0), Point(star * half, 0.0, 0.0)};

    // twelve webs: one against each wall, two either side of each diagonal
    std::vector<std::shared_ptr<Element>> elements;
    for (size_t i = 0; i < 4; i++) {
        const size_t next = (i + 1) % 4;
        const size_t before = (i + 3) % 4;
        elements.push_back(to_shell(sail, {corners[i], corners[next], stars[i]}, thickness, subdivisions));
        elements.push_back(to_shell(sail, {crown, stars[before], corners[i]}, thickness, subdivisions));
        elements.push_back(to_shell(sail, {crown, corners[i], stars[i]}, thickness, subdivisions));
    }

    // twenty ribs: the diagonals, the tiercerons, the liernes and the wall arches
    for (size_t i = 0; i < 4; i++) {
        const size_t next = (i + 1) % 4;
        elements.push_back(to_rib(sail, crown, corners[i], rib, subdivisions));
        elements.push_back(to_rib(sail, corners[i], stars[i], rib, subdivisions));
        elements.push_back(to_rib(sail, corners[next], stars[i], rib, subdivisions));
        elements.push_back(to_rib(sail, stars[i], crown, rib, subdivisions));
        elements.push_back(to_rib(sail, corners[i], corners[next], rib, subdivisions));
    }

    return elements;
}

} // namespace wood_vault
