#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding: flat plates cut as bands with curved edges, stacked and hinged to their neighbours along those edges, open like an
/// accordion; a curved hinge forces every plate to bend about its rulings, so the stack rises into a corrugated vault, every crease
/// keeping its plane and its curvature growing as the cut edge's over the cosine of the opening, and the hinges alternating between the
/// two faces of the stack (the axis shift) so it folds completely closed. Drawn open, with the plates mitred on their crease planes, on
/// parallel planes or on inclined ones. The given crease is a planar curve on one of the planes; the plate after it is the developable strip of rulings leaving it
/// perpendicular to it at the opening to its plane, toward the curve's concave side, cut by the next plane, and the plate before it is
/// that strip's mirror image in the crease's plane; every further plate is the mirror image of its neighbour in the plane of their shared
/// crease, cut by the plane beyond it. Every crease stays planar and every plate a trimmed developable, cut from a flat band with curved
/// edges. Planes fanned around a horizontal axis above the vault draw its ends in (synclastic), below it spread them; planes tilted alternately about the span articulate the ridges and valleys at the apex or at the supports, about a
/// vertical axis to the sides; planes fanned around a vertical axis close the vault into a corrugated dome or, on a flaring
/// crease, a funnel column.
///
/// Drawn open: with inclined planes the stack folds closed only by bending its rulings, so the plates are mitred, the
/// two faces of every plate cut by the planes of its two creases, where its neighbours meet it as its mirror image.
///
/// Fields: the crease, the opening, the planes, the thickness and the sampling; the plates and the creases are in the session.
class CurvedCreaseUnfolding : public WoodSession {
public:
    const NurbsCurve crease; // The given crease, a planar curve on one of the planes.
    const double opening; // radians, the angle between the rulings and the crease plane.
    const std::vector<Plane> planes; // The crease planes in order, one of them the crease's own, a plate between every two.
    const double thickness; // mm, every plate.
    const size_t samples; // Rulings per plate the creases are interpolated through.

    /// The plates between every two planes as the session named name.
    CurvedCreaseUnfolding(
        const NurbsCurve& crease,
        double opening,
        const std::vector<Plane>& planes,
        double thickness = 40.0,
        const std::string& name = "curved_crease_unfolding",
        size_t samples = 48
    );

    /// Not copied, as a session is not.
    CurvedCreaseUnfolding(const CurvedCreaseUnfolding&) = delete;

    /// Not assigned, as it is not copied.
    CurvedCreaseUnfolding& operator=(const CurvedCreaseUnfolding&) = delete;

    /// The largest relative length error of any plate's development, the proof every plate is cut flat.
    double distortion() const {
        return _distortion;
    }

    /// mm, how far the creases' feet rise above or sink below the given crease's, its ends on the floor: 0 for vertical planes, and
    /// small for planes turned in pairs that cancel; a vault whose planes all lean about the span lifts its supports toward its ends,
    /// every mirror image in a leaning plane tilting the rulings at the feet out of the floor.
    double drift() const {
        return _drift;
    }

    /// The catenary span wide and rise high over the x axis in the xz plane, centred on the origin, interpolated through points.
    static NurbsCurve catenary(double span, double rise, size_t points = 32);

    /// count + 1 planes through the axis from origin, each turned by angle (radians) about the axis from the one before, the middle one
    /// containing first: a dome's or a funnel's meridian planes around a vertical axis, a vault drawn in or spread over a horizontal one.
    static std::vector<Plane> fan(const Point& origin, const Vector& axis, const Vector& first, size_t count, double angle);

    /// count + 1 planes spacing apart along along, the middle one through origin and square to along, every second one from it turned by
    /// tilt and -tilt in turn (radians) about axis through its point, so the turns of the mirror images cancel and the vault runs straight:
    /// plates alternately wider at one end of the crease and at the other (axis vertical), or at the apex and at the supports (axis along
    /// the span, at origin's height).
    static std::vector<Plane> zigzag(const Point& origin, const Vector& along, const Vector& axis, size_t count, double spacing, double tilt);

    /// count + 1 planes spacing apart along along, the middle one through origin and square to along, the i-th from it turned by i lean
    /// (radians) about axis through its point: with axis along the span on the floor the supports keep their spacing and the planes lean
    /// toward the middle at the apex (lean > 0, a synclastic vault) or away from it (lean < 0, anticlastic).
    static std::vector<Plane> leaning(const Point& origin, const Vector& along, const Vector& axis, size_t count, double spacing, double lean);

private:
    double _distortion = 0.0;
    double _drift = 0.0;

    /// One plate's mid-surface: the crease points on its first plane, the rulings' unit directions and the faces' unit normals.
    struct Strip {
        std::vector<Point> points;
        std::vector<Vector> rulings;
        std::vector<Vector> normals;
    };

    /// The plane the crease lies on, the nearest to its ends and middle.
    size_t compute_start() const;

    /// The strip of the plate after the crease: the crease sampled, the rulings at the opening to its plane toward the crease's concave
    /// side and to the side the next plane cuts them on, the normals the tangent crossed with the ruling.
    Strip compute_first(size_t start) const;

    /// The plates from the strip on plane from toward the last plane (step 1) or the first (step -1), plate_<k> between planes k and
    /// k + 1 in the group plates, each one's far crease crease_<k> in the group creases; the strip mirrored in the far plane each time.
    void add_plates(Strip strip, size_t from, int step);

    /// The strip of the plate beyond the plane: the rulings mirrored in it and turned to leave the crease on its far side, the normals
    /// mirrored, the points kept for the caller to move onto the plane.
    static Strip mirrored(const Strip& strip, const Plane& plane);

    /// Where the line from point along direction meets the plane, as a distance along the direction; -1 when parallel.
    static double cut(const Point& point, const Vector& direction, const Plane& plane);
};

inline CurvedCreaseUnfolding::CurvedCreaseUnfolding(
    const NurbsCurve& crease,
    double opening,
    const std::vector<Plane>& planes,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      crease(crease),
      opening(opening),
      planes(planes),
      thickness(thickness),
      samples(std::max<size_t>(samples, 4)) {

    if (planes.size() < 2 || thickness <= 0.0 || opening <= 0.0 || opening >= 0.5 * Tolerance::PI)
        throw std::invalid_argument("A curved-crease unfolding needs two planes, a thickness and an opening between 0 and 90 degrees");

    // start: the plane the crease lies on
    const size_t start = compute_start();

    // first: the crease and the rulings and face normals of the plate after it, or before it when the crease is on the last plane
    const Strip first = compute_first(start);
    const int step = start + 1 < planes.size() ? 1 : -1;

    // before: the plate on the crease's other side, the first plate's mirror image in the crease's plane, its rulings leaving the crease
    Strip before = mirrored(first, planes[start]);

    for (Vector& ruling : before.rulings)
        ruling = ruling * -1.0;

    // plates: from the crease to one end, each the mirror image of the one before, then from the crease to the other end
    add_plates(first, start, step);
    add_plates(before, start, -step);

    // the given crease
    Polyline drawn(first.points);
    drawn.name = fmt::format("crease_{}", start);
    add_polyline(drawn, group_named("creases"));

    // contacts: every two plates that share a face, found by the session's contact detection
    compute_face_contacts(0);
}

inline NurbsCurve CurvedCreaseUnfolding::catenary(double span, double rise, size_t points) {

    // the catenary's parameter a from rise = a (cosh(span / 2a) - 1), by bisection: the rise falls as a grows
    double low = 1e-3 * span;
    double high = 1e3 * span;

    for (int i = 0; i < 200; i++) {
        const double a = 0.5 * (low + high);
        (a * (std::cosh(0.5 * span / a) - 1.0) > rise ? low : high) = a;
    }

    const double a = 0.5 * (low + high);
    std::vector<Point> curve;

    for (size_t k = 0; k <= points; k++) {
        const double u = -0.5 * span + span * static_cast<double>(k) / static_cast<double>(points);
        curve.push_back(Point(u, 0.0, rise - a * (std::cosh(u / a) - 1.0)));
    }

    return NurbsCurve::create_interpolated(curve);
}

inline size_t CurvedCreaseUnfolding::compute_start() const {

    const std::array<Point, 3> points = {
        crease.point_at(crease.domain_start()),
        crease.point_at(0.5 * (crease.domain_start() + crease.domain_end())),
        crease.point_at(crease.domain_end()),
    };
    size_t start = 0;
    double nearest = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < planes.size(); i++) {
        double distance = 0.0;

        for (const Point& point : points)
            distance = std::max(distance, std::abs((point - planes[i].origin()).dot(planes[i].z_axis().normalized())));

        if (distance < nearest) {
            nearest = distance;
            start = i;
        }
    }

    return start;
}

inline CurvedCreaseUnfolding::Strip CurvedCreaseUnfolding::compute_first(size_t start) const {

    // the crease sampled, on its plane
    const Vector normal = planes[start].z_axis().normalized();
    const Plane& next = planes[start + 1 < planes.size() ? start + 1 : start - 1];
    Strip strip;
    std::vector<Vector> tangents;

    for (size_t j = 0; j <= samples; j++) {
        const double t = crease.domain_start() + (crease.domain_end() - crease.domain_start()) * static_cast<double>(j) / static_cast<double>(samples);
        strip.points.push_back(crease.point_at(t));
        tangents.push_back(crease.tangent_at(t).normalized());

        if (std::abs((strip.points.back() - planes[start].origin()).dot(normal)) > 1e-6 * crease.length())
            throw std::invalid_argument("The crease lies on none of the planes");
    }

    // the concave side: the side the tangent turns to
    double turning = 0.0;

    for (size_t j = 0; j < samples; j++)
        turning += tangents[j].cross(tangents[j + 1]).dot(normal);

    // the plane's side the next plane cuts the rulings on, tried at the middle of the crease
    const double c = std::cos(opening);
    const double s = std::sin(opening);
    const size_t middle = samples / 2;
    const Vector inward = (turning >= 0.0 ? normal.cross(tangents[middle]) : tangents[middle].cross(normal)).normalized();
    const double side = cut(strip.points[middle], inward * c + normal * s, next) > 0.0 ? 1.0 : -1.0;

    for (size_t j = 0; j <= samples; j++) {
        const Vector across = (turning >= 0.0 ? normal.cross(tangents[j]) : tangents[j].cross(normal)).normalized();
        const Vector ruling = (across * c + normal * (side * s)).normalized();
        strip.rulings.push_back(ruling);
        strip.normals.push_back(tangents[j].cross(ruling).normalized());
    }

    return strip;
}

inline void CurvedCreaseUnfolding::add_plates(Strip strip, size_t from, int step) {

    const std::shared_ptr<TreeNode> plates = group_named("plates");
    const std::shared_ptr<TreeNode> creases = group_named("creases");
    const double half = 0.5 * thickness;

    for (size_t k = from; step > 0 ? k + 1 < planes.size() : k > 0; k += step) {
        // the four edges: each face's rulings, half the thickness off the mid-surface, cut by the plate's two planes
        const size_t next = k + step;
        std::array<std::vector<Point>, 4> edges;
        std::vector<Point> ends;

        for (size_t j = 0; j <= samples; j++) {
            const double reach = cut(strip.points[j], strip.rulings[j], planes[next]);

            if (reach <= 0.0)
                throw std::invalid_argument(fmt::format("Plane {} does not cut ruling {} ahead of the crease on plane {}", next, j, k));

            ends.push_back(strip.points[j] + strip.rulings[j] * reach);

            for (size_t face = 0; face < 2; face++) {
                const Point start = strip.points[j] + strip.normals[j] * (face == 0 ? half : -half);
                edges[2 * face].push_back(start + strip.rulings[j] * cut(start, strip.rulings[j], planes[k]));
                edges[2 * face + 1].push_back(start + strip.rulings[j] * cut(start, strip.rulings[j], planes[next]));
            }
        }

        const NurbsCurve outer_a = NurbsCurve::create_interpolated(edges[0], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve outer_b = NurbsCurve::create_interpolated(edges[1], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve inner_a = NurbsCurve::create_interpolated(edges[2], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve inner_b = NurbsCurve::create_interpolated(edges[3], CurveNurbsKnotStyle::Uniform);
        add(std::make_shared<PlateCurved>(outer_a, outer_b, inner_a, inner_b, fmt::format("plate_{}", std::min(k, next))), plates);

        // its development, the proof it is cut flat
        const Development development(outer_a, outer_b, samples);
        _distortion = std::max(_distortion, development.distortion);

        // the crease it ends on, its feet against the given crease's: the ends of it at the floor, its lowest end and any as low
        const double start_height = crease.point_at(crease.domain_start())[2];
        const double end_height = crease.point_at(crease.domain_end())[2];
        const double floor = std::min(start_height, end_height);

        if (start_height < floor + 1.0)
            _drift = std::max(_drift, std::abs(ends.front()[2] - floor));

        if (end_height < floor + 1.0)
            _drift = std::max(_drift, std::abs(ends.back()[2] - floor));

        Polyline drawn(ends);
        drawn.name = fmt::format("crease_{}", next);
        add_polyline(drawn, creases);

        // the next plate: this one mirrored in the far plane, starting on the crease they share
        strip = mirrored(strip, planes[next]);
        strip.points = ends;
    }
}

inline CurvedCreaseUnfolding::Strip CurvedCreaseUnfolding::mirrored(const Strip& strip, const Plane& plane) {

    const Vector normal = plane.z_axis().normalized();
    Strip image = strip;

    for (size_t j = 0; j < strip.points.size(); j++) {
        image.rulings[j] = (strip.rulings[j] - normal * (2.0 * strip.rulings[j].dot(normal))) * -1.0;
        image.normals[j] = strip.normals[j] - normal * (2.0 * strip.normals[j].dot(normal));
    }

    return image;
}

inline double CurvedCreaseUnfolding::cut(const Point& point, const Vector& direction, const Plane& plane) {

    const Vector normal = plane.z_axis();
    const double along = direction.dot(normal);

    if (std::abs(along) < 1e-12)
        return -1.0;

    return (plane.origin() - point).dot(normal) / along;
}

inline std::vector<Plane> CurvedCreaseUnfolding::fan(const Point& origin, const Vector& axis, const Vector& first, size_t count, double angle) {

    const Vector k = axis.normalized();
    const Vector start = (first - k * first.dot(k)).normalized();
    std::vector<Plane> planes;

    for (size_t i = 0; i <= count; i++) {
        const double turn = angle * (static_cast<double>(i) - 0.5 * static_cast<double>(count));
        const Vector direction = start * std::cos(turn) + k.cross(start) * std::sin(turn);
        planes.push_back(Plane::from_point_normal(origin, k.cross(direction)));
    }

    return planes;
}

inline std::vector<Plane> CurvedCreaseUnfolding::zigzag(const Point& origin, const Vector& along, const Vector& axis, size_t count, double spacing, double tilt) {

    const Vector a = along.normalized();
    const Vector k = axis.normalized();
    const long middle = static_cast<long>(count / 2);
    std::vector<Plane> planes;

    for (size_t i = 0; i <= count; i++) {
        const long offset = static_cast<long>(i) - middle;
        const long phase = ((offset % 4) + 4) % 4;
        const double turn = phase == 1 ? tilt : phase == 3 ? -tilt : 0.0;
        const Vector normal = a * std::cos(turn) + k.cross(a) * std::sin(turn);
        planes.push_back(Plane::from_point_normal(origin + a * (spacing * static_cast<double>(offset)), normal));
    }

    return planes;
}

inline std::vector<Plane> CurvedCreaseUnfolding::leaning(const Point& origin, const Vector& along, const Vector& axis, size_t count, double spacing, double lean) {

    const Vector a = along.normalized();
    const Vector k = axis.normalized();
    const double middle = 0.5 * static_cast<double>(count);
    std::vector<Plane> planes;

    for (size_t i = 0; i <= count; i++) {
        const double offset = static_cast<double>(i) - middle;
        const double turn = lean * offset;
        const Vector normal = a * std::cos(turn) + k.cross(a) * std::sin(turn);
        planes.push_back(Plane::from_point_normal(origin + a * (spacing * offset), normal));
    }

    return planes;
}
