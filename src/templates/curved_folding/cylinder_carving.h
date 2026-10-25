#pragma once
#include "wood_session.h"
#include "development.h"

using namespace session_cpp;
using namespace wood_session;

/// A cylinder carved by cylinders in sequence, ((a - b) - c) - d, one concave plate per carved patch.
class CylinderCarving : public WoodSession {
public:
    /// One cylinder: a point on its axis, the axis direction and the radius.
    struct Cylinder {
        Point point; // A point on the axis.
        Vector direction; // The axis direction.
        double radius; // mm, the cylinder.
    };

    const Cylinder stock; // The carved cylinder a, from its point along its direction for length.
    const std::vector<Cylinder> carvers; // The cylinders b, c, d and so on that carve it, in sequence.
    const double length; // mm, the stock from its point along its axis.
    const double thickness; // mm, every plate, behind the carved surface.
    const double min_angle; // radians, the least angle two carved patches may meet at.
    const size_t samples; // Generators every plate is interpolated through.

private:
    /// The part of one generator a plate keeps, with what ends it at each side.
    struct Part {
        double low; // mm, along the generator, where the part starts.
        double high; // mm, where it ends.
        int low_by; // What ends it at the start: 0 the stock, 1 and 2 its end planes, 3 + j carver j.
        int high_by; // What ends it at the end, the same way.
        int splitter; // The carver that cuts it in the middle, leaving two pieces; -1 for none.
    };

    double _distortion = 0.0; // The largest relative length error of any plate's development.

public:
    /// The carving as the session named name.
    CylinderCarving(
        const Cylinder& stock,
        const std::vector<Cylinder>& carvers,
        double length = 12000.0,
        double thickness = 60.0,
        double min_angle = 10.0 * Tolerance::TO_RADIANS,
        const std::string& name = "cylinder_carving",
        size_t samples = 128
    );

    /// Not copied, as a session is not.
    CylinderCarving(const CylinderCarving&) = delete;

    /// Not assigned, as it is not copied.
    CylinderCarving& operator=(const CylinderCarving&) = delete;

    /// The largest relative length error of any plate's development, 0 for an exact cylinder.
    double distortion() const {
        return _distortion;
    }

private:
    /// Every carver's patch a plate, plate_<k> in the group plates: its generators inside the stock and outside every other carver.
    void add_plates();

    /// The generator of carver k at angle on the circle grown by offset: its point and its direction.
    std::pair<Point, Vector> generator(size_t k, double angle, double offset) const;

    /// The part of carver k's generator at angle grown by offset a plate keeps: inside the stock where the carved surface is, so
    /// a free edge is cut square, between the stock's end planes and outside every other carver grown by offset, the longest.
    std::optional<Part> part(size_t k, double angle, double offset) const;

    /// The parameters along the line between which it lies inside the cylinder grown by offset; none when it misses it.
    static std::optional<std::pair<double, double>> inside(const Cylinder& cylinder, double offset, const Point& point, const Vector& direction);

    /// The angles of carver k's longest run of generators that keep a millimetre on the surface and behind it, its ends exact.
    std::optional<std::pair<double, double>> compute_run(size_t k) const;

    /// Whether carver k's generator at angle keeps a millimetre on the surface and behind it.
    bool keeps(size_t k, double angle) const;

    /// The angles between first and last where an end of the surface's or the plate's part changes what ends it.
    std::vector<double> compute_breaks(size_t k, double first, double last) const;

    /// What ends each of the four boundaries at angle: the surface's start and end, the plate's start and end.
    std::array<int, 4> enders(size_t k, double angle) const;

    /// Throws when a carver leaves no patch, cuts another's patch in two or meets one at less than min_angle.
    void validate() const;

    /// The unit normal of cylinder c at point, away from its axis.
    static Vector radial(const Cylinder& cylinder, const Point& point);
};

inline CylinderCarving::CylinderCarving(
    const Cylinder& stock,
    const std::vector<Cylinder>& carvers,
    double length,
    double thickness,
    double min_angle,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      stock(stock),
      carvers(carvers),
      length(length),
      thickness(thickness),
      min_angle(min_angle),
      samples(std::max<size_t>(samples, 8)) {

    if (carvers.empty() || length <= 0.0 || thickness <= 0.0)
        throw std::invalid_argument("A cylinder carving needs a carver, a length and a thickness");

    // validation: every carver leaves one patch, none cut in two, every two meeting at a clear angle
    validate();

    // plates: every carver's surface left inside the stock and outside the other carvers, its plate behind it in the stock
    add_plates();

    // contacts: every two plates that meet on a crease, found by the session's contact detection
    compute_face_contacts(0);
}

inline std::optional<std::pair<double, double>> CylinderCarving::inside(const Cylinder& cylinder, double offset, const Point& point, const Vector& direction) {

    // |w + s d| = radius for the parts of w and d square to the axis
    const Vector axis = cylinder.direction.normalized();
    const Vector w = point - cylinder.point;
    const Vector w_across = w - axis * w.dot(axis);
    const Vector d_across = direction - axis * direction.dot(axis);
    const double r = cylinder.radius + offset;
    const double a = d_across.dot(d_across);
    const double b = 2.0 * w_across.dot(d_across);
    const double c = w_across.dot(w_across) - r * r;

    if (a < 1e-12)
        return c < 0.0 ? std::optional<std::pair<double, double>>({-1e12, 1e12}) : std::nullopt;

    const double discriminant = b * b - 4.0 * a * c;

    if (discriminant <= 0.0)
        return std::nullopt;

    const double root = std::sqrt(discriminant);

    return std::make_pair((-b - root) / (2.0 * a), (-b + root) / (2.0 * a));
}

inline std::pair<Point, Vector> CylinderCarving::generator(size_t k, double angle, double offset) const {

    // the carver's circle in a frame square to its axis
    const Vector axis = carvers[k].direction.normalized();
    const Vector helper = std::abs(axis[2]) < 0.9 ? Vector(0.0, 0.0, 1.0) : Vector(1.0, 0.0, 0.0);
    const Vector u = axis.cross(helper).normalized();
    const Vector v = axis.cross(u).normalized();
    const double r = carvers[k].radius + offset;
    const Point point = carvers[k].point + u * (r * std::cos(angle)) + v * (r * std::sin(angle));

    return {point, axis};
}

inline std::optional<CylinderCarving::Part> CylinderCarving::part(size_t k, double angle, double offset) const {

    // inside the stock where the carved surface is, the same stretch of the generator behind it
    const std::pair<Point, Vector> surface = generator(k, angle, 0.0);
    const std::pair<Point, Vector> line = generator(k, angle, offset);
    const std::optional<std::pair<double, double>> in_stock = inside(stock, 0.0, surface.first, surface.second);

    if (!in_stock)
        return std::nullopt;

    Part kept = {in_stock->first, in_stock->second, 0, 0, -1};

    // between the stock's two end planes
    const Vector axis = stock.direction.normalized();
    const double speed = line.second.dot(axis);
    const double at = (line.first - stock.point).dot(axis);

    if (std::abs(speed) < 1e-12 && (at < 0.0 || at > length))
        return std::nullopt;

    if (std::abs(speed) >= 1e-12) {
        const double first = -at / speed;
        const double second = (length - at) / speed;
        const double near = std::min(first, second);
        const double far = std::max(first, second);
        const int near_by = first < second ? 1 : 2;

        if (near > kept.low)
            kept = {near, kept.high, near_by, kept.high_by, -1};

        if (far < kept.high)
            kept = {kept.low, far, kept.low_by, 3 - near_by, -1};
    }

    // outside every other carver grown by offset: every cut leaves the longer piece
    for (size_t j = 0; j < carvers.size() && kept.low < kept.high; j++) {
        if (j == k)
            continue;

        const std::optional<std::pair<double, double>> cut = inside(carvers[j], offset, line.first, line.second);

        if (!cut || cut->second <= kept.low || cut->first >= kept.high)
            continue;

        // a cut in the middle leaves two pieces a millimetre long or more: the longer kept, the carver named
        const double before = cut->first - kept.low;
        const double after = kept.high - cut->second;
        const int splitter = before > 1.0 && after > 1.0 ? static_cast<int>(j) : kept.splitter;

        if (before >= after)
            kept = {kept.low, cut->first, kept.low_by, 3 + static_cast<int>(j), splitter};
        else
            kept = {cut->second, kept.high, 3 + static_cast<int>(j), kept.high_by, splitter};
    }

    if (kept.low >= kept.high)
        return std::nullopt;

    return kept;
}

inline Vector CylinderCarving::radial(const Cylinder& cylinder, const Point& point) {

    const Vector axis = cylinder.direction.normalized();
    const Vector offset = point - cylinder.point;

    return (offset - axis * offset.dot(axis)).normalized();
}

inline void CylinderCarving::validate() const {

    for (size_t k = 0; k < carvers.size(); k++) {
        const std::optional<std::pair<double, double>> run = compute_run(k);

        if (!run)
            throw std::invalid_argument(fmt::format("Carver {} does not cut the stock deep enough to leave a plate", k));

        // along the run: no generator cut in two, every crease with another carver at a clear angle
        const size_t count = 4 * samples;

        for (size_t i = 0; i <= count; i++) {
            const double angle = run->first + (run->second - run->first) * static_cast<double>(i) / static_cast<double>(count);
            const std::optional<Part> kept = part(k, angle, 0.0);

            if (!kept)
                continue;

            if (kept->splitter >= 0)
                throw std::invalid_argument(fmt::format("Carver {} cuts carver {}'s patch in two", kept->splitter, k));

            const std::pair<Point, Vector> line = generator(k, angle, 0.0);

            for (const std::pair<double, int>& end : {std::make_pair(kept->low, kept->low_by), std::make_pair(kept->high, kept->high_by)}) {
                if (end.second < 3)
                    continue;

                const Point point = line.first + line.second * end.first;
                const Cylinder& other = carvers[static_cast<size_t>(end.second - 3)];
                const double cosine = std::clamp(radial(carvers[k], point).dot(radial(other, point)), -1.0, 1.0);
                const double meet = std::acos(cosine);

                if (meet < min_angle)
                    throw std::invalid_argument(fmt::format("Carvers {} and {} meet at {:.1f} degrees, under the least {:.1f}", k, end.second - 3, meet * Tolerance::TO_DEGREES, min_angle * Tolerance::TO_DEGREES));
            }
        }
    }
}

inline std::optional<std::pair<double, double>> CylinderCarving::compute_run(size_t k) const {

    // every generator around the circle, whether it keeps a millimetre on the surface and behind it
    const size_t count = 1440;
    std::vector<bool> carves;

    for (size_t i = 0; i < count; i++) {
        const double angle = 2.0 * Tolerance::PI * static_cast<double>(i) / static_cast<double>(count);
        carves.push_back(keeps(k, angle));
    }

    // the longest run of carving generators, round the circle
    size_t best_start = 0;
    size_t best_length = 0;

    for (size_t start = 0; start < count; start++) {
        if (!carves[start] || carves[(start + count - 1) % count])
            continue;

        size_t run = 0;

        while (run < count && carves[(start + run) % count])
            run++;

        if (run > best_length) {
            best_start = start;
            best_length = run;
        }
    }

    if (best_length < 3)
        return std::nullopt;

    // both ends of the run by bisection to the last generator that keeps a millimetre, so neighbours meet without a gap
    const double step = 2.0 * Tolerance::PI / static_cast<double>(count);
    std::array<double, 2> ends = {step * static_cast<double>(best_start), step * static_cast<double>(best_start + best_length - 1)};

    for (size_t e = 0; e < 2 && best_length < count; e++) {
        double inside = ends[e];
        double outside = e == 0 ? inside - step : inside + step;

        for (int i = 0; i < 50; i++) {
            const double middle = 0.5 * (inside + outside);

            if (keeps(k, middle))
                inside = middle;
            else
                outside = middle;
        }

        ends[e] = inside;
    }

    return std::make_pair(ends[0], ends[1]);
}

inline bool CylinderCarving::keeps(size_t k, double angle) const {

    const std::optional<Part> outer = part(k, angle, 0.0);
    const std::optional<Part> inner = part(k, angle, thickness);

    return outer && inner && outer->high - outer->low > 1.0 && inner->high - inner->low > 1.0;
}

inline std::array<int, 4> CylinderCarving::enders(size_t k, double angle) const {

    const std::optional<Part> outer = part(k, angle, 0.0);
    const std::optional<Part> inner = part(k, angle, thickness);

    if (!outer || !inner)
        return {-1, -1, -1, -1};

    return {outer->low_by, outer->high_by, inner->low_by, inner->high_by};
}

inline std::vector<double> CylinderCarving::compute_breaks(size_t k, double first, double last) const {

    // fine steps over the run; where what ends a boundary changes, the angle of the change by bisection
    const size_t count = 4 * samples;
    std::vector<double> breaks = {first};

    for (size_t i = 0; i < count; i++) {
        double low = first + (last - first) * static_cast<double>(i) / static_cast<double>(count);
        double high = first + (last - first) * static_cast<double>(i + 1) / static_cast<double>(count);
        const std::array<int, 4> at_low = enders(k, low);

        if (at_low == enders(k, high))
            continue;

        for (int step = 0; step < 50; step++) {
            const double middle = 0.5 * (low + high);

            if (enders(k, middle) == at_low)
                low = middle;
            else
                high = middle;
        }

        if (0.5 * (low + high) - breaks.back() > 1e-6)
            breaks.push_back(0.5 * (low + high));
    }

    if (last - breaks.back() > 1e-6)
        breaks.push_back(last);
    else
        breaks.back() = last;

    return breaks;
}

inline void CylinderCarving::add_plates() {

    const std::shared_ptr<TreeNode> group = add_group("plates");

    for (size_t k = 0; k < carvers.size(); k++) {
        const std::optional<std::pair<double, double>> run = compute_run(k);

        if (!run)
            continue;

        // the run cut where any boundary changes what ends it, every piece smooth, the four boundaries cut at the same angles
        const std::vector<double> breaks = compute_breaks(k, run->first, run->second);
        std::array<std::vector<NurbsCurve>, 4> pieces;

        for (size_t b = 0; b + 1 < breaks.size(); b++) {
            const double share = (breaks[b + 1] - breaks[b]) / (run->second - run->first);
            const size_t steps = std::max<size_t>(3, static_cast<size_t>(std::ceil(share * static_cast<double>(samples))));
            std::array<std::vector<Point>, 4> points;

            // every generator of the piece clipped on the carved surface and, a thickness behind it, on the carvers grown alike
            for (size_t i = 0; i <= steps; i++) {
                const double angle = breaks[b] + (breaks[b + 1] - breaks[b]) * static_cast<double>(i) / static_cast<double>(steps);
                const std::pair<Point, Vector> outer = generator(k, angle, 0.0);
                const std::pair<Point, Vector> inner = generator(k, angle, thickness);
                const std::optional<Part> outer_part = part(k, angle, 0.0);
                const std::optional<Part> inner_part = part(k, angle, thickness);

                if (!outer_part || !inner_part)
                    throw std::invalid_argument(fmt::format("Carver {} leaves no plate behind its generator at {:.3f} radians", k, angle));

                points[0].push_back(outer.first + outer.second * outer_part->low);
                points[1].push_back(outer.first + outer.second * outer_part->high);
                points[2].push_back(inner.first + inner.second * inner_part->low);
                points[3].push_back(inner.first + inner.second * inner_part->high);
            }

            for (size_t c = 0; c < 4; c++)
                pieces[c].push_back(NurbsCurve::create_interpolated(points[c], CurveNurbsKnotStyle::Uniform));
        }

        // every boundary the pieces joined with a corner at every change
        std::array<NurbsCurve, 4> curves;

        for (size_t c = 0; c < 4; c++) {
            const std::vector<NurbsCurve> joined = NurbsCurve::join(pieces[c], 1e-6);

            if (joined.size() != 1)
                throw std::invalid_argument(fmt::format("Carver {}'s boundary {} breaks into {} pieces", k, c, joined.size()));

            curves[c] = joined[0];
        }

        const std::shared_ptr<PlateCurved> plate = std::make_shared<PlateCurved>(
            curves[0],
            curves[1],
            curves[2],
            curves[3],
            fmt::format("plate_{}", k)
        );
        add(plate, group);

        // its development, the proof it is cut flat
        const Development development(curves[0], curves[1], samples);
        _distortion = std::max(_distortion, development.distortion);
    }
}
