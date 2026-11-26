#pragma once
#include "wood_session.h"
#include "../development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// Flat-foldable curved-crease unfolding (CCU) of developable plates with axis-shift thickness and isometric deployment.
class CurvedCreaseUnfolding : public WoodSession {
public:
    const NurbsCurve crease; // Target planar crease on one of the osculating planes.
    const double opening; // radians, target reflection angle between rulings and osculating plane.
    const std::vector<Plane> planes; // Osculating crease planes in order, a plate between every two.
    const double thickness; // mm, uniform plate thickness.
    const size_t samples; // Rulings per plate the creases are interpolated through.
    const std::vector<double> deployments; // Normalized unfolding angles w = alpha / opening in [0, 1].

private:
    double _distortion = 0.0; // Largest relative diagonal error of any plate's flat development.
    double _drift = 0.0; // mm, largest vertical support drift of crease feet from the floor.

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// Unfolded or partially deployed CCU shell at deployment w in [0, 1] (0 = closed flat stack, 1 = target 3D shell).
    CurvedCreaseUnfolding(
        const NurbsCurve& crease,
        double opening,
        const std::vector<Plane>& planes,
        double thickness = 40.0,
        const std::string& name = "curved_crease_unfolding",
        size_t samples = 48,
        double deployment = 1.0
    );

    /// Multi-stage kinematic deployment sequence side by side in groups stage_0, stage_1, ...
    CurvedCreaseUnfolding(
        const NurbsCurve& crease,
        double opening,
        const std::vector<Plane>& planes,
        const std::vector<double>& deployments,
        double thickness = 40.0,
        const std::string& name = "unfolding_sequence",
        size_t samples = 48
    );

    /// Not copied, as a session is not.
    CurvedCreaseUnfolding(const CurvedCreaseUnfolding&) = delete;

    /// Not assigned, as it is not copied.
    CurvedCreaseUnfolding& operator=(const CurvedCreaseUnfolding&) = delete;

    // ═══════════════════════════════════════════════════════════════════════════
    // Measurements
    // ═══════════════════════════════════════════════════════════════════════════

    /// Largest relative length error of any plate's flat development, 0 on exact developables.
    double distortion() const {
        return _distortion;
    }

    /// mm, how far the target creases' feet rise above or sink below the given crease's floor.
    double drift() const {
        return _drift;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Crease and plane factories
    // ═══════════════════════════════════════════════════════════════════════════

    /// Funicular catenary arch of span and rise over the x axis in the xz plane, centred on the origin.
    static NurbsCurve catenary(double span, double rise, size_t points = 48);

    /// Parallel osculating planes along along; step > 0 curves ridges into a synclastic vault, step < 0 into an anticlastic saddle.
    static std::vector<Plane> vault(
        const Point& origin,
        const Vector& along,
        const Vector& axis,
        size_t count,
        double spacing,
        double step = 0.0
    );

    /// Radial meridian planes around axis through origin, turned by angle from one to the next, the middle containing first.
    static std::vector<Plane>
    fan(const Point& origin, const Vector& axis, const Vector& first, size_t count, double angle);

    /// Osculating planes spacing apart along along, alternating +-tilt about axis every corrugation pair.
    static std::vector<Plane> zigzag(
        const Point& origin,
        const Vector& along,
        const Vector& axis,
        size_t count,
        double spacing,
        double tilt
    );

    /// Osculating planes spacing apart along along, the i-th from the middle turned by i * lean about axis.
    static std::vector<Plane> leaning(
        const Point& origin,
        const Vector& along,
        const Vector& axis,
        size_t count,
        double spacing,
        double lean
    );

private:
    /// Intrinsic geometry and axis-shift miter reaches of one plate between plane k and plane next.
    struct TargetStep {
        size_t from = 0;
        size_t to = 0;
        Vector plane_normal; // Unit normal of plane[from] pointing toward plane[to].
        std::vector<Point> neutral_start; // Neutral mid-surface points on plane[from].
        std::vector<Point> neutral_end; // Neutral mid-surface points on plane[to].
        std::vector<double> cos_gamma; // Ruling cosine against crease tangent.
        std::vector<double> sin_gamma; // Ruling sine against crease tangent.
        double inward_sign = 1.0; // Sign of ruling projection along plane_normal x tangent.
        std::vector<double> sin_alpha; // Ruling sine against plane[from] along the crease.
        std::vector<double> neutral_reach; // Neutral mid-surface ruling length to plane[to].
        std::vector<double> minus_start; // Reach on (-0.5*t) face to plane[from] (hinge at from).
        std::vector<double> minus_end; // Reach on (-0.5*t) face to plane[to] (outer miter tip).
        std::vector<double> plus_start; // Reach on (+0.5*t) face to plane[from] (outer miter tip).
        std::vector<double> plus_end; // Reach on (+0.5*t) face to plane[to] (hinge at to).
    };

    /// Deployed four-edge strip and hinge curves of one plate.
    struct DeployedPlate {
        std::vector<Point> minus_a;
        std::vector<Point> minus_b;
        std::vector<Point> plus_a;
        std::vector<Point> plus_b;
        std::vector<Point> neutral_a;
        std::vector<Point> neutral_b;
        std::vector<Point> crease_from;
        std::vector<Point> crease_to;
        size_t to_index = 0;
    };

    /// Deployed stage of all plates and the leading crease.
    struct DeployedStage {
        std::vector<DeployedPlate> plates;
        std::vector<Point> start_crease;
        size_t start_index = 0;
    };

    /// Index of the plane nearest the crease's ends and middle.
    size_t compute_start() const;

    /// Target strips on both sides of start at full opening, with axis-shift miter reaches.
    std::vector<TargetStep> compute_target(size_t start) const;

    /// Trace target neutral strips and miter reaches from start toward one end (step = +1 or -1).
    void trace_target_side(
        std::vector<Point> neutral,
        std::vector<Vector> rulings,
        size_t from,
        int step,
        std::vector<TargetStep>& steps
    ) const;

    /// Reconstruct all plates isometrically at deployment fraction w in [0, 1], translated by shift.
    DeployedStage compute_deployed(
        size_t start,
        const std::vector<TargetStep>& steps,
        double fraction,
        const Vector& shift
    ) const;

    /// Uncurl a planar crease on plane for reflection angle alpha = fraction * opening (Eq. 5 & 8).
    std::vector<Point> uncurl_crease(
        const std::vector<Point>& points,
        const Plane& plane,
        double fraction
    ) const;

    /// Rulings and thickness normals along a neutral curve for a given osculating normal and deployment fraction.
    void compute_strip_frame(
        const std::vector<Point>& curve,
        const Vector& plane_normal,
        const TargetStep& info,
        double fraction,
        std::vector<Vector>& rulings,
        std::vector<Vector>& normals
    ) const;

    /// Add one deployed stage's plates and creases to the session and return each plate's flat development.
    std::vector<Development> add_stage(
        const DeployedStage& stage,
        const std::string& group_name,
        const std::string& prefix,
        bool measure_drift
    );

    /// Add 2D cutting patterns of every plate side by side in the xy-plane under group "flat".
    void add_flat(const std::vector<Development>& developments);

    /// Unit tangents at uniform samples of an interpolated cubic through points.
    static std::vector<Vector> sample_tangents(const std::vector<Point>& points);

    /// Reflection of vector in the plane with unit normal.
    static Vector reflected(const Vector& vector, const Vector& normal);

    /// Signed distance along direction from point to plane; NaN when parallel.
    static double cut(const Point& point, const Vector& direction, const Plane& plane);
};

inline CurvedCreaseUnfolding::CurvedCreaseUnfolding(
    const NurbsCurve& crease,
    double opening,
    const std::vector<Plane>& planes,
    double thickness,
    const std::string& name,
    size_t samples,
    double deployment
)
    : CurvedCreaseUnfolding(
          crease, opening, planes, std::vector<double>{deployment}, thickness, name, samples
      ) {}

inline CurvedCreaseUnfolding::CurvedCreaseUnfolding(
    const NurbsCurve& crease,
    double opening,
    const std::vector<Plane>& planes,
    const std::vector<double>& deployments,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name), crease(crease), opening(opening), planes(planes), thickness(thickness),
      samples(std::max<size_t>(samples, 4)), deployments(deployments) {

    if (planes.size() < 2 || thickness <= 0.0 || opening <= 0.0 || opening >= 0.5 * Tolerance::PI ||
        deployments.empty())
        throw std::invalid_argument(
            "A curved-crease unfolding needs two planes, a thickness, an opening in (0, 90) degrees and at least one deployment"
        );

    for (double fraction : deployments)
        if (fraction < 0.0 || fraction > 1.0)
            throw std::invalid_argument("Deployment fraction must lie in [0, 1]");

    // start: the osculating plane the given crease lies on
    const size_t start = compute_start();

    // target: intrinsic strip angles, neutral widths and axis-shift miter reaches at w = 1
    const std::vector<TargetStep> target = compute_target(start);

    // stages: deploy one shell or a side-by-side unfolding sequence along the unfolding direction
    const bool sequence = deployments.size() > 1;
    Vector unfold_axis = planes[start].z_axis().normalized();
    if (start + 1 < planes.size()) {
        const Vector step_dir = planes[start + 1].origin() - planes[start].origin();
        if (step_dir.magnitude() > 1e-6)
            unfold_axis = step_dir.normalized();
    }

    const double gap = 0.35 * crease.point_at(crease.domain_start()).distance(crease.point_at(crease.domain_end()));
    double cursor = 0.0;
    std::vector<Development> developments;

    for (size_t stage_index = 0; stage_index < deployments.size(); stage_index++) {
        const DeployedStage unshifted = compute_deployed(start, target, deployments[stage_index], Vector(0.0, 0.0, 0.0));
        double low_along = std::numeric_limits<double>::infinity();
        double high_along = -std::numeric_limits<double>::infinity();
        for (const DeployedPlate& plate : unshifted.plates)
            for (const std::vector<Point>* edge : {&plate.minus_a, &plate.minus_b, &plate.plus_a, &plate.plus_b})
                for (const Point& point : *edge) {
                    const double coord = (point - planes[start].origin()).dot(unfold_axis);
                    low_along = std::min(low_along, coord);
                    high_along = std::max(high_along, coord);
                }

        const Vector shift = sequence ? unfold_axis * (cursor - low_along) : Vector(0.0, 0.0, 0.0);
        cursor += (high_along - low_along) + gap;

        const DeployedStage stage = sequence
            ? compute_deployed(start, target, deployments[stage_index], shift)
            : unshifted;
        const std::string group_name = sequence ? fmt::format("stage_{}", stage_index) : "plates";
        const std::string prefix = sequence ? fmt::format("stage_{}", stage_index) : "plate";
        const bool measure = !sequence || deployments[stage_index] == 1.0;
        const std::vector<Development> stage_developments = add_stage(stage, group_name, prefix, measure);
        if (developments.empty())
            developments = stage_developments;
    }

    // flat: 2D unrolled cutting patterns in the xy-plane beside the model
    add_flat(developments);

    // contacts: shared miter faces in any fully unfolded stage
    for (double fraction : deployments)
        if (std::abs(fraction - 1.0) < 1e-12) {
            compute_face_contacts(0);
            break;
        }

    // assembly: pairs of neighbours on their crease when mitered at full opening, sequential stack plates when partially unfolded
    if (!sequence) {
        std::vector<std::shared_ptr<FaceCurvedElement>> placed;
        const size_t plate_count = planes.size() - 1;
        for (size_t i = 0; i < plate_count; i++)
            placed.push_back(get_element_by_name<FaceCurvedElement>(fmt::format("plate_{}", i)));
        if (std::abs(deployments[0] - 1.0) < 1e-12) {
            std::vector<size_t> keys;
            for (size_t i = 0; i < placed.size(); i++)
                keys.push_back(i / 2);
            assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("fold", placed, keys))};
        } else {
            assemblies = {AssemblyModule::module(name, AssemblyModule::parts_of(placed))};
        }
    } else {
        for (size_t stage_index = 0; stage_index < deployments.size(); stage_index++) {
            const std::string stage_name = fmt::format("stage_{}", stage_index);
            std::vector<std::shared_ptr<FaceCurvedElement>> placed;
            for (size_t i = 0; i + 1 < planes.size(); i++)
                placed.push_back(get_element_by_name<FaceCurvedElement>(fmt::format("{}_{}", stage_name, i)));
            if (std::abs(deployments[stage_index] - 1.0) < 1e-12) {
                std::vector<size_t> keys;
                for (size_t i = 0; i < placed.size(); i++)
                    keys.push_back(i / 2);
                assemblies.push_back(AssemblyModule::module(stage_name, AssemblyModule::modules_of("fold", placed, keys)));
            } else {
                assemblies.push_back(AssemblyModule::module(stage_name, AssemblyModule::parts_of(placed)));
            }
        }
    }
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
        const Vector normal = planes[i].z_axis().normalized();
        double distance = 0.0;

        for (const Point& point : points)
            distance = std::max(distance, std::abs((point - planes[i].origin()).dot(normal)));

        if (distance < nearest) {
            nearest = distance;
            start = i;
        }
    }

    return start;
}

inline std::vector<CurvedCreaseUnfolding::TargetStep>
CurvedCreaseUnfolding::compute_target(size_t start) const {

    const Vector normal = planes[start].z_axis().normalized();
    const Plane& neighbour = planes[start + 1 < planes.size() ? start + 1 : start - 1];
    const double domain = crease.domain_end() - crease.domain_start();
    std::vector<Point> points;
    std::vector<Vector> tangents;

    for (size_t j = 0; j <= samples; j++) {
        const double t =
            crease.domain_start() + domain * static_cast<double>(j) / static_cast<double>(samples);
        points.push_back(crease.point_at(t));
        tangents.push_back(crease.tangent_at(t).normalized());

        if (std::abs((points.back() - planes[start].origin()).dot(normal)) > 1e-6 * crease.length())
            throw std::invalid_argument("The crease lies on none of the planes");
    }

    // concave side of the given crease
    double turning = 0.0;
    for (size_t j = 0; j < samples; j++)
        turning += tangents[j].cross(tangents[j + 1]).dot(normal);

    const double c = std::cos(opening);
    const double s = std::sin(opening);
    const size_t middle = samples / 2;
    const Vector inward_mid =
        (turning >= 0.0 ? normal.cross(tangents[middle]) : tangents[middle].cross(normal)).normalized();
    const double side =
        cut(points[middle], inward_mid * c + normal * s, neighbour) > 0.0 ? 1.0 : -1.0;

    std::vector<Vector> first_rulings;
    for (size_t j = 0; j <= samples; j++) {
        const Vector across =
            (turning >= 0.0 ? normal.cross(tangents[j]) : tangents[j].cross(normal)).normalized();
        first_rulings.push_back((across * c + normal * (side * s)).normalized());
    }

    const int primary_step = start + 1 < planes.size() ? 1 : -1;
    std::vector<Vector> opposite_rulings;
    for (const Vector& ruling : first_rulings)
        opposite_rulings.push_back(reflected(ruling, normal));

    std::vector<TargetStep> steps(planes.size() - 1);
    trace_target_side(points, first_rulings, start, primary_step, steps);
    trace_target_side(points, opposite_rulings, start, -primary_step, steps);

    return steps;
}

inline void CurvedCreaseUnfolding::trace_target_side(
    std::vector<Point> neutral,
    std::vector<Vector> rulings,
    size_t from,
    int step,
    std::vector<TargetStep>& steps
) const {

    const double half = 0.5 * thickness;
    const size_t middle = samples / 2;

    for (size_t k = from; step > 0 ? k + 1 < planes.size() : k > 0; k += step) {
        const size_t next = static_cast<size_t>(static_cast<long>(k) + step);
        Vector plane_normal = planes[k].z_axis().normalized();
        if (rulings[middle].dot(plane_normal) < 0.0)
            plane_normal = plane_normal * -1.0;

        const std::vector<Vector> tangents = sample_tangents(neutral);
        TargetStep info;
        info.from = k;
        info.to = next;
        info.plane_normal = plane_normal;
        info.neutral_start = neutral;

        std::vector<double> across_proj(samples + 1, 0.0);
        for (size_t j = 0; j <= samples; j++) {
            const Vector across = plane_normal.cross(tangents[j]).normalized();
            const double cg = std::clamp(rulings[j].dot(tangents[j]), -1.0, 1.0);
            const double sg = std::sqrt(std::max(0.0, 1.0 - cg * cg));
            info.cos_gamma.push_back(cg);
            info.sin_gamma.push_back(sg);
            across_proj[j] = sg > 1e-12 ? rulings[j].dot(across) / sg : 1.0;
            const double sb = sg > 1e-12 ? rulings[j].dot(plane_normal) / sg : 0.0;
            info.sin_alpha.push_back(std::clamp(sb, 0.0, 1.0));
        }
        info.inward_sign = across_proj[middle] >= 0.0 ? 1.0 : -1.0;

        std::vector<Point> ends;
        for (size_t j = 0; j <= samples; j++) {
            Vector strip_normal = tangents[j].cross(rulings[j]).normalized();
            if (strip_normal.dot(plane_normal) < 0.0)
                strip_normal = strip_normal * -1.0;

            const double reach = cut(neutral[j], rulings[j], planes[next]);
            const Point minus_pt = neutral[j] - strip_normal * half;
            const Point plus_pt = neutral[j] + strip_normal * half;
            const double m0 = cut(minus_pt, rulings[j], planes[k]);
            const double m1 = cut(minus_pt, rulings[j], planes[next]);
            const double p0 = cut(plus_pt, rulings[j], planes[k]);
            const double p1 = cut(plus_pt, rulings[j], planes[next]);

            if (!std::isfinite(reach) || reach <= 0.0 || !std::isfinite(m1) || m1 <= 0.0 ||
                !std::isfinite(p1) || p1 <= 0.0)
                throw std::invalid_argument(fmt::format(
                    "Plane {} does not cut ruling {} ahead of the crease on plane {}", next, j, k
                ));

            info.neutral_reach.push_back(reach);
            info.minus_start.push_back(m0);
            info.minus_end.push_back(m1);
            info.plus_start.push_back(p0);
            info.plus_end.push_back(p1);
            ends.push_back(neutral[j] + rulings[j] * reach);
        }

        info.neutral_end = ends;
        steps[std::min(k, next)] = info;

        const Vector far_normal = planes[next].z_axis().normalized();
        for (size_t j = 0; j <= samples; j++)
            rulings[j] = reflected(rulings[j], far_normal) * -1.0;
        neutral = ends;
    }
}

inline CurvedCreaseUnfolding::DeployedStage CurvedCreaseUnfolding::compute_deployed(
    size_t start,
    const std::vector<TargetStep>& steps,
    double fraction,
    const Vector& shift
) const {

    const double half = 0.5 * thickness;
    const size_t plate_count = planes.size() - 1;
    DeployedStage stage;
    stage.plates.resize(plate_count);
    stage.start_index = start;

    const TargetStep& seed = steps[start < plate_count ? start : start - 1];
    const std::vector<Point>& start_target =
        seed.from == start ? seed.neutral_start : seed.neutral_end;
    const std::vector<Point> uncurled = uncurl_crease(start_target, planes[start], fraction);
    for (const Point& point : uncurled)
        stage.start_crease.push_back(point + shift);

    // primary direction first to establish the shared axis-shifted hinge at start
    const int primary = start < plate_count ? 1 : -1;
    std::vector<Point> shared_start_hinge;

    for (int pass = 0; pass < 2; pass++) {
        const int step = pass == 0 ? primary : -primary;
        if ((step > 0 && start >= plate_count) || (step < 0 && start == 0))
            continue;

        const size_t first_idx = std::min(start, static_cast<size_t>(static_cast<long>(start) + step));
        Vector osculating = steps[first_idx].plane_normal;
        if (fraction == 0.0 && pass == 1)
            osculating = steps[std::min(start, static_cast<size_t>(static_cast<long>(start) + primary))]
                             .plane_normal *
                         -1.0;

        std::vector<Point> current_hinge = shared_start_hinge;
        std::vector<Point> hint_curve = stage.start_crease;

        for (size_t k = start; step > 0 ? k + 1 < planes.size() : k > 0; k += step) {
            const size_t next = static_cast<size_t>(static_cast<long>(k) + step);
            const size_t idx = std::min(k, next);
            const TargetStep& info = steps[idx];

            std::vector<Point> neutral_a(samples + 1);
            std::vector<Vector> rulings;
            std::vector<Vector> normals;

            if (fraction == 1.0) {
                for (size_t j = 0; j <= samples; j++)
                    neutral_a[j] = info.neutral_start[j] + shift;
                osculating = info.plane_normal;
                compute_strip_frame(neutral_a, osculating, info, 1.0, rulings, normals);
                if (k == start && pass == 0) {
                    shared_start_hinge.resize(samples + 1);
                    for (size_t j = 0; j <= samples; j++)
                        shared_start_hinge[j] =
                            (neutral_a[j] - normals[j] * half) + rulings[j] * info.minus_start[j];
                }
            } else if (k == start && pass == 0) {
                neutral_a = stage.start_crease;
                compute_strip_frame(neutral_a, osculating, info, fraction, rulings, normals);
                shared_start_hinge.resize(samples + 1);
                for (size_t j = 0; j <= samples; j++)
                    shared_start_hinge[j] =
                        (neutral_a[j] - normals[j] * half) + rulings[j] * info.minus_start[j];
            } else {
                compute_strip_frame(hint_curve, osculating, info, fraction, rulings, normals);
                for (size_t j = 0; j <= samples; j++)
                    neutral_a[j] =
                        current_hinge[j] + normals[j] * half - rulings[j] * info.minus_start[j];
                compute_strip_frame(neutral_a, osculating, info, fraction, rulings, normals);
                for (size_t j = 0; j <= samples; j++)
                    neutral_a[j] =
                        current_hinge[j] + normals[j] * half - rulings[j] * info.minus_start[j];
            }

            DeployedPlate plate;
            plate.to_index = next;
            plate.neutral_a = neutral_a;
            for (size_t j = 0; j <= samples; j++) {
                const Point minus_pt = neutral_a[j] - normals[j] * half;
                const Point plus_pt = neutral_a[j] + normals[j] * half;
                plate.neutral_b.push_back(neutral_a[j] + rulings[j] * info.neutral_reach[j]);
                plate.minus_a.push_back(minus_pt + rulings[j] * info.minus_start[j]);
                plate.minus_b.push_back(minus_pt + rulings[j] * info.minus_end[j]);
                plate.plus_a.push_back(plus_pt + rulings[j] * info.plus_start[j]);
                plate.plus_b.push_back(plus_pt + rulings[j] * info.plus_end[j]);
            }
            plate.crease_from = plate.minus_a;
            plate.crease_to = plate.plus_b;
            stage.plates[idx] = plate;

            current_hinge = plate.plus_b;
            hint_curve = plate.neutral_b;
            if (fraction > 0.0 && fraction < 1.0) {
                const Plane fitted = Plane::from_points_pca(current_hinge);
                Vector next_normal = fitted.z_axis().normalized();
                if (next_normal.dot(osculating) < 0.0)
                    next_normal = next_normal * -1.0;
                osculating = next_normal;
            }
        }
    }

    return stage;
}

inline std::vector<Point> CurvedCreaseUnfolding::uncurl_crease(
    const std::vector<Point>& points,
    const Plane& plane,
    double fraction
) const {

    if (std::abs(fraction - 1.0) < 1e-12)
        return points;

    const Vector u = plane.x_axis().normalized();
    const Vector v = plane.y_axis().normalized();
    const Point origin = plane.origin();
    const size_t count = points.size() - 1;

    std::vector<std::array<double, 2>> xy_target(count + 1);
    for (size_t j = 0; j <= count; j++) {
        const Vector offset = points[j] - origin;
        xy_target[j] = {offset.dot(u), offset.dot(v)};
    }

    std::vector<double> lengths(count);
    std::vector<double> headings(count);
    for (size_t j = 0; j < count; j++) {
        const double dx = xy_target[j + 1][0] - xy_target[j][0];
        const double dy = xy_target[j + 1][1] - xy_target[j][1];
        lengths[j] = std::hypot(dx, dy);
        headings[j] = std::atan2(dy, dx);
    }

    const double ratio = std::cos(opening) / std::cos(fraction * opening);
    std::vector<double> turns(count - 1);
    for (size_t j = 0; j + 1 < count; j++) {
        const double delta = std::atan2(
            std::sin(headings[j + 1] - headings[j]),
            std::cos(headings[j + 1] - headings[j])
        );
        const double scaled = std::clamp(ratio * std::sin(0.5 * delta), -1.0, 1.0);
        turns[j] = 2.0 * std::asin(scaled);
    }

    const size_t middle = count / 2;
    std::vector<double> deployed_headings(count);
    deployed_headings[middle] = headings[middle];
    for (size_t j = middle; j + 1 < count; j++)
        deployed_headings[j + 1] = deployed_headings[j] + turns[j];
    for (size_t j = middle; j > 0; j--)
        deployed_headings[j - 1] = deployed_headings[j] - turns[j - 1];

    std::vector<std::array<double, 2>> xy(count + 1);
    xy[middle] = xy_target[middle];
    for (size_t j = middle; j < count; j++)
        xy[j + 1] = {
            xy[j][0] + lengths[j] * std::cos(deployed_headings[j]),
            xy[j][1] + lengths[j] * std::sin(deployed_headings[j]),
        };
    for (size_t j = middle; j > 0; j--)
        xy[j - 1] = {
            xy[j][0] - lengths[j - 1] * std::cos(deployed_headings[j - 1]),
            xy[j][1] - lengths[j - 1] * std::sin(deployed_headings[j - 1]),
        };

    double target_floor = std::numeric_limits<double>::infinity();
    double deployed_floor = std::numeric_limits<double>::infinity();
    std::vector<Point> result(count + 1);
    for (size_t j = 0; j <= count; j++) {
        result[j] = origin + u * xy[j][0] + v * xy[j][1];
        target_floor = std::min(target_floor, points[j][2]);
        deployed_floor = std::min(deployed_floor, result[j][2]);
    }

    const Vector lift(0.0, 0.0, target_floor - deployed_floor);
    for (Point& point : result)
        point = point + lift;

    return result;
}

inline void CurvedCreaseUnfolding::compute_strip_frame(
    const std::vector<Point>& curve,
    const Vector& plane_normal,
    const TargetStep& info,
    double fraction,
    std::vector<Vector>& rulings,
    std::vector<Vector>& normals
) const {

    const std::vector<Vector> tangents = sample_tangents(curve);
    const size_t middle = samples / 2;
    const double mid_alpha = std::asin(std::clamp(info.sin_alpha[middle], 1e-12, 1.0));
    const double scale = std::sin(fraction * mid_alpha) / std::sin(mid_alpha);

    rulings.resize(samples + 1);
    normals.resize(samples + 1);

    for (size_t j = 0; j <= samples; j++) {
        const Vector across = plane_normal.cross(tangents[j]).normalized();
        const double sa = std::clamp(scale * info.sin_alpha[j], 0.0, 1.0);
        const double ca = std::sqrt(std::max(0.0, 1.0 - sa * sa));
        const Vector ruling = (
            tangents[j] * info.cos_gamma[j] +
            (across * (info.inward_sign * ca) + plane_normal * sa) * info.sin_gamma[j]
        ).normalized();
        Vector strip_normal = tangents[j].cross(ruling).normalized();
        if (strip_normal.dot(plane_normal) < 0.0)
            strip_normal = strip_normal * -1.0;

        rulings[j] = ruling;
        normals[j] = strip_normal;
    }
}

inline std::vector<Development> CurvedCreaseUnfolding::add_stage(
    const DeployedStage& stage,
    const std::string& group_name,
    const std::string& prefix,
    bool measure_drift
) {

    const std::shared_ptr<TreeNode> plate_group = group_named(group_name);
    const std::shared_ptr<TreeNode> creases = group_named("creases");
    const double start_height = crease.point_at(crease.domain_start())[2];
    const double end_height = crease.point_at(crease.domain_end())[2];
    const double floor = std::min(start_height, end_height);

    Polyline start_line(stage.start_crease);
    start_line.name = fmt::format("{}_crease_{}", prefix, stage.start_index);
    add_polyline(start_line, creases);

    std::vector<Development> developments;

    for (size_t i = 0; i < stage.plates.size(); i++) {
        const DeployedPlate& data = stage.plates[i];
        NurbsCurve outer_a = NurbsCurve::create_interpolated(data.plus_a, CurveNurbsKnotStyle::Uniform);
        NurbsCurve outer_b = NurbsCurve::create_interpolated(data.plus_b, CurveNurbsKnotStyle::Uniform);
        NurbsCurve inner_a = NurbsCurve::create_interpolated(data.minus_a, CurveNurbsKnotStyle::Uniform);
        NurbsCurve inner_b = NurbsCurve::create_interpolated(data.minus_b, CurveNurbsKnotStyle::Uniform);

        // ensure outward closed_mesh winding: ((outer_b - outer_a) x tangent) . (outer_a - inner_a) > 0
        const double mid_t = 0.5 * (outer_a.domain_start() + outer_a.domain_end());
        const Vector tangent = outer_a.tangent_at(mid_t);
        const Vector across = outer_b.point_at(0.5 * (outer_b.domain_start() + outer_b.domain_end())) - outer_a.point_at(mid_t);
        const Vector up = outer_a.point_at(mid_t) - inner_a.point_at(0.5 * (inner_a.domain_start() + inner_a.domain_end()));
        if (across.cross(tangent).dot(up) < 0.0) {
            std::swap(outer_a, outer_b);
            std::swap(inner_a, inner_b);
        }

        const std::shared_ptr<FaceCurvedElement> plate = std::make_shared<FaceCurvedElement>(
            outer_a, outer_b, inner_a, inner_b, fmt::format("{}_{}", prefix, i)
        );
        add(plate, plate_group);

        developments.emplace_back(*plate, samples, fmt::format("flat_{}", i));
        _distortion = std::max(_distortion, developments.back().distortion);

        if (measure_drift) {
            if (start_height < floor + 1.0)
                _drift = std::max(_drift, std::abs(data.neutral_b.front()[2] - floor));
            if (end_height < floor + 1.0)
                _drift = std::max(_drift, std::abs(data.neutral_b.back()[2] - floor));
        }

        Polyline hinge_line(data.crease_to);
        hinge_line.name = fmt::format("{}_crease_{}", prefix, data.to_index);
        add_polyline(hinge_line, creases);
    }

    return developments;
}

inline void CurvedCreaseUnfolding::add_flat(const std::vector<Development>& developments) {

    const std::shared_ptr<TreeNode> flat = group_named("flat");
    double max_x = -std::numeric_limits<double>::infinity();
    for (const std::shared_ptr<Element>& element : elements())
        for (const Point& corner : element->aabb().corners())
            max_x = std::max(max_x, corner[0]);

    double cursor_x = (std::isfinite(max_x) ? max_x : 0.0) + 1200.0;

    for (size_t i = 0; i < developments.size(); i++) {
        double low_x = std::numeric_limits<double>::infinity();
        double high_x = -std::numeric_limits<double>::infinity();
        double low_y = std::numeric_limits<double>::infinity();
        for (const Polyline* loop : {&developments[i].bottom, &developments[i].top})
            for (const Point& point : loop->get_points()) {
                low_x = std::min(low_x, point[0]);
                high_x = std::max(high_x, point[0]);
                low_y = std::min(low_y, point[1]);
            }
        const Xform shift = Xform::translation(cursor_x - low_x, -low_y, 0.0);
        if (developments[i].piece) {
            const std::shared_ptr<FaceCurvedElement> flat_piece =
                std::make_shared<FaceCurvedElement>(*developments[i].piece);
            flat_piece->name = fmt::format("flat_{}", i);
            flat_piece->place(shift);
            add(flat_piece, flat);
        }
        cursor_x += (high_x - low_x) + 300.0;
    }
}

inline std::vector<Vector> CurvedCreaseUnfolding::sample_tangents(const std::vector<Point>& points) {

    const NurbsCurve curve = NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform);
    const double domain = curve.domain_end() - curve.domain_start();
    const size_t count = points.size() - 1;
    std::vector<Vector> tangents;
    tangents.reserve(count + 1);

    for (size_t j = 0; j <= count; j++) {
        const double t =
            curve.domain_start() + domain * static_cast<double>(j) / static_cast<double>(count);
        tangents.push_back(curve.tangent_at(t).normalized());
    }

    return tangents;
}

inline Vector CurvedCreaseUnfolding::reflected(const Vector& vector, const Vector& normal) {
    return vector - normal * (2.0 * vector.dot(normal));
}

inline double
CurvedCreaseUnfolding::cut(const Point& point, const Vector& direction, const Plane& plane) {

    const Vector normal = plane.z_axis();
    const double along = direction.dot(normal);

    if (std::abs(along) < 1e-12)
        return std::numeric_limits<double>::quiet_NaN();

    return (plane.origin() - point).dot(normal) / along;
}

inline NurbsCurve CurvedCreaseUnfolding::catenary(double span, double rise, size_t points) {

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

inline std::vector<Plane> CurvedCreaseUnfolding::vault(
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

inline std::vector<Plane> CurvedCreaseUnfolding::fan(
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

inline std::vector<Plane> CurvedCreaseUnfolding::zigzag(
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

inline std::vector<Plane> CurvedCreaseUnfolding::leaning(
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
