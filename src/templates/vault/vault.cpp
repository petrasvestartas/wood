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
// Cylindrical Groin Surfaces & Gothic / Star Ribbed Vaults
// (Bern Cathedral / Tracing the Past / Willis / Wendland / Jobbik)
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// Evaluates the cylindrical groin vault surface point S(x, y) and unit upward normal N(x, y) at plan point (x, y).
/// Every vault bay is formed by two orthogonal circular barrel cylinders intersecting along the diagonals |x|/hx = |y|/hy
/// so that zero doubly-curved patches exist anywhere in the vault.
std::pair<Point, Vector> eval_vault_surface(
    double x,
    double y,
    double hx,
    double hy,
    double rise,
    ArchMethod /*method*/,
    const Point& ref_mid
) {
    const double h_safe_rise = std::max(1.0, rise);
    const double h_ref = std::max(1.0, hx);
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

    const double ux_ref = std::abs(ref_mid[0]) / std::max(1.0, hx);
    const double uy_ref = std::abs(ref_mid[1]) / std::max(1.0, hy);
    const double ux_pt  = std::abs(x) / std::max(1.0, hx);
    const double uy_pt  = std::abs(y) / std::max(1.0, hy);

    const bool use_ns = (std::abs(ux_ref - uy_ref) > 1e-6) ? (ux_ref <= uy_ref) : (ux_pt <= uy_pt);
    if (use_ns) {
        const double c_x = (x / std::max(1.0, hx)) * h_ref;
        double z = 0.0, dz_dc = 0.0;
        cyl_arch(c_x, h_ref, h_safe_rise, z, dz_dc);
        const double dz_dx = dz_dc * (h_ref / std::max(1.0, hx));
        Vector n(-dz_dx, 0.0, 1.0);
        n.normalize_self();
        return {Point(x, y, z), n};
    } else {
        const double c_y = (y / std::max(1.0, hy)) * h_ref;
        double z = 0.0, dz_dc = 0.0;
        cyl_arch(c_y, h_ref, h_safe_rise, z, dz_dc);
        const double dz_dy = dz_dc * (h_ref / std::max(1.0, hy));
        Vector n(0.0, -dz_dy, 1.0);
        n.normalize_self();
        return {Point(x, y, z), n};
    }
}

struct RibEdgeSpec {
    int u = 0;
    int v = 0;
    double width_scale = 1.0;
    std::string role = "rib";
    int tier_override = -1;
    size_t path_idx = 0;
};

struct RibPathSpec {
    std::vector<int> nodes;
    double width_scale = 1.0;
    std::string role = "rib";
    int tier_override = -1;
};

double wrap_two_pi(double angle) {
    double a = std::fmod(angle, Tolerance::TWO_PI);
    if (a < 0.0)
        a += Tolerance::TWO_PI;
    return a;
}

/// Builds a pure circular barrel cylinder cubic NURBS surface over [-hx, hx] x [-hy, hy] for the active vault web at ref_mid.
/// Uses exact Greville abscissae for X and Y so X(u, v) and Y(u, v) are linear to machine precision (1e-13 mm),
/// and solves the 1D Greville collocation system so Z matches the circular cylinder to < 1e-6 mm.
NurbsSurface compute_vault_web_surface(
    double hx,
    double hy,
    double rise,
    ArchMethod surface_method,
    const Point& ref_mid,
    int count
) {
    const double m_span = static_cast<double>(count - 3);
    NurbsSurface surface(3, false, 4, 4, count, count);
    const auto knot_val = [&](int k) -> double {
        return std::clamp(static_cast<double>(k - 2), 0.0, m_span);
    };
    for (int dir = 0; dir < 2; ++dir)
        for (int k = 0; k < count + 2; ++k)
            surface.set_nurbsknot(dir, k, knot_val(k));

    // Exact Greville abscissae xi[i] = (knot(i) + knot(i+1) + knot(i+2)) / 3.0
    std::vector<double> xi(count);
    std::vector<double> x_cv(count);
    std::vector<double> y_cv(count);
    for (int i = 0; i < count; ++i) {
        xi[i] = (knot_val(i) + knot_val(i + 1) + knot_val(i + 2)) / 3.0;
        x_cv[i] = -hx + 2.0 * hx * (xi[i] / m_span);
        y_cv[i] = -hy + 2.0 * hy * (xi[i] / m_span);
    }

    const bool is_ns = (std::abs(ref_mid[0]) / std::max(1.0, hx) <=
                        std::abs(ref_mid[1]) / std::max(1.0, hy) + 1e-6);
    std::vector<double> z_target(count);
    for (int i = 0; i < count; ++i) {
        const Point p_eval = is_ns ? Point(x_cv[i], 0.0, 0.0) : Point(0.0, y_cv[i], 0.0);
        z_target[i] = eval_vault_surface(
            p_eval[0], p_eval[1], hx, hy, rise, surface_method, ref_mid
        ).first[2];
    }

    std::vector<double> z_cv = z_target;
    for (int iter = 0; iter < 32; ++iter) {
        for (int i = 0; i < count; ++i)
            for (int j = 0; j < count; ++j)
                surface.set_cv(i, j, Point(x_cv[i], y_cv[j], is_ns ? z_cv[i] : z_cv[j]));
        for (int i = 1; i + 1 < count; ++i) {
            const double z_eval = is_ns ? surface.point_at(xi[i], 0.0)[2]
                                        : surface.point_at(0.0, xi[i])[2];
            z_cv[i] += z_target[i] - z_eval;
        }
        for (int i = 0; i < count / 2; ++i) {
            const double avg = 0.5 * (z_cv[i] + z_cv[count - 1 - i]);
            z_cv[i] = avg;
            z_cv[count - 1 - i] = avg;
        }
    }

    for (int i = 0; i < count; ++i)
        for (int j = 0; j < count; ++j)
            surface.set_cv(i, j, Point(x_cv[i], y_cv[j], is_ns ? z_cv[i] : z_cv[j]));

    return surface;
}

/// Maps a 2D plan point (x, y) in [-hx, hx] x [-hy, hy] to (u, v) on the web NURBS surface.
Point compute_vault_uv(const NurbsSurface& surface, double hx, double hy, const Point& plan) {
    const std::pair<double, double> u = surface.domain(0);
    const std::pair<double, double> v = surface.domain(1);
    return Point(
        u.first + ((plan[0] + hx) / (2.0 * hx)) * (u.second - u.first),
        v.first + ((plan[1] + hy) / (2.0 * hy)) * (v.second - v.first),
        0.0
    );
}

/// A straight parameter-space curve from a to b.
NurbsCurve to_uv_line(const Point& a, const Point& b) {
    return NurbsCurve::create(false, 1, {Point(a[0], a[1], 0.0), Point(b[0], b[1], 0.0)});
}

/// Builds a K-sided cylindrical infill panel as a trimmed NURBS BRep solid with zero corner singularities.
std::shared_ptr<Element> to_vault_panel(
    const NurbsSurface& surface,
    double hx,
    double hy,
    const std::vector<Point>& corners,
    double thickness,
    int subdivisions,
    const std::string& panel_name = "vault_panel"
) {
    const size_t k_cnt = corners.size();
    const Xform out = Xform::translation(0.0, 0.0, thickness);

    NurbsSurface below = surface;
    below.transpose();
    const NurbsSurface above = surface.transformed(out);

    BRep brep;
    brep.name = panel_name;
    const int s_below = brep.add_surface(below);
    const int s_above = brep.add_surface(above);

    std::vector<int> inner;
    std::vector<int> outer;
    std::vector<Point> uvs;
    inner.reserve(k_cnt);
    outer.reserve(k_cnt);
    uvs.reserve(k_cnt);
    for (const Point& corner : corners) {
        uvs.push_back(compute_vault_uv(surface, hx, hy, corner));
        inner.push_back(brep.add_vertex(surface.point_at(uvs.back()[0], uvs.back()[1])));
        outer.push_back(brep.add_vertex(above.point_at(uvs.back()[0], uvs.back()[1])));
    }

    std::vector<int> e_inner;
    std::vector<int> e_outer;
    std::vector<int> e_rise;
    std::vector<NurbsCurve> curves;
    e_inner.reserve(k_cnt);
    e_outer.reserve(k_cnt);
    e_rise.reserve(k_cnt);
    curves.reserve(k_cnt);
    for (size_t k = 0; k < k_cnt; ++k) {
        const size_t next = (k + 1) % k_cnt;
        std::vector<Point> samples;
        samples.reserve(subdivisions + 1);
        for (int t = 0; t <= subdivisions; ++t) {
            const Point uv =
                uvs[k] + (uvs[next] - uvs[k]) * (static_cast<double>(t) / subdivisions);
            samples.push_back(surface.point_at(uv[0], uv[1]));
        }
        NurbsCurve edge_crv = NurbsCurve::create_interpolated(
            samples, CurveNurbsKnotStyle::Uniform, CurveInterpStyle::Occt
        );
        edge_crv.set_domain(0.0, 1.0);
        curves.push_back(edge_crv);
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
    for (size_t k = 0; k < k_cnt; ++k) {
        const size_t next = (k + 1) % k_cnt;
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

    return std::make_shared<Element>(brep, panel_name);
}

/// Shared builder that takes a 2D planar rib graph (nodes, edges, rib_paths) and constructs:
/// 1. 4 corner support blocks (support_block)
/// 2. Single-plane mitered planar BeamCurved ribs in vertical planes whose top centerlines lie on the cylindrical vault surface
/// 3. K-sided cylindrical BRep infill panels (panel_name) whose top and bottom edges follow the beam centerlines in plan
std::vector<std::shared_ptr<Element>> build_cylindrical_ribbed_vault_from_graph(
    double hx,
    double hy,
    double rise,
    double rib_width,
    double rib_height,
    double panel_thickness,
    int n_sub,
    ArchMethod surface_method,
    const std::vector<Point>& nodes,
    const std::vector<RibEdgeSpec>& edges,
    const std::vector<RibPathSpec>& rib_paths,
    const std::string& panel_name
) {
    // Precompute pure circular barrel cylinder NURBS surfaces for N/S and E/W quadrants
    const int surf_cv_count = std::max(35, 2 * n_sub + 3);
    const int edge_sub = std::max(24, n_sub);
    const NurbsSurface surf_ns = compute_vault_web_surface(
        hx, hy, rise, surface_method, Point(0.0, 0.5 * hy, 0.0), surf_cv_count
    );
    const NurbsSurface surf_ew = compute_vault_web_surface(
        hx, hy, rise, surface_method, Point(0.5 * hx, 0.0, 0.0), surf_cv_count
    );

    const auto eval_surf_smooth = [&](const NurbsSurface& s, double x, double y) -> Point {
        const double cx = std::clamp(x, -hx, hx);
        const double cy = std::clamp(y, -hy, hy);
        const Point uv0 = compute_vault_uv(s, hx, hy, Point(cx, cy, 0.0));
        const Point p0 = s.point_at(uv0[0], uv0[1]);
        const double dx_out = x - cx;
        const double dy_out = y - cy;
        if (std::abs(dx_out) <= 1e-9 && std::abs(dy_out) <= 1e-9)
            return p0;
        const double eps = 1.0;
        double dz_dx = 0.0;
        double dz_dy = 0.0;
        if (std::abs(dx_out) > 1e-9) {
            const double step_x = (cx >= 0.0) ? -eps : eps;
            const Point uvx = compute_vault_uv(s, hx, hy, Point(cx + step_x, cy, 0.0));
            dz_dx = (s.point_at(uvx[0], uvx[1])[2] - p0[2]) / step_x;
        }
        if (std::abs(dy_out) > 1e-9) {
            const double step_y = (cy >= 0.0) ? -eps : eps;
            const Point uvy = compute_vault_uv(s, hx, hy, Point(cx, cy + step_y, 0.0));
            dz_dy = (s.point_at(uvy[0], uvy[1])[2] - p0[2]) / step_y;
        }
        return Point(x, y, p0[2] + dz_dx * dx_out + dz_dy * dy_out);
    };

    const auto eval_nurbs_cyl = [&](double x, double y) -> Point {
        const double cx = std::clamp(x, -hx, hx);
        const double cy = std::clamp(y, -hy, hy);
        const double ux = std::abs(cx) / std::max(1.0, hx);
        const double uy = std::abs(cy) / std::max(1.0, hy);
        const NurbsSurface& s = (ux <= uy) ? surf_ns : surf_ew;
        const Point uv = compute_vault_uv(s, hx, hy, Point(cx, cy, 0.0));
        return s.point_at(uv[0], uv[1]);
    };

    // Build adjacency & sort outgoing edges CCW by plan angle at every node
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

    // Minimal Corner Column Heads (support_block) at nodes 0..3 so incoming ribs never collide
    std::vector<std::shared_ptr<Element>> result;
    std::array<std::vector<std::pair<Vector, double>>, 4> corner_stops;

    for (int c = 0; c < 4; ++c) {
        std::vector<AdjItem> list = adj[c];
        if (c == 3 && list.size() >= 2) {
            for (AdjItem& item : list)
                if (item.angle < 0.5 * Tolerance::PI)
                    item.angle += Tolerance::TWO_PI;
            std::sort(list.begin(), list.end(), [](const AdjItem& a, const AdjItem& b) {
                return a.angle < b.angle;
            });
        }
        const size_t m = list.size();
        if (m < 2)
            continue;

        std::vector<double> hs(m, 0.5 * rib_width);
        std::vector<Vector> us(m);
        for (size_t j = 0; j < m; ++j) {
            hs[j] = 0.5 * rib_width * edges[list[j].edge_idx].width_scale;
            us[j] = Vector(-list[j].dir_xy[1], list[j].dir_xy[0], 0.0);
        }

        // Componentwise minimal stop distances r[j] so no two ribs collide and every rib's
        // full width [-hs[j], +hs[j]] bears flush on its perpendicular column-head facet.
        std::vector<double> r(m, 0.0);
        r.front() = std::max(r.front(), hs.back());
        r.back() = std::max(r.back(), hs.front());
        for (size_t j = 0; j + 1 < m; ++j) {
            const double dphi = list[j + 1].angle - list[j].angle;
            if (dphi > 1e-4 && dphi < Tolerance::PI - 1e-4) {
                const double cd = std::cos(dphi);
                const double sd = std::sin(dphi);
                r[j] = std::max(r[j], (hs[j + 1] + hs[j] * cd) / sd);
                r[j + 1] = std::max(r[j + 1], (hs[j] + hs[j + 1] * cd) / sd);
            }
        }
        for (int iter = 0; iter < 800; ++iter) {
            bool changed = false;
            for (size_t j = 0; j + 1 < m; ++j) {
                const double dphi = list[j + 1].angle - list[j].angle;
                if (dphi <= 1e-4 || dphi >= Tolerance::PI - 1e-4)
                    continue;
                const double cd = std::cos(dphi);
                const double sd = std::sin(dphi);
                const double req_next = r[j] * cd + hs[j] * sd;
                const double req_curr = r[j + 1] * cd + hs[j + 1] * sd;
                if (req_next > r[j + 1] + 1e-12) {
                    r[j + 1] = req_next;
                    changed = true;
                }
                if (req_curr > r[j] + 1e-12) {
                    r[j] = req_curr;
                    changed = true;
                }
            }
            if (!changed)
                break;
        }

        for (size_t j = 0; j < m; ++j)
            corner_stops[c].emplace_back(list[j].dir_xy, r[j]);

        const Point& cp = nodes[c];
        std::vector<Point> raw_poly;
        raw_poly.push_back(cp - list.front().dir_xy * hs.back() - us.front() * hs.front());
        raw_poly.push_back(cp + list.front().dir_xy * r.front() - us.front() * hs.front());
        for (size_t j = 0; j + 1 < m; ++j) {
            const double dphi = std::max(1e-4, list[j + 1].angle - list[j].angle);
            const double tj = (r[j + 1] - r[j] * std::cos(dphi)) / std::sin(dphi);
            raw_poly.push_back(cp + list[j].dir_xy * r[j] + us[j] * tj);
        }
        raw_poly.push_back(cp + list.back().dir_xy * r.back() + us.back() * hs.back());

        // Split any inner facet edge crossing the groin line |x|/hx == |y|/hy so top fan
        // triangles from raw_poly[0] stay inside one barrel cylinder quadrant beneath vault_panel.
        const auto groin_val = [&](const Point& p) -> double {
            return std::abs(p[0]) / std::max(1.0, hx) - std::abs(p[1]) / std::max(1.0, hy);
        };
        std::vector<Point> poly_2d;
        for (size_t i = 0; i < raw_poly.size(); ++i) {
            const Point& a = raw_poly[i];
            const Point& b = raw_poly[(i + 1) % raw_poly.size()];
            if (poly_2d.empty() || poly_2d.back().distance(a) > 1e-4)
                poly_2d.push_back(a);
            if (i >= 1 && i + 1 < raw_poly.size()) {
                const double ga = groin_val(a);
                const double gb = groin_val(b);
                if ((ga < -1e-6 && gb > 1e-6) || (ga > 1e-6 && gb < -1e-6)) {
                    const double t = ga / (ga - gb);
                    const Point mid = a + (b - a) * t;
                    if (mid.distance(a) > 1e-3 && mid.distance(b) > 1e-3)
                        poly_2d.push_back(mid);
                }
            }
        }

        double min_rib_bot_z = -rib_height;
        for (size_t j = 0; j < m; ++j) {
            const Point p_stop = cp + list[j].dir_xy * r[j];
            const Point p_top = eval_nurbs_cyl(p_stop[0], p_stop[1]);
            const Point p_in = eval_nurbs_cyl(
                cp[0] + list[j].dir_xy[0] * (r[j] + 2.0),
                cp[1] + list[j].dir_xy[1] * (r[j] + 2.0)
            );
            const double dz_ds = std::max(0.0, 0.5 * (p_in[2] - p_top[2]));
            const double z_bot = p_top[2] - rib_height * std::hypot(1.0, dz_ds);
            min_rib_bot_z = std::min(min_rib_bot_z, z_bot - 10.0);
        }
        const double z_base = min_rib_bot_z;
        std::vector<Point> bot_loop;
        std::vector<Point> top_loop;
        bot_loop.reserve(poly_2d.size());
        top_loop.reserve(poly_2d.size());
        for (const Point& p2 : poly_2d) {
            const Point p_surf = eval_nurbs_cyl(p2[0], p2[1]);
            top_loop.emplace_back(p2[0], p2[1], p_surf[2]);
            bot_loop.emplace_back(p2[0], p2[1], z_base);
        }

        std::vector<std::vector<Point>> head_faces;
        head_faces.emplace_back(bot_loop.rbegin(), bot_loop.rend());
        for (size_t i = 1; i + 1 < top_loop.size(); ++i)
            head_faces.push_back({top_loop[0], top_loop[i], top_loop[i + 1]});
        for (size_t i = 0; i < poly_2d.size(); ++i) {
            const size_t next = (i + 1) % poly_2d.size();
            head_faces.push_back({bot_loop[i], bot_loop[next], top_loop[next], top_loop[i]});
        }
        result.push_back(
            std::make_shared<wood_session::Block>(Mesh::from_polylines(head_faces, 1e-6), "support_block")
        );
    }

    // Structural rib hierarchy & single-plane T-section / miter cuts
    const auto path_tier = [&](size_t p_idx) -> int {
        const RibPathSpec& p = rib_paths[p_idx];
        if (p.tier_override >= 0)
            return p.tier_override;
        if (p.role == "wall_arch")
            return 0;
        if (p.role == "diagonal_rib") {
            const Point& u_pt = nodes[p.nodes.front()];
            const Point& v_pt = nodes[p.nodes.back()];
            const Point mid = u_pt + (v_pt - u_pt) * 0.5;
            return (mid[0] * mid[1] >= 0.0 && p.nodes.size() >= 3 && p.nodes.front() == 0 && p.nodes.back() == 2)
                       ? 0
                       : 1;
        }
        if (p.role == "fan_rib")
            return 1;
        if (p.role == "ridge_rib")
            return 2;
        if (p.role == "tierceron_rib" || p.role == "net_rib")
            return 3;
        return 4;
    };

    const auto edge_tier = [&](size_t e_idx) -> int {
        if (edges[e_idx].tier_override >= 0)
            return edges[e_idx].tier_override;
        return path_tier(edges[e_idx].path_idx);
    };

    const auto slope_pullback = [&](const Point& p_ref, const Vector& dir_out) -> double {
        const Point p0 = eval_nurbs_cyl(p_ref[0], p_ref[1]);
        const Point p1 = eval_nurbs_cyl(p_ref[0] + dir_out[0] * 2.0, p_ref[1] + dir_out[1] * 2.0);
        const double dz_ds = 0.5 * (p1[2] - p0[2]);
        if (dz_ds <= 0.0)
            return 0.0;
        const double sin_a = dz_ds / std::hypot(1.0, dz_ds);
        return rib_height * sin_a;
    };

    const auto endpoint_cuts = [&](size_t p_idx, int node_id, const Vector& dir_out, double len_ref, double& trim_offset) -> std::vector<Plane> {
        const Point& vp = nodes[node_id];
        const std::vector<AdjItem>& list = adj[node_id];
        const double hw = 0.5 * rib_width * rib_paths[p_idx].width_scale;

        if (node_id >= 0 && node_id < 4) {
            double r_stop = hw;
            double best = 0.95;
            for (const auto& [d_cand, r_cand] : corner_stops[node_id]) {
                const double align = d_cand.dot(dir_out);
                if (align > best) {
                    best = align;
                    r_stop = r_cand;
                }
            }
            const Point stop_pt = vp + dir_out * r_stop;
            trim_offset = r_stop - slope_pullback(stop_pt, dir_out) - 25.0;
            return {Plane::from_point_normal(stop_pt, dir_out)};
        }

        int pos = -1;
        double best_align = 0.95;
        for (size_t j = 0; j < list.size(); ++j) {
            const double align = list[j].dir_xy.dot(dir_out);
            if (align > best_align) {
                best_align = align;
                pos = static_cast<int>(j);
            }
        }
        if (pos < 0)
            return {};

        const AdjItem& curr = list[pos];
        const size_t m = list.size();
        if (m == 1) {
            trim_offset = -25.0;
            return {Plane::from_point_normal(vp, curr.dir_xy)};
        }

        int through_a = -1;
        int through_b = -1;
        int best_max_tier = 999;
        int best_sum_tier = 999;
        double best_dot = 1.0;
        for (size_t a = 0; a < m; ++a) {
            for (size_t b = a + 1; b < m; ++b) {
                const double d = list[a].dir_xy.dot(list[b].dir_xy);
                if (d < -0.985) {
                    const int ta = edge_tier(list[a].edge_idx);
                    const int tb = edge_tier(list[b].edge_idx);
                    const int max_t = std::max(ta, tb);
                    const int sum_t = ta + tb;
                    if (max_t < best_max_tier ||
                        (max_t == best_max_tier && sum_t < best_sum_tier) ||
                        (max_t == best_max_tier && sum_t == best_sum_tier && d < best_dot)) {
                        best_max_tier = max_t;
                        best_sum_tier = sum_t;
                        best_dot = d;
                        through_a = static_cast<int>(a);
                        through_b = static_cast<int>(b);
                    }
                }
            }
        }

        if (pos == through_a || pos == through_b) {
            const AdjItem& other = (pos == through_a) ? list[through_b] : list[through_a];
            Vector n_join = curr.dir_xy - other.dir_xy;
            if (n_join.magnitude() < 1e-6)
                n_join = curr.dir_xy;
            else
                n_join.normalize_self();
            trim_offset = -slope_pullback(vp, dir_out) - 25.0;
            return {Plane::from_point_normal(vp, n_join)};
        }

        // Exact cyclic wedge miter / through-rib T-butt against immediate CW (prev) and CCW (next) neighbors
        const size_t prev_idx = (static_cast<size_t>(pos) + m - 1) % m;
        const size_t next_idx = (static_cast<size_t>(pos) + 1) % m;
        const AdjItem& prev_item = list[prev_idx];
        const AdjItem& next_item = list[next_idx];
        const double theta_prev = wrap_two_pi(curr.angle - prev_item.angle);
        const double theta_next = wrap_two_pi(next_item.angle - curr.angle);

        std::vector<Plane> planes;
        if (theta_prev > 0.02 && theta_prev < Tolerance::PI - 0.02) {
            const bool prev_is_through =
                (static_cast<int>(prev_idx) == through_a || static_cast<int>(prev_idx) == through_b);
            if (prev_is_through) {
                const Vector n_prev(-prev_item.dir_xy[1], prev_item.dir_xy[0], 0.0);
                const double hw_prev = 0.5 * rib_width * edges[prev_item.edge_idx].width_scale;
                planes.push_back(Plane::from_point_normal(vp + n_prev * hw_prev, n_prev));
            } else {
                Vector b_prev = curr.dir_xy + prev_item.dir_xy;
                if (b_prev.magnitude() > 1e-6) {
                    b_prev.normalize_self();
                    const Vector n_bis(-b_prev[1], b_prev[0], 0.0);
                    planes.push_back(Plane::from_point_normal(vp, n_bis));
                }
            }
        }

        if (theta_next > 0.02 && theta_next < Tolerance::PI - 0.02) {
            const bool next_is_through =
                (static_cast<int>(next_idx) == through_a || static_cast<int>(next_idx) == through_b);
            if (next_is_through) {
                const Vector n_next(next_item.dir_xy[1], -next_item.dir_xy[0], 0.0);
                const double hw_next = 0.5 * rib_width * edges[next_item.edge_idx].width_scale;
                const Plane cand = Plane::from_point_normal(vp + n_next * hw_next, n_next);
                if (planes.empty() || planes.back().z_axis().dot(cand.z_axis()) < 0.9999)
                    planes.push_back(cand);
            } else {
                Vector b_next = curr.dir_xy + next_item.dir_xy;
                if (b_next.magnitude() > 1e-6) {
                    b_next.normalize_self();
                    const Vector n_bis(b_next[1], -b_next[0], 0.0);
                    const Plane cand = Plane::from_point_normal(vp, n_bis);
                    if (planes.empty() || planes.back().z_axis().dot(cand.z_axis()) < 0.9999)
                        planes.push_back(cand);
                }
            }
        }

        if (planes.empty()) {
            trim_offset = -25.0;
            return {Plane::from_point_normal(vp, curr.dir_xy)};
        }

        const Vector u_perp(-curr.dir_xy[1], curr.dir_xy[0], 0.0);
        double s_left = -1e9;
        double s_right = -1e9;
        for (const Plane& pl : planes) {
            const double denom = std::max(0.05, curr.dir_xy.dot(pl.z_axis()));
            const double base_num = (pl.origin() - vp).dot(pl.z_axis());
            const double off = hw * u_perp.dot(pl.z_axis());
            s_left = std::max(s_left, (base_num - off) / denom);
            s_right = std::max(s_right, (base_num + off) / denom);
        }
        const double min_s = std::min(s_left, s_right);
        const Point cut_ref = vp + dir_out * std::max(0.0, min_s);
        trim_offset = std::max(-0.45 * len_ref, min_s - slope_pullback(cut_ref, dir_out) - 20.0);
        return planes;
    };

    // Generate planar single-curved BeamCurved ribs strictly in their vertical planes
    for (size_t p_idx = 0; p_idx < rib_paths.size(); ++p_idx) {
        const RibPathSpec& path = rib_paths[p_idx];
        const int u = path.nodes.front();
        const int v = path.nodes.back();
        const Point& p_u = nodes[u];
        const Point& p_v = nodes[v];
        const double dx = p_v[0] - p_u[0];
        const double dy = p_v[1] - p_u[1];
        const double len_2d = std::hypot(dx, dy);
        if (len_2d < 1e-3)
            continue;
        const Vector dir_2d(dx / len_2d, dy / len_2d, 0.0);

        double offset_u = 0.0;
        double offset_v = 0.0;
        std::vector<Plane> cuts_u = endpoint_cuts(p_idx, u, dir_2d, len_2d, offset_u);
        std::vector<Plane> cuts_v = endpoint_cuts(p_idx, v, dir_2d * -1.0, len_2d, offset_v);

        const double s_start = offset_u;
        const double s_end   = len_2d - offset_v;
        if (s_end <= s_start + 10.0)
            continue;

        const double mid_x = 0.5 * (p_u[0] + p_v[0]);
        const double mid_y = 0.5 * (p_u[1] + p_v[1]);
        const NurbsSurface& rib_surf =
            (std::abs(mid_x) / std::max(1.0, hx) <= std::abs(mid_y) / std::max(1.0, hy))
                ? surf_ns
                : surf_ew;

        const int n_rib_sub = std::max(edge_sub, static_cast<int>(path.nodes.size()) * 8);
        std::vector<Point> surf_pts(n_rib_sub + 1);
        for (int k = 0; k <= n_rib_sub; ++k) {
            const double s = s_start + (s_end - s_start) * (static_cast<double>(k) / n_rib_sub);
            const Point plan_pt = p_u + dir_2d * s;
            surf_pts[k] = eval_surf_smooth(rib_surf, plan_pt[0], plan_pt[1]);
        }

        // Keep up strictly in the vertical plane of the rib so every rib has zero sideways twist
        // and its top centerline, axis centerline, and bottom centerline are straight in XY plan!
        std::vector<Point> axis(n_rib_sub + 1);
        std::vector<Vector> ups(n_rib_sub + 1);
        for (int k = 0; k <= n_rib_sub; ++k) {
            const Vector tang =
                (k == 0) ? (surf_pts[1] - surf_pts[0]).normalized()
                : (k == n_rib_sub) ? (surf_pts[n_rib_sub] - surf_pts[n_rib_sub - 1]).normalized()
                                   : (surf_pts[k + 1] - surf_pts[k - 1]).normalized();
            Vector up = Vector(0.0, 0.0, 1.0) - tang * tang[2];
            if (up.magnitude() < 1e-6)
                up = Vector(0.0, 0.0, 1.0);
            else
                up.normalize_self();
            ups[k] = up;
            axis[k] = surf_pts[k] - up * (0.5 * rib_height);
        }

        auto beam = std::make_shared<wood_session::BeamCurved>(
            NurbsCurve::create_interpolated(
                axis, CurveNurbsKnotStyle::Uniform, CurveInterpStyle::Occt
            ),
            rib_width * path.width_scale,
            rib_height,
            ups,
            1,
            6,
            n_rib_sub,
            path.role
        );
        beam->cuts = std::move(cuts_u);
        beam->cuts.insert(beam->cuts.end(), cuts_v.begin(), cuts_v.end());
        result.push_back(beam);
    }

    // Generate K-gon trimmed NURBS BRep infill panels in each minimal interior cell
    std::vector<std::array<bool, 2>> visited_he(edges.size(), {false, false});

    for (size_t start_e = 0; start_e < edges.size(); ++start_e) {
        for (int start_dir = 0; start_dir < 2; ++start_dir) {
            if (visited_he[start_e][start_dir])
                continue;

            std::vector<int> cycle_nodes;
            size_t curr_e = start_e;
            int curr_dir = start_dir;
            bool valid_cycle = true;

            for (int step = 0; step < 16; ++step) {
                visited_he[curr_e][curr_dir] = true;
                const int u = (curr_dir == 0) ? edges[curr_e].u : edges[curr_e].v;
                const int v = (curr_dir == 0) ? edges[curr_e].v : edges[curr_e].u;
                cycle_nodes.push_back(u);

                const std::vector<AdjItem>& list = adj[v];
                int pos = -1;
                for (size_t j = 0; j < list.size(); ++j)
                    if (list[j].edge_idx == curr_e) {
                        pos = static_cast<int>(j);
                        break;
                    }
                if (pos < 0 || list.size() < 2) {
                    valid_cycle = false;
                    break;
                }

                const size_t next_slot = (static_cast<size_t>(pos) + list.size() - 1) % list.size();
                curr_e = list[next_slot].edge_idx;
                curr_dir = (edges[curr_e].u == v) ? 0 : 1;

                if (curr_e == start_e && curr_dir == start_dir)
                    break;
            }

            if (!valid_cycle || cycle_nodes.size() < 3 || cycle_nodes.size() > 6)
                continue;

            const size_t k_cnt = cycle_nodes.size();
            double area2 = 0.0;
            double sum_x = 0.0;
            double sum_y = 0.0;
            for (size_t i = 0; i < k_cnt; ++i) {
                const Point& p0 = nodes[cycle_nodes[i]];
                const Point& p1 = nodes[cycle_nodes[(i + 1) % k_cnt]];
                area2 += p0[0] * p1[1] - p1[0] * p0[1];
                sum_x += p0[0];
                sum_y += p0[1];
            }
            if (area2 <= 1e4)
                continue;
            const Point cell_ref(
                sum_x / static_cast<double>(k_cnt), sum_y / static_cast<double>(k_cnt), 0.0
            );

            // All panel vertices follow the straight 2D beam centerlines right into nodes[v]
            std::vector<Point> panel_poly;
            panel_poly.reserve(k_cnt);
            for (size_t i = 0; i < k_cnt; ++i)
                panel_poly.push_back(nodes[cycle_nodes[i]]);

            std::vector<Point> clean_poly;
            for (size_t i = 0; i < panel_poly.size(); ++i) {
                const Point& p_curr = panel_poly[i];
                const Point& p_next = panel_poly[(i + 1) % panel_poly.size()];
                if (p_curr.distance(p_next) > 5.0)
                    clean_poly.push_back(p_curr);
            }
            if (clean_poly.size() < 3)
                continue;

            const double ux = std::abs(cell_ref[0]) / std::max(1.0, hx);
            const double uy = std::abs(cell_ref[1]) / std::max(1.0, hy);
            const NurbsSurface& web_surf = (ux <= uy + 1e-6) ? surf_ns : surf_ew;

            result.push_back(
                to_vault_panel(web_surf, hx, hy, clean_poly, panel_thickness, edge_sub, panel_name)
            );
        }
    }

    return result;
}

} // namespace

std::vector<std::shared_ptr<Element>>
star_vault(double span, double rise, double thickness, double star, double rib, int subdivisions) {
    const double half = 0.5 * span;
    const int n_sub = std::max(8, subdivisions);
    const double s_clamped = std::clamp(star, 0.25, 0.85);

    std::vector<Point> nodes = {
        Point(-half, -half, 0.0),           // 0: SW corner
        Point( half, -half, 0.0),           // 1: SE corner
        Point( half,  half, 0.0),           // 2: NE corner
        Point(-half,  half, 0.0),           // 3: NW corner
        Point(0.0, -s_clamped * half, 0.0), // 4: S star boss
        Point( s_clamped * half, 0.0, 0.0), // 5: E star boss
        Point(0.0,  s_clamped * half, 0.0), // 6: N star boss
        Point(-s_clamped * half, 0.0, 0.0), // 7: W star boss
        Point(0.0, 0.0, 0.0),               // 8: Central crown boss
        Point(0.0, -half, 0.0),             // 9: S wall crown
        Point( half, 0.0, 0.0),             // 10: E wall crown
        Point(0.0,  half, 0.0),             // 11: N wall crown
        Point(-half, 0.0, 0.0)              // 12: W wall crown
    };

    std::vector<RibEdgeSpec> edges;
    std::vector<RibPathSpec> rib_paths;
    const auto add_rib = [&](const std::vector<int>& chain, int tier) {
        const size_t p_idx = rib_paths.size();
        rib_paths.push_back({chain, 1.0, "rib", tier});
        for (size_t k = 0; k + 1 < chain.size(); ++k) {
            const int u = chain[k];
            const int v = chain[k + 1];
            edges.push_back({u, v, 1.0, "rib", tier, p_idx});
        }
    };

    // 4 perimeter wall arches, 3 diagonal ribs (1 continuous SW-NE + 2 half-diagonals), 4 full ridge liernes, 8 tiercerons = 19 ribs
    add_rib({0, 8, 2}, 0);
    add_rib({1, 8}, 1);
    add_rib({3, 8}, 1);
    for (int i = 0; i < 4; ++i) {
        const int next = (i + 1) % 4;
        add_rib({i, 9 + i, next}, 0);
        add_rib({9 + i, 4 + i, 8}, 2);
        add_rib({i, 4 + i}, 3);
        add_rib({next, 4 + i}, 3);
    }

    return build_cylindrical_ribbed_vault_from_graph(
        half,
        half,
        rise,
        rib,
        rib,
        thickness,
        n_sub,
        ArchMethod::groin_cylinders,
        nodes,
        edges,
        rib_paths,
        "web"
    );
}

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
    const double panel_thickness = 35.0;

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
    std::vector<RibPathSpec> rib_paths;
    const auto add_rib_path = [&](const std::vector<int>& chain, double w_scale, const std::string& role) {
        if (chain.size() < 2)
            return;
        const size_t p_idx = rib_paths.size();
        rib_paths.push_back({chain, w_scale, role, -1});
        for (size_t k = 0; k + 1 < chain.size(); ++k) {
            const int u = chain[k];
            const int v = chain[k + 1];
            if (u == v)
                continue;
            bool exists = false;
            for (const RibEdgeSpec& e : edges)
                if ((e.u == u && e.v == v) || (e.u == v && e.v == u)) {
                    exists = true;
                    break;
                }
            if (!exists)
                edges.push_back({u, v, w_scale, role, -1, p_idx});
        }
    };

    if (pattern != GothicPattern::fan_conoid) {
        for (int i = 0; i < 4; ++i) {
            const int c_curr = i;
            const int c_next = (i + 1) % 4;
            const int m_wall = 4 + i;
            add_rib_path({c_curr, m_wall, c_next}, 1.0, "wall_arch");
        }
    }

    if (pattern == GothicPattern::quadripartite) {
        // Bern Cathedral / Wells classic quadripartite cross vault
        add_rib_path({0, 8, 2}, 1.0, "diagonal_rib");
        add_rib_path({1, 8}, 1.0, "diagonal_rib");
        add_rib_path({3, 8}, 1.0, "diagonal_rib");
        for (int i = 0; i < 4; ++i)
            add_rib_path({4 + i, 8}, 1.0, "ridge_rib");
    } else if (pattern == GothicPattern::tierceron) {
        // Bern Cathedral / Exeter tierceron star vault
        const std::array<int, 4> r_nodes = {
            add_node(0.0, -0.5 * hy),
            add_node(0.5 * hx, 0.0),
            add_node(0.0, 0.5 * hy),
            add_node(-0.5 * hx, 0.0)
        };
        add_rib_path({0, 8, 2}, 1.0, "diagonal_rib");
        add_rib_path({1, 8}, 1.0, "diagonal_rib");
        add_rib_path({3, 8}, 1.0, "diagonal_rib");
        for (int i = 0; i < 4; ++i) {
            const int c_next = (i + 1) % 4;
            add_rib_path({4 + i, r_nodes[i], 8}, 1.0, "ridge_rib");
            add_rib_path({i, r_nodes[i]}, 1.0, "tierceron_rib");
            add_rib_path({c_next, r_nodes[i]}, 1.0, "tierceron_rib");
        }
    } else if (pattern == GothicPattern::star_stellar) {
        // Bern Cathedral 4-lozenge stellar star vault (4 central diamond lozenges meeting at crown 8)
        const double s = 0.62;
        const double d = 0.36;
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
        add_rib_path({0, d_nodes[0], 8, d_nodes[2], 2}, 1.0, "diagonal_rib");
        add_rib_path({1, d_nodes[1], 8}, 1.0, "diagonal_rib");
        add_rib_path({3, d_nodes[3], 8}, 1.0, "diagonal_rib");
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            add_rib_path({4 + i, s_nodes[i]}, 1.0, "ridge_rib");
            add_rib_path({i, s_nodes[i]}, 1.0, "tierceron_rib");
            add_rib_path({next, s_nodes[i]}, 1.0, "tierceron_rib");
            add_rib_path({d_nodes[i], s_nodes[i]}, 1.0, "lierne_rib");
            add_rib_path({d_nodes[i], s_nodes[prev]}, 1.0, "lierne_rib");
        }
    } else if (pattern == GothicPattern::lierne_star) {
        // Bern Cathedral / Ely Choir double-star lierne vault
        const double r_out = 0.68;
        const double r_in  = 0.34;
        const double d_mid = 0.48;
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
        add_rib_path({0, dn[0], 8, dn[2], 2}, 1.0, "diagonal_rib");
        add_rib_path({1, dn[1], 8}, 1.0, "diagonal_rib");
        add_rib_path({3, dn[3], 8}, 1.0, "diagonal_rib");
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            add_rib_path({4 + i, ro[i], ri[i], 8}, 1.0, "ridge_rib");
            add_rib_path({i, ro[i]}, 1.0, "tierceron_rib");
            add_rib_path({next, ro[i]}, 1.0, "tierceron_rib");
            add_rib_path({dn[i], ri[i]}, 1.0, "lierne_rib");
            add_rib_path({dn[i], ri[prev]}, 1.0, "lierne_rib");
            add_rib_path({dn[i], ro[i]}, 1.0, "lierne_rib");
            add_rib_path({dn[i], ro[prev]}, 1.0, "lierne_rib");
        }
    } else if (pattern == GothicPattern::net_reticulated) {
        // Bern Cathedral Choir / Parler reticulated diamond-lattice net vault (all cells within single cylinder quadrants)
        const std::array<int, 4> q_nodes = {
            add_node(0.0, -0.62 * hy),
            add_node(0.62 * hx, 0.0),
            add_node(0.0, 0.62 * hy),
            add_node(-0.62 * hx, 0.0)
        };
        const std::array<int, 4> r_inner = {
            add_node(0.0, -0.30 * hy),
            add_node(0.30 * hx, 0.0),
            add_node(0.0, 0.30 * hy),
            add_node(-0.30 * hx, 0.0)
        };
        const std::array<int, 4> d_outer = {
            add_node(-0.52 * hx, -0.52 * hy),
            add_node( 0.52 * hx, -0.52 * hy),
            add_node( 0.52 * hx,  0.52 * hy),
            add_node(-0.52 * hx,  0.52 * hy)
        };
        const std::array<int, 4> d_inner = {
            add_node(-0.24 * hx, -0.24 * hy),
            add_node( 0.24 * hx, -0.24 * hy),
            add_node( 0.24 * hx,  0.24 * hy),
            add_node(-0.24 * hx,  0.24 * hy)
        };
        add_rib_path(
            {0, d_outer[0], d_inner[0], 8, d_inner[2], d_outer[2], 2}, 1.0, "diagonal_rib"
        );
        add_rib_path({1, d_outer[1], d_inner[1], 8}, 1.0, "diagonal_rib");
        add_rib_path({3, d_outer[3], d_inner[3], 8}, 1.0, "diagonal_rib");
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            add_rib_path({4 + i, q_nodes[i]}, 1.0, "ridge_rib");
            add_rib_path({i, q_nodes[i]}, 1.0, "tierceron_rib");
            add_rib_path({next, q_nodes[i]}, 1.0, "tierceron_rib");
            add_rib_path({d_outer[i], q_nodes[i]}, 1.0, "net_rib");
            add_rib_path({d_outer[i], q_nodes[prev]}, 1.0, "net_rib");
            add_rib_path({d_outer[i], r_inner[i]}, 1.0, "net_rib");
            add_rib_path({d_outer[i], r_inner[prev]}, 1.0, "net_rib");
            add_rib_path({d_inner[i], r_inner[i]}, 1.0, "net_rib");
            add_rib_path({d_inner[i], r_inner[prev]}, 1.0, "net_rib");
        }
    } else if (pattern == GothicPattern::fan_conoid) {
        // Polygonal fan vault on intersecting circular barrel cylinders (straight radiating fan ribs + straight polygonal collar ribs)
        const double r_col = 0.68 * std::min(hx, hy);
        const std::array<int, 4> r_out = {
            add_node(0.0, -0.66 * hy),
            add_node(0.66 * hx, 0.0),
            add_node(0.0, 0.66 * hy),
            add_node(-0.66 * hx, 0.0)
        };
        const std::array<int, 4> r_in = {
            add_node(0.0, -0.33 * hy),
            add_node(0.33 * hx, 0.0),
            add_node(0.0, 0.33 * hy),
            add_node(-0.33 * hx, 0.0)
        };
        const double d_diag = r_col / std::sqrt(2.0);
        const std::array<int, 4> d_col = {
            add_node(-hx + d_diag, -hy + d_diag),
            add_node( hx - d_diag, -hy + d_diag),
            add_node( hx - d_diag,  hy - d_diag),
            add_node(-hx + d_diag,  hy - d_diag)
        };

        // Collar nodes along each half-wall and fan ray at radial distance r_col from each corner
        std::array<std::array<int, 2>, 4> w_col{};
        std::array<std::array<int, 2>, 4> f_out_col{};
        std::array<std::array<int, 2>, 4> f_in_col{};
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const std::array<int, 2> corners_q = {i, next};
            for (int s = 0; s < 2; ++s) {
                const Point cp = nodes[corners_q[s]];
                const Point pw = cp + (nodes[4 + i] - cp).normalized() * r_col;
                const Point po = cp + (nodes[r_out[i]] - cp).normalized() * r_col;
                const Point pi = cp + (nodes[r_in[i]] - cp).normalized() * r_col;
                w_col[i][s]     = add_node(pw[0], pw[1]);
                f_out_col[i][s] = add_node(po[0], po[1]);
                f_in_col[i][s]  = add_node(pi[0], pi[1]);
            }
        }

        // 4 unbroken perimeter wall arches
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            add_rib_path({i, w_col[i][0], 4 + i, w_col[i][1], next}, 1.0, "wall_arch");
        }
        // Unbroken primary SW-NE diagonal and secondary SE-NW half-diagonals
        add_rib_path({0, d_col[0], 8, d_col[2], 2}, 1.0, "diagonal_rib");
        add_rib_path({1, d_col[1], 8}, 1.0, "diagonal_rib");
        add_rib_path({3, d_col[3], 8}, 1.0, "diagonal_rib");

        // Unbroken ridge ribs, radiating fan ribs from corners to ridge, and straight horizontal collar ribs
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const std::array<int, 2> corners_q = {i, next};
            add_rib_path({4 + i, r_out[i], r_in[i], 8}, 1.0, "ridge_rib");
            for (int s = 0; s < 2; ++s) {
                const int c = corners_q[s];
                add_rib_path({c, f_out_col[i][s], r_out[i]}, 1.0, "fan_rib");
                add_rib_path({c, f_in_col[i][s],  r_in[i]},  1.0, "fan_rib");
                add_rib_path({w_col[i][s], f_out_col[i][s]}, 1.0, "collar_rib");
                add_rib_path({f_out_col[i][s], f_in_col[i][s]}, 1.0, "collar_rib");
                add_rib_path({f_in_col[i][s], d_col[c]}, 1.0, "collar_rib");
            }
        }
    }

    return build_cylindrical_ribbed_vault_from_graph(
        hx,
        hy,
        rise,
        rib_width,
        rib_height,
        panel_thickness,
        n_sub,
        surface_method,
        nodes,
        edges,
        rib_paths,
        "vault_panel"
    );
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
    const std::shared_ptr<TreeNode> panels_group = add_group("panels");
    for (const std::shared_ptr<Element>& elem : gothic_ribbed_vault(
             span_x, span_y, rise, rib_width, rib_height, pattern, arch_method, subdivisions
         )) {
        if (elem->name == "support_block")
            add(elem, supports_group);
        else if (elem->name == "vault_panel")
            add(elem, panels_group);
        else
            add(elem, ribs_group);
    }
}

}
