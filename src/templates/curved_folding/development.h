#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The development of a ruled strip between two curves, unfolded flat triangle by triangle, with its distortion and twist.
class Development {
public:
    const size_t samples; // Rulings the strip is unfolded with, from the start of the curves to their end.
    Polyline outline; // The flat outline, closed: the first curve's development, the last ruling, the second curve's back and the first ruling.
    double distortion; // The largest relative change of a quad's unshared diagonal, 0 on a developable strip.
    double twist; // The largest |ruling . (tangent_a x tangent_b)| of unit vectors, 0 on a developable strip.

    /// The development of the strip between a and b, unfolded with the given number of rulings.
    Development(const NurbsCurve& a, const NurbsCurve& b, size_t samples = 64);

private:
    /// The point at the given distances from two flat points, on the side of their line away from a third point.
    static Point third_corner(
        const Point& from,
        const Point& to,
        double from_distance,
        double to_distance,
        const Point& away
    );
};

inline Development::Development(const NurbsCurve& a, const NurbsCurve& b, size_t samples)
    : samples(std::max<size_t>(samples, 2)),
      distortion(0.0),
      twist(0.0) {

    // the rulings: the two curves sampled at equal normalized parameters, and the twist of each out of its tangents' plane
    std::vector<Point> starts;
    std::vector<Point> ends;

    for (size_t k = 0; k <= this->samples; k++) {
        const double t = static_cast<double>(k) / static_cast<double>(this->samples);
        const double t_a = a.domain_start() + t * (a.domain_end() - a.domain_start());
        const double t_b = b.domain_start() + t * (b.domain_end() - b.domain_start());
        const Point start = a.point_at(t_a);
        const Point end = b.point_at(t_b);
        starts.push_back(start);
        ends.push_back(end);

        Vector ruling = end - start;
        Vector tangent_a = a.tangent_at(t_a);
        Vector tangent_b = b.tangent_at(t_b);
        ruling.normalize_self();
        tangent_a.normalize_self();
        tangent_b.normalize_self();
        twist = std::max(twist, std::abs(ruling.dot(tangent_a.cross(tangent_b))));
    }

    // the unfolding: the first ruling on the y axis, then every quad laid flat on its first triangle, and its unshared diagonal's error
    std::vector<Point> flat_starts = {Point(0.0, 0.0, 0.0)};
    std::vector<Point> flat_ends = {Point(0.0, starts[0].distance(ends[0]), 0.0)};

    for (size_t k = 1; k <= this->samples; k++) {
        const Point away_start = k == 1 ? Point(-1.0, 0.0, 0.0) : flat_ends[k - 2];
        const Point flat_start = third_corner(
            flat_starts[k - 1],
            flat_ends[k - 1],
            starts[k - 1].distance(starts[k]),
            ends[k - 1].distance(starts[k]),
            away_start
        );
        const Point flat_end = third_corner(
            flat_ends[k - 1],
            flat_start,
            ends[k - 1].distance(ends[k]),
            starts[k].distance(ends[k]),
            flat_starts[k - 1]
        );
        flat_starts.push_back(flat_start);
        flat_ends.push_back(flat_end);

        const double diagonal = starts[k - 1].distance(ends[k]);
        distortion = std::max(distortion, std::abs(flat_starts[k - 1].distance(flat_end) - diagonal) / diagonal);
    }

    // the outline: along the first curve, back along the second, closed
    std::vector<Point> points = flat_starts;
    points.insert(points.end(), flat_ends.rbegin(), flat_ends.rend());
    points.push_back(flat_starts[0]);
    outline = Polyline(points);
}

inline Point Development::third_corner(
    const Point& from,
    const Point& to,
    double from_distance,
    double to_distance,
    const Point& away
) {

    const double base = from.distance(to);
    const double along = (from_distance * from_distance - to_distance * to_distance + base * base) / (2.0 * base);
    const double across = std::sqrt(std::max(0.0, from_distance * from_distance - along * along));

    Vector axis = to - from;
    axis.normalize_self();
    const Vector side(-axis[1], axis[0], 0.0);
    const Point foot = from + axis * along;
    const Point left = foot + side * across;
    const Point right = foot - side * across;

    return left.distance(away) > right.distance(away) ? left : right;
}
