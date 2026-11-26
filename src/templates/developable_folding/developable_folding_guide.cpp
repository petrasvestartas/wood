#include "src/templates/developable_folding/developable_folding_guide.h"

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CurvedFoldGuide
// ═══════════════════════════════════════════════════════════════════════════
CurvedFoldGuide CurvedFoldGuide::tent_roof() {
    return CurvedFoldGuide();
}

// ═══════════════════════════════════════════════════════════════════════════
// CurvedCreaseUnfoldingGuide: creases and planes
// ═══════════════════════════════════════════════════════════════════════════
NurbsCurve CurvedCreaseUnfoldingGuide::catenary(double span, double rise, size_t points) {

    double low = 1e-3 * span;
    double high = 1e3 * span;

    for (int iter = 0; iter < 200; iter++) {
        const double parameter = 0.5 * (low + high);
        if (parameter * (std::cosh(0.5 * span / parameter) - 1.0) > rise)
            low = parameter;
        else
            high = parameter;
    }

    const double parameter = 0.5 * (low + high);
    const size_t steps = std::max<size_t>(points, 8);
    std::vector<Point> sampled;

    for (size_t k = 0; k <= steps; k++) {
        const double x = -0.5 * span + span * static_cast<double>(k) / static_cast<double>(steps);
        sampled.push_back(Point(x, 0.0, rise - parameter * (std::cosh(x / parameter) - 1.0)));
    }

    return NurbsCurve::create_interpolated(sampled, CurveNurbsKnotStyle::Uniform);
}

std::vector<Plane> CurvedCreaseUnfoldingGuide::vault(
    const Point& origin,
    const Vector& along,
    const Vector& axis,
    size_t count,
    double spacing,
    double step
) {

    const Vector a = along.normalized();
    const Vector k = (axis - a * axis.dot(a)).normalized();
    const Vector up = a.cross(k).normalized();
    const size_t half_count = count / 2;

    std::vector<double> positive = {0.0};
    for (size_t i = 0; i < count - half_count; i++) {
        const double pair = static_cast<double>(i / 2) + 0.5;
        const double delta = (i % 2 == 0) ? (spacing + pair * step) : (spacing - pair * step);
        positive.push_back(positive.back() + delta);
    }

    std::vector<double> negative = {0.0};
    for (size_t i = 0; i < half_count; i++) {
        const double pair = static_cast<double>(i / 2) + 0.5;
        const double delta = (i % 2 == 0) ? (spacing + pair * step) : (spacing - pair * step);
        negative.push_back(negative.back() - delta);
    }

    std::vector<Plane> planes;
    for (size_t i = negative.size(); i > 1; i--)
        planes.push_back(Plane(origin + a * negative[i - 1], k, up));
    for (double offset : positive)
        planes.push_back(Plane(origin + a * offset, k, up));

    return planes;
}

std::vector<Plane> CurvedCreaseUnfoldingGuide::fan(
    const Point& origin, const Vector& axis, const Vector& first, size_t count, double angle
) {

    const Vector k = axis.normalized();
    const Vector start = (first - k * first.dot(k)).normalized();
    std::vector<Plane> planes;

    for (size_t i = 0; i <= count; i++) {
        const double turn = angle * (static_cast<double>(i) - 0.5 * static_cast<double>(count));
        const Vector radial = start * std::cos(turn) + k.cross(start) * std::sin(turn);
        planes.push_back(Plane(origin, radial, k));
    }

    return planes;
}

std::vector<Plane> CurvedCreaseUnfoldingGuide::zigzag(
    const Point& origin,
    const Vector& along,
    const Vector& axis,
    size_t count,
    double spacing,
    double tilt
) {

    const Vector a = along.normalized();
    const Vector k = (axis - a * axis.dot(a)).normalized();
    const long middle = static_cast<long>(count / 2);
    std::vector<Plane> planes;

    for (size_t i = 0; i <= count; i++) {
        const long offset = static_cast<long>(i) - middle;
        const long phase = ((offset % 4) + 4) % 4;
        const double turn = phase == 1 ? tilt : phase == 3 ? -tilt : 0.0;
        const Vector normal = a * std::cos(turn) + k.cross(a) * std::sin(turn);
        planes.push_back(
            Plane(origin + a * (spacing * static_cast<double>(offset)), k, normal.cross(k))
        );
    }

    return planes;
}

std::vector<Plane> CurvedCreaseUnfoldingGuide::leaning(
    const Point& origin,
    const Vector& along,
    const Vector& axis,
    size_t count,
    double spacing,
    double lean
) {

    const Vector a = along.normalized();
    const Vector k = (axis - a * axis.dot(a)).normalized();
    const double middle = 0.5 * static_cast<double>(count);
    std::vector<Plane> planes;

    for (size_t i = 0; i <= count; i++) {
        const double offset = static_cast<double>(i) - middle;
        const double turn = lean * offset;
        const Vector normal = a * std::cos(turn) + k.cross(a) * std::sin(turn);
        planes.push_back(Plane(origin + a * (spacing * offset), k, normal.cross(k)));
    }

    return planes;
}

// ═══════════════════════════════════════════════════════════════════════════
// CurvedCreaseUnfoldingGuide: presets
// ═══════════════════════════════════════════════════════════════════════════
CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::monoclastic() {
    return CurvedCreaseUnfoldingGuide();
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::synclastic() {

    CurvedCreaseUnfoldingGuide guide;
    guide.planes =
        vault(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 12, 450.0, 95.0);
    return guide;
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::anticlastic() {

    CurvedCreaseUnfoldingGuide guide;
    guide.planes =
        vault(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 12, 450.0, -95.0);
    return guide;
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::catenary_vault() {

    CurvedCreaseUnfoldingGuide guide;
    guide.crease = catenary(6000.0, 2800.0);
    guide.planes =
        vault(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 8, 420.0, 0.0);
    return guide;
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::unfolding_sequence() {

    CurvedCreaseUnfoldingGuide guide = catenary_vault();
    guide.deployments = {0.0, 0.35, 0.70, 1.0};
    return guide;
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::alternating() {

    CurvedCreaseUnfoldingGuide guide;
    guide.planes = zigzag(
        Point(0.0, 0.0, 2250.0),
        Vector(0.0, 1.0, 0.0),
        Vector(1.0, 0.0, 0.0),
        12,
        450.0,
        5.0 * Tolerance::TO_RADIANS
    );
    return guide;
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::level_crown() {

    CurvedCreaseUnfoldingGuide guide;
    guide.planes = leaning(
        Point(0.0, 0.0, 4500.0),
        Vector(0.0, 1.0, 0.0),
        Vector(1.0, 0.0, 0.0),
        6,
        450.0,
        -3.0 * Tolerance::TO_RADIANS
    );
    return guide;
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::asymmetric() {

    CurvedCreaseUnfoldingGuide guide;
    guide.planes = zigzag(
        Point(0.0, 0.0, 0.0),
        Vector(0.0, 1.0, 0.0),
        Vector(0.0, 0.0, 1.0),
        12,
        450.0,
        3.0 * Tolerance::TO_RADIANS
    );
    return guide;
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::dome() {

    CurvedCreaseUnfoldingGuide guide;
    guide.crease = Primitives::arc(
        Point(6000.0, 0.0, 0.0),
        Point(6000.0 * std::cos(0.7), 0.0, 6000.0 * std::sin(0.7)),
        Point(6000.0 * std::cos(1.4), 0.0, 6000.0 * std::sin(1.4))
    );
    guide.opening = 60.0 * Tolerance::TO_RADIANS;
    guide.planes =
        fan(Point(0.0, 0.0, 0.0),
            Vector(0.0, 0.0, 1.0),
            Vector(1.0, 0.0, 0.0),
            24,
            15.0 * Tolerance::TO_RADIANS);
    guide.thickness = 40.0;
    return guide;
}

CurvedCreaseUnfoldingGuide CurvedCreaseUnfoldingGuide::funnel() {

    CurvedCreaseUnfoldingGuide guide;
    guide.crease = Primitives::arc(
        Point(800.0, 0.0, 0.0),
        Point(
            5800.0 - 5000.0 * std::cos(0.25 * Tolerance::PI),
            0.0,
            5000.0 * std::sin(0.25 * Tolerance::PI)
        ),
        Point(5800.0, 0.0, 5000.0)
    );
    guide.opening = 75.0 * Tolerance::TO_RADIANS;
    guide.planes =
        fan(Point(0.0, 0.0, 0.0),
            Vector(0.0, 0.0, 1.0),
            Vector(1.0, 0.0, 0.0),
            32,
            11.25 * Tolerance::TO_RADIANS);
    guide.thickness = 40.0;
    return guide;
}

// ═══════════════════════════════════════════════════════════════════════════
// CreaseQuadrantsGuide and CurvedCreaseTubeGuide
// ═══════════════════════════════════════════════════════════════════════════
CreaseQuadrantsGuide CreaseQuadrantsGuide::arc_crease() {
    return CreaseQuadrantsGuide();
}

CurvedCreaseTubeGuide CurvedCreaseTubeGuide::cellular_vault() {
    return CurvedCreaseTubeGuide();
}

// ═══════════════════════════════════════════════════════════════════════════
// SpaceCurveFoldGuide
// ═══════════════════════════════════════════════════════════════════════════
Polyline SpaceCurveFoldGuide::helix(double radius, double pitch, double turns, size_t points) {

    std::vector<Point> sampled;

    for (size_t i = 0; i <= points; i++) {
        const double turn = turns * static_cast<double>(i) / static_cast<double>(points);
        const double t = 2.0 * Tolerance::PI * turn;
        sampled.push_back(Point(radius * std::cos(t), radius * std::sin(t), pitch * turn));
    }

    return Polyline(sampled);
}

Polyline SpaceCurveFoldGuide::wave(double radius, double sweep, double rise, double waves, size_t points) {

    std::vector<Point> sampled;

    for (size_t i = 0; i <= points; i++) {
        const double along = static_cast<double>(i) / static_cast<double>(points);
        const double t = sweep * (along - 0.5);
        const double z = 0.5 * rise * (1.0 - std::cos(2.0 * Tolerance::PI * waves * along));
        sampled.push_back(Point(radius * std::sin(t), radius * (std::cos(t) - 1.0), z));
    }

    return Polyline(sampled);
}

SpaceCurveFoldGuide SpaceCurveFoldGuide::helix_fold() {
    return SpaceCurveFoldGuide();
}

SpaceCurveFoldGuide SpaceCurveFoldGuide::wave_fold() {

    SpaceCurveFoldGuide guide;
    guide.curve = wave(3000.0, 3.0, 800.0, 1.0, 40);
    return guide;
}

SpaceCurveFoldGuide SpaceCurveFoldGuide::zigzag_fold() {

    SpaceCurveFoldGuide guide;
    guide.zigzag = 150.0;
    guide.deployments = {1.0, 0.5, 0.2, 0.05, 0.02};
    guide.thickness = 0.0;
    return guide;
}

SpaceCurveFoldGuide SpaceCurveFoldGuide::smooth_helix_fold() {

    SpaceCurveFoldGuide guide;
    guide.curve = helix(2000.0, 3000.0, 1.5, 180);
    guide.deployments = {1.0, 0.5, 0.25, 0.1};
    guide.smooth = true;
    return guide;
}

// ═══════════════════════════════════════════════════════════════════════════
// CurvedMiuraGuide
// ═══════════════════════════════════════════════════════════════════════════
CurvedMiuraGuide CurvedMiuraGuide::helix_miura() {
    return CurvedMiuraGuide();
}

CurvedMiuraGuide CurvedMiuraGuide::wave_miura() {

    CurvedMiuraGuide guide;
    guide.curve = SpaceCurveFoldGuide::wave(3000.0, 3.0, 800.0, 1.0, 40);
    return guide;
}

CurvedMiuraGuide CurvedMiuraGuide::zigzag_miura() {

    CurvedMiuraGuide guide;
    guide.zigzag = 150.0;
    guide.deployments = {1.0, 0.5, 0.2, 0.05, 0.02};
    guide.thickness = 0.0;
    return guide;
}

}
