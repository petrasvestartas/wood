#pragma once
#include "wood_session.h"
#include "../development.h"

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
    const bool closed; // Whether the carvers must carve all of the stock's surface, the patches leaving no gap.
    const size_t samples; // Generators every plate is interpolated through.

private:
    /// One piece of a generator a plate keeps, with what ends it at each side.
    struct Part {
        double low; // mm, along the generator, where the piece starts.
        double high; // mm, where it ends.
        int low_by; // What ends it at the start: 0 the stock, 1 and 2 its end planes, 3 + j carver j.
        int high_by; // What ends it at the end, the same way.
    };

    /// One carved patch of a carver: the angles of its generators and, per fine step, the piece it keeps.
    struct Patch {
        double first; // radians, its first generator.
        double last; // radians, its last generator.
        double behind_first; // radians, the first generator of the plate behind it, a thickness deep.
        double behind_last; // radians, the last.
        std::vector<std::pair<double, Part>> track; // Per fine step its angle and its piece on the surface.
    };

    static constexpr double least = 0.02; // mm, the shortest piece of a generator a plate keeps, so it reaches its corners.

    double _distortion = 0.0; // The largest relative length error of any plate's development.

public:
    /// The carving as the session named name.
    CylinderCarving(
        const Cylinder& stock,
        const std::vector<Cylinder>& carvers,
        double length = 12000.0,
        double thickness = 60.0,
        double min_angle = 10.0 * Tolerance::TO_RADIANS,
        bool closed = false,
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
    /// Every carver's patches, every patch a plate, plate_<k>_<p> in the group plates.
    void add_plates();

    /// The generator of carver k at angle on the circle grown by offset: its point and its direction.
    std::pair<Point, Vector> generator(size_t k, double angle, double offset) const;

    /// The pieces of carver k's generator at angle grown by offset a plate keeps: inside the stock where the carved surface is,
    /// so a free edge is cut square, between the stock's end planes and outside every other carver grown by offset.
    std::vector<Part> parts(size_t k, double angle, double offset) const;

    /// The parameters along the line between which it lies inside the cylinder grown by offset; none when it misses it.
    static std::optional<std::pair<double, double>> inside(const Cylinder& cylinder, double offset, const Point& point, const Vector& direction);

    /// The piece of the generator at angle grown by offset that overlaps the reference most, the least long; none when none does.
    std::optional<Part> pick(size_t k, double angle, double offset, const Part& reference) const;

    /// The last angle from angle towards side (+1 or -1) whose generator grown by offset keeps a piece overlapping the reference:
    /// stepped to the first that does not, or back to the first that does, then bisected.
    double reach(size_t k, double angle, double side, double offset, const Part& reference) const;

    /// Carver k's patches: its pieces followed from generator to generator by their overlap, the ends of every one exact on the
    /// carved surface and behind it, where a crease behind reaches an end plane at another angle than the crease on the surface.
    std::vector<Patch> compute_patches(size_t k) const;

    /// The patch's piece nearest the angle, the reference for every pick inside it.
    static const Part& reference(const Patch& patch, double angle);

    /// What ends the patch's piece at angle grown by offset at its start and at its end; -1 where it keeps none.
    std::array<int, 2> enders(size_t k, const Patch& patch, double angle, double offset) const;

    /// The angles from first to last where an end of the patch's piece grown by offset changes what ends it.
    std::vector<double> compute_breaks(size_t k, const Patch& patch, double offset, double first, double last) const;

    /// Throws when two carved patches meet at less than min_angle, or, closed, when a patch ends on the stock's surface: the point
    /// it ends at lies in no other carver, so the stock's surface beside it is left uncarved, a gap between the patches.
    void validate(size_t k, const Patch& patch) const;

    /// The unit normal of cylinder c at point, away from its axis.
    static Vector radial(const Cylinder& cylinder, const Point& point);
};

inline CylinderCarving::CylinderCarving(
    const Cylinder& stock,
    const std::vector<Cylinder>& carvers,
    double length,
    double thickness,
    double min_angle,
    bool closed,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      stock(stock),
      carvers(carvers),
      length(length),
      thickness(thickness),
      min_angle(min_angle),
      closed(closed),
      samples(std::max<size_t>(samples, 8)) {

    if (carvers.empty() || length <= 0.0 || thickness <= 0.0)
        throw std::invalid_argument("A cylinder carving needs a carver, a length and a thickness");

    // plates: every carver's surface left inside the stock and outside the other carvers, every patch of it a plate behind it
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

inline Vector CylinderCarving::radial(const Cylinder& cylinder, const Point& point) {

    const Vector axis = cylinder.direction.normalized();
    const Vector offset = point - cylinder.point;

    return (offset - axis * offset.dot(axis)).normalized();
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

inline std::vector<CylinderCarving::Part> CylinderCarving::parts(size_t k, double angle, double offset) const {

    // inside the stock where the carved surface is, the same stretch of the generator behind it
    const std::pair<Point, Vector> surface = generator(k, angle, 0.0);
    const std::pair<Point, Vector> line = generator(k, angle, offset);
    const std::optional<std::pair<double, double>> in_stock = inside(stock, 0.0, surface.first, surface.second);

    if (!in_stock)
        return {};

    Part whole = {in_stock->first, in_stock->second, 0, 0};

    // between the stock's two end planes
    const Vector axis = stock.direction.normalized();
    const double speed = line.second.dot(axis);
    const double at = (line.first - stock.point).dot(axis);

    if (std::abs(speed) < 1e-12 && (at < 0.0 || at > length))
        return {};

    if (std::abs(speed) >= 1e-12) {
        const double first = -at / speed;
        const double second = (length - at) / speed;
        const int near_by = first < second ? 1 : 2;

        if (std::min(first, second) > whole.low)
            whole = {std::min(first, second), whole.high, near_by, whole.high_by};

        if (std::max(first, second) < whole.high)
            whole = {whole.low, std::max(first, second), whole.low_by, 3 - near_by};
    }

    // every other carver grown by offset cut out of every piece, leaving a piece before it and one after
    std::vector<Part> kept;

    if (whole.low < whole.high)
        kept.push_back(whole);

    for (size_t j = 0; j < carvers.size(); j++) {
        if (j == k)
            continue;

        const std::optional<std::pair<double, double>> cut = inside(carvers[j], offset, line.first, line.second);

        if (!cut)
            continue;

        std::vector<Part> next;
        const int by = 3 + static_cast<int>(j);

        for (const Part& piece : kept) {
            if (cut->second <= piece.low || cut->first >= piece.high) {
                next.push_back(piece);
                continue;
            }

            if (cut->first > piece.low)
                next.push_back({piece.low, cut->first, piece.low_by, by});

            if (cut->second < piece.high)
                next.push_back({cut->second, piece.high, by, piece.high_by});
        }

        kept = next;
    }

    return kept;
}

inline std::optional<CylinderCarving::Part> CylinderCarving::pick(size_t k, double angle, double offset, const Part& reference) const {

    std::optional<Part> best;
    double best_overlap = 0.0;

    // the reference grown by offset, as a tilted end plane meets the generator behind up to that much further along it
    for (const Part& piece : parts(k, angle, offset)) {
        const double overlap = std::min(piece.high, reference.high + offset) - std::max(piece.low, reference.low - offset);

        if (piece.high - piece.low > least && overlap > best_overlap) {
            best = piece;
            best_overlap = overlap;
        }
    }

    return best;
}

inline double CylinderCarving::reach(size_t k, double angle, double side, double offset, const Part& reference) const {

    // a fine step at a time to the change, one angle that keeps a piece and one that does not
    const double step = 2.0 * Tolerance::PI / 1440.0;
    double keep = angle;
    double lose = angle + side * step;

    if (pick(k, angle, offset, reference)) {
        for (int i = 0; i < 1440 && pick(k, lose, offset, reference); i++) {
            keep = lose;
            lose = lose + side * step;
        }
    } else {
        lose = angle;
        keep = angle - side * step;

        for (int i = 0; i < 1440 && !pick(k, keep, offset, reference); i++) {
            lose = keep;
            keep = keep - side * step;
        }
    }

    // the change by bisection
    for (int i = 0; i < 50; i++) {
        const double middle = 0.5 * (keep + lose);

        if (pick(k, middle, offset, reference))
            keep = middle;
        else
            lose = middle;
    }

    return keep;
}

inline const CylinderCarving::Part& CylinderCarving::reference(const Patch& patch, double angle) {

    size_t nearest = 0;

    for (size_t i = 1; i < patch.track.size(); i++)
        if (std::abs(patch.track[i].first - angle) < std::abs(patch.track[nearest].first - angle))
            nearest = i;

    return patch.track[nearest].second;
}

inline std::vector<CylinderCarving::Patch> CylinderCarving::compute_patches(size_t k) const {

    // the fine steps round the circle, started at a generator that keeps nothing so no patch runs over the start
    const size_t count = 1440;
    const double step = 2.0 * Tolerance::PI / static_cast<double>(count);
    std::vector<std::vector<Part>> pieces(count);
    size_t start = count;

    for (size_t i = 0; i < count; i++) {
        const double angle = step * static_cast<double>(i);

        for (const Part& piece : parts(k, angle, 0.0))
            if (piece.high - piece.low > least)
                pieces[i].push_back(piece);

        if (pieces[i].empty() && start == count)
            start = i;
    }

    if (start == count)
        throw std::invalid_argument(fmt::format("Carver {} carves the stock all round, a patch with no end", k));

    // every piece followed to the next step's piece it overlaps most, a new patch where none came before it
    std::vector<Patch> patches;
    std::vector<int> open;

    for (size_t n = 1; n <= count; n++) {
        const size_t i = (start + n) % count;
        const double angle = step * static_cast<double>(start + n);
        std::vector<int> next(pieces[i].size(), -1);

        for (size_t p = 0; p < pieces[i].size(); p++) {
            double best_overlap = 0.0;

            for (const int o : open) {
                const Part& last = patches[static_cast<size_t>(o)].track.back().second;
                const double overlap = std::min(pieces[i][p].high, last.high) - std::max(pieces[i][p].low, last.low);
                bool claimed = false;

                for (const int taken : next)
                    claimed = claimed || taken == o;

                if (overlap > best_overlap && !claimed) {
                    best_overlap = overlap;
                    next[p] = o;
                }
            }

            if (next[p] < 0) {
                patches.push_back({angle, angle, angle, angle, {}});
                next[p] = static_cast<int>(patches.size() - 1);
            }

            patches[static_cast<size_t>(next[p])].track.push_back({angle, pieces[i][p]});
            patches[static_cast<size_t>(next[p])].last = angle;
        }

        open = next;
    }

    // every patch of three steps or more, both ends exact on the surface and behind it, so neighbours meet without a gap
    std::vector<Patch> kept;

    for (Patch& patch : patches) {
        if (patch.track.size() < 3)
            continue;

        const Part& front = patch.track.front().second;
        const Part& back = patch.track.back().second;
        patch.first = reach(k, patch.first, -1.0, 0.0, front);
        patch.last = reach(k, patch.last, 1.0, 0.0, back);
        patch.behind_first = reach(k, patch.first, -1.0, thickness, front);
        patch.behind_last = reach(k, patch.last, 1.0, thickness, back);
        kept.push_back(patch);
    }

    return kept;
}

inline std::array<int, 2> CylinderCarving::enders(size_t k, const Patch& patch, double angle, double offset) const {

    const std::optional<Part> piece = pick(k, angle, offset, reference(patch, angle));

    if (!piece)
        return {-1, -1};

    return {piece->low_by, piece->high_by};
}

inline std::vector<double> CylinderCarving::compute_breaks(size_t k, const Patch& patch, double offset, double first, double last) const {

    // fine steps from first to last; where what ends a boundary changes, the angle of the change by bisection
    const size_t count = 4 * samples;
    std::vector<double> breaks = {first};

    for (size_t i = 0; i < count; i++) {
        double low = first + (last - first) * static_cast<double>(i) / static_cast<double>(count);
        double high = first + (last - first) * static_cast<double>(i + 1) / static_cast<double>(count);
        const std::array<int, 2> at_low = enders(k, patch, low, offset);

        if (at_low == enders(k, patch, high, offset))
            continue;

        for (int step = 0; step < 50; step++) {
            const double middle = 0.5 * (low + high);

            if (enders(k, patch, middle, offset) == at_low)
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
        breaks.back() = patch.last;

    return breaks;
}

inline void CylinderCarving::validate(size_t k, const Patch& patch) const {

    // every crease of the patch with another carver at a clear angle; closed, no end on the stock's surface
    for (const std::pair<double, Part>& step : patch.track) {
        const std::pair<Point, Vector> line = generator(k, step.first, 0.0);

        if (closed && (step.second.low_by == 0 || step.second.high_by == 0)) {
            const Point point = line.first + line.second * (step.second.low_by == 0 ? step.second.low : step.second.high);
            throw std::invalid_argument(fmt::format("Carver {} ends on the stock's surface at ({:.0f}, {:.0f}, {:.0f}), leaving it uncarved beside it, a gap between the patches", k, point[0], point[1], point[2]));
        }

        for (const std::pair<double, int>& end : {std::make_pair(step.second.low, step.second.low_by), std::make_pair(step.second.high, step.second.high_by)}) {
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

inline void CylinderCarving::add_plates() {

    const std::shared_ptr<TreeNode> group = add_group("plates");

    for (size_t k = 0; k < carvers.size(); k++) {
        const std::vector<Patch> patches = compute_patches(k);

        for (size_t p = 0; p < patches.size(); p++) {
            const Patch& patch = patches[p];
            validate(k, patch);

            // the patch cut where a boundary changes what ends it, on the surface and behind it alike, every piece smooth
            const std::vector<double> breaks = compute_breaks(k, patch, 0.0, patch.first, patch.last);
            const std::vector<double> behind = compute_breaks(k, patch, thickness, patch.behind_first, patch.behind_last);

            if (breaks.size() != behind.size())
                throw std::invalid_argument(fmt::format("Carver {}'s plate {} ends in {} pieces on the surface and {} behind it", k, p, breaks.size() - 1, behind.size() - 1));

            std::array<std::vector<NurbsCurve>, 4> pieces;

            for (size_t b = 0; b + 1 < breaks.size(); b++) {
                const double share = (breaks[b + 1] - breaks[b]) / (patch.last - patch.first);
                const size_t steps = std::max<size_t>(3, static_cast<size_t>(std::ceil(share * static_cast<double>(samples))));
                std::array<std::vector<Point>, 4> points;

                // every generator of the piece on the carved surface and its match a thickness behind it, the two pieces spanned
                // alike so their corners meet on the same edge of a side face
                for (size_t i = 0; i <= steps; i++) {
                    const double t = static_cast<double>(i) / static_cast<double>(steps);
                    const double angle = breaks[b] + (breaks[b + 1] - breaks[b]) * t;
                    const double behind_angle = behind[b] + (behind[b + 1] - behind[b]) * t;
                    const std::pair<Point, Vector> outer = generator(k, angle, 0.0);
                    const std::pair<Point, Vector> inner = generator(k, behind_angle, thickness);
                    const std::optional<Part> outer_part = pick(k, angle, 0.0, reference(patch, angle));
                    const std::optional<Part> inner_part = pick(k, behind_angle, thickness, reference(patch, behind_angle));

                    if (!outer_part || !inner_part)
                        throw std::invalid_argument(fmt::format("Carver {} leaves no plate at its generator at {:.3f} radians", k, outer_part ? behind_angle : angle));

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
                fmt::format("plate_{}_{}", k, p)
            );
            add(plate, group);

            // its development, the proof it is cut flat
            const Development development(curves[0], curves[1], samples);
            _distortion = std::max(_distortion, development.distortion);
        }
    }
}
