#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding, after Scheder-Bieschin, Van Mele and Block, "Curved-Crease Flat-Foldable Bending-Active Plate Structures"
/// (Towards Radical Regeneration, DMSB 2023) and Tachi's closed folding (Transformables 2013): flat plates with a curved edge, cut as
/// bands of equal width, are stacked and hinged to their neighbours along the curved edges, one edge on top and the other at the bottom
/// of the stack. Pulled open like an accordion, every plate bends about its rulings into a developable strip and the stack becomes a
/// corrugated vault: the curved edges are the creases, each in a plane parallel to the stack, each plate the mirror image of the one
/// before it in the plane of their shared crease. A straight edge would only hinge; the curved one forces the bending, so the profile
/// of the vault comes from the plan of the plates alone.
///
/// The mechanism has one degree of freedom, the opening a, the angle between a plate's rulings and the crease plane (0 flat-folded): a
/// crease keeps its length and its plane, and its curvature in space is the flat edge's curvature over cos a (Fuchs and Tabachnikov,
/// cos a = k2D / k3D, the paper's Eq. 5), so the arch rises and narrows as it opens. The rulings are perpendicular to the crease (the
/// paper's ultra-special case of a planar crease with a constant reflection angle): a plate's mid-surface runs its width along them at a
/// to the crease plane, so a circular crease gives exact cones and every plate develops flat at every opening.
///
/// The thickness is the paper's axis shift (Shimoda, Tachi and Sato 2020): a plate's two hinges lie on its two faces, the crease planes
/// stand width sin a + thickness cos a apart, and the plates fold completely closed into a stack, which mitred plates cannot.
///
/// Fields: the target crease, the opening it is reached at, the plates' width, count and thickness, the openings drawn and the sampling;
/// every state's plates and hinges, and the plate as cut, are in the session.
class CurvedCreaseUnfolding : public WoodSession {
public:
    /// A crease: its points and its unit normals in its plane.
    struct Crease {
        std::vector<Point> points;
        std::vector<Vector> normals;
    };

    const NurbsCurve target; // The crease wanted at the design opening, a planar curve in the xz plane, its chord taken as the floor.
    const double design; // radians, the opening at which the crease is the target.
    const double width; // mm, every plate's width across its curved edges as cut, along its rulings.
    const size_t count; // The plates in the stack.
    const std::vector<double> openings; // radians, the openings drawn side by side along x, 0 the flat-folded stack.
    const double thickness; // mm, every plate.
    const size_t samples; // Segments the crease is bent with; even, so a circular crease has a middle point.

    /// The unfolding of count plates width wide as the session named name, drawn at every opening.
    CurvedCreaseUnfolding(
        const NurbsCurve& target,
        double design,
        double width,
        size_t count,
        const std::vector<double>& openings,
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

    /// mm, the largest distance of a plate's mid-surface developed at any opening from the outline of the plate as cut, both unfolded
    /// from their first ruling: the proof the opening bends the plates without stretching them.
    double isometry() const {
        return _isometry;
    }

    /// mm, the crease's span and rise at the opening of state i.
    std::pair<double, double> span_rise(size_t i) const {
        return _span_rise[i];
    }

    /// The circular arc span wide and rise high over the x axis, centred on the origin.
    static NurbsCurve arc(double span, double rise);

    /// The catenary span wide and rise high over the x axis, centred on the origin, interpolated through points.
    static NurbsCurve catenary(double span, double rise, size_t points = 32);

private:
    double _step = 0.0;
    std::vector<double> _turns;
    double _distortion = 0.0;
    double _isometry = 0.0;
    std::vector<std::pair<double, double>> _span_rise;

    /// The target cut into samples segments of equal arc length: _step, their length, and _turns, the turn of its tangent along each,
    /// all made equal when they differ by less than a thousandth, a circle, so its curves come out as exact arcs.
    void compute_turns();

    /// The crease at an opening, samples + 1 points in the xz plane with its unit normals toward the side the rulings run to, inside an
    /// arch; the chord level on the x axis and centred on the origin: every segment a circular arc _step long turning by its turn times
    /// cos(design) / cos(opening), the crease's curvature as cut over its curvature open, so its length never changes.
    Crease compute_crease(double opening) const;

    /// The curve through the crease's points moved by offset along the normals and to y; an exact arc when the crease is a circle.
    NurbsCurve offset_curve(const Crease& crease, double offset, double y) const;

    /// One state's plates, plate_<state>_<k> in the group state_<state>, and hinges, hinge_<state>_<k>, from x on along x; the
    /// state's largest x returned for the next.
    double add_state(size_t state, double opening, double x);

    /// The plate as cut, flat on the floor from x on: the band between the curved edge and its offset by the width.
    void add_flat(double x);
};

inline CurvedCreaseUnfolding::CurvedCreaseUnfolding(
    const NurbsCurve& target,
    double design,
    double width,
    size_t count,
    const std::vector<double>& openings,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      target(target),
      design(design),
      width(width),
      count(count),
      openings(openings),
      thickness(thickness),
      samples(std::max<size_t>(samples + samples % 2, 4)) {

    if (width <= 0.0 || thickness <= 0.0 || count == 0 || openings.empty() || design <= 0.0 || design >= 0.5 * Tolerance::PI)
        throw std::invalid_argument("A curved-crease unfolding needs a width, a thickness, plates, openings and a design opening below 90 degrees");

    for (double opening : openings)
        if (opening < 0.0 || opening >= 0.5 * Tolerance::PI)
            throw std::invalid_argument("An opening lies between 0, flat-folded, and 90 degrees");

    // turns: the target at equal arc length and its turning angle at every point, the curvature the plates are cut and bent to
    compute_turns();

    // states: the same stack at every opening side by side along x, each plate the mirror image of the one before in their crease's plane
    double x = 0.0;

    for (size_t i = 0; i < openings.size(); i++) {
        const double right = add_state(i, openings[i], x);
        x = right + 0.2 * _step * static_cast<double>(this->samples);
    }

    // flat: one plate as cut, the band between two curved edges, beside the last state
    add_flat(x);
}

inline void CurvedCreaseUnfolding::compute_turns() {

    // the target sampled densely in the xz plane, with its length up to every sample
    const size_t dense = 16 * samples;
    const double delta = (target.domain_end() - target.domain_start()) / static_cast<double>(dense);
    std::vector<Point> points;
    std::vector<double> lengths = {0.0};

    for (size_t k = 0; k <= dense; k++) {
        const Point point = target.point_at(target.domain_start() + delta * static_cast<double>(k));
        points.push_back(Point(point[0], 0.0, point[2]));

        if (k > 0)
            lengths.push_back(lengths.back() + points[k].distance(points[k - 1]));
    }

    // the tangent's angle from x toward z at equal arc lengths, unwrapped
    _step = lengths.back() / static_cast<double>(samples);
    std::vector<double> angles;
    size_t j = 0;

    for (size_t k = 0; k <= samples; k++) {
        const double length = _step * static_cast<double>(k);

        while (j + 2 < lengths.size() && lengths[j + 1] < length)
            j++;

        const double f = std::clamp((length - lengths[j]) / (lengths[j + 1] - lengths[j]), 0.0, 1.0);
        const Vector tangent = target.tangent_at(target.domain_start() + delta * (static_cast<double>(j) + f));
        const double angle = std::atan2(tangent[2], tangent[0]);
        angles.push_back(k == 0 ? angle : angles.back() + std::remainder(angle - angles.back(), 2.0 * Tolerance::PI));
    }

    // the turn along every segment
    _turns.clear();

    for (size_t k = 0; k < samples; k++)
        _turns.push_back(angles[k + 1] - angles[k]);

    // a circle: every turn the mean
    const double mean = std::accumulate(_turns.begin(), _turns.end(), 0.0) / static_cast<double>(_turns.size());
    bool circular = std::abs(mean) > 1e-9;

    for (double turn : _turns)
        circular = circular && std::abs(turn - mean) < 1e-3 * std::abs(mean);

    if (circular)
        std::fill(_turns.begin(), _turns.end(), mean);
}

inline CurvedCreaseUnfolding::Crease CurvedCreaseUnfolding::compute_crease(double opening) const {

    // the segments laid one after another, each an arc of its turn scaled by the curvature ratio, its chord shorter than its length
    const double ratio = std::cos(design) / std::cos(opening);
    std::vector<Point> points = {Point(0.0, 0.0, 0.0)};
    std::vector<double> angles = {0.0};

    for (size_t k = 0; k < samples; k++) {
        const double turn = _turns[k] * ratio;
        const double chord = std::abs(turn) < 1e-12 ? _step : _step * std::sin(0.5 * turn) / (0.5 * turn);
        const double middle = angles.back() + 0.5 * turn;
        points.push_back(points.back() + Vector(std::cos(middle), 0.0, std::sin(middle)) * chord);
        angles.push_back(angles.back() + turn);
    }

    // the chord turned onto the x axis and centred on the origin, the normals y x tangent: down at the crown of an arch
    const Point end = points.back();
    const double chord = std::atan2(end[2], end[0]);
    const double half = 0.5 * std::sqrt(end[0] * end[0] + end[2] * end[2]);
    Crease crease;

    for (size_t k = 0; k <= samples; k++) {
        const Point point = points[k];
        const double angle = angles[k] - chord;
        crease.points.push_back(Point(point[0] * std::cos(chord) + point[2] * std::sin(chord) - half, 0.0, -point[0] * std::sin(chord) + point[2] * std::cos(chord)));
        crease.normals.push_back(Vector(std::sin(angle), 0.0, -std::cos(angle)));
    }

    return crease;
}

inline NurbsCurve CurvedCreaseUnfolding::offset_curve(const Crease& crease, double offset, double y) const {

    std::vector<Point> points;

    for (size_t k = 0; k <= samples; k++)
        points.push_back(crease.points[k] + crease.normals[k] * offset + Vector(0.0, y, 0.0));

    // a circle's offset is a concentric circle through its ends and middle; any other crease interpolated point by point, the rulings
    // joining equal parameters
    if (std::all_of(_turns.begin(), _turns.end(), [&](double turn) { return turn == _turns.front(); }))
        return Primitives::arc(points.front(), points[samples / 2], points.back());

    return NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform);
}

inline double CurvedCreaseUnfolding::add_state(size_t state, double opening, double x) {

    const double s = std::sin(opening);
    const double c = std::cos(opening);
    const double half = 0.5 * thickness;

    // the crease and its normals, moved so the lowest face stands on the floor and the state starts at x
    Crease crease = compute_crease(opening);
    const std::array<double, 4> offsets = {-half * s, width * c - half * s, half * s, width * c + half * s};
    double low = std::numeric_limits<double>::infinity();
    double left = std::numeric_limits<double>::infinity();
    double right = -std::numeric_limits<double>::infinity();

    for (size_t k = 0; k <= samples; k++)
        for (double offset : offsets) {
            const Point point = crease.points[k] + crease.normals[k] * offset;
            low = std::min(low, point[2]);
            left = std::min(left, point[0]);
            right = std::max(right, point[0]);
        }

    for (Point& point : crease.points)
        point = point + Vector(x - left, 0.0, -low);

    _span_rise.push_back({crease.points.front().distance(crease.points.back()), crease.points[samples / 2][2] - crease.points.front()[2]});

    // the first plate's four edges, outer_a, outer_b, inner_a, inner_b: its mid-surface from the crease at y 0 along the rulings, width
    // long at the opening to the crease plane, its faces half the thickness to either side; its hinges the inner face's first edge and
    // the outer face's second, the axis shift
    const std::array<double, 4> heights = {half * c, width * s + half * c, -half * c, width * s - half * c};
    const double hinge = heights[1];
    const double period = 2.0 * (width * s + thickness * c);

    // plates: every second one the first's mirror image in the plane of the hinge they share, the stack repeating every two plates
    const std::shared_ptr<TreeNode> plates = group_named("plates");
    const std::shared_ptr<TreeNode> group = add_group(fmt::format("state_{}", state), plates);
    const std::shared_ptr<TreeNode> hinges = group_named("hinges");

    for (size_t k = 0; k < count; k++) {
        const double base = period * static_cast<double>(k / 2);
        std::array<double, 4> ys = {};

        for (size_t e = 0; e < 4; e++)
            ys[e] = base + (k % 2 == 0 ? heights[e] : 2.0 * hinge - heights[e]);

        const NurbsCurve outer_a = offset_curve(crease, offsets[0], ys[0]);
        const NurbsCurve outer_b = offset_curve(crease, offsets[1], ys[1]);
        const NurbsCurve inner_a = offset_curve(crease, offsets[2], ys[2]);
        const NurbsCurve inner_b = offset_curve(crease, offsets[3], ys[3]);
        add(std::make_shared<PlateCurved>(outer_a, outer_b, inner_a, inner_b, fmt::format("plate_{}_{}", state, k)), group);

        // its development, the proof it is cut flat
        const Development development(outer_a, outer_b, samples);
        _distortion = std::max(_distortion, development.distortion);

        // the hinge to the next plate: the outer face's second edge of an even plate, the mirrored inner face's first of an odd one
        if (k + 1 < count) {
            const size_t edge = k % 2 == 0 ? 1 : 2;
            std::vector<Point> points;

            for (size_t j = 0; j <= samples; j++)
                points.push_back(crease.points[j] + crease.normals[j] * offsets[edge] + Vector(0.0, ys[edge], 0.0));

            Polyline drawn(points);
            drawn.name = fmt::format("hinge_{}_{}", state, k);
            add_polyline(drawn, hinges);
        }
    }

    // isometry: the mid-surface at this opening developed onto the plate as cut, every corner of its outline to the nearest edge of the cut
    const Crease cut = compute_crease(0.0);
    const Development open(offset_curve(crease, 0.0, 0.0), offset_curve(crease, width * c, width * s), 16 * samples);
    const Development flat(offset_curve(cut, 0.0, 0.0), offset_curve(cut, width, 0.0), 16 * samples);

    for (size_t k = 0; k < open.outline.point_count(); k++) {
        size_t edge = 0;
        Point nearest;
        _isometry = std::max(_isometry, flat.outline.closest_distance_and_point(open.outline.get_point(k), edge, nearest));
    }

    return x + right - left;
}

inline void CurvedCreaseUnfolding::add_flat(double x) {

    // the curved edge as cut and its offset by the width, laid flat on the floor
    const Crease cut = compute_crease(0.0);
    double left = std::numeric_limits<double>::infinity();

    for (size_t k = 0; k <= samples; k++)
        left = std::min({left, cut.points[k][0], (cut.points[k] + cut.normals[k] * width)[0]});

    std::vector<Point> outline;

    for (size_t k = 0; k <= samples; k++)
        outline.push_back(Point(x - left + cut.points[k][0], cut.points[k][2], 0.0));

    for (size_t k = samples + 1; k-- > 0;) {
        const Point point = cut.points[k] + cut.normals[k] * width;
        outline.push_back(Point(x - left + point[0], point[2], 0.0));
    }

    outline.push_back(outline.front());
    Polyline drawn(outline);
    drawn.name = "flat_plate";
    add_polyline(drawn, add_group("flat"));
}

inline NurbsCurve CurvedCreaseUnfolding::arc(double span, double rise) {

    return Primitives::arc(Point(-0.5 * span, 0.0, 0.0), Point(0.0, 0.0, rise), Point(0.5 * span, 0.0, 0.0));
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
