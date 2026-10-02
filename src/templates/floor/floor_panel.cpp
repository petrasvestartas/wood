#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor::geometry {

const double SCAN_STEP = 0.5; // degrees between the sweep directions scanned for rule A's root
const double SCAN_RANGE = 85.0; // degrees either side of n0 - n1 the scan covers: the second root, where both ribs shift alike, lies at 90
const double GRAZING = 1e-3; // |n . r| below which a sweep runs along a rib face and is skipped
const size_t BISECTIONS = 200; // halvings of the bracket, far past the last bit of the angle

/// The vector flattened to the plan.
static Vector flat(const Vector& vector) {
    return Vector(vector[0], vector[1], 0.0);
}

/// The plan unit vector turned by degrees about z from the reference.
static Vector turned(const Vector& reference, double degrees) {

    const double a = degrees * M_PI / 180.0;
    const Vector x = flat(reference).normalized();

    return Vector(x[0] * std::cos(a) - x[1] * std::sin(a), x[0] * std::sin(a) + x[1] * std::cos(a), 0.0);
}

/// The polyline's points projected along the axis onto the plane.
static Polyline along(const Polyline& polyline, const Plane& plane, const Vector& axis) {
    return polyline.transformed(Xform::project_to_plane_by_axis(plane, axis));
}

/// The farthest pair of same-index vertices of two polylines, mm.
static double largest_shift(const Polyline& a, const Polyline& b) {

    double shift = 0.0;

    for (size_t i = 0; i < std::min(a.point_count(), b.point_count()); i++)
        shift = std::max(shift, (a.get_point(i) - b.get_point(i)).magnitude());

    return shift;
}

// ═══════════════════════════════════════════════════════════════════════════
// Rule A
// ═══════════════════════════════════════════════════════════════════════════

/// The rib sweep's two shifts: how far along r each rib's outer face trace moves to reach its central face, thickness / (n . r).
static std::array<double, 2> shifts(const std::array<Vector, 2>& normals, double thickness, const Vector& r) {
    return {thickness / normals[0].dot(r), thickness / normals[1].dot(r)};
}

/// Rule A's closure for one sweep: the sine between the plan chords joining the two central traces at the start and at the vertex, zero when one ruling joins both.
static double closure(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& r) {

    const std::array<double, 2> a = shifts(normals, thickness, r);
    const size_t n = shadows[0].point_count() - 1;
    const Vector start = flat((shadows[1].get_point(0) + r * a[1]) - (shadows[0].get_point(0) + r * a[0]));
    const Vector vertex = flat((shadows[1].get_point(n) + r * a[1]) - (shadows[0].get_point(n) + r * a[0]));

    return start.cross(vertex)[2] / (start.magnitude() * vertex.magnitude());
}

/// Whether the sweep at degrees from the reference crosses both rib faces, and on which side of each.
static bool sweep_sides(const std::array<Vector, 2>& normals, const Vector& reference, double degrees, std::array<bool, 2>& sides) {

    const Vector r = turned(reference, degrees);
    sides = {normals[0].dot(r) > 0.0, normals[1].dot(r) > 0.0};

    return std::abs(normals[0].dot(r)) > GRAZING && std::abs(normals[1].dot(r)) > GRAZING;
}

/// The root of the closure between two scanned angles by bisection.
static double bisect(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& reference, double lo, double hi) {

    double f_lo = closure(shadows, normals, thickness, turned(reference, lo));

    for (size_t i = 0; i < BISECTIONS && lo != hi; i++) {
        const double mid = 0.5 * (lo + hi);
        const double f_mid = closure(shadows, normals, thickness, turned(reference, mid));

        if (f_mid == 0.0 || mid == lo || mid == hi)
            return mid;

        if ((f_mid > 0.0) == (f_lo > 0.0)) {
            lo = mid;
            f_lo = f_mid;
        } else
            hi = mid;
    }

    return 0.5 * (lo + hi);
}

/// Rule A's rib sweep: the root of the closure nearest the reference n0 - n1, scanned in steps without crossing a rib face and refined by bisection; the reference itself when no root is bracketed.
static Vector rib_sweep(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& reference) {

    double best = 0.0;
    bool found = false;

    for (double lo = -SCAN_RANGE; lo < SCAN_RANGE; lo += SCAN_STEP) {
        const double hi = lo + SCAN_STEP;
        std::array<bool, 2> sides_lo;
        std::array<bool, 2> sides_hi;

        if (!sweep_sides(normals, reference, lo, sides_lo) || !sweep_sides(normals, reference, hi, sides_hi) || sides_lo != sides_hi)
            continue;

        const double f_lo = closure(shadows, normals, thickness, turned(reference, lo));
        const double f_hi = closure(shadows, normals, thickness, turned(reference, hi));

        if ((f_lo > 0.0) == (f_hi > 0.0) && f_lo != 0.0 && f_hi != 0.0)
            continue;

        const double root = bisect(shadows, normals, thickness, reference, lo, hi);

        if (!found || std::abs(root) < std::abs(best))
            best = root;

        found = true;
    }

    return turned(reference, found ? best : 0.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Layers
// ═══════════════════════════════════════════════════════════════════════════

/// compas_tf's central layers: rib 0's shadow offsets projected along the ruling onto both central faces.
static std::array<std::array<Polyline, 3>, 2> compas_layers(const std::array<Polyline, 2>& soffits, const std::array<Polyline, 3>& shadow0, const std::array<Plane, 2>& faces, const Vector& ruling) {

    std::array<std::array<Polyline, 3>, 2> traces;

    for (size_t k = 0; k < 2; k++)
        traces[k] = {soffits[k], along(shadow0[1], faces[k], ruling), along(shadow0[2], faces[k], ruling)};

    return traces;
}

/// The model's central layers: the panel soffit's offsets by tsections and twice that in the panel's own cross-section, projected along the ruling onto both central faces.
static std::array<std::array<Polyline, 3>, 2> section_layers(const std::array<Polyline, 2>& soffits, const std::array<Plane, 2>& faces, const Vector& ruling, double tsections) {

    const Polyline section = along(soffits[0], Plane::from_point_normal(soffits[0].get_point(0), ruling), ruling);
    const Polyline offset1 = offset_polyline(section, tsections);
    const Polyline offset2 = offset_polyline(section, 2.0 * tsections);
    std::array<std::array<Polyline, 3>, 2> traces;

    for (size_t k = 0; k < 2; k++)
        traces[k] = {soffits[k], along(offset1, faces[k], ruling), along(offset2, faces[k], ruling)};

    return traces;
}

CentralPanel central_panel(const ConstructionPlanes& cp, const std::vector<std::array<Polyline, 3>>& parabolas, const FloorSizes& sizes, CentralLayers layers) {

    const std::array<Plane, 2> faces = {cp.inner_ribs[0][1], cp.inner_ribs[1][1]};
    const std::array<Vector, 2> normals = {faces[0].z_axis(), faces[1].z_axis()};
    const std::array<Polyline, 2> shadows = {parabolas[2][0], parabolas[3][0]};
    const Vector reference = flat(normals[0] - normals[1]).normalized();

    CentralPanel panel;
    panel.rib_sweep = rib_sweep(shadows, normals, sizes.inner_ribs, reference);
    const std::array<Polyline, 2> soffits = {along(shadows[0], faces[0], panel.rib_sweep), along(shadows[1], faces[1], panel.rib_sweep)};
    panel.ruling = flat(soffits[1].get_point(0) - soffits[0].get_point(0)).normalized();

    for (size_t k = 0; k < 2; k++)
        panel.obliqueness[k] = std::acos(std::clamp(std::abs(normals[k].dot(panel.rib_sweep)), 0.0, 1.0)) * 180.0 / M_PI;

    panel.residual = largest_shift(along(soffits[0], faces[1], panel.ruling), soffits[1]);

    const std::array<std::array<Polyline, 3>, 2> compas = compas_layers(soffits, parabolas[2], faces, panel.ruling);
    const std::array<std::array<Polyline, 3>, 2> section = section_layers(soffits, faces, panel.ruling, sizes.tsections);

    for (size_t j = 0; j < 2; j++)
        panel.layer_shift[j] = std::max(largest_shift(compas[0][j + 1], section[0][j + 1]), largest_shift(compas[1][j + 1], section[1][j + 1]));

    panel.traces = layers == CentralLayers::compas ? compas : section;

    return panel;
}

}
