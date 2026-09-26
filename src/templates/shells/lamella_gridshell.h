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
    double step = 100.0; // Tracing step along a lamella on the surface; a sample nearer a crossing than twice the gap is dropped.
};

// ═══════════════════════════════════════════════════════════════════════════
// Tracing
// ═══════════════════════════════════════════════════════════════════════════

/// The surface vector of a (du, dv, 0) direction at uv.
inline Vector compute_tangent(const NurbsSurface& surface, const Point& uv, const Vector& direction) {

    const std::vector<Vector> derivatives = surface.evaluate(uv[0], uv[1], 1);

    return derivatives[2] * direction[0] + derivatives[1] * direction[1];
}

/// The two lamella directions at uv as (du, dv, 0), each one unit long on the surface: u and v for curves 0, for curves 1 the asymptotic directions where II(d, d) = 0, none where the Gaussian curvature is positive.
inline std::vector<Vector> compute_directions(const NurbsSurface& surface, int curves, const Point& uv) {

    const std::vector<Vector> derivatives = surface.evaluate(uv[0], uv[1], 2);
    std::vector<Vector> directions;
    if (curves == 0) {
        directions.push_back(Vector(1.0, 0.0, 0.0) / derivatives[3].magnitude());
        directions.push_back(Vector(0.0, 1.0, 0.0) / derivatives[1].magnitude());
        return directions;
    }

    const Vector normal = surface.normal_at(uv[0], uv[1]);
    const double suu = derivatives[5].dot(normal); // II(u, u)
    const double suv = derivatives[4].dot(normal); // II(u, v)
    const double svv = derivatives[2].dot(normal); // II(v, v)
    const double mean = (suu + svv) / 2.0;
    const double radius = std::hypot((suu - svv) / 2.0, suv);
    if (radius < 1e-12 || std::abs(mean) > radius)
        return directions;

    const double phi = std::atan2(suv, (suu - svv) / 2.0);
    const double spread = std::acos(-mean / radius);
    for (const double sign : {-1.0, 1.0}) {
        const Vector direction(std::cos((phi + sign * spread) / 2.0), std::sin((phi + sign * spread) / 2.0), 0.0);
        directions.push_back(direction / compute_tangent(surface, uv, direction).magnitude());
    }

    return directions;
}

/// The lamella direction at uv nearest previous on the surface, turned to agree with it; zero where there is none.
inline Vector compute_slope(const NurbsSurface& surface, int curves, const Point& uv, const Vector& previous) {

    Vector slope(0.0, 0.0, 0.0);
    double best = -1.0;
    for (const Vector& direction : compute_directions(surface, curves, uv)) {
        const double dot = compute_tangent(surface, uv, direction).dot(previous);
        if (std::abs(dot) > best) {
            best = std::abs(dot);
            slope = dot < 0.0 ? -direction : direction;
        }
    }

    return slope;
}

/// The share of delta from uv that stays inside the surface domain, 1 when all of it does.
inline double compute_share(const NurbsSurface& surface, const Point& uv, const Vector& delta) {

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

/// Points in (u, v, 0) from seed along the lamella direction nearest direction: RK4 steps of step mm, the last clipped to the domain edge; ends early where the direction field does.
inline std::vector<Point> compute_path(const NurbsSurface& surface, int curves, const Point& seed, Vector direction, double step) {

    std::vector<Point> points{seed};
    for (int k = 0; k < 100000; k++) {
        const Point uv = points.back();
        const Vector k1 = compute_slope(surface, curves, uv, direction) * step;
        const Vector k2 = compute_slope(surface, curves, uv + k1 * 0.5, direction) * step;
        const Vector k3 = compute_slope(surface, curves, uv + k2 * 0.5, direction) * step;
        const Vector k4 = compute_slope(surface, curves, uv + k3, direction) * step;
        const Vector delta = (k1 + k2 * 2.0 + k3 * 2.0 + k4) / 6.0;
        const double share = compute_share(surface, uv, delta);
        if (k1.magnitude() == 0.0 || share == 0.0)
            break;

        points.push_back(uv + delta * share);
        direction = surface.point_at(points.back()[0], points.back()[1]) - surface.point_at(uv[0], uv[1]);
        if (share < 1.0)
            break;
    }

    return points;
}

/// The lamella through seed both ways along the direction nearest hint, from one end to the other.
inline std::vector<Point> compute_curve(const NurbsSurface& surface, int curves, const Point& seed, const Vector& hint, double step) {

    const Vector along = compute_tangent(surface, seed, compute_slope(surface, curves, seed, hint));
    std::vector<Point> points = compute_path(surface, curves, seed, -along, step);
    const std::vector<Point> ahead = compute_path(surface, curves, seed, along, step);
    std::reverse(points.begin(), points.end());
    points.insert(points.end(), ahead.begin() + 1, ahead.end());

    return points;
}

/// count lamellas of family 0 or 1, seeded at even arc lengths along a spine of the other family through the middle of the domain.
inline std::vector<std::vector<Point>> compute_family(const NurbsSurface& surface, int curves, int family, int count, double step) {

    const Point centre((surface.domain(0).first + surface.domain(0).second) / 2.0, (surface.domain(1).first + surface.domain(1).second) / 2.0, 0.0);
    const std::vector<Point> spine = compute_curve(surface, curves, centre, compute_tangent(surface, centre, compute_directions(surface, curves, centre)[1 - family]), step);

    std::vector<double> lengths{0.0};
    for (size_t k = 0; k + 1 < spine.size(); k++)
        lengths.push_back(lengths.back() + (surface.point_at(spine[k + 1][0], spine[k + 1][1]) - surface.point_at(spine[k][0], spine[k][1])).magnitude());

    std::vector<std::vector<Point>> lamellas;
    size_t k = 0;
    for (int i = 0; i < count; i++) {
        const double at = lengths.back() * (i + 0.5) / count;
        while (k + 2 < lengths.size() && lengths[k + 1] < at)
            k++;

        const Point seed = spine[k] + (spine[k + 1] - spine[k]) * ((at - lengths[k]) / (lengths[k + 1] - lengths[k]));
        const Vector chord = surface.point_at(spine[k + 1][0], spine[k + 1][1]) - surface.point_at(spine[k][0], spine[k][1]);
        lamellas.push_back(compute_curve(surface, curves, seed, surface.normal_at(seed[0], seed[1]).cross(chord), step));
    }

    return lamellas;
}

// ═══════════════════════════════════════════════════════════════════════════
// Crossings
// ═══════════════════════════════════════════════════════════════════════════

/// Where a top lamella crosses a bottom lamella, found in (u, v).
struct Crossing {
    size_t top; // Top lamella.
    size_t bottom; // Bottom lamella.
    double along_top; // Position along the top lamella, segment index plus fraction.
    double along_bottom; // Position along the bottom lamella, segment index plus fraction.
    Point uv; // Surface parameters, w 0.
};

/// Every crossing of the two families, segment against segment in (u, v).
inline std::vector<Crossing> compute_crossings(const std::vector<std::vector<Point>>& tops, const std::vector<std::vector<Point>>& bottoms) {

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
                    if (top >= 0.0 && top < 1.0 && bottom >= 0.0 && bottom < 1.0)
                        crossings.push_back(Crossing{a, b, i + top, j + bottom, tops[a][i] + ahead * top});
                }

    return crossings;
}

/// The frame of a lamella at uv: x its direction nearest the segment of points at along, y the normal cross x, z the normal.
inline Plane compute_frame(const NurbsSurface& surface, int curves, const std::vector<Point>& points, double along, const Point& uv) {

    const size_t i = std::min(static_cast<size_t>(along), points.size() - 2);
    const Vector chord = surface.point_at(points[i + 1][0], points[i + 1][1]) - surface.point_at(points[i][0], points[i][1]);
    const Vector tangent = compute_tangent(surface, uv, compute_slope(surface, curves, uv, chord));

    return Plane(surface.point_at(uv[0], uv[1]), tangent, surface.normal_at(uv[0], uv[1]).cross(tangent));
}

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

/// Frames along one lamella: every traced point not within twice the gap of a crossing, three on the tangent at each crossing, gap apart, so the boards run straight past the stud, and one a gap past each end, so the end sections stand on their own normal.
inline std::vector<Plane> compute_stations(const NurbsSurface& surface, int curves, const std::vector<Point>& points, const std::vector<std::pair<double, Plane>>& marks, const Lamella& lamella) {

    std::vector<Plane> stations;
    for (size_t k = 0; k < points.size(); k++) {
        const Plane frame = compute_frame(surface, curves, points, static_cast<double>(k), points[k]);
        bool free = true;
        for (const std::pair<double, Plane>& mark : marks)
            free = free && (frame.origin() - mark.second.origin()).magnitude() > 2.0 * lamella.gap;

        if (free)
            stations.push_back(frame);

        for (const std::pair<double, Plane>& mark : marks)
            if (mark.first >= k && mark.first < k + 1)
                for (int side = -1; side <= 1; side++)
                    stations.emplace_back(mark.second.origin() + mark.second.x_axis() * (side * lamella.gap), mark.second.x_axis(), mark.second.y_axis());
    }

    const Plane first = stations.front();
    const Plane last = stations.back();
    stations.insert(stations.begin(), Plane(first.origin() - first.x_axis() * lamella.gap, first.x_axis(), first.y_axis()));
    stations.emplace_back(last.origin() + last.x_axis() * lamella.gap, last.x_axis(), last.y_axis());

    return stations;
}

/// One board on the lamella centreline: a section lift along each station's normal and shift across it, so every section is the rectangle the local normal and the normal cross the tangent span.
inline std::shared_ptr<Beam> compute_board(const std::vector<Plane>& stations, double lift, double shift, const Lamella& lamella, const std::string& name) {

    std::vector<Point> points;
    std::vector<Vector> directions;
    for (const Plane& station : stations) {
        points.push_back(station.origin());
        directions.push_back(station.z_axis());
    }

    directions.pop_back();
    const Polyline section = profile_rectangle(lamella.thickness, lamella.height)[0].translated(Vector(shift, lift, 0.0));

    return std::make_shared<Beam>(Polyline(points), std::vector<Polyline>{section}, directions, name);
}

/// A stud along the normal through both layers: a hexagon of three flat pairs gap apart, one against each layer's boards and one across the long corners; a 60 degree crossing gives the regular hexagon.
inline std::shared_ptr<Column> compute_stud(const Plane& top, const Plane& bottom, const Lamella& lamella, const std::string& name) {

    const Vector normal = top.z_axis();
    const Vector across = top.y_axis();
    const Vector other = bottom.y_axis().dot(normal.cross(across)) < 0.0 ? -bottom.y_axis() : bottom.y_axis();
    const std::vector<Vector> flats = across.dot(other) >= 0.0
        ? std::vector<Vector>{across, other, (other - across).normalized(), -across, -other, -(other - across).normalized()}
        : std::vector<Vector>{across, (across + other).normalized(), other, -across, -(across + other).normalized(), -other};

    const double reach = lamella.spacing / 2.0 + lamella.height / 2.0 + lamella.overrun;
    const Point base = top.origin() - normal * reach;
    std::vector<Point> points;
    for (size_t i = 0; i <= flats.size(); i++) {
        const Vector& first = flats[i % flats.size()];
        const Vector& second = flats[(i + 1) % flats.size()];
        points.push_back(base + (first + second) * (lamella.gap / 2.0 / (1.0 + first.dot(second))));
    }

    return std::make_shared<Column>(Line::from_points(base, top.origin() + normal * reach), Polyline(points), name);
}

/// A two-directional lamella gridshell on a NURBS surface: two upright boards gap apart per lamella, the first curve family a layer up the normal, the second a layer down, a hexagonal stud in both gaps at every crossing.
struct Gridshell {
    std::vector<std::shared_ptr<Beam>> top; // lamella_top_i_a and _b per lamella of the first family, spacing / 2 up the normal.
    std::vector<std::shared_ptr<Beam>> bottom; // lamella_bottom_j_a and _b per lamella of the second family, spacing / 2 down the normal.
    std::vector<std::shared_ptr<Column>> studs; // stud_i_j where top lamella i crosses bottom lamella j, flats touching the four boards.
    std::vector<std::vector<Plane>> frames; // Stations of every lamella on the surface, x along it, z the normal; the top lamellas first.

    /// The gridshell on count_top and count_bottom lamellas: curves 0 the u and v iso-curves, 1 the asymptotic curves (Gaussian curvature at most 0), each family seeded along a spine of the other through the middle of the domain.
    static Gridshell from_surface(const NurbsSurface& surface, int curves, int count_top, int count_bottom, const Lamella& lamella) {

        const std::vector<std::vector<Point>> tops = compute_family(surface, curves, 0, count_top, lamella.step);
        const std::vector<std::vector<Point>> bottoms = compute_family(surface, curves, 1, count_bottom, lamella.step);
        std::vector<std::vector<std::pair<double, Plane>>> top_marks(tops.size());
        std::vector<std::vector<std::pair<double, Plane>>> bottom_marks(bottoms.size());
        Gridshell gridshell;

        for (const Crossing& crossing : compute_crossings(tops, bottoms)) {
            const Plane top = compute_frame(surface, curves, tops[crossing.top], crossing.along_top, crossing.uv);
            const Plane bottom = compute_frame(surface, curves, bottoms[crossing.bottom], crossing.along_bottom, crossing.uv);
            top_marks[crossing.top].emplace_back(crossing.along_top, top);
            bottom_marks[crossing.bottom].emplace_back(crossing.along_bottom, bottom);
            gridshell.studs.push_back(compute_stud(top, bottom, lamella, fmt::format("stud_{}_{}", crossing.top, crossing.bottom)));
        }

        for (size_t i = 0; i < tops.size(); i++)
            gridshell.frames.push_back(compute_stations(surface, curves, tops[i], top_marks[i], lamella));

        for (size_t j = 0; j < bottoms.size(); j++)
            gridshell.frames.push_back(compute_stations(surface, curves, bottoms[j], bottom_marks[j], lamella));

        const double lift = lamella.spacing / 2.0;
        const double shift = (lamella.gap + lamella.thickness) / 2.0;
        for (size_t i = 0; i < gridshell.frames.size(); i++) {
            const bool upper = i < tops.size();
            const std::string name = upper ? fmt::format("lamella_top_{}", i) : fmt::format("lamella_bottom_{}", i - tops.size());
            std::vector<std::shared_ptr<Beam>>& layer = upper ? gridshell.top : gridshell.bottom;
            layer.push_back(compute_board(gridshell.frames[i], upper ? lift : -lift, shift, lamella, name + "_a"));
            layer.push_back(compute_board(gridshell.frames[i], upper ? lift : -lift, -shift, lamella, name + "_b"));
        }

        return gridshell;
    }
};

} // namespace wood_gridshell
