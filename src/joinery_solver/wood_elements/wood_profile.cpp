#include "pch.h"
#include "wood_profile.h"

namespace wood_session {

using namespace session_cpp;

/// A closed loop from plan corners.
static Polyline loop(std::vector<Point> points) {

    points.push_back(points.front());

    return Polyline(points);
}

/// A counter-clockwise rectangle centred on the origin.
static Polyline box(double x, double y, double width, double depth) {
    return loop({Point(x - width / 2.0, y - depth / 2.0, 0.0), Point(x + width / 2.0, y - depth / 2.0, 0.0), Point(x + width / 2.0, y + depth / 2.0, 0.0), Point(x - width / 2.0, y + depth / 2.0, 0.0)});
}

// ═══════════════════════════════════════════════════════════════════════════
// Profiles
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Polyline> profile_rectangle(double width, double depth) {
    return {box(0.0, 0.0, width, depth)};
}

std::vector<Polyline> profile_round(double diameter, int segments) {

    std::vector<Point> points;
    for (int i = 0; i < segments; i++)
        points.emplace_back(diameter / 2.0 * std::cos(Tolerance::TWO_PI * (i + 0.5) / segments), diameter / 2.0 * std::sin(Tolerance::TWO_PI * (i + 0.5) / segments), 0.0);

    return {loop(points)};
}

std::vector<Polyline> profile_w(double width, double depth, double flange, double web) {

    const double x = width / 2.0;
    const double y = depth / 2.0;
    const double w = web / 2.0;
    const double f = y - flange;

    return {loop({Point(-x, -y, 0.0), Point(x, -y, 0.0), Point(x, -f, 0.0), Point(w, -f, 0.0), Point(w, f, 0.0), Point(x, f, 0.0), Point(x, y, 0.0), Point(-x, y, 0.0), Point(-x, f, 0.0), Point(-w, f, 0.0), Point(-w, -f, 0.0), Point(-x, -f, 0.0)})};
}

std::vector<Polyline> profile_hss(double width, double depth, double thickness) {
    return {box(0.0, 0.0, width, depth), box(0.0, 0.0, width - 2.0 * thickness, depth - 2.0 * thickness).reversed()};
}

std::vector<Polyline> profile_double(double width, double depth, double gap) {
    return {box(-(width + gap) / 2.0, 0.0, width, depth), box((width + gap) / 2.0, 0.0, width, depth)};
}

std::vector<Polyline> profile_slab_band(double width, double depth) {
    return {box(0.0, 0.0, width, depth)};
}

std::vector<Polyline> profile_t(double width, double depth, double web, double flange) {

    const double x = width / 2.0;
    const double y = depth / 2.0;
    const double w = web / 2.0;
    const double f = y - flange;

    return {loop({Point(-w, -y, 0.0), Point(w, -y, 0.0), Point(w, f, 0.0), Point(x, f, 0.0), Point(x, y, 0.0), Point(-x, y, 0.0), Point(-x, f, 0.0), Point(-w, f, 0.0)})};
}

std::pair<double, double> compute_size(const std::vector<Polyline>& profile) {

    if (profile.empty())
        return {0.0, 0.0};

    double x0 = 0.0;
    double x1 = 0.0;
    double y0 = 0.0;
    double y1 = 0.0;
    for (const Polyline& ring : profile)
        for (const Point& point : ring.get_points()) {
            x0 = std::min(x0, point[0]);
            x1 = std::max(x1, point[0]);
            y0 = std::min(y0, point[1]);
            y1 = std::max(y1, point[1]);
        }

    return {x1 - x0, y1 - y0};
}

double compute_support(const std::vector<Polyline>& profile, const Vector& direction) {

    double support = 0.0;
    if (profile.empty())
        return support;

    for (const Point& point : profile[0].get_points())
        support = std::max(support, point[0] * direction[0] + point[1] * direction[1]);

    return support;
}

std::vector<Polyline> compute_scaled(const std::vector<Polyline>& profile, double width, double depth) {

    const std::pair<double, double> size = compute_size(profile);
    const double sx = width > 0.0 && size.first > 0.0 ? width / size.first : 1.0;
    const double sy = depth > 0.0 && size.second > 0.0 ? depth / size.second : 1.0;

    std::vector<Polyline> scaled;
    for (const Polyline& ring : profile) {
        std::vector<Point> points;
        for (const Point& point : ring.get_points())
            points.emplace_back(point[0] * sx, point[1] * sy, 0.0);
        scaled.emplace_back(points);
    }

    return scaled;
}

Polyline profile_section(const Point& at, const Vector& direction, const Vector& up, const Polyline& ring) {

    const Vector along = direction.normalized();
    Vector rise = up.is_parallel_to(along) == 0 ? up : Vector::z_axis();
    if (rise.is_parallel_to(along) != 0)
        rise = Vector::x_axis();

    const Vector side = rise.cross(along).normalized();
    rise = along.cross(side).normalized();

    std::vector<Point> points;
    for (const Point& point : ring.get_points())
        points.push_back(at + side * point[0] + rise * point[1]);

    return Polyline(points);
}

} // namespace wood_session
