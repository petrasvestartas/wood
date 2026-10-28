#include "pch.h"
#include "src/templates/vault/vault.h"
#include "primitives.h"

using namespace session_cpp;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Voussoirs
// ═══════════════════════════════════════════════════════════════════════════

/// The circle of an arch over a span with its intrados crown at rise.
struct Arc {
    double radius = 0.0;
    double centre = 0.0;
    double sector = 0.0;
};

/// The arc through both springings at z 0 and the crown at rise.
Arc compute_arc(double span, double rise) {

    const double radius = rise / 2.0 + span * span / (8.0 * rise);

    return {radius, rise - radius, std::asin(std::min(1.0, span / 2.0 / radius))};
}

/// A voussoir lofted from its intrados corners to its extrados corners, both turned so the intrados faces away from the extrados.
std::shared_ptr<wood_session::Block>
to_voussoir(std::vector<Point> intrados, std::vector<Point> extrados, const std::string& name) {

    const Point centre = Point::centroid(intrados);
    Vector normal(0.0, 0.0, 0.0);
    for (size_t i = 0; i < intrados.size(); i++)
        normal =
            normal + (intrados[i] - centre).cross(intrados[(i + 1) % intrados.size()] - centre);
    if (normal.dot(Point::centroid(extrados) - Point::centroid(intrados)) < 0.0) {
        std::reverse(intrados.begin(), intrados.end());
        std::reverse(extrados.begin(), extrados.end());
    }

    return std::make_shared<wood_session::Block>(
        std::vector<Polyline>{Polyline(intrados).closed(), Polyline(extrados).closed()}, name
    );
}

/// A stone lofted from an intrados loop to an extrados loop of paired corners.
std::shared_ptr<wood_session::Block>
to_stone(std::vector<Point> intrados, std::vector<Point> extrados, const std::string& name) {

    const Point centre = Point::centroid(intrados);
    Vector normal(0.0, 0.0, 0.0);
    for (size_t i = 0; i < intrados.size(); i++)
        normal =
            normal + (intrados[i] - centre).cross(intrados[(i + 1) % intrados.size()] - centre);
    if (normal.dot(Point::centroid(extrados) - centre) < 0.0) {
        std::reverse(intrados.begin(), intrados.end());
        std::reverse(extrados.begin(), extrados.end());
    }

    std::vector<std::vector<Point>> faces = {
        std::vector<Point>(intrados.rbegin(), intrados.rend()), extrados
    };
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

/// The pieces of a length cut into rings, shifted by offset.
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
std::shared_ptr<Element>
to_ring(const Arc& arc, double thickness, double from, double to, double near, double far) {

    const double outer = arc.radius + thickness;
    const std::vector<Point> intrados = {
        compute_section(arc, arc.radius, from, near),
        compute_section(arc, arc.radius, to, near),
        compute_section(arc, arc.radius, to, far),
        compute_section(arc, arc.radius, from, far)
    };
    const std::vector<Point> extrados = {
        compute_section(arc, outer, from, near),
        compute_section(arc, outer, to, near),
        compute_section(arc, outer, to, far),
        compute_section(arc, outer, from, far)
    };

    return to_voussoir(intrados, extrados, "voussoir");
}

std::vector<std::shared_ptr<Element>>
arch(double span, double rise, double thickness, double depth, int voussoirs) {

    const Arc arc = compute_arc(span, rise);
    const double step = 2.0 * arc.sector / voussoirs;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int k = 0; k < voussoirs; k++)
        blocks.push_back(to_ring(
            arc, thickness, -arc.sector + k * step, -arc.sector + (k + 1) * step, 0.0, depth
        ));

    return blocks;
}

std::vector<std::shared_ptr<Element>> barrel(
    double span,
    double rise,
    double thickness,
    double length,
    int courses,
    int rings,
    double stagger,
    bool closed
) {

    const Arc arc = compute_arc(span, rise);
    const double step = 2.0 * arc.sector / courses;
    const double ring = length / rings;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int k = 0; k < courses; k++) {
        std::vector<std::pair<double, double>> pieces =
            compute_pieces(length, ring, k % 2 == 1 ? stagger * ring : 0.0);
        if (closed) {
            pieces.insert(pieces.begin(), {-ring, 0.0});
            pieces.emplace_back(length, length + ring);
        }

        for (const std::pair<double, double>& piece : pieces)
            blocks.push_back(to_ring(
                arc,
                thickness,
                -arc.sector + k * step,
                -arc.sector + (k + 1) * step,
                piece.first,
                piece.second
            ));
    }

    return blocks;
}

// ═══════════════════════════════════════════════════════════════════════════
// Domes and walls
// ═══════════════════════════════════════════════════════════════════════════

/// A point of a sphere about the origin at radius, theta from the zenith and phi around, lowered by drop.
Point compute_sphere(double radius, double theta, double phi, double drop) {
    return Point(
        radius * std::sin(theta) * std::cos(phi),
        radius * std::sin(theta) * std::sin(phi),
        radius * std::cos(theta) - drop
    );
}

std::vector<std::shared_ptr<Element>> dome(
    double radius,
    double bottom,
    double top,
    int meridians,
    int hoops,
    double oculus,
    double springing
) {

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

std::vector<std::shared_ptr<Element>>
wall(double length, double height, double thickness, double brick, double course, double stagger) {

    std::vector<std::shared_ptr<Element>> blocks;
    const std::vector<std::pair<double, double>> rows = compute_pieces(height, course, 0.0);
    for (size_t row = 0; row < rows.size(); row++) {
        for (const std::pair<double, double>& piece :
             compute_pieces(length, brick, row % 2 == 1 ? stagger * brick : 0.0)) {
            const double bottom = rows[row].first;
            const std::vector<Point> base = {
                Point(piece.first, 0.0, bottom),
                Point(piece.second, 0.0, bottom),
                Point(piece.second, thickness, bottom),
                Point(piece.first, thickness, bottom)
            };
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

/// The voussoirs of the web of a square bay towards side outward.
std::vector<std::shared_ptr<Element>> to_web(
    const Arc& arc,
    double span,
    double thickness,
    int courses,
    int rings,
    const Vector& outward,
    bool groin
) {

    const Vector across = Vector(0.0, 0.0, 1.0).cross(outward);
    const double half = span / 2.0;
    const double outer = arc.radius + thickness;
    const Plane diagonal_back =
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), (outward - across).normalized());
    const Plane diagonal_front =
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), (outward + across).normalized());
    const std::vector<Plane> diagonals = {diagonal_back, diagonal_front};

    const auto place = [&](double radius, double angle, double station) {
        const double section = radius * std::sin(angle);
        const double height = arc.centre + radius * std::cos(angle);
        const Vector plan =
            groin ? outward * station + across * section : outward * section + across * station;
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
            const std::vector<Point> intrados = {
                place(arc.radius, a, near),
                place(arc.radius, b, near),
                place(arc.radius, b, far),
                place(arc.radius, a, far)
            };
            const std::vector<Point> extrados = {
                place(outer, a, near),
                place(outer, b, near),
                place(outer, b, far),
                place(outer, a, far)
            };
            std::shared_ptr<wood_session::Block> block =
                to_voussoir(intrados, extrados, "voussoir");
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
std::vector<std::shared_ptr<Element>>
to_bay(double span, double rise, double thickness, int courses, int rings, bool groin) {

    const Arc arc = compute_arc(span, rise);
    std::vector<std::shared_ptr<Element>> blocks;
    for (const Vector& outward :
         {Vector(1.0, 0.0, 0.0),
          Vector(0.0, 1.0, 0.0),
          Vector(-1.0, 0.0, 0.0),
          Vector(0.0, -1.0, 0.0)})
        for (const std::shared_ptr<Element>& block :
             to_web(arc, span, thickness, courses, rings, outward, groin))
            blocks.push_back(block);

    return blocks;
}

/// A point on a cross-vault web in arch coordinates.
Point compute_web(
    const Arc& arc,
    const Vector& outward,
    const Vector& across,
    double radius,
    double angle,
    double station
) {

    const Vector plan = outward * station + across * (radius * std::sin(angle));

    return Point(plan[0], plan[1], arc.centre + radius * std::cos(angle));
}

std::vector<std::shared_ptr<Element>>
cross_vault(double span, double rise, double thickness, int courses, int rings) {

    const Arc arc = compute_arc(span, rise);
    const double outer = arc.radius + thickness;
    const double end = outer * std::sin(arc.sector);
    const double ring = 2.0 * end / rings;
    const double step = arc.sector / courses;
    const std::vector<Vector> sides = {
        Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(-1.0, 0.0, 0.0), Vector(0.0, -1.0, 0.0)
    };

    std::vector<std::shared_ptr<Element>> blocks;
    for (int m = 0; m < courses; m++) {
        const double up = m * step;
        const double down = up + step;

        std::vector<double> joints = {end};
        for (double station = end - (m % 2 == 0 ? ring / 2.0 : ring); station > 1e-9;
             station -= ring)
            joints.push_back(station);
        std::sort(joints.begin(), joints.end());

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

            std::vector<Point> intrados;
            std::vector<Point> extrados;
            for (const double radius : {arc.radius, outer}) {
                const double low = radius * std::sin(down);
                const double high = radius * std::sin(up);
                std::vector<Point>& loop = radius == arc.radius ? intrados : extrados;
                loop = {
                    compute_web(arc, outward, across, radius, down, groin),
                    compute_web(arc, outward, across, radius, down, low),
                    compute_web(arc, across, back, radius, -down, groin),
                    compute_web(arc, across, back, radius, -up, groin),
                    compute_web(arc, outward, across, radius, up, high),
                    compute_web(arc, outward, across, radius, up, groin)
                };
            }
            blocks.push_back(to_stone(intrados, extrados, "groin"));

            for (const double sign : {-1.0, 1.0})
                for (size_t j = 0; j + 1 < joints.size(); j++) {
                    if (joints[j] < groin - 1e-9)
                        continue;

                    std::vector<Point> inner;
                    std::vector<Point> outside;
                    for (const double radius : {arc.radius, outer}) {
                        std::vector<Point>& loop = radius == arc.radius ? inner : outside;
                        loop = {
                            compute_web(arc, outward, across, radius, sign * down, joints[j]),
                            compute_web(arc, outward, across, radius, sign * down, joints[j + 1]),
                            compute_web(arc, outward, across, radius, sign * up, joints[j + 1]),
                            compute_web(arc, outward, across, radius, sign * up, joints[j])
                        };
                    }
                    blocks.push_back(to_voussoir(inner, outside, "voussoir"));
                }
        }
    }

    return blocks;
}

std::vector<std::shared_ptr<Element>>
cloister_vault(double span, double rise, double thickness, int courses, int rings) {
    return to_bay(span, rise, thickness, courses, rings, false);
}

// ═══════════════════════════════════════════════════════════════════════════
// Star vault
// ═══════════════════════════════════════════════════════════════════════════

/// The sphere through the four corners of a square bay at z 0 with its top at rise.
struct Sail {
    double radius = 0.0;
    double centre = 0.0;
};

/// The sail over a bay of span rising to rise.
Sail compute_sail(double span, double rise) {

    const double radius = (span * span / 2.0 + rise * rise) / (2.0 * rise);

    return {radius, rise - radius};
}

/// The sail intrados over a plan point, moved out from the sphere centre by offset.
Point compute_on_sail(const Sail& sail, const Point& plan, double offset) {

    const Point centre(0.0, 0.0, sail.centre);
    const Point point(
        plan[0],
        plan[1],
        sail.centre + std::sqrt(std::max(
                          0.0, sail.radius * sail.radius - plan[0] * plan[0] - plan[1] * plan[1]
                      ))
    );

    return centre + (point - centre) * ((sail.radius + offset) / sail.radius);
}

/// The sail intrados as one cubic NURBS surface whose parameters are the plan coordinates over the bay.
NurbsSurface compute_sail_surface(const Sail& sail, double half, int count) {

    NurbsSurface surface(3, false, 4, 4, count, count);
    for (int dir = 0; dir < 2; dir++)
        for (int k = 0; k < count + 2; k++)
            surface.set_nurbsknot(
                dir, k, std::clamp(static_cast<double>(k - 2), 0.0, static_cast<double>(count - 3))
            );

    for (int i = 0; i < count; i++)
        for (int j = 0; j < count; j++) {
            const double x =
                -half + 2.0 * half * std::clamp(i - 1.0, 0.0, count - 3.0) / (count - 3);
            const double y =
                -half + 2.0 * half * std::clamp(j - 1.0, 0.0, count - 3.0) / (count - 3);
            surface.set_cv(i, j, compute_on_sail(sail, Point(x, y, 0.0), 0.0));
        }

    return surface;
}

/// The parameters of a plan point on the sail surface.
Point compute_uv(const NurbsSurface& surface, double half, const Point& plan) {

    const std::pair<double, double> u = surface.domain(0);
    const std::pair<double, double> v = surface.domain(1);

    return Point(
        u.first + (plan[0] + half) / (2.0 * half) * (u.second - u.first),
        v.first + (plan[1] + half) / (2.0 * half) * (v.second - v.first),
        0.0
    );
}

/// A straight parameter-space curve from a to b.
NurbsCurve to_uv_line(const Point& a, const Point& b) {
    return NurbsCurve::create(false, 1, {Point(a[0], a[1], 0.0), Point(b[0], b[1], 0.0)});
}

/// The web over a counter-clockwise plan triangle as a BRep solid.
std::shared_ptr<Element> to_shell(
    const Sail& sail,
    const NurbsSurface& surface,
    double half,
    const std::vector<Point>& corners,
    double thickness,
    int subdivisions
) {

    const Point centre(0.0, 0.0, sail.centre);
    const double scale = (sail.radius + thickness) / sail.radius;
    const Xform out = Xform::scale_uniform(centre, scale);

    NurbsSurface below = surface;
    below.transpose();
    const NurbsSurface above = surface.transformed(out);

    BRep brep;
    brep.name = "web";
    const int s_below = brep.add_surface(below);
    const int s_above = brep.add_surface(above);

    std::vector<int> inner;
    std::vector<int> outer;
    std::vector<Point> uvs;
    for (const Point& corner : corners) {
        uvs.push_back(compute_uv(surface, half, corner));
        inner.push_back(brep.add_vertex(surface.point_at(uvs.back()[0], uvs.back()[1])));
        outer.push_back(brep.add_vertex(above.point_at(uvs.back()[0], uvs.back()[1])));
    }

    std::vector<int> e_inner;
    std::vector<int> e_outer;
    std::vector<int> e_rise;
    std::vector<NurbsCurve> curves;
    for (size_t k = 0; k < 3; k++) {
        const size_t next = (k + 1) % 3;
        std::vector<Point> samples;
        for (int t = 0; t <= subdivisions; t++) {
            const Point uv =
                uvs[k] + (uvs[next] - uvs[k]) * (static_cast<double>(t) / subdivisions);
            samples.push_back(surface.point_at(uv[0], uv[1]));
        }
        curves.push_back(Primitives::create_interpolated(samples));
        const int inner_curve = brep.add_curve_3d(curves.back());
        e_inner.push_back(brep.add_edge(inner_curve, inner[k], inner[next]));
        const int outer_curve = brep.add_curve_3d(curves.back().transformed(out));
        e_outer.push_back(brep.add_edge(outer_curve, outer[k], outer[next]));
        const NurbsCurve rise = NurbsCurve::create(
            false, 1, {brep.m_vertices[inner[k]].point, brep.m_vertices[outer[k]].point}
        );
        const int rise_curve = brep.add_curve_3d(rise);
        e_rise.push_back(brep.add_edge(rise_curve, inner[k], outer[k]));
    }

    std::vector<BRepRef> faces;
    std::vector<BRepRef> wire_below;
    std::vector<BRepRef> wire_above;
    for (size_t k = 0; k < 3; k++) {
        const size_t next = (k + 1) % 3;
        const int below_uv = brep.add_curve_2d(
            to_uv_line(Point(uvs[k][1], uvs[k][0], 0.0), Point(uvs[next][1], uvs[next][0], 0.0))
        );
        brep.add_pcurve(e_inner[k], s_below, below_uv);
        brep.add_pcurve(e_outer[k], s_above, brep.add_curve_2d(to_uv_line(uvs[k], uvs[next])));
        wire_above.push_back({e_outer[k], BRepOrientation::Forward});
        wire_below.insert(wire_below.begin(), {e_inner[k], BRepOrientation::Reversed});

        const NurbsSurface side = Primitives::create_ruled(curves[k], curves[k].transformed(out));
        const int s_side = brep.add_surface(side);
        const int side_bottom_uv =
            brep.add_curve_2d(to_uv_line(Point(0.0, 0.0, 0.0), Point(1.0, 0.0, 0.0)));
        brep.add_pcurve(e_inner[k], s_side, side_bottom_uv);
        const int side_top_uv =
            brep.add_curve_2d(to_uv_line(Point(0.0, 1.0, 0.0), Point(1.0, 1.0, 0.0)));
        brep.add_pcurve(e_outer[k], s_side, side_top_uv);
        const int side_start_uv =
            brep.add_curve_2d(to_uv_line(Point(0.0, 0.0, 0.0), Point(0.0, 1.0, 0.0)));
        brep.add_pcurve(e_rise[k], s_side, side_start_uv);
        const int side_end_uv =
            brep.add_curve_2d(to_uv_line(Point(1.0, 0.0, 0.0), Point(1.0, 1.0, 0.0)));
        brep.add_pcurve(e_rise[next], s_side, side_end_uv);
        const int wire = brep.add_wire(
            {{e_inner[k], BRepOrientation::Forward},
             {e_rise[next], BRepOrientation::Forward},
             {e_outer[k], BRepOrientation::Reversed},
             {e_rise[k], BRepOrientation::Reversed}}
        );
        faces.push_back(
            {brep.add_face(s_side, {{wire, BRepOrientation::Forward}}), BRepOrientation::Forward}
        );
    }
    const int below_wire = brep.add_wire(wire_below);
    faces.push_back(
        {brep.add_face(s_below, {{below_wire, BRepOrientation::Forward}}), BRepOrientation::Forward}
    );
    const int above_wire = brep.add_wire(wire_above);
    faces.push_back(
        {brep.add_face(s_above, {{above_wire, BRepOrientation::Forward}}), BRepOrientation::Forward}
    );
    brep.add_solid({{brep.add_shell(faces), BRepOrientation::Forward}});

    return std::make_shared<Element>(brep, "web");
}

/// A rib along a plan segment under the sail.
std::shared_ptr<Element>
to_rib(const Sail& sail, const Point& start, const Point& end, double rib, int subdivisions) {

    std::vector<Point> axis;
    std::vector<Vector> ups;
    for (int k = 0; k <= subdivisions; k++) {
        axis.push_back(compute_on_sail(
            sail, start + (end - start) * (static_cast<double>(k) / subdivisions), -rib / 2.0
        ));
        ups.push_back((axis.back() - Point(0.0, 0.0, sail.centre)).normalized());
    }

    return std::make_shared<wood_session::BeamCurved>(
        NurbsCurve::create_interpolated(axis), rib, rib, ups, 1, 6, subdivisions, "rib"
    );
}

std::vector<std::shared_ptr<Element>>
star_vault(double span, double rise, double thickness, double star, double rib, int subdivisions) {

    const Sail sail = compute_sail(span, rise);
    const double half = span / 2.0;
    const Point crown(0.0, 0.0, 0.0);
    const std::vector<Point> corners = {
        Point(half, half, 0.0),
        Point(-half, half, 0.0),
        Point(-half, -half, 0.0),
        Point(half, -half, 0.0)
    };
    const std::vector<Point> stars = {
        Point(0.0, star * half, 0.0),
        Point(-star * half, 0.0, 0.0),
        Point(0.0, -star * half, 0.0),
        Point(star * half, 0.0, 0.0)
    };

    // twelve webs on one NURBS sail: one against each wall, two either side of each diagonal
    const NurbsSurface surface = compute_sail_surface(sail, half, subdivisions + 3);
    std::vector<std::shared_ptr<Element>> elements;
    for (size_t i = 0; i < 4; i++) {
        const size_t next = (i + 1) % 4;
        const size_t before = (i + 3) % 4;
        elements.push_back(to_shell(
            sail, surface, half, {corners[i], corners[next], stars[i]}, thickness, subdivisions
        ));
        elements.push_back(to_shell(
            sail, surface, half, {crown, stars[before], corners[i]}, thickness, subdivisions
        ));
        elements.push_back(
            to_shell(sail, surface, half, {crown, corners[i], stars[i]}, thickness, subdivisions)
        );
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

// ═══════════════════════════════════════════════════════════════════════════
// Gothic ribbed vaults (LiDAR cathedral patterns — Tracing the Past / Willis / Wendland / Jobbik)
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// Evaluates the top vault surface point S(x, y) and unit upward normal N(x, y) at plan point (x, y).
/// web_dir selects which cylinder web is active on groin/cloister surfaces when (x, y) lies on a diagonal crease.
std::pair<Point, Vector> eval_vault_surface(
    double x,
    double y,
    double hx,
    double hy,
    double rise,
    ArchMethod method,
    const Point& ref_mid
) {
    const double h_safe_rise = std::max(1.0, rise);
    const auto cyl_arch = [](double coord, double half_span, double arch_rise, double& z_out, double& dz_dc) {
        if (arch_rise <= 1e-6) {
            z_out = 0.0;
            dz_dc = 0.0;
            return;
        }
        const double r = (half_span * half_span + arch_rise * arch_rise) / (2.0 * arch_rise);
        const double z_c = arch_rise - r;
        const double c = std::clamp(coord, -half_span, half_span);
        const double dz = std::sqrt(std::max(1e-6, r * r - c * c));
        z_out = z_c + dz;
        dz_dc = -c / dz;
    };

    if (method == ArchMethod::groin_cylinders) {
        const double ux = std::abs(ref_mid[0]) / std::max(1.0, hx);
        const double uy = std::abs(ref_mid[1]) / std::max(1.0, hy);
        if (ux <= uy + 1e-6) {
            double z = 0.0, dz_dx = 0.0;
            cyl_arch(x, hx, h_safe_rise, z, dz_dx);
            Vector n(-dz_dx, 0.0, 1.0);
            n.normalize_self();
            return {Point(x, y, z), n};
        } else {
            double z = 0.0, dz_dy = 0.0;
            cyl_arch(y, hy, h_safe_rise, z, dz_dy);
            Vector n(0.0, -dz_dy, 1.0);
            n.normalize_self();
            return {Point(x, y, z), n};
        }
    }

    if (method == ArchMethod::cloister_cylinders) {
        // Domical cylinder groin surface: boundary arches rise to 0.65 * rise, crown rises to rise
        const double ux = std::abs(ref_mid[0]) / std::max(1.0, hx);
        const double uy = std::abs(ref_mid[1]) / std::max(1.0, hy);
        const double h_wall  = 0.65 * h_safe_rise;
        const double h_ridge = 0.35 * h_safe_rise;
        if (ux <= uy + 1e-6) {
            double zx = 0.0, dz_dx = 0.0;
            double zy = 0.0, dz_dy = 0.0;
            cyl_arch(x, hx, h_wall, zx, dz_dx);
            cyl_arch(y, hy, h_ridge, zy, dz_dy);
            Vector n(-dz_dx, -dz_dy, 1.0);
            n.normalize_self();
            return {Point(x, y, zx + zy), n};
        } else {
            double zy = 0.0, dz_dy = 0.0;
            double zx = 0.0, dz_dx = 0.0;
            cyl_arch(y, hy, h_wall, zy, dz_dy);
            cyl_arch(x, hx, h_ridge, zx, dz_dx);
            Vector n(-dz_dx, -dz_dy, 1.0);
            n.normalize_self();
            return {Point(x, y, zx + zy), n};
        }
    }

    // Default: spherical_sail through the 4 corners (+-hx, +-hy, 0) rising to crown (0, 0, rise)
    const double r_diag_sq = hx * hx + hy * hy;
    const double r_sail = (r_diag_sq + h_safe_rise * h_safe_rise) / (2.0 * h_safe_rise);
    const double z_c = h_safe_rise - r_sail;
    const double r_xy_sq = std::min(r_sail * r_sail, x * x + y * y);
    const double dz = std::sqrt(std::max(0.0, r_sail * r_sail - r_xy_sq));
    const double z = z_c + dz;
    Vector n(x / r_sail, y / r_sail, dz / r_sail);
    n.normalize_self();
    return {Point(x, y, z), n};
}

struct RibEdgeSpec {
    int u = 0;
    int v = 0;
    double width_scale = 1.0;
    std::string role = "rib";
};

double wrap_two_pi(double angle) {
    double a = std::fmod(angle, Tolerance::TWO_PI);
    if (a < 0.0)
        a += Tolerance::TWO_PI;
    return a;
}

} // namespace

std::vector<std::shared_ptr<Element>> gothic_ribbed_vault(
    double span_x,
    double span_y,
    double rise,
    double rib_width,
    double rib_height,
    GothicPattern pattern,
    ArchMethod surface_method,
    int subdivisions
) {
    const double hx = 0.5 * span_x;
    const double hy = 0.5 * span_y;
    const int n_sub = std::max(8, subdivisions);

    // -------------------------------------------------------------------------
    // 1. Build the 2D planar straight-line graph (V, E) on [-hx, hx] x [-hy, hy]
    //    Vertices 0..3 are ALWAYS the 4 corner springers (SW, SE, NE, NW)
    //    Vertices 4..7 are the 4 wall midpoints (S, E, N, W)
    //    Vertex 8 is the central crown boss (0, 0)
    // -------------------------------------------------------------------------
    std::vector<Point> nodes = {
        Point(-hx, -hy, 0.0), // 0: SW corner
        Point( hx, -hy, 0.0), // 1: SE corner
        Point( hx,  hy, 0.0), // 2: NE corner
        Point(-hx,  hy, 0.0), // 3: NW corner
        Point(0.0, -hy, 0.0), // 4: S wall midpoint
        Point( hx, 0.0, 0.0), // 5: E wall midpoint
        Point(0.0,  hy, 0.0), // 6: N wall midpoint
        Point(-hx, 0.0, 0.0), // 7: W wall midpoint
        Point(0.0, 0.0, 0.0)  // 8: Central crown boss
    };
    const auto add_node = [&](double x, double y) -> int {
        for (size_t i = 0; i < nodes.size(); ++i)
            if (std::hypot(nodes[i][0] - x, nodes[i][1] - y) < 1e-3)
                return static_cast<int>(i);
        nodes.emplace_back(x, y, 0.0);
        return static_cast<int>(nodes.size() - 1);
    };

    std::vector<RibEdgeSpec> edges;
    const auto add_edge = [&](int u, int v, double w_scale, const std::string& role) {
        if (u == v)
            return;
        for (const RibEdgeSpec& e : edges)
            if ((e.u == u && e.v == v) || (e.u == v && e.v == u))
                return;
        edges.push_back({u, v, w_scale, role});
    };

    // Symmetric perimeter wall arches (all 4 boundaries identical)
    if (pattern != GothicPattern::fan_conoid) {
        for (int i = 0; i < 4; ++i) {
            const int c_curr = i;
            const int c_next = (i + 1) % 4;
            const int m_wall = 4 + i;
            add_edge(c_curr, m_wall, 1.0, "wall_arch");
            add_edge(m_wall, c_next, 1.0, "wall_arch");
        }
    }

    if (pattern == GothicPattern::quadripartite) {
        for (int i = 0; i < 4; ++i) {
            add_edge(i, 8, 1.0, "diagonal_rib");
            add_edge(4 + i, 8, 1.0, "ridge_rib");
        }
    } else if (pattern == GothicPattern::tierceron) {
        // Starcut ridge bosses at +-0.5 along both orthogonal ridges (4-fold symmetric)
        const std::array<int, 4> r_nodes = {
            add_node(0.0, -0.5 * hy),
            add_node(0.5 * hx, 0.0),
            add_node(0.0, 0.5 * hy),
            add_node(-0.5 * hx, 0.0)
        };
        for (int i = 0; i < 4; ++i) {
            const int c_next = (i + 1) % 4;
            add_edge(i, 8, 1.0, "diagonal_rib");
            add_edge(4 + i, r_nodes[i], 1.0, "ridge_rib");
            add_edge(r_nodes[i], 8, 1.0, "ridge_rib");
            add_edge(i, r_nodes[i], 1.0, "tierceron_rib");
            add_edge(c_next, r_nodes[i], 1.0, "tierceron_rib");
        }
    } else if (pattern == GothicPattern::star_stellar) {
        // 8-pointed Sterngewoelbe / Boveda Estrellada star vault
        const double s = 0.44;
        const double d = 0.54;
        const std::array<int, 4> s_nodes = {
            add_node(0.0, -s * hy),
            add_node(s * hx, 0.0),
            add_node(0.0, s * hy),
            add_node(-s * hx, 0.0)
        };
        const std::array<int, 4> d_nodes = {
            add_node(-d * hx, -d * hy),
            add_node( d * hx, -d * hy),
            add_node( d * hx,  d * hy),
            add_node(-d * hx,  d * hy)
        };
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            add_edge(i, d_nodes[i], 1.0, "diagonal_rib");
            add_edge(d_nodes[i], 8, 1.0, "diagonal_rib");
            add_edge(4 + i, s_nodes[i], 1.0, "ridge_rib");
            add_edge(s_nodes[i], 8, 1.0, "lierne_rib");
            add_edge(i, s_nodes[i], 1.0, "tierceron_rib");
            add_edge(next, s_nodes[i], 1.0, "tierceron_rib");
            add_edge(d_nodes[i], s_nodes[i], 1.0, "lierne_rib");
            add_edge(d_nodes[i], s_nodes[prev], 1.0, "lierne_rib");
        }
    } else if (pattern == GothicPattern::lierne_star) {
        // Ely Choir / Wells Lady Chapel double-star lierne vault with petal lozenges
        const double r_out = 0.66;
        const double r_in  = 0.34;
        const double d_mid = 0.50;
        const std::array<int, 4> ro = {
            add_node(0.0, -r_out * hy),
            add_node(r_out * hx, 0.0),
            add_node(0.0, r_out * hy),
            add_node(-r_out * hx, 0.0)
        };
        const std::array<int, 4> ri = {
            add_node(0.0, -r_in * hy),
            add_node(r_in * hx, 0.0),
            add_node(0.0, r_in * hy),
            add_node(-r_in * hx, 0.0)
        };
        const std::array<int, 4> dn = {
            add_node(-d_mid * hx, -d_mid * hy),
            add_node( d_mid * hx, -d_mid * hy),
            add_node( d_mid * hx,  d_mid * hy),
            add_node(-d_mid * hx,  d_mid * hy)
        };
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            add_edge(i, dn[i], 1.0, "diagonal_rib");
            add_edge(dn[i], 8, 1.0, "diagonal_rib");
            add_edge(4 + i, ro[i], 1.0, "ridge_rib");
            add_edge(ro[i], ri[i], 1.0, "ridge_rib");
            add_edge(ri[i], 8, 1.0, "ridge_rib");
            add_edge(i, ro[i], 1.0, "tierceron_rib");
            add_edge(next, ro[i], 1.0, "tierceron_rib");
            add_edge(dn[i], ri[i], 1.0, "lierne_rib");
            add_edge(dn[i], ri[prev], 1.0, "lierne_rib");
            add_edge(dn[i], ro[i], 1.0, "lierne_rib");
            add_edge(dn[i], ro[prev], 1.0, "lierne_rib");
        }
    } else if (pattern == GothicPattern::net_reticulated) {
        // Late-Gothic Netzgewoelbe diamond-lattice net vault (Jobbik 2024 / Parler)
        const std::array<int, 4> q_nodes = {
            add_node(0.0, -0.58 * hy),
            add_node(0.58 * hx, 0.0),
            add_node(0.0, 0.58 * hy),
            add_node(-0.58 * hx, 0.0)
        };
        const std::array<int, 4> d_outer = {
            add_node(-0.44 * hx, -0.44 * hy),
            add_node( 0.44 * hx, -0.44 * hy),
            add_node( 0.44 * hx,  0.44 * hy),
            add_node(-0.44 * hx,  0.44 * hy)
        };
        const std::array<int, 4> r_inner = {
            add_node(0.0, -0.26 * hy),
            add_node(0.26 * hx, 0.0),
            add_node(0.0, 0.26 * hy),
            add_node(-0.26 * hx, 0.0)
        };
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            // Corner springers to outer diamond & ridge nodes
            add_edge(i, d_outer[i], 1.0, "diagonal_rib");
            add_edge(i, q_nodes[i], 1.0, "tierceron_rib");
            add_edge(next, q_nodes[i], 1.0, "tierceron_rib");
            // Wall midpoints to outer ridge nodes
            add_edge(4 + i, q_nodes[i], 1.0, "ridge_rib");
            // Outer rhomboidal net ring
            add_edge(d_outer[i], q_nodes[i], 1.0, "net_rib");
            add_edge(d_outer[i], q_nodes[prev], 1.0, "net_rib");
            // Inner rhomboidal net ring
            add_edge(d_outer[i], r_inner[i], 1.0, "net_rib");
            add_edge(d_outer[i], r_inner[prev], 1.0, "net_rib");
            // Central Parler diamond lozenge (Raute)
            add_edge(r_inner[i], r_inner[next], 1.0, "net_rib");
        }
    } else if (pattern == GothicPattern::fan_conoid) {
        // Perpendicular 2-tier fan / conoid vault (Gloucester Cloisters / King's College)
        const double h_min = std::min(hx, hy);
        const double r1 = 0.42 * h_min;
        const double r2 = 0.76 * h_min;
        const std::array<double, 4> base_angles = {
            0.0, 0.5 * Tolerance::PI, Tolerance::PI, 1.5 * Tolerance::PI
        };
        std::array<std::array<int, 5>, 4> ring1{};
        std::array<std::array<int, 5>, 4> ring2{};
        for (int c = 0; c < 4; ++c) {
            for (int k = 0; k < 5; ++k) {
                const double ang = base_angles[c] + k * (0.125 * Tolerance::PI);
                ring1[c][k] = add_node(
                    nodes[c][0] + r1 * std::cos(ang),
                    nodes[c][1] + r1 * std::sin(ang)
                );
                ring2[c][k] = add_node(
                    nodes[c][0] + r2 * std::cos(ang),
                    nodes[c][1] + r2 * std::sin(ang)
                );
            }
        }
        const std::array<int, 4> spandrel = {
            add_node(0.0, -0.34 * hy),
            add_node(0.34 * hx, 0.0),
            add_node(0.0, 0.34 * hy),
            add_node(-0.34 * hx, 0.0)
        };
        for (int c = 0; c < 4; ++c) {
            const int next = (c + 1) % 4;
            const int prev = (c + 3) % 4;
            // 5 radiating conoid ribs across 2 concentric tiers
            for (int k = 0; k < 5; ++k) {
                const std::string role = (k == 0 || k == 4) ? "wall_arch" : "fan_rib";
                add_edge(c, ring1[c][k], 1.0, role);
                add_edge(ring1[c][k], ring2[c][k], 1.0, role);
            }
            // Inner and outer conoid collar arc segments
            for (int k = 0; k < 4; ++k) {
                add_edge(ring1[c][k], ring1[c][k + 1], 1.0, "collar_rib");
                add_edge(ring2[c][k], ring2[c][k + 1], 1.0, "collar_rib");
            }
            // Boundary wall segments between outer conoid collars and wall midpoints
            add_edge(ring2[c][0], 4 + c, 1.0, "wall_arch");
            add_edge(4 + c, ring2[next][4], 1.0, "wall_arch");
            // Central spandrel & ridge ribs
            add_edge(4 + c, spandrel[c], 1.0, "ridge_rib");
            add_edge(spandrel[c], 8, 1.0, "ridge_rib");
            add_edge(ring2[c][2], 8, 1.0, "diagonal_rib");
            add_edge(ring2[c][1], spandrel[c], 1.0, "lierne_rib");
            add_edge(ring2[c][3], spandrel[prev], 1.0, "lierne_rib");
        }
    }

    // -------------------------------------------------------------------------
    // 2. Build adjacency & sort outgoing edges CCW by plan angle at every node
    // -------------------------------------------------------------------------
    struct AdjItem {
        size_t edge_idx = 0;
        int other = 0;
        double angle = 0.0;
        Vector dir_xy;
    };
    std::vector<std::vector<AdjItem>> adj(nodes.size());
    for (size_t e_idx = 0; e_idx < edges.size(); ++e_idx) {
        const int u = edges[e_idx].u;
        const int v = edges[e_idx].v;
        const double dx = nodes[v][0] - nodes[u][0];
        const double dy = nodes[v][1] - nodes[u][1];
        const double len = std::hypot(dx, dy);
        if (len < 1e-6)
            continue;
        const Vector dir_uv(dx / len, dy / len, 0.0);
        const Vector dir_vu(-dx / len, -dy / len, 0.0);
        adj[u].push_back({e_idx, v, wrap_two_pi(std::atan2(dy, dx)), dir_uv});
        adj[v].push_back({e_idx, u, wrap_two_pi(std::atan2(-dy, -dx)), dir_vu});
    }
    for (std::vector<AdjItem>& list : adj)
        std::sort(list.begin(), list.end(), [](const AdjItem& a, const AdjItem& b) {
            return a.angle < b.angle;
        });

    // -------------------------------------------------------------------------
    // 3. Corner Support Blocks (Tas-de-charge / Springer Plinths) at nodes 0..3
    // -------------------------------------------------------------------------
    std::vector<std::shared_ptr<Element>> result;
    std::array<double, 4> corner_radii = {0.0, 0.0, 0.0, 0.0};

    // Determine common corner stop radius so all 4 corners are symmetric
    double min_corner_dphi = Tolerance::PI;
    for (int c = 0; c < 4; ++c) {
        const std::vector<AdjItem>& list = adj[c];
        for (size_t j = 0; j + 1 < list.size(); ++j) {
            const double dphi = wrap_two_pi(list[j + 1].angle - list[j].angle);
            if (dphi > 1e-4 && dphi < Tolerance::PI)
                min_corner_dphi = std::min(min_corner_dphi, dphi);
        }
    }
    const double r_sep = (0.5 * rib_width) / std::tan(0.5 * std::max(0.15, min_corner_dphi));
    const double r_corner_common = std::max(r_sep + 25.0, 2.5 * rib_width);
    for (int c = 0; c < 4; ++c)
        corner_radii[c] = r_corner_common;

    // Outward wall normals for the two walls meeting at each corner c in {0..3}
    const std::array<Vector, 4> wall_out_first = {
        Vector(0.0, -1.0, 0.0), // C0: South wall first (phi=0), West wall last (phi=90)
        Vector(1.0,  0.0, 0.0), // C1: East wall first (phi=90), South wall last (phi=180)
        Vector(0.0,  1.0, 0.0), // C2: North wall first (phi=180), East wall last (phi=270)
        Vector(-1.0, 0.0, 0.0)  // C3: West wall first (phi=270), North wall last (phi=0)
    };
    const std::array<Vector, 4> wall_out_last = {
        Vector(-1.0, 0.0, 0.0),
        Vector(0.0, -1.0, 0.0),
        Vector(1.0,  0.0, 0.0),
        Vector(0.0,  1.0, 0.0)
    };

    for (int c = 0; c < 4; ++c) {
        std::vector<AdjItem> list = adj[c];
        if (c == 3 && list.size() >= 2) {
            // At NW corner C3 (-hx, +hy), angles are 270..360 (and 0 along North wall); unwrap 0 -> 2*pi
            for (AdjItem& item : list)
                if (item.angle < 0.5 * Tolerance::PI)
                    item.angle += Tolerance::TWO_PI;
            std::sort(list.begin(), list.end(), [](const AdjItem& a, const AdjItem& b) {
                return a.angle < b.angle;
            });
        }
        if (list.size() < 2)
            continue;

        const double r_stop = corner_radii[c];
        const double hw = 0.5 * rib_width;
        const Point& cp = nodes[c];

        std::vector<Point> poly_2d;
        // Outer corner point offset by hw along both outer walls
        poly_2d.push_back(cp + wall_out_first[c] * hw + wall_out_last[c] * hw);
        // Outer stop point on first wall rib
        poly_2d.push_back(cp + list.front().dir_xy * r_stop + wall_out_first[c] * hw);
        // Interior bisector intersections between consecutive stop planes
        for (size_t j = 0; j + 1 < list.size(); ++j) {
            const double dphi = list[j + 1].angle - list[j].angle;
            const double bis_ang = 0.5 * (list[j].angle + list[j + 1].angle);
            const double dist = r_stop / std::max(0.25, std::cos(0.5 * dphi));
            poly_2d.emplace_back(
                cp[0] + dist * std::cos(bis_ang),
                cp[1] + dist * std::sin(bis_ang),
                0.0
            );
        }
        // Outer stop point on last wall rib
        poly_2d.push_back(cp + list.back().dir_xy * r_stop + wall_out_last[c] * hw);

        std::vector<Point> bot_loop;
        std::vector<Point> top_loop;
        const double z_base = -rib_height;
        for (const Point& p2 : poly_2d) {
            const auto [p_surf, n_surf] =
                eval_vault_surface(p2[0], p2[1], hx, hy, rise, surface_method, cp);
            top_loop.push_back(p_surf);
            bot_loop.emplace_back(p2[0], p2[1], z_base);
        }
        result.push_back(to_voussoir(bot_loop, top_loop, "support_block"));
    }

    // -------------------------------------------------------------------------
    // 4. Helper to compute end-cut planes for edge e_idx at endpoint node_id
    // -------------------------------------------------------------------------
    const auto endpoint_cuts = [&](size_t e_idx, int node_id, double& trim_offset) -> std::vector<Plane> {
        const Point& vp = nodes[node_id];
        const std::vector<AdjItem>& list = adj[node_id];
        int pos = -1;
        for (size_t j = 0; j < list.size(); ++j)
            if (list[j].edge_idx == e_idx) {
                pos = static_cast<int>(j);
                break;
            }
        if (pos < 0)
            return {};

        const AdjItem& curr = list[pos];
        // At a corner springer (0..3), stop cleanly at the Corner Support Block facet plane!
        if (node_id >= 0 && node_id < 4) {
            const double r_stop = corner_radii[node_id];
            trim_offset = r_stop - 25.0;
            const Point stop_pt = vp + curr.dir_xy * r_stop;
            return {Plane::from_point_normal(stop_pt, curr.dir_xy)};
        }

        // At an interior or wall-midpoint node (node_id >= 4):
        // Extend uncut axis 25 mm past the node and cut with the two exact angle-bisector planes!
        trim_offset = -25.0;
        const size_t m = list.size();
        if (m == 1)
            return {Plane::from_point_normal(vp, curr.dir_xy)};

        const AdjItem& cw  = list[(pos + m - 1) % m];
        const AdjItem& ccw = list[(pos + 1) % m];

        double dphi_cw  = wrap_two_pi(curr.angle - cw.angle);
        double dphi_ccw = wrap_two_pi(ccw.angle - curr.angle);
        if (dphi_cw < 1e-4)
            dphi_cw = Tolerance::PI;
        if (dphi_ccw < 1e-4)
            dphi_ccw = Tolerance::PI;
        dphi_cw  = std::min(Tolerance::PI, dphi_cw);
        dphi_ccw = std::min(Tolerance::PI, dphi_ccw);

        const double bis_cw  = curr.angle - 0.5 * dphi_cw;
        const double bis_ccw = curr.angle + 0.5 * dphi_ccw;

        Vector n_cw(-std::sin(bis_cw), std::cos(bis_cw), 0.0);
        Vector n_ccw(std::sin(bis_ccw), -std::cos(bis_ccw), 0.0);
        n_cw.normalize_self();
        n_ccw.normalize_self();

        std::vector<Plane> planes;
        planes.push_back(Plane::from_point_normal(vp, n_cw));
        if ((n_cw - n_ccw).magnitude() > 1e-3)
            planes.push_back(Plane::from_point_normal(vp, n_ccw));
        return planes;
    };

    // -------------------------------------------------------------------------
    // 5. Generate every BeamCurved rib with its top face on the vault surface
    //    and its ends trimmed by the support-block / bisector miter planes
    // -------------------------------------------------------------------------
    for (size_t e_idx = 0; e_idx < edges.size(); ++e_idx) {
        const RibEdgeSpec& spec = edges[e_idx];
        const Point& p_u = nodes[spec.u];
        const Point& p_v = nodes[spec.v];
        const double dx = p_v[0] - p_u[0];
        const double dy = p_v[1] - p_u[1];
        const double len_2d = std::hypot(dx, dy);
        if (len_2d < 1e-3)
            continue;
        const Vector dir_2d(dx / len_2d, dy / len_2d, 0.0);
        const Point mid_2d = p_u + dir_2d * (0.5 * len_2d);

        double offset_u = 0.0;
        double offset_v = 0.0;
        std::vector<Plane> cuts_u = endpoint_cuts(e_idx, spec.u, offset_u);
        std::vector<Plane> cuts_v = endpoint_cuts(e_idx, spec.v, offset_v);

        const double s_start = offset_u;
        const double s_end   = len_2d - offset_v;
        if (s_end <= s_start + 10.0)
            continue;

        // First evaluate top-surface points and surface normals along [s_start, s_end]
        std::vector<Point> surf_pts(n_sub + 1);
        std::vector<Vector> surf_norms(n_sub + 1);
        for (int k = 0; k <= n_sub; ++k) {
            const double s = s_start + (s_end - s_start) * (static_cast<double>(k) / n_sub);
            const Point plan_pt = p_u + dir_2d * s;
            const auto [sp, sn] = eval_vault_surface(
                plan_pt[0], plan_pt[1], hx, hy, rise, surface_method, mid_2d
            );
            surf_pts[k] = sp;
            surf_norms[k] = sn;
        }

        // Offset each station inward along the cross-section up-vector by -rib_height / 2
        // so the top face (+rib_height / 2) lies flush on surf_pts[k]!
        std::vector<Point> axis(n_sub + 1);
        std::vector<Vector> ups(n_sub + 1);
        for (int k = 0; k <= n_sub; ++k) {
            const Vector tang =
                (k == 0) ? (surf_pts[1] - surf_pts[0]).normalized()
                : (k == n_sub) ? (surf_pts[n_sub] - surf_pts[n_sub - 1]).normalized()
                               : (surf_pts[k + 1] - surf_pts[k - 1]).normalized();
            Vector up = surf_norms[k] - tang * surf_norms[k].dot(tang);
            if (up.magnitude() < 1e-6)
                up = Vector::z_axis();
            up.normalize_self();
            ups[k] = up;
            axis[k] = surf_pts[k] - up * (0.5 * rib_height);
        }

        auto beam = std::make_shared<wood_session::BeamCurved>(
            NurbsCurve::create_interpolated(axis),
            rib_width * spec.width_scale,
            rib_height,
            ups,
            1,
            6,
            n_sub,
            spec.role
        );
        beam->cuts = std::move(cuts_u);
        beam->cuts.insert(beam->cuts.end(), cuts_v.begin(), cuts_v.end());
        result.push_back(beam);
    }

    return result;
}

GothicRibbedVault::GothicRibbedVault(
    double span_x,
    double span_y,
    double rise,
    double rib_width,
    double rib_height,
    GothicPattern pattern,
    ArchMethod arch_method,
    int subdivisions,
    const std::string& name
)
    : WoodSession(name),
      span_x(span_x),
      span_y(span_y),
      rise(rise),
      rib_width(rib_width),
      rib_height(rib_height),
      pattern(pattern),
      arch_method(arch_method),
      subdivisions(subdivisions) {

    const std::shared_ptr<TreeNode> supports_group = add_group("supports");
    const std::shared_ptr<TreeNode> ribs_group = add_group("ribs");
    for (const std::shared_ptr<Element>& elem : gothic_ribbed_vault(
             span_x, span_y, rise, rib_width, rib_height, pattern, arch_method, subdivisions
         )) {
        if (elem->name == "support_block")
            add(elem, supports_group);
        else
            add(elem, ribs_group);
    }
}

}
