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
std::shared_ptr<wood_session::NodeBlockElement>
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

    return std::make_shared<wood_session::NodeBlockElement>(
        std::vector<Polyline>{Polyline(intrados).closed(), Polyline(extrados).closed()}, name
    );
}

/// A stone lofted from an intrados loop to an extrados loop of paired corners.
std::shared_ptr<wood_session::NodeBlockElement>
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

    return std::make_shared<wood_session::NodeBlockElement>(Mesh::from_polylines(kept, 1e-6), name);
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

std::vector<std::shared_ptr<Element>> arch(const ArchGuide& guide) {

    const Arc arc = compute_arc(guide.span, guide.rise);
    const double step = 2.0 * arc.sector / guide.voussoirs;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int k = 0; k < guide.voussoirs; k++)
        blocks.push_back(to_ring(
            arc, guide.thickness, -arc.sector + k * step, -arc.sector + (k + 1) * step, 0.0, guide.depth
        ));

    return blocks;
}

std::vector<std::shared_ptr<Element>> barrel(const BarrelGuide& guide) {

    const Arc arc = compute_arc(guide.span, guide.rise);
    const double step = 2.0 * arc.sector / guide.courses;
    const double ring = guide.length / guide.rings;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int k = 0; k < guide.courses; k++) {
        std::vector<std::pair<double, double>> pieces =
            compute_pieces(guide.length, ring, k % 2 == 1 ? guide.stagger * ring : 0.0);
        if (guide.closed) {
            pieces.insert(pieces.begin(), {-ring, 0.0});
            pieces.emplace_back(guide.length, guide.length + ring);
        }

        for (const std::pair<double, double>& piece : pieces)
            blocks.push_back(to_ring(
                arc,
                guide.thickness,
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

std::vector<std::shared_ptr<Element>> dome(const DomeGuide& guide) {

    const double first = guide.oculus * Tolerance::TO_RADIANS;
    const double last = guide.springing * Tolerance::TO_RADIANS;
    const double drop = guide.radius * std::cos(last);
    const double around = Tolerance::TWO_PI / guide.meridians;
    const double down = (last - first) / guide.hoops;

    std::vector<std::shared_ptr<Element>> blocks;
    for (int i = 0; i < guide.hoops; i++) {
        const double upper = first + i * down;
        const double lower = upper + down;
        const double outer_upper = guide.radius + guide.top + (guide.bottom - guide.top) * (upper - first) / (last - first);
        const double outer_lower = guide.radius + guide.top + (guide.bottom - guide.top) * (lower - first) / (last - first);
        const double shift = i % 2 == 1 ? around / 2.0 : 0.0;

        // six corners: the hoops above and below joint at the middle of this voussoir, so its edges bend there and every face is shared
        for (int j = 0; j < guide.meridians; j++) {
            const double left = j * around + shift;
            const std::vector<double> phis = {left, left + around / 2.0, left + around};
            std::vector<Point> intrados;
            std::vector<Point> extrados;
            for (const double phi : phis) {
                intrados.push_back(compute_sphere(guide.radius, upper, phi, drop));
                extrados.push_back(compute_sphere(outer_upper, upper, phi, drop));
            }
            for (auto phi = phis.rbegin(); phi != phis.rend(); ++phi) {
                intrados.push_back(compute_sphere(guide.radius, lower, *phi, drop));
                extrados.push_back(compute_sphere(outer_lower, lower, *phi, drop));
            }
            blocks.push_back(to_voussoir(intrados, extrados, "voussoir"));
        }
    }

    return blocks;
}

std::vector<std::shared_ptr<Element>> wall(const WallGuide& guide) {

    std::vector<std::shared_ptr<Element>> blocks;
    const std::vector<std::pair<double, double>> rows = compute_pieces(guide.height, guide.course, 0.0);
    for (size_t row = 0; row < rows.size(); row++) {
        for (const std::pair<double, double>& piece :
             compute_pieces(guide.length, guide.brick, row % 2 == 1 ? guide.stagger * guide.brick : 0.0)) {
            const double bottom = rows[row].first;
            const std::vector<Point> base = {
                Point(piece.first, 0.0, bottom),
                Point(piece.second, 0.0, bottom),
                Point(piece.second, guide.thickness, bottom),
                Point(piece.first, guide.thickness, bottom)
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

namespace {

/// Local arch coordinate frame for placing intrados and extrados corners on a square-bay web.
struct WebFrame {
    Arc arc; // Circular arch profile of the bay.
    Vector outward; // Outward unit direction of the web side.
    Vector across; // Cross-bay unit direction perpendicular to outward.
    bool groin = false; // Whether the barrel runs outward (groin) or across (cloister).

    /// Evaluate a 3D point on the web cylinder at radius, angle, and station.
    Point place(double radius, double angle, double station) const {

        const double section = radius * std::sin(angle);
        const double height = arc.centre + radius * std::cos(angle);
        const Vector plan =
            groin ? outward * station + across * section : outward * section + across * station;

        return Point(plan[0], plan[1], height);
    }
};

} // namespace

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
    const WebFrame frame{arc, outward, across, groin};

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
                frame.place(arc.radius, a, near),
                frame.place(arc.radius, b, near),
                frame.place(arc.radius, b, far),
                frame.place(arc.radius, a, far)
            };
            const std::vector<Point> extrados = {
                frame.place(outer, a, near),
                frame.place(outer, b, near),
                frame.place(outer, b, far),
                frame.place(outer, a, far)
            };
            std::shared_ptr<wood_session::NodeBlockElement> block =
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

std::vector<std::shared_ptr<Element>> cross_vault(const CrossVaultGuide& guide) {

    const Arc arc = compute_arc(guide.span, guide.rise);
    const double outer = arc.radius + guide.thickness;
    const double end = outer * std::sin(arc.sector);
    const double ring = 2.0 * end / guide.rings;
    const double step = arc.sector / guide.courses;
    const std::vector<Vector> sides = {
        Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(-1.0, 0.0, 0.0), Vector(0.0, -1.0, 0.0)
    };

    std::vector<std::shared_ptr<Element>> blocks;
    for (int m = 0; m < guide.courses; m++) {
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

std::vector<std::shared_ptr<Element>> cloister_vault(const CloisterVaultGuide& guide) {
    return to_bay(guide.span, guide.rise, guide.thickness, guide.courses, guide.rings, false);
}

// ═══════════════════════════════════════════════════════════════════════════
// Cylindrical Groin Surfaces & Gothic / Star Ribbed Vaults
// (Bern Cathedral / Tracing the Past / Willis / Wendland / Jobbik)
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// Evaluate height z and slope dz/dc on a circular arch of half_span and arch_rise.
void cyl_arch(
    double coord,
    double half_span,
    double arch_rise,
    double& z_out,
    double& dz_dc
) {

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
}

/// Evaluate the cylindrical groin vault surface point S(x, y) and unit upward normal N(x, y) at plan point (x, y).
std::pair<Point, Vector> eval_vault_surface(
    double x,
    double y,
    double hx,
    double hy,
    double rise,
    const Point& ref_mid
) {

    const double h_safe_rise = std::max(1.0, rise);
    const double h_ref = std::max(1.0, hx);
    const double ux_ref = std::abs(ref_mid[0]) / std::max(1.0, hx);
    const double uy_ref = std::abs(ref_mid[1]) / std::max(1.0, hy);
    const double ux_pt = std::abs(x) / std::max(1.0, hx);
    const double uy_pt = std::abs(y) / std::max(1.0, hy);

    const bool use_ns = (std::abs(ux_ref - uy_ref) > 1e-6) ? (ux_ref <= uy_ref) : (ux_pt <= uy_pt);

    if (use_ns) {
        const double c_x = (x / std::max(1.0, hx)) * h_ref;
        double z = 0.0;
        double dz_dc = 0.0;
        cyl_arch(
            c_x,
            h_ref,
            h_safe_rise,
            z,
            dz_dc
        );
        const double dz_dx = dz_dc * (h_ref / std::max(1.0, hx));
        Vector n(-dz_dx, 0.0, 1.0);

        if (!n.normalize_self())
            n = Vector(0.0, 0.0, 1.0);

        return {Point(x, y, z), n};
    }

    const double c_y = (y / std::max(1.0, hy)) * h_ref;
    double z = 0.0;
    double dz_dc = 0.0;
    cyl_arch(
        c_y,
        h_ref,
        h_safe_rise,
        z,
        dz_dc
    );
    const double dz_dy = dz_dc * (h_ref / std::max(1.0, hy));
    Vector n(0.0, -dz_dy, 1.0);

    if (!n.normalize_self())
        n = Vector(0.0, 0.0, 1.0);

    return {Point(x, y, z), n};
}

/// Undirected segment between two plan nodes of a rib graph.
struct RibEdgeSpec {
    int u = 0; // Start node index.
    int v = 0; // End node index.
    double width_scale = 1.0; // Multiplier on base rib_width.
    std::string role = "rib"; // Structural role label.
    size_t path_idx = 0; // Index of the owning continuous rib path.
};

/// Continuous multi-segment rib chain across plan nodes.
struct RibPathSpec {
    std::vector<int> nodes; // Ordered node indices along the rib.
    double width_scale = 1.0; // Multiplier on base rib_width.
    std::string role = "rib"; // Structural role label.
};

/// Planar 2D rib graph of nodes, undirected edges, and continuous rib chains.
struct RibGraph {
    std::vector<Point> nodes; // Plan vertices at z = 0.
    std::vector<RibEdgeSpec> edges; // Undirected rib segments between adjacent nodes.
    std::vector<RibPathSpec> rib_paths; // Continuous multi-segment rib chains.

    /// Insert or reuse a plan node at (x, y) within 1e-3 mm and return its index.
    int add_node(double x, double y) {

        for (size_t i = 0; i < nodes.size(); ++i)
            if (std::hypot(nodes[i][0] - x, nodes[i][1] - y) < 1e-3)
                return static_cast<int>(i);

        nodes.emplace_back(x, y, 0.0);

        return static_cast<int>(nodes.size() - 1);
    }

    /// Append a continuous rib chain and register any new undirected edges along it.
    void add_rib_path(const std::vector<int>& chain, double w_scale, const std::string& role) {

        if (chain.size() < 2)
            return;

        const size_t p_idx = rib_paths.size();
        rib_paths.push_back({chain, w_scale, role});

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
                edges.push_back({u, v, w_scale, role, p_idx});
        }
    }

    /// Return whether edge e_idx belongs to a rib chain that continues straight through node_id.
    bool is_through_edge(size_t e_idx, int node_id) const {

        const std::vector<int>& ch = rib_paths[edges[e_idx].path_idx].nodes;

        return ch.size() >= 3 && node_id != ch.front() && node_id != ch.back();
    }
};

/// Wrap an angle in radians into [0, 2*pi).
double wrap_two_pi(double angle) {

    double a = std::fmod(angle, Tolerance::TWO_PI);

    if (a < 0.0)
        a += Tolerance::TWO_PI;

    return a;
}

/// Return the clamped uniform cubic knot value at index k over [0, m_span].
double knot_val(int k, double m_span) {
    return std::clamp(static_cast<double>(k - 2), 0.0, m_span);
}

/// Build a pure circular barrel cylinder cubic NURBS surface over [-hx, hx] x [-hy, hy] for the active vault web at ref_mid.
NurbsSurface compute_vault_web_surface(
    double hx,
    double hy,
    double rise,
    const Point& ref_mid,
    int count
) {

    const bool is_ns = (std::abs(ref_mid[0]) / std::max(1.0, hx) <=
                        std::abs(ref_mid[1]) / std::max(1.0, hy) + 1e-6);

    // a cylinder is its profile swept along a straight generator: cubic across, one straight span along
    const double m_span = static_cast<double>(count - 3);
    NurbsSurface surface(
        3,
        false,
        is_ns ? 4 : 2,
        is_ns ? 2 : 4,
        is_ns ? count : 2,
        is_ns ? 2 : count
    );

    for (int k = 0; k < count + 2; ++k)
        surface.set_nurbsknot(is_ns ? 0 : 1, k, knot_val(k, m_span));

    for (int k = 0; k < 2; ++k)
        surface.set_nurbsknot(is_ns ? 1 : 0, k, k == 0 ? 0.0 : m_span);

    // Exact Greville abscissae xi[i] = (knot(i) + knot(i+1) + knot(i+2)) / 3.0
    std::vector<double> xi(count);
    std::vector<double> x_cv(count);
    std::vector<double> y_cv(count);

    for (int i = 0; i < count; ++i) {
        xi[i] = (knot_val(i, m_span) + knot_val(i + 1, m_span) + knot_val(i + 2, m_span)) / 3.0;
        x_cv[i] = -hx + 2.0 * hx * (xi[i] / m_span);
        y_cv[i] = -hy + 2.0 * hy * (xi[i] / m_span);
    }

    std::vector<double> z_target(count);

    for (int i = 0; i < count; ++i) {
        const Point p_eval = is_ns ? Point(x_cv[i], 0.0, 0.0) : Point(0.0, y_cv[i], 0.0);
        z_target[i] = eval_vault_surface(
            p_eval[0],
            p_eval[1],
            hx,
            hy,
            rise,
            ref_mid
        ).first[2];
    }

    // the cylinder's profile collocated on the curve alone: the surface repeats it along its generators
    NurbsCurve profile(
        3,
        false,
        4,
        count
    );

    for (int k = 0; k < count + 2; ++k)
        profile.set_nurbsknot(k, knot_val(k, m_span));

    std::vector<double> z_cv = z_target;
    for (int iter = 0; iter < 32; ++iter) {
        for (int i = 0; i < count; ++i)
            profile.set_cv(i, Point(xi[i], 0.0, z_cv[i]));
        for (int i = 1; i + 1 < count; ++i)
            z_cv[i] += z_target[i] - profile.point_at(xi[i])[2];
        for (int i = 0; i < count / 2; ++i) {
            const double avg = 0.5 * (z_cv[i] + z_cv[count - 1 - i]);
            z_cv[i] = avg;
            z_cv[count - 1 - i] = avg;
        }
    }

    for (int i = 0; i < count; ++i)
        for (int end = 0; end < 2; ++end) {
            if (is_ns)
                surface.set_cv(i, end, Point(x_cv[i], end == 0 ? -hy : hy, z_cv[i]));
            else
                surface.set_cv(end, i, Point(end == 0 ? -hx : hx, y_cv[i], z_cv[i]));
        }

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

/// Paired orthogonal barrel cylinder NURBS surfaces over [-hx, hx] x [-hy, hy].
struct VaultCylinders {
    double hx = 0.0; // Half-span along x.
    double hy = 0.0; // Half-span along y.
    double rib_height = 0.0; // Vertical depth of the ribs.
    NurbsSurface surf_ns; // North-south barrel cylinder surface.
    NurbsSurface surf_ew; // East-west barrel cylinder surface.

    /// Evaluate surface s at (x, y), extrapolating linearly outside [-hx, hx] x [-hy, hy].
    Point eval_surf_smooth(const NurbsSurface& s, double x, double y) const {

        const double cx = std::clamp(x, -hx, hx);
        const double cy = std::clamp(y, -hy, hy);
        const Point uv0 = compute_vault_uv(
            s,
            hx,
            hy,
            Point(cx, cy, 0.0)
        );
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
            const Point uvx = compute_vault_uv(
                s,
                hx,
                hy,
                Point(cx + step_x, cy, 0.0)
            );
            dz_dx = (s.point_at(uvx[0], uvx[1])[2] - p0[2]) / step_x;
        }

        if (std::abs(dy_out) > 1e-9) {
            const double step_y = (cy >= 0.0) ? -eps : eps;
            const Point uvy = compute_vault_uv(
                s,
                hx,
                hy,
                Point(cx, cy + step_y, 0.0)
            );
            dz_dy = (s.point_at(uvy[0], uvy[1])[2] - p0[2]) / step_y;
        }

        return Point(x, y, p0[2] + dz_dx * dx_out + dz_dy * dy_out);
    }

    /// Evaluate the active groin cylinder at (x, y) clamped to [-hx, hx] x [-hy, hy].
    Point eval_nurbs_cyl(double x, double y) const {

        const double cx = std::clamp(x, -hx, hx);
        const double cy = std::clamp(y, -hy, hy);
        const double ux = std::abs(cx) / std::max(1.0, hx);
        const double uy = std::abs(cy) / std::max(1.0, hy);
        const NurbsSurface& s = (ux <= uy) ? surf_ns : surf_ew;
        const Point uv = compute_vault_uv(
            s,
            hx,
            hy,
            Point(cx, cy, 0.0)
        );

        return s.point_at(uv[0], uv[1]);
    }

    /// Signed normalized distance from the groin diagonal |x|/hx = |y|/hy.
    double groin_val(const Point& p) const {
        return std::abs(p[0]) / std::max(1.0, hx) - std::abs(p[1]) / std::max(1.0, hy);
    }

    /// Horizontal pullback distance corresponding to rib_height along an ascending rib slope.
    double slope_pullback(const Point& p_ref, const Vector& dir_out) const {

        const Point p0 = eval_nurbs_cyl(p_ref[0], p_ref[1]);
        const Point p1 = eval_nurbs_cyl(p_ref[0] + dir_out[0] * 2.0, p_ref[1] + dir_out[1] * 2.0);
        const double dz_ds = 0.5 * (p1[2] - p0[2]);

        if (dz_ds <= 0.0)
            return 0.0;

        const double sin_a = dz_ds / std::hypot(1.0, dz_ds);

        return rib_height * sin_a;
    }
};

/// Directed outgoing rib edge entry at a plan node.
struct AdjItem {
    size_t edge_idx = 0; // Index in RibGraph::edges.
    int other = 0; // Neighbor node index.
    double angle = 0.0; // Outgoing plan angle in radians.
    Vector dir_xy; // Outgoing unit 2D direction vector.
};

/// Order incident rib edges counter-clockwise by plan angle.
bool adj_angle_less(const AdjItem& a, const AdjItem& b) {
    return a.angle < b.angle;
}

/// Per-node cyclic adjacency and corner stop data for computing rib end cuts.
struct RibNodeCuts {
    const RibGraph& graph; // Planar rib graph.
    const VaultCylinders& cyls; // Paired barrel cylinder surfaces.
    const std::vector<std::vector<AdjItem>>& adj; // Outgoing edges sorted CCW per node.
    const std::array<std::vector<std::pair<Vector, double>>, 4>&
        corner_stops; // Column-head stop facets at corners 0..3.
    double rib_width = 0.0; // Base rib width in mm.

    /// Compute miter or T-butt end cut planes and axial trim offset for rib path p_idx at node_id.
    std::vector<Plane> endpoint_cuts(
        size_t p_idx,
        int node_id,
        const Vector& dir_out,
        double len_ref,
        double& trim_offset
    ) const {

        const Point& vp = graph.nodes[node_id];
        const std::vector<AdjItem>& list = adj[node_id];
        const double hw = 0.5 * rib_width * graph.rib_paths[p_idx].width_scale;

        if (node_id >= 0 && node_id < 4) {
            double r_stop = hw;
            double best = 0.95;

            for (const std::pair<Vector, double>& stop : corner_stops[node_id]) {
                const double align = stop.first.dot(dir_out);

                if (align > best) {
                    best = align;
                    r_stop = stop.second;
                }
            }

            const Point stop_pt = vp + dir_out * r_stop;
            trim_offset = r_stop - cyls.slope_pullback(stop_pt, dir_out) - 25.0;

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

        // Exact cyclic wedge miter / through-rib T-butt against immediate CW (prev) and CCW (next) neighbors
        const size_t prev_idx = (static_cast<size_t>(pos) + m - 1) % m;
        const size_t next_idx = (static_cast<size_t>(pos) + 1) % m;
        const AdjItem& prev_item = list[prev_idx];
        const AdjItem& next_item = list[next_idx];
        const double theta_prev = wrap_two_pi(curr.angle - prev_item.angle);
        const double theta_next = wrap_two_pi(next_item.angle - curr.angle);

        std::vector<Plane> planes;

        if (theta_prev > 0.02 && theta_prev < Tolerance::PI - 0.02) {
            if (graph.is_through_edge(prev_item.edge_idx, node_id)) {
                const Vector n_prev(-prev_item.dir_xy[1], prev_item.dir_xy[0], 0.0);
                const double hw_prev =
                    0.5 * rib_width * graph.edges[prev_item.edge_idx].width_scale;
                planes.push_back(Plane::from_point_normal(vp + n_prev * hw_prev, n_prev));
            } else {
                Vector b_prev = curr.dir_xy + prev_item.dir_xy;

                if (b_prev.magnitude() > 1e-6 && b_prev.normalize_self()) {
                    const Vector n_bis(-b_prev[1], b_prev[0], 0.0);
                    planes.push_back(Plane::from_point_normal(vp, n_bis));
                }
            }
        }

        if (theta_next > 0.02 && theta_next < Tolerance::PI - 0.02) {
            if (graph.is_through_edge(next_item.edge_idx, node_id)) {
                const Vector n_next(next_item.dir_xy[1], -next_item.dir_xy[0], 0.0);
                const double hw_next =
                    0.5 * rib_width * graph.edges[next_item.edge_idx].width_scale;
                const Plane cand = Plane::from_point_normal(vp + n_next * hw_next, n_next);

                if (planes.empty() || planes.back().z_axis().dot(cand.z_axis()) < 0.9999)
                    planes.push_back(cand);
            } else {
                Vector b_next = curr.dir_xy + next_item.dir_xy;

                if (b_next.magnitude() > 1e-6 && b_next.normalize_self()) {
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
        std::vector<double> a_vals;
        std::vector<double> b_vals;
        a_vals.reserve(planes.size());
        b_vals.reserve(planes.size());

        for (const Plane& pl : planes) {
            const double denom = std::max(0.05, curr.dir_xy.dot(pl.z_axis()));
            a_vals.push_back((pl.origin() - vp).dot(pl.z_axis()) / denom);
            b_vals.push_back(u_perp.dot(pl.z_axis()) / denom);
        }

        std::vector<double> t_cands = {-hw, hw};

        for (size_t i = 0; i < planes.size(); ++i)
            for (size_t j = i + 1; j < planes.size(); ++j) {
                const double db = b_vals[i] - b_vals[j];

                if (std::abs(db) > 1e-9) {
                    const double t_star = (a_vals[j] - a_vals[i]) / db;

                    if (t_star >= -hw && t_star <= hw)
                        t_cands.push_back(t_star);
                }
            }

        double min_s = 1e9;

        for (const double tc : t_cands) {
            double env = -1e9;

            for (size_t i = 0; i < planes.size(); ++i)
                env = std::max(env, a_vals[i] + b_vals[i] * tc);

            min_s = std::min(min_s, env);
        }

        const Point cut_ref = vp + dir_out * std::max(0.0, min_s);
        trim_offset =
            std::max(-0.45 * len_ref, min_s - cyls.slope_pullback(cut_ref, dir_out) - 20.0);

        return planes;
    }
};

/// Build corner support heads, planar single-curved mitered ribs, and cylindrical BRep infill panels from a 2D rib graph.
std::vector<std::shared_ptr<Element>> build_cylindrical_ribbed_vault_from_graph(
    double hx,
    double hy,
    double rise,
    double rib_width,
    double rib_height,
    double panel_thickness,
    int n_sub,
    const RibGraph& graph,
    const std::string& panel_name
) {

    const std::vector<Point>& nodes = graph.nodes;
    const std::vector<RibEdgeSpec>& edges = graph.edges;
    const std::vector<RibPathSpec>& rib_paths = graph.rib_paths;

    // Precompute pure circular barrel cylinder NURBS surfaces for N/S and E/W quadrants
    const int surf_cv_count = std::max(35, 2 * n_sub + 3);
    const int edge_sub = std::max(24, n_sub);
    const VaultCylinders cyls{
        hx,
        hy,
        rib_height,
        compute_vault_web_surface(
            hx,
            hy,
            rise,
            Point(0.0, 0.5 * hy, 0.0),
            surf_cv_count
        ),
        compute_vault_web_surface(
            hx,
            hy,
            rise,
            Point(0.5 * hx, 0.0, 0.0),
            surf_cv_count
        )
    };
    const NurbsSurface& surf_ns = cyls.surf_ns;
    const NurbsSurface& surf_ew = cyls.surf_ew;

    // Build adjacency & sort outgoing edges CCW by plan angle at every node
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
        std::sort(list.begin(), list.end(), adj_angle_less);

    // Minimal Corner EdgeStraightElement Heads (support_block) at nodes 0..3 so incoming ribs never collide
    std::vector<std::shared_ptr<Element>> result;
    std::array<std::vector<std::pair<Vector, double>>, 4> corner_stops;

    for (int c = 0; c < 4; ++c) {
        std::vector<AdjItem> list = adj[c];

        if (c == 3 && list.size() >= 2) {
            for (AdjItem& item : list)
                if (item.angle < 0.5 * Tolerance::PI)
                    item.angle += Tolerance::TWO_PI;

            std::sort(list.begin(), list.end(), adj_angle_less);
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
        std::vector<Point> poly_2d;

        for (size_t i = 0; i < raw_poly.size(); ++i) {
            const Point& a = raw_poly[i];
            const Point& b = raw_poly[(i + 1) % raw_poly.size()];

            if (poly_2d.empty() || poly_2d.back().distance(a) > 1e-4)
                poly_2d.push_back(a);

            if (i >= 1 && i + 1 < raw_poly.size()) {
                const double ga = cyls.groin_val(a);
                const double gb = cyls.groin_val(b);

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
            const Point p_top = cyls.eval_nurbs_cyl(p_stop[0], p_stop[1]);
            const Point p_in = cyls.eval_nurbs_cyl(
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
            const Point p_surf = cyls.eval_nurbs_cyl(p2[0], p2[1]);
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

        result.push_back(std::make_shared<wood_session::NodeBlockElement>(
            Mesh::from_polylines(head_faces, 1e-6), "support_block"
        ));
    }

    const RibNodeCuts cutter{graph, cyls, adj, corner_stops, rib_width};

    // Generate planar single-curved EdgeCurvedElement ribs strictly in their vertical planes
    std::vector<std::shared_ptr<wood_session::EdgeCurvedElement>> path_beams(rib_paths.size());

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
        std::vector<Plane> cuts_u = cutter.endpoint_cuts(
            p_idx,
            u,
            dir_2d,
            len_2d,
            offset_u
        );
        std::vector<Plane> cuts_v = cutter.endpoint_cuts(
            p_idx,
            v,
            dir_2d * -1.0,
            len_2d,
            offset_v
        );

        const double s_start = offset_u;
        const double s_end = len_2d - offset_v;

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
            surf_pts[k] = cyls.eval_surf_smooth(rib_surf, plan_pt[0], plan_pt[1]);
        }

        // Keep up strictly in the vertical plane of the rib so every rib has zero sideways twist
        // and its top centerline, axis centerline, and bottom centerline are straight in XY plan!
        std::vector<Point> axis(n_rib_sub + 1);
        std::vector<Vector> ups(n_rib_sub + 1);

        for (int k = 0; k <= n_rib_sub; ++k) {
            const Vector tang =
                (k == 0)           ? (surf_pts[1] - surf_pts[0]).normalized()
                : (k == n_rib_sub) ? (surf_pts[n_rib_sub] - surf_pts[n_rib_sub - 1]).normalized()
                                   : (surf_pts[k + 1] - surf_pts[k - 1]).normalized();
            Vector up = Vector(0.0, 0.0, 1.0) - tang * tang[2];

            if (up.magnitude() < 1e-6 || !up.normalize_self())
                up = Vector(0.0, 0.0, 1.0);

            ups[k] = up;
            axis[k] = surf_pts[k] - up * (0.5 * rib_height);
        }

        const std::shared_ptr<wood_session::EdgeCurvedElement> beam =
            std::make_shared<wood_session::EdgeCurvedElement>(
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
        path_beams[p_idx] = beam;
        result.push_back(beam);
    }

    // Interlocking half-depth cross-lap joints wherever continuous ribs cross at an interior node
    for (size_t nid = 4; nid < nodes.size(); ++nid) {
        std::vector<size_t> through_paths;
        for (size_t p_idx = 0; p_idx < rib_paths.size(); ++p_idx) {
            if (!path_beams[p_idx])
                continue;
            const std::vector<int>& ch = rib_paths[p_idx].nodes;
            for (size_t k = 1; k + 1 < ch.size(); ++k) {
                if (ch[k] == static_cast<int>(nid)) {
                    through_paths.push_back(p_idx);
                    break;
                }
            }
        }
        if (through_paths.size() < 2)
            continue;

        const Point& vp = nodes[nid];
        for (size_t ia = 0; ia < through_paths.size(); ++ia) {
            for (size_t ib = ia + 1; ib < through_paths.size(); ++ib) {
                const size_t pa = through_paths[ia];
                const size_t pb = through_paths[ib];
                const RibPathSpec& spec_a = rib_paths[pa];
                const RibPathSpec& spec_b = rib_paths[pb];
                Vector dir_a = nodes[spec_a.nodes.back()] - nodes[spec_a.nodes.front()];
                Vector dir_b = nodes[spec_b.nodes.back()] - nodes[spec_b.nodes.front()];
                dir_a[2] = 0.0;
                dir_b[2] = 0.0;
                if (dir_a.magnitude() < 1e-6 || dir_b.magnitude() < 1e-6)
                    continue;
                dir_a.normalize_self();
                dir_b.normalize_self();
                if (std::abs(dir_a.dot(dir_b)) > 0.995)
                    continue;

                const double hw_a = 0.5 * rib_width * spec_a.width_scale;
                const double hw_b = 0.5 * rib_width * spec_b.width_scale;

                Vector n_b(-dir_b[1], dir_b[0], 0.0);
                if (dir_a.dot(n_b) < 0.0)
                    n_b = n_b * -1.0;
                path_beams[pa]->cross_laps.push_back({
                    Plane::from_point_normal(vp - n_b * hw_b, n_b * -1.0),
                    Plane::from_point_normal(vp + n_b * hw_b, n_b),
                    false
                });
                path_beams[pa]->invalidate_geometry();

                Vector n_a(-dir_a[1], dir_a[0], 0.0);
                if (dir_b.dot(n_a) < 0.0)
                    n_a = n_a * -1.0;
                path_beams[pb]->cross_laps.push_back({
                    Plane::from_point_normal(vp - n_a * hw_a, n_a * -1.0),
                    Plane::from_point_normal(vp + n_a * hw_a, n_a),
                    true
                });
                path_beams[pb]->invalidate_geometry();
            }
        }
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

            result.push_back(to_vault_panel(
                web_surf,
                hx,
                hy,
                clean_poly,
                panel_thickness,
                edge_sub,
                panel_name
            ));
        }
    }

    return result;
}

} // namespace

std::vector<std::shared_ptr<Element>> gothic_ribbed_vault(const GothicRibbedVaultGuide& guide) {

    const double hx = 0.5 * guide.span_x;
    const double hy = 0.5 * guide.span_y;
    const int n_sub = std::max(8, guide.subdivisions);
    const double panel_thickness = 35.0;

    RibGraph graph;
    graph.nodes = {
        Point(-hx, -hy, 0.0), // 0: SW corner
        Point(hx, -hy, 0.0), // 1: SE corner
        Point(hx, hy, 0.0), // 2: NE corner
        Point(-hx, hy, 0.0), // 3: NW corner
        Point(0.0, -hy, 0.0), // 4: S wall midpoint
        Point(hx, 0.0, 0.0), // 5: E wall midpoint
        Point(0.0, hy, 0.0), // 6: N wall midpoint
        Point(-hx, 0.0, 0.0), // 7: W wall midpoint
        Point(0.0, 0.0, 0.0) // 8: Central crown boss
    };

    if (guide.pattern != GothicRibbedVaultGuide::FAN_CONOID) {
        for (int i = 0; i < 4; ++i) {
            const int c_curr = i;
            const int c_next = (i + 1) % 4;
            const int m_wall = 4 + i;
            graph.add_rib_path({c_curr, m_wall, c_next}, 1.0, "wall_arch");
        }
    }

    if (guide.pattern == GothicRibbedVaultGuide::QUADRIPARTITE) {
        // Bern Cathedral / Wells classic quadripartite cross vault
        graph.add_rib_path({0, 8, 2}, 1.0, "diagonal_rib");
        graph.add_rib_path({1, 8, 3}, 1.0, "diagonal_rib");

        for (int i = 0; i < 4; ++i)
            graph.add_rib_path({4 + i, 8}, 1.0, "ridge_rib");
    } else if (guide.pattern == GothicRibbedVaultGuide::TIERCERON) {
        // Bern Cathedral / Exeter tierceron star vault
        const std::array<int, 4> r_nodes = {
            graph.add_node(0.0, -0.5 * hy),
            graph.add_node(0.5 * hx, 0.0),
            graph.add_node(0.0, 0.5 * hy),
            graph.add_node(-0.5 * hx, 0.0)
        };
        graph.add_rib_path({0, 8, 2}, 1.0, "diagonal_rib");
        graph.add_rib_path({1, 8, 3}, 1.0, "diagonal_rib");

        for (int i = 0; i < 4; ++i) {
            const int c_next = (i + 1) % 4;
            graph.add_rib_path({4 + i, r_nodes[i], 8}, 1.0, "ridge_rib");
            graph.add_rib_path({i, r_nodes[i]}, 1.0, "tierceron_rib");
            graph.add_rib_path({c_next, r_nodes[i]}, 1.0, "tierceron_rib");
        }
    } else if (guide.pattern == GothicRibbedVaultGuide::STAR_STELLAR) {
        // Bern Cathedral 4-lozenge stellar star vault (4 central diamond lozenges meeting at crown 8)
        const double s = 0.62;
        const double d = 0.36;
        const std::array<int, 4> s_nodes = {
            graph.add_node(0.0, -s * hy),
            graph.add_node(s * hx, 0.0),
            graph.add_node(0.0, s * hy),
            graph.add_node(-s * hx, 0.0)
        };
        const std::array<int, 4> d_nodes = {
            graph.add_node(-d * hx, -d * hy),
            graph.add_node(d * hx, -d * hy),
            graph.add_node(d * hx, d * hy),
            graph.add_node(-d * hx, d * hy)
        };
        graph.add_rib_path({0, d_nodes[0], 8, d_nodes[2], 2}, 1.0, "diagonal_rib");
        graph.add_rib_path({1, d_nodes[1], 8, d_nodes[3], 3}, 1.0, "diagonal_rib");

        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            graph.add_rib_path({4 + i, s_nodes[i]}, 1.0, "ridge_rib");
            graph.add_rib_path({i, s_nodes[i]}, 1.0, "tierceron_rib");
            graph.add_rib_path({next, s_nodes[i]}, 1.0, "tierceron_rib");
            graph.add_rib_path({d_nodes[i], s_nodes[i]}, 1.0, "lierne_rib");
            graph.add_rib_path({d_nodes[i], s_nodes[prev]}, 1.0, "lierne_rib");
        }
    } else if (guide.pattern == GothicRibbedVaultGuide::LIERNE_STAR) {
        // Bern Cathedral / Ely Choir double-star lierne vault
        const double r_out = 0.68;
        const double r_in = 0.34;
        const double d_mid = 0.48;
        const std::array<int, 4> ro = {
            graph.add_node(0.0, -r_out * hy),
            graph.add_node(r_out * hx, 0.0),
            graph.add_node(0.0, r_out * hy),
            graph.add_node(-r_out * hx, 0.0)
        };
        const std::array<int, 4> ri = {
            graph.add_node(0.0, -r_in * hy),
            graph.add_node(r_in * hx, 0.0),
            graph.add_node(0.0, r_in * hy),
            graph.add_node(-r_in * hx, 0.0)
        };
        const std::array<int, 4> dn = {
            graph.add_node(-d_mid * hx, -d_mid * hy),
            graph.add_node(d_mid * hx, -d_mid * hy),
            graph.add_node(d_mid * hx, d_mid * hy),
            graph.add_node(-d_mid * hx, d_mid * hy)
        };
        graph.add_rib_path({0, dn[0], 8, dn[2], 2}, 1.0, "diagonal_rib");
        graph.add_rib_path({1, dn[1], 8, dn[3], 3}, 1.0, "diagonal_rib");

        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            graph.add_rib_path({4 + i, ro[i], ri[i], 8}, 1.0, "ridge_rib");
            graph.add_rib_path({i, ro[i]}, 1.0, "tierceron_rib");
            graph.add_rib_path({next, ro[i]}, 1.0, "tierceron_rib");
            graph.add_rib_path({dn[i], ri[i]}, 1.0, "lierne_rib");
            graph.add_rib_path({dn[i], ri[prev]}, 1.0, "lierne_rib");
            graph.add_rib_path({dn[i], ro[i]}, 1.0, "lierne_rib");
            graph.add_rib_path({dn[i], ro[prev]}, 1.0, "lierne_rib");
        }
    } else if (guide.pattern == GothicRibbedVaultGuide::NET_RETICULATED) {
        // Bern Cathedral Choir / Parler reticulated diamond-lattice net vault (collinear crossing net chords: 2*0.525*0.21/(0.525+0.21) == 0.30)
        const std::array<int, 4> q_nodes = {
            graph.add_node(0.0, -0.62 * hy),
            graph.add_node(0.62 * hx, 0.0),
            graph.add_node(0.0, 0.62 * hy),
            graph.add_node(-0.62 * hx, 0.0)
        };
        const std::array<int, 4> r_inner = {
            graph.add_node(0.0, -0.30 * hy),
            graph.add_node(0.30 * hx, 0.0),
            graph.add_node(0.0, 0.30 * hy),
            graph.add_node(-0.30 * hx, 0.0)
        };
        const std::array<int, 4> d_outer = {
            graph.add_node(-0.525 * hx, -0.525 * hy),
            graph.add_node(0.525 * hx, -0.525 * hy),
            graph.add_node(0.525 * hx, 0.525 * hy),
            graph.add_node(-0.525 * hx, 0.525 * hy)
        };
        const std::array<int, 4> d_inner = {
            graph.add_node(-0.21 * hx, -0.21 * hy),
            graph.add_node(0.21 * hx, -0.21 * hy),
            graph.add_node(0.21 * hx, 0.21 * hy),
            graph.add_node(-0.21 * hx, 0.21 * hy)
        };
        graph.add_rib_path(
            {0, d_outer[0], d_inner[0], 8, d_inner[2], d_outer[2], 2}, 1.0, "diagonal_rib"
        );
        graph.add_rib_path(
            {1, d_outer[1], d_inner[1], 8, d_inner[3], d_outer[3], 3}, 1.0, "diagonal_rib"
        );

        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const int prev = (i + 3) % 4;
            graph.add_rib_path({4 + i, q_nodes[i]}, 1.0, "ridge_rib");
            graph.add_rib_path({i, q_nodes[i]}, 1.0, "tierceron_rib");
            graph.add_rib_path({next, q_nodes[i]}, 1.0, "tierceron_rib");
            graph.add_rib_path({d_outer[i], q_nodes[i]}, 1.0, "net_rib");
            graph.add_rib_path({d_outer[i], q_nodes[prev]}, 1.0, "net_rib");
            graph.add_rib_path({d_outer[i], r_inner[i], d_inner[next]}, 1.0, "net_rib");
            graph.add_rib_path({d_outer[next], r_inner[i], d_inner[i]}, 1.0, "net_rib");
        }
    } else if (guide.pattern == GothicRibbedVaultGuide::FAN_CONOID) {
        // Polygonal fan vault on intersecting circular barrel cylinders (straight radiating fan ribs + straight polygonal collar ribs)
        const double r_col = 0.68 * std::min(hx, hy);
        const std::array<int, 4> r_out = {
            graph.add_node(0.0, -0.66 * hy),
            graph.add_node(0.66 * hx, 0.0),
            graph.add_node(0.0, 0.66 * hy),
            graph.add_node(-0.66 * hx, 0.0)
        };
        const std::array<int, 4> r_in = {
            graph.add_node(0.0, -0.33 * hy),
            graph.add_node(0.33 * hx, 0.0),
            graph.add_node(0.0, 0.33 * hy),
            graph.add_node(-0.33 * hx, 0.0)
        };
        const double d_diag = r_col / std::sqrt(2.0);
        const std::array<int, 4> d_col = {
            graph.add_node(-hx + d_diag, -hy + d_diag),
            graph.add_node(hx - d_diag, -hy + d_diag),
            graph.add_node(hx - d_diag, hy - d_diag),
            graph.add_node(-hx + d_diag, hy - d_diag)
        };

        // Collar nodes along each half-wall and fan ray at radial distance r_col from each corner
        std::array<std::array<int, 2>, 4> w_col{};
        std::array<std::array<int, 2>, 4> f_out_col{};
        std::array<std::array<int, 2>, 4> f_in_col{};

        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const std::array<int, 2> corners_q = {i, next};

            for (int s = 0; s < 2; ++s) {
                const Point cp = graph.nodes[corners_q[s]];
                const Point pw = cp + (graph.nodes[4 + i] - cp).normalized() * r_col;
                const Point po = cp + (graph.nodes[r_out[i]] - cp).normalized() * r_col;
                const Point pi = cp + (graph.nodes[r_in[i]] - cp).normalized() * r_col;
                w_col[i][s] = graph.add_node(pw[0], pw[1]);
                f_out_col[i][s] = graph.add_node(po[0], po[1]);
                f_in_col[i][s] = graph.add_node(pi[0], pi[1]);
            }
        }

        // 4 unbroken perimeter wall arches
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            graph.add_rib_path({i, w_col[i][0], 4 + i, w_col[i][1], next}, 1.0, "wall_arch");
        }

        // Two interlocking unbroken diagonal arches
        graph.add_rib_path({0, d_col[0], 8, d_col[2], 2}, 1.0, "diagonal_rib");
        graph.add_rib_path({1, d_col[1], 8, d_col[3], 3}, 1.0, "diagonal_rib");

        // Unbroken ridge ribs, radiating fan ribs from corners to ridge, and straight horizontal collar ribs
        for (int i = 0; i < 4; ++i) {
            const int next = (i + 1) % 4;
            const std::array<int, 2> corners_q = {i, next};
            graph.add_rib_path({4 + i, r_out[i], r_in[i], 8}, 1.0, "ridge_rib");

            for (int s = 0; s < 2; ++s) {
                const int c = corners_q[s];
                graph.add_rib_path({c, f_out_col[i][s], r_out[i]}, 1.0, "fan_rib");
                graph.add_rib_path({c, f_in_col[i][s], r_in[i]}, 1.0, "fan_rib");
                graph.add_rib_path({w_col[i][s], f_out_col[i][s]}, 1.0, "collar_rib");
                graph.add_rib_path({f_out_col[i][s], f_in_col[i][s]}, 1.0, "collar_rib");
                graph.add_rib_path({f_in_col[i][s], d_col[c]}, 1.0, "collar_rib");
            }
        }
    }

    return build_cylindrical_ribbed_vault_from_graph(
        hx,
        hy,
        guide.rise,
        guide.rib_width,
        guide.rib_height,
        panel_thickness,
        n_sub,
        graph,
        "vault_panel"
    );
}

wood_session::AssemblyModule gothic_assembly(const std::string& name, const std::vector<std::shared_ptr<Element>>& elements) {

    std::array<std::vector<wood_session::AssemblyModule>, 3> stages;
    for (const std::shared_ptr<Element>& element : elements)
        stages[element->name == "support_block" ? 0 : element->name == "vault_panel" ? 2 : 1].push_back(wood_session::AssemblyModule::part(element));
    std::vector<wood_session::AssemblyModule> whole = stages[0];
    whole.insert(whole.end(), stages[1].begin(), stages[1].end());
    whole.insert(whole.end(), stages[2].begin(), stages[2].end());

    return wood_session::AssemblyModule::module(name, whole);
}

}
