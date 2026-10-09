#pragma once
#include "wood_session.h"
#include "wood_profile.h"

namespace wood_gridshell {

using namespace session_cpp;
using namespace wood_session;

/// Board, gap and stud sizes of a two-layer lamella gridshell, in mm.
struct Lamella {
    double height = 140.0; // Board depth along the surface normal.
    double thickness = 20.0; // Board thickness across the lamella.
    double gap = 60.0; // Clear distance between the two boards of a lamella, the stud width across every flat.
    double spacing = 180.0; // Normal distance between the top and the bottom layer centrelines; below height the layers overlap.
    double overrun = 20.0; // Stud length past the outer face of each layer.
    double step = 100.0; // Tracing step along a lamella on the surface.
};

/// Where a top lamella crosses a bottom lamella, with the frame of each there.
struct Crossing {
    size_t top; // Top lamella.
    size_t bottom; // Bottom lamella.
    double along_top; // Position along the top lamella, segment index plus fraction.
    double along_bottom; // Position along the bottom lamella, segment index plus fraction.
    Plane top_frame; // The top lamella's frame at the crossing: x along it, z the normal.
    Plane bottom_frame; // The bottom lamella's frame at the crossing.
};

// ═══════════════════════════════════════════════════════════════════════════
// LamellaGridshell
// ═══════════════════════════════════════════════════════════════════════════

/// A two-layer lamella gridshell on a NURBS surface: two upright boards per lamella and a hexagonal stud at every crossing.
///
/// Fields: the surface, the curve family, the lamella counts and sizes, and the stations of every lamella.
/// Boards are `board_top_<i>_<side>` and `board_bottom_<j>_<side>`, studs `stud_<i>_<j>`.
class LamellaGridshell : public WoodSession {
public:
    const NurbsSurface surface; // The surface the lamellas are traced on.
    const int curves; // 0 the u and v iso-curves, 1 the asymptotic curves.
    const int count_top; // Lamellas of the first family, the top layer.
    const int count_bottom; // Lamellas of the second family, the bottom layer.
    const Lamella lamella; // Board, gap and stud sizes.

    /// A cubic saddle of side size, negative Gaussian curvature everywhere, curving faster at one end.
    static NurbsSurface default_surface(double size = 10000.0);

    /// The gridshell on surface, its lamellas on the iso-curves (curves 0) or the asymptotic curves (curves 1).
    explicit LamellaGridshell(
        const NurbsSurface& surface = default_surface(),
        int curves = 1,
        int count_top = 9,
        int count_bottom = 9,
        const Lamella& lamella = Lamella(),
        const std::string& name = "lamella_gridshell"
    );

    /// The frames along every lamella, x along it, z the normal; the top lamellas first.
    const std::vector<std::vector<Plane>>& stations() const;

private:
    std::vector<std::vector<Plane>> _stations; // The frames along every lamella, the top lamellas first.

    /// The surface vector of a (du, dv, 0) direction at uv.
    Vector compute_tangent(const Point& uv, const Vector& direction) const;

    /// The two lamella directions at uv as (du, dv, 0), none where an asymptotic direction does not exist.
    std::vector<Vector> compute_directions(const Point& uv) const;

    /// The lamella direction at uv nearest previous on the surface, zero where there is none.
    Vector compute_slope(const Point& uv, const Vector& previous) const;

    /// The share of delta from uv that stays inside the surface domain.
    double compute_share(const Point& uv, const Vector& delta) const;

    /// Points in (u, v, 0) from seed along direction by RK4 steps, the last clipped to the domain edge.
    std::vector<Point> compute_path(const Point& seed, Vector direction) const;

    /// The lamella through seed both ways along the direction nearest hint.
    std::vector<Point> compute_curve(const Point& seed, const Vector& hint) const;

    /// count lamellas of family 0 or 1, seeded at even arc lengths along a spine of the other family.
    std::vector<std::vector<Point>> compute_family(int family, int count) const;

    /// A lamella's (u, v) points on the surface.
    Polyline compute_polyline(const std::vector<Point>& points) const;

    /// The frame of a lamella at uv: x its direction nearest the segment at along, z the normal.
    Plane compute_frame(const std::vector<Point>& points, double along, const Point& uv) const;

    /// Every crossing of the two families, segment against segment in (u, v).
    std::vector<Crossing> compute_crossings(const std::vector<std::vector<Point>>& tops, const std::vector<std::vector<Point>>& bottoms) const;

    /// Frames along one lamella, straight a gap either side of each of its crossings and one gap past each end.
    std::vector<Plane> compute_stations(const std::vector<Point>& points, const std::vector<std::pair<double, Plane>>& marks) const;

    /// One board on the stations, its section lift along the normal and shift across it.
    std::shared_ptr<Beam> compute_board(
        const std::vector<Plane>& stations,
        double lift,
        double shift,
        const std::string& name
    ) const;

    /// A stud along the normal through both layers, a hexagon of three flat pairs gap apart.
    std::shared_ptr<Column> compute_stud(const Crossing& crossing) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

inline LamellaGridshell::LamellaGridshell(
    const NurbsSurface& surface,
    int curves,
    int count_top,
    int count_bottom,
    const Lamella& lamella,
    const std::string& name
)
    : WoodSession(name),
      surface(surface),
      curves(curves),
      count_top(count_top),
      count_bottom(count_bottom),
      lamella(lamella) {

    // surface: the NURBS surface the lamellas are traced on
    add_nurbssurface(surface, add_group("surface"));

    // lamellas: the first family traced across the second, each seeded along a spine of the other
    const std::vector<std::vector<Point>> tops = compute_family(0, count_top);
    const std::vector<std::vector<Point>> bottoms = compute_family(1, count_bottom);
    const std::shared_ptr<TreeNode> lamellas = add_group("lamellas");

    for (const std::vector<Point>& top : tops)
        add_polyline(compute_polyline(top), lamellas);

    for (const std::vector<Point>& bottom : bottoms)
        add_polyline(compute_polyline(bottom), lamellas);

    // crossings: where a top lamella crosses a bottom one, with the frame of each there
    const std::vector<Crossing> crossings = compute_crossings(tops, bottoms);
    const std::shared_ptr<TreeNode> crossings_group = add_group("crossings");
    std::vector<std::vector<std::pair<double, Plane>>> top_marks(tops.size());
    std::vector<std::vector<std::pair<double, Plane>>> bottom_marks(bottoms.size());

    for (const Crossing& crossing : crossings) {
        top_marks[crossing.top].emplace_back(crossing.along_top, crossing.top_frame);
        bottom_marks[crossing.bottom].emplace_back(crossing.along_bottom, crossing.bottom_frame);
        add_point(crossing.top_frame.origin(), crossings_group);
    }

    // stations: frames along every lamella, straight past each crossing
    const std::shared_ptr<TreeNode> stations_group = add_group("stations");

    for (size_t i = 0; i < tops.size(); i++)
        _stations.push_back(compute_stations(tops[i], top_marks[i]));

    for (size_t j = 0; j < bottoms.size(); j++)
        _stations.push_back(compute_stations(bottoms[j], bottom_marks[j]));

    for (const std::vector<Plane>& frames : _stations)
        for (const Plane& frame : frames) {
            const Point base = frame.origin() - frame.z_axis() * lamella.spacing;
            const Point tip = frame.origin() + frame.z_axis() * lamella.spacing;
            add_line(Line::from_points(base, tip), stations_group);
        }

    // boards: two boards per lamella gap apart, the top family spacing / 2 up the normal, the bottom one as far down
    const double lift = lamella.spacing / 2.0;
    const double shift = (lamella.gap + lamella.thickness) / 2.0;
    const std::shared_ptr<TreeNode> model = add_group("model");
    const std::shared_ptr<TreeNode> boards = add_group("boards", model);
    const std::shared_ptr<TreeNode> top_boards = add_group("top", boards);
    const std::shared_ptr<TreeNode> bottom_boards = add_group("bottom", boards);

    for (size_t i = 0; i < _stations.size(); i++) {
        const bool upper = i < tops.size();
        const std::string layer = upper ? "top" : "bottom";
        const size_t index = upper ? i : i - tops.size();
        const std::shared_ptr<TreeNode> group = upper ? top_boards : bottom_boards;

        for (size_t side = 0; side < 2; side++) {
            const std::shared_ptr<Beam> board = compute_board(
                _stations[i],
                upper ? lift : -lift,
                side == 0 ? shift : -shift,
                fmt::format("board_{}_{}_{}", layer, index, side)
            );
            add(board, group);
        }
    }

    // studs: a hexagonal stud along the normal through both layers at every crossing
    const std::shared_ptr<TreeNode> studs = add_group("studs", model);

    for (const Crossing& crossing : crossings)
        add(compute_stud(crossing), studs);

    // contacts: a face contact between every stud and the four boards it touches
    compute_face_contacts();
}

inline const std::vector<std::vector<Plane>>& LamellaGridshell::stations() const {

    return _stations;
}

inline NurbsSurface LamellaGridshell::default_surface(double size) {

    const double rise = 0.15;
    const double skew = 0.5;
    std::vector<Point> points;

    for (int u = 0; u < 4; u++)
        for (int v = 0; v < 4; v++) {
            const double across = v / 1.5 - 1.0;
            const double along = u / 1.5 - 1.0;
            const double height = rise * size / 2.0 * (across * across * (1.0 + skew * across) - along * along);
            points.emplace_back((across + 1.0) * size / 2.0, along * size / 2.0, height);
        }

    return NurbsSurface::create(
        false,
        false,
        3,
        3,
        4,
        4,
        points
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// Tracing
// ═══════════════════════════════════════════════════════════════════════════

inline Vector LamellaGridshell::compute_tangent(const Point& uv, const Vector& direction) const {

    const std::vector<Vector> derivatives = surface.evaluate(uv[0], uv[1], 1);

    return derivatives[2] * direction[0] + derivatives[1] * direction[1];
}

inline std::vector<Vector> LamellaGridshell::compute_directions(const Point& uv) const {

    const std::vector<Vector> derivatives = surface.evaluate(uv[0], uv[1], 2);
    std::vector<Vector> directions;

    if (curves == 0) {
        directions.push_back(Vector(1.0, 0.0, 0.0) / derivatives[3].magnitude());
        directions.push_back(Vector(0.0, 1.0, 0.0) / derivatives[1].magnitude());
        return directions;
    }

    const Vector normal = surface.normal_at(uv[0], uv[1]);
    const double suu = derivatives[5].dot(normal);
    const double suv = derivatives[4].dot(normal);
    const double svv = derivatives[2].dot(normal);
    const double mean = (suu + svv) / 2.0;
    const double radius = std::hypot((suu - svv) / 2.0, suv);

    if (radius < 1e-12 || std::abs(mean) > radius)
        return directions;

    const double phi = std::atan2(suv, (suu - svv) / 2.0);
    const double spread = std::acos(-mean / radius);

    for (const double sign : {-1.0, 1.0}) {
        const double angle = (phi + sign * spread) / 2.0;
        const Vector direction(std::cos(angle), std::sin(angle), 0.0);
        directions.push_back(direction / compute_tangent(uv, direction).magnitude());
    }

    return directions;
}

inline Vector LamellaGridshell::compute_slope(const Point& uv, const Vector& previous) const {

    Vector slope(0.0, 0.0, 0.0);
    double best = -1.0;

    for (const Vector& direction : compute_directions(uv)) {
        const double dot = compute_tangent(uv, direction).dot(previous);

        if (std::abs(dot) > best) {
            best = std::abs(dot);
            slope = dot < 0.0 ? -direction : direction;
        }
    }

    return slope;
}

inline double LamellaGridshell::compute_share(const Point& uv, const Vector& delta) const {

    double share = 1.0;

    for (int dir = 0; dir < 2; dir++) {
        const std::pair<double, double> domain = surface.domain(dir);

        if (uv[dir] + delta[dir] < domain.first)
            share = std::min(share, (domain.first - uv[dir]) / delta[dir]);

        if (uv[dir] + delta[dir] > domain.second)
            share = std::min(share, (domain.second - uv[dir]) / delta[dir]);
    }

    return std::max(share, 0.0);
}

inline std::vector<Point> LamellaGridshell::compute_path(const Point& seed, Vector direction) const {

    std::vector<Point> points{seed};

    for (int k = 0; k < 100000; k++) {
        const Point uv = points.back();
        const Vector k1 = compute_slope(uv, direction) * lamella.step;
        const Vector k2 = compute_slope(uv + k1 * 0.5, direction) * lamella.step;
        const Vector k3 = compute_slope(uv + k2 * 0.5, direction) * lamella.step;
        const Vector k4 = compute_slope(uv + k3, direction) * lamella.step;
        const Vector delta = (k1 + k2 * 2.0 + k3 * 2.0 + k4) / 6.0;
        const double share = compute_share(uv, delta);

        if (k1.magnitude() == 0.0 || share == 0.0)
            break;

        points.push_back(uv + delta * share);
        const Point& next = points.back();
        direction = surface.point_at(next[0], next[1]) - surface.point_at(uv[0], uv[1]);

        if (share < 1.0)
            break;
    }

    return points;
}

inline std::vector<Point> LamellaGridshell::compute_curve(const Point& seed, const Vector& hint) const {

    const Vector slope = compute_slope(seed, hint);
    const Vector along = compute_tangent(seed, slope);
    std::vector<Point> points = compute_path(seed, -along);
    const std::vector<Point> ahead = compute_path(seed, along);
    std::reverse(points.begin(), points.end());
    points.insert(points.end(), ahead.begin() + 1, ahead.end());

    return points;
}

inline std::vector<std::vector<Point>> LamellaGridshell::compute_family(int family, int count) const {

    const std::pair<double, double> domain_u = surface.domain(0);
    const std::pair<double, double> domain_v = surface.domain(1);
    const Point centre((domain_u.first + domain_u.second) / 2.0, (domain_v.first + domain_v.second) / 2.0, 0.0);
    const std::vector<Vector> directions = compute_directions(centre);
    const Vector spine_direction = compute_tangent(centre, directions[1 - family]);
    const std::vector<Point> spine = compute_curve(centre, spine_direction);
    std::vector<double> lengths{0.0};

    for (size_t k = 0; k + 1 < spine.size(); k++) {
        const Point start = surface.point_at(spine[k][0], spine[k][1]);
        const Point end = surface.point_at(spine[k + 1][0], spine[k + 1][1]);
        lengths.push_back(lengths.back() + (end - start).magnitude());
    }

    std::vector<std::vector<Point>> lamellas;
    size_t k = 0;

    for (int i = 0; i < count; i++) {
        const double at = lengths.back() * (i + 0.5) / count;

        while (k + 2 < lengths.size() && lengths[k + 1] < at)
            k++;

        const double fraction = (at - lengths[k]) / (lengths[k + 1] - lengths[k]);
        const Point seed = spine[k] + (spine[k + 1] - spine[k]) * fraction;
        const Point start = surface.point_at(spine[k][0], spine[k][1]);
        const Point end = surface.point_at(spine[k + 1][0], spine[k + 1][1]);
        const Vector normal = surface.normal_at(seed[0], seed[1]);
        const Vector hint = normal.cross(end - start);
        lamellas.push_back(compute_curve(seed, hint));
    }

    return lamellas;
}

inline Polyline LamellaGridshell::compute_polyline(const std::vector<Point>& points) const {

    std::vector<Point> on_surface;

    for (const Point& uv : points)
        on_surface.push_back(surface.point_at(uv[0], uv[1]));

    return Polyline(on_surface);
}

// ═══════════════════════════════════════════════════════════════════════════
// Crossings
// ═══════════════════════════════════════════════════════════════════════════

inline Plane LamellaGridshell::compute_frame(const std::vector<Point>& points, double along, const Point& uv) const {

    const size_t i = std::min(static_cast<size_t>(along), points.size() - 2);
    const Point start = surface.point_at(points[i][0], points[i][1]);
    const Point end = surface.point_at(points[i + 1][0], points[i + 1][1]);
    const Vector slope = compute_slope(uv, end - start);
    const Vector tangent = compute_tangent(uv, slope);
    const Point origin = surface.point_at(uv[0], uv[1]);
    const Vector normal = surface.normal_at(uv[0], uv[1]);

    return Plane(origin, tangent, normal.cross(tangent));
}

inline std::vector<Crossing> LamellaGridshell::compute_crossings(const std::vector<std::vector<Point>>& tops, const std::vector<std::vector<Point>>& bottoms) const {

    std::vector<Crossing> crossings;

    for (size_t a = 0; a < tops.size(); a++)
        for (size_t b = 0; b < bottoms.size(); b++)
            for (size_t i = 0; i + 1 < tops[a].size(); i++)
                for (size_t j = 0; j + 1 < bottoms[b].size(); j++) {
                    const Vector ahead = tops[a][i + 1] - tops[a][i];
                    const Vector across = bottoms[b][j + 1] - bottoms[b][j];
                    const Vector offset = bottoms[b][j] - tops[a][i];
                    const double denominator = ahead[0] * across[1] - ahead[1] * across[0];

                    if (std::abs(denominator) < 1e-30)
                        continue;

                    const double top = (offset[0] * across[1] - offset[1] * across[0]) / denominator;
                    const double bottom = (offset[0] * ahead[1] - offset[1] * ahead[0]) / denominator;

                    if (top < 0.0 || top >= 1.0 || bottom < 0.0 || bottom >= 1.0)
                        continue;

                    const Point uv = tops[a][i] + ahead * top;
                    const Plane top_frame = compute_frame(tops[a], i + top, uv);
                    const Plane bottom_frame = compute_frame(bottoms[b], j + bottom, uv);
                    crossings.push_back(Crossing{a, b, i + top, j + bottom, top_frame, bottom_frame});
                }

    return crossings;
}

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

inline std::vector<Plane> LamellaGridshell::compute_stations(const std::vector<Point>& points, const std::vector<std::pair<double, Plane>>& marks) const {

    std::vector<Plane> stations;

    for (size_t k = 0; k < points.size(); k++) {
        const Plane frame = compute_frame(points, static_cast<double>(k), points[k]);
        bool free = true;

        for (const std::pair<double, Plane>& mark : marks)
            free = free && (frame.origin() - mark.second.origin()).magnitude() > 2.0 * lamella.gap;

        if (free)
            stations.push_back(frame);

        for (const std::pair<double, Plane>& mark : marks) {
            if (mark.first < k || mark.first >= k + 1)
                continue;

            const Plane& crossing = mark.second;

            for (int side = -1; side <= 1; side++) {
                const Point origin = crossing.origin() + crossing.x_axis() * (side * lamella.gap);
                stations.emplace_back(origin, crossing.x_axis(), crossing.y_axis());
            }
        }
    }

    const Plane first = stations.front();
    const Plane last = stations.back();
    const Point before = first.origin() - first.x_axis() * lamella.gap;
    const Point after = last.origin() + last.x_axis() * lamella.gap;
    stations.insert(stations.begin(), Plane(before, first.x_axis(), first.y_axis()));
    stations.emplace_back(after, last.x_axis(), last.y_axis());

    return stations;
}

inline std::shared_ptr<Beam> LamellaGridshell::compute_board(
    const std::vector<Plane>& stations,
    double lift,
    double shift,
    const std::string& name
) const {

    std::vector<Point> points;
    std::vector<Vector> directions;

    for (const Plane& station : stations) {
        points.push_back(station.origin());
        directions.push_back(station.z_axis());
    }

    directions.pop_back();
    const Polyline rectangle = profile_rectangle(lamella.thickness, lamella.height)[0];
    const Polyline section = rectangle.translated(Vector(shift, lift, 0.0));

    return std::make_shared<Beam>(
        Polyline(points),
        std::vector<Polyline>{section},
        directions,
        name
    );
}

inline std::shared_ptr<Column> LamellaGridshell::compute_stud(const Crossing& crossing) const {

    const Plane& top = crossing.top_frame;
    const Vector normal = top.z_axis();
    const Vector across = top.y_axis();
    const Vector& bottom_across = crossing.bottom_frame.y_axis();
    const Vector other = bottom_across.dot(normal.cross(across)) < 0.0 ? -bottom_across : bottom_across;
    const Vector difference = (other - across).normalized();
    const Vector sum = (across + other).normalized();
    const std::vector<Vector> flats = across.dot(other) >= 0.0
        ? std::vector<Vector>{across, other, difference, -across, -other, -difference}
        : std::vector<Vector>{across, sum, other, -across, -sum, -other};

    const double reach = lamella.spacing / 2.0 + lamella.height / 2.0 + lamella.overrun;
    const Point base = top.origin() - normal * reach;
    const Point tip = top.origin() + normal * reach;
    std::vector<Point> points;

    for (size_t i = 0; i <= flats.size(); i++) {
        const Vector& first = flats[i % flats.size()];
        const Vector& second = flats[(i + 1) % flats.size()];
        points.push_back(base + (first + second) * (lamella.gap / 2.0 / (1.0 + first.dot(second))));
    }

    return std::make_shared<Column>(
        Line::from_points(base, tip),
        Polyline(points),
        fmt::format("stud_{}_{}", crossing.top, crossing.bottom)
    );
}

} // namespace wood_gridshell
