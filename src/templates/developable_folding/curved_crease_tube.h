#pragma once
#include "wood_session.h"
#include "../development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// Flat-foldable composite curved-crease tubular vault on non-planar space curves with harmonic torsion (Tachi 2013).
class CurvedCreaseTube : public WoodSession {
public:
    const double span; // mm, horizontal chord span of the reference space curve at target opening.
    const double torsion_ratio; // Harmonic torsion amplitude ratio a = tau_max / kappa_0.
    const double opening; // radians, target half-folding angle alpha_0 in (0, pi / 2).
    const double width_left; // mm, perpendicular strip width w_L(0) at both supports.
    const double width_right; // mm, perpendicular strip width w_R(0) at both supports.
    const size_t tubes; // Number of parallelogram tubes chained from corner (b, b) to (b + 1, b + 1).
    const double thickness; // mm, uniform timber plate thickness.
    const size_t samples; // Stations sampled along the arch.
    const std::vector<double> deployments; // Folding-angle ratios w = alpha / opening > 0.

private:
    double _distortion = 0.0; // Largest relative diagonal error of any plate's flat development.
    double _min_speed = 1.0; // Smallest arclength speed ds_{u,v} / ds > 0 (regression margin).
    double _midspan_taper = 1.0; // Width taper ratio w_R(0.5 * s_0) / w_R(0) at mid-arch.

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// Composite tubular vault at deployment w = alpha / opening (1 = coplanar ground supports at target opening).
    CurvedCreaseTube(
        double span = 8000.0,
        double torsion_ratio = 0.30,
        double opening = 45.0 * Tolerance::TO_RADIANS,
        double width_left = 480.0,
        double width_right = 360.0,
        size_t tubes = 6,
        double thickness = 24.0,
        const std::string& name = "curved_crease_tube",
        size_t samples = 48,
        double deployment = 1.0
    );

    /// Side-by-side 1-DOF rigid-folding motion stages across multiple deployment fractions.
    CurvedCreaseTube(
        double span,
        double torsion_ratio,
        double opening,
        double width_left,
        double width_right,
        size_t tubes,
        const std::vector<double>& deployments,
        double thickness = 24.0,
        const std::string& name = "curved_crease_tube_sequence",
        size_t samples = 48
    );

    /// Not copied, as a session is not.
    CurvedCreaseTube(const CurvedCreaseTube&) = delete;

    /// Not assigned, as it is not copied.
    CurvedCreaseTube& operator=(const CurvedCreaseTube&) = delete;

    // ═══════════════════════════════════════════════════════════════════════════
    // Measurements
    // ═══════════════════════════════════════════════════════════════════════════

    /// Largest relative diagonal error of any plate's flat development.
    double distortion() const {
        return _distortion;
    }

    /// Smallest ruling arclength speed ds_{u,v} / ds across the grid; strictly positive away from regression.
    double min_speed() const {
        return _min_speed;
    }

    /// Mid-arch cross-section width taper w_R(0.5 * s_0) / w_R(0) induced by space-curve torsion.
    double midspan_taper() const {
        return _midspan_taper;
    }

private:
    /// Sampled Frenet-Serret frame along a constant-curvature harmonic-torsion space curve.
    struct FrenetSamples {
        std::vector<Point> points;
        std::vector<Vector> tangents;
        std::vector<Vector> normals;
        std::vector<Vector> binormals;
    };

    /// Intrinsic alpha-invariant strip widths and ruling sector angles along s in [0, s_0].
    struct IntrinsicProfile {
        double arc_angle = 0.0; // Dimensionless total curvature kappa_0 * s_0 satisfying T(0.5 * s_0) . T(0) = 0.
        double arc_length = 0.0; // mm, total reference space-curve arclength s_0.
        double curvature = 0.0; // 1/mm, reference space-curve curvature kappa_0.
        std::vector<double> cos_beta;
        std::vector<double> sin_beta;
        std::vector<double> width_l;
        std::vector<double> width_r;
        std::vector<double> length_l;
        std::vector<double> length_r;
        double min_speed = 1.0;
        double midspan_taper = 1.0;
    };

    /// Four-polyline thick ruled wall of one tube plate.
    struct TubePlate {
        std::string suffix;
        size_t tube_index = 0;
        std::vector<Point> outer_a;
        std::vector<Point> outer_b;
        std::vector<Point> inner_a;
        std::vector<Point> inner_b;
    };

    /// Deployed plates and spine creases of one vault stage.
    struct DeployedVault {
        std::vector<TubePlate> plates;
        std::vector<std::vector<Point>> creases;
    };

    /// Midpoint Darboux exponential integration of a space curve on [0, length].
    static FrenetSamples integrate_frenet(
        double length,
        double curvature,
        double torsion_amplitude,
        double period_length,
        size_t steps
    );

    /// Solves T(0.5 * s_0) . T(0) = 0 for kappa_0 * s_0 and computes the alpha-invariant strip profile.
    IntrinsicProfile compute_profile() const;

    /// Builds one mitered L- or R-wall of a parallelogram tube across all stations.
    TubePlate build_wall(
        const IntrinsicProfile& profile,
        const std::vector<Point>& p1,
        const std::vector<Vector>& tangents,
        const std::vector<Vector>& rl,
        const std::vector<Vector>& rr,
        const std::vector<double>& widths,
        const std::vector<Vector>& rulings,
        const std::vector<Vector>& wall_normals,
        size_t u,
        size_t v,
        double y_out,
        double start_shift,
        double end_shift,
        double cos_2a,
        double sin_2a,
        size_t tube_index,
        const char* suffix
    ) const;

    /// Integrates the deployed Frenet frame at alpha = w * opening and builds every tube's four mitered walls.
    DeployedVault compute_stage(const IntrinsicProfile& profile, double deployment, const Vector& shift) const;

    /// Adds a stage's curved plates and creases to the session and returns the flat developments.
    std::vector<Development> add_stage(
        const DeployedVault& stage,
        const std::string& group_name,
        const std::string& prefix,
        bool measure
    );

    /// Places the unfolded 3D flat solid pieces in a tubes x 4 layout inside group flat.
    void add_flat(const std::vector<Development>& developments);
};

inline CurvedCreaseTube::CurvedCreaseTube(
    double span,
    double torsion_ratio,
    double opening,
    double width_left,
    double width_right,
    size_t tubes,
    double thickness,
    const std::string& name,
    size_t samples,
    double deployment
)
    : CurvedCreaseTube(
          span,
          torsion_ratio,
          opening,
          width_left,
          width_right,
          tubes,
          std::vector<double>{deployment},
          thickness,
          name,
          samples
      ) {}

inline CurvedCreaseTube::CurvedCreaseTube(
    double span,
    double torsion_ratio,
    double opening,
    double width_left,
    double width_right,
    size_t tubes,
    const std::vector<double>& deployments,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name), span(span), torsion_ratio(torsion_ratio), opening(opening),
      width_left(width_left), width_right(width_right), tubes(tubes), thickness(thickness),
      samples(std::max<size_t>(samples, 8)), deployments(deployments) {

    if (span <= 0.0 || width_left <= 0.0 || width_right <= 0.0 || tubes == 0 || thickness <= 0.0 ||
        opening <= 0.05 || opening >= 0.5 * Tolerance::PI - 0.05 || deployments.empty())
        throw std::invalid_argument(
            "A curved-crease tube needs positive span, widths, tube count and thickness, opening in (3, 87) degrees and at least one deployment"
        );

    for (double fraction : deployments) {
        const double alpha = fraction * opening;
        if (alpha <= 0.02 || alpha >= 0.5 * Tolerance::PI - 0.02)
            throw std::invalid_argument("Deployed half-angle w * opening must lie strictly inside (0, pi / 2)");
    }

    // profile: coplanar-support space-curve root and alpha-invariant strip widths and sector angles
    const IntrinsicProfile profile = compute_profile();
    _min_speed = profile.min_speed;
    _midspan_taper = profile.midspan_taper;

    // stages: single composite vault or side-by-side 1-DOF rigid-folding sequence
    const bool sequence = deployments.size() > 1;
    const double gap = 0.28 * span;
    double cursor = 0.0;
    std::vector<Development> developments;

    for (size_t stage_index = 0; stage_index < deployments.size(); stage_index++) {
        const DeployedVault unshifted = compute_stage(profile, deployments[stage_index], Vector(0.0, 0.0, 0.0));
        double low_y = std::numeric_limits<double>::infinity();
        double high_y = -std::numeric_limits<double>::infinity();
        for (const TubePlate& plate : unshifted.plates)
            for (const std::vector<Point>* edge : {&plate.outer_a, &plate.outer_b, &plate.inner_a, &plate.inner_b})
                for (const Point& point : *edge) {
                    low_y = std::min(low_y, point[1]);
                    high_y = std::max(high_y, point[1]);
                }

        const Vector shift = sequence ? Vector(0.0, cursor - low_y, 0.0) : Vector(0.0, 0.0, 0.0);
        cursor += (high_y - low_y) + gap;

        const DeployedVault stage = sequence ? compute_stage(profile, deployments[stage_index], shift) : unshifted;
        const std::string group_name = sequence ? fmt::format("stage_{}", stage_index) : "plates";
        const std::string prefix = sequence ? fmt::format("stage_{}_tube", stage_index) : "tube";
        const bool measure = !sequence || std::abs(deployments[stage_index] - 1.0) < 1e-12;
        const std::vector<Development> stage_developments = add_stage(stage, group_name, prefix, measure);
        if (developments.empty())
            developments = stage_developments;
    }

    // flat: unfolded 3D beveled flat timber pieces in a tubes x 4 nesting sheet
    add_flat(developments);

    // contacts: shared bevel and spine faces across every tube
    compute_face_contacts(0);

    // assembly: four walls per tube module, then tubes assembled laterally across the vault
    if (!sequence) {
        std::vector<std::shared_ptr<FaceCurvedElement>> placed;
        std::vector<size_t> keys;
        const std::array<const char*, 4> order = {"bottom_left", "top_left", "bottom_right", "top_right"};
        for (size_t b = 0; b < tubes; b++) {
            for (const char* suffix : order) {
                placed.push_back(get_element_by_name<FaceCurvedElement>(fmt::format("tube_{}_{}", b, suffix)));
                keys.push_back(b);
            }
        }
        if (tubes > 1)
            assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("tube", placed, keys))};
        else
            assemblies = {AssemblyModule::module(name, AssemblyModule::parts_of(placed))};
    } else {
        const std::array<const char*, 4> order = {"bottom_left", "top_left", "bottom_right", "top_right"};
        for (size_t stage_index = 0; stage_index < deployments.size(); stage_index++) {
            std::vector<std::shared_ptr<FaceCurvedElement>> placed;
            std::vector<size_t> keys;
            for (size_t b = 0; b < tubes; b++) {
                for (const char* suffix : order) {
                    placed.push_back(
                        get_element_by_name<FaceCurvedElement>(fmt::format("stage_{}_tube_{}_{}", stage_index, b, suffix))
                    );
                    keys.push_back(b);
                }
            }
            const std::string stage_name = fmt::format("stage_{}", stage_index);
            if (tubes > 1)
                assemblies.push_back(AssemblyModule::module(stage_name, AssemblyModule::modules_of("tube", placed, keys)));
            else
                assemblies.push_back(AssemblyModule::module(stage_name, AssemblyModule::parts_of(placed)));
        }
    }
}

inline CurvedCreaseTube::FrenetSamples CurvedCreaseTube::integrate_frenet(
    double length,
    double curvature,
    double torsion_amplitude,
    double period_length,
    size_t steps
) {

    const double ds = length / static_cast<double>(steps);
    Vector tangent(0.0, 0.0, 1.0);
    Vector normal(1.0, 0.0, 0.0);
    Vector binormal = tangent.cross(normal);
    Point position(0.0, 0.0, 0.0);

    FrenetSamples out;
    out.points.reserve(steps + 1);
    out.tangents.reserve(steps + 1);
    out.normals.reserve(steps + 1);
    out.binormals.reserve(steps + 1);
    out.points.push_back(position);
    out.tangents.push_back(tangent);
    out.normals.push_back(normal);
    out.binormals.push_back(binormal);

    for (size_t i = 0; i < steps; i++) {
        const double sm = (static_cast<double>(i) + 0.5) * ds;
        const double tau = torsion_amplitude * std::sin(2.0 * Tolerance::PI * sm / period_length);
        const Vector darboux = tangent * tau + binormal * curvature;
        const double rate = darboux.magnitude();
        const double angle = rate * ds;

        Vector next_t = tangent;
        Vector next_n = normal;
        if (angle > 1e-15) {
            const Vector axis = darboux * (1.0 / rate);
            const double ca = std::cos(angle);
            const double sa = std::sin(angle);
            next_t = tangent * ca + axis.cross(tangent) * sa + axis * (axis.dot(tangent) * (1.0 - ca));
            next_n = normal * ca + axis.cross(normal) * sa + axis * (axis.dot(normal) * (1.0 - ca));
        }

        position = position + (tangent + next_t) * (0.5 * ds);
        tangent = next_t.normalized();
        normal = (next_n - tangent * next_n.dot(tangent)).normalized();
        binormal = tangent.cross(normal);

        out.points.push_back(position);
        out.tangents.push_back(tangent);
        out.normals.push_back(normal);
        out.binormals.push_back(binormal);
    }

    return out;
}

inline CurvedCreaseTube::IntrinsicProfile CurvedCreaseTube::compute_profile() const {

    // Solve T(0.5 * s_0) . T(0) = 0 for dimensionless total curvature arc_angle = kappa_0 * s_0 >= pi
    double arc_angle = Tolerance::PI;
    if (std::abs(torsion_ratio) > 1e-12) {
        double lo = Tolerance::PI;
        double hi = 5.0;
        for (int iter = 0; iter < 60; iter++) {
            const double mid = 0.5 * (lo + hi);
            const FrenetSamples half = integrate_frenet(0.5 * mid, 1.0, torsion_ratio, mid, 256);
            if (half.tangents.back()[2] > 0.0)
                lo = mid;
            else
                hi = mid;
        }
        arc_angle = 0.5 * (lo + hi);
    }

    const size_t fine_steps = 16 * samples;
    const FrenetSamples unit_arch = integrate_frenet(arc_angle, 1.0, torsion_ratio, arc_angle, fine_steps);
    const Vector unit_chord = unit_arch.points.back() - unit_arch.points.front();
    const double chord_xy = std::hypot(unit_chord[0], unit_chord[1]);
    const double scale = span / chord_xy;

    IntrinsicProfile profile;
    profile.arc_angle = arc_angle;
    profile.arc_length = arc_angle * scale;
    profile.curvature = 1.0 / scale;
    profile.cos_beta.resize(samples + 1);
    profile.sin_beta.resize(samples + 1);
    profile.width_l.resize(samples + 1);
    profile.width_r.resize(samples + 1);
    profile.length_l.resize(samples + 1);
    profile.length_r.resize(samples + 1);

    const double k0 = profile.curvature;
    const double s0 = profile.arc_length;
    const double k2d = k0 * std::cos(opening);
    const double sin_a0 = std::sin(opening);
    const double tan_a0 = std::tan(opening);
    double min_speed = std::numeric_limits<double>::infinity();

    for (size_t j = 0; j <= samples; j++) {
        const double s = s0 * static_cast<double>(j) / static_cast<double>(samples);
        const double phase = 2.0 * Tolerance::PI * s / s0;
        const double tau0 = torsion_ratio * k0 * std::sin(phase);
        const double dtau0 = torsion_ratio * k0 * (2.0 * Tolerance::PI / s0) * std::cos(phase);

        const double cot_b = tau0 / (k0 * sin_a0);
        const double dcot_b = dtau0 / (k0 * sin_a0);
        const double beta = std::atan2(1.0, cot_b);
        const double cb = std::cos(beta);
        const double sb = std::sin(beta);

        const double phi = (torsion_ratio * arc_angle / (2.0 * Tolerance::PI * tan_a0)) * (1.0 - std::cos(phase));
        const double wl = width_left * std::exp(-phi);
        const double wr = width_right * std::exp(phi);
        const double dwl = -wl * tau0 / tan_a0;
        const double dwr = wr * tau0 / tan_a0;

        profile.cos_beta[j] = cb;
        profile.sin_beta[j] = sb;
        profile.width_l[j] = wl;
        profile.width_r[j] = wr;
        profile.length_l[j] = wl / sb;
        profile.length_r[j] = wr / sb;

        const double term_l = (dwl * cot_b + wl * dcot_b) - wl * k2d;
        const double term_r = (dwr * cot_b + wr * dcot_b) + wr * k2d;
        for (size_t b = 0; b < tubes; b++) {
            for (const std::array<size_t, 2>& uv :
                 std::array<std::array<size_t, 2>, 4>{{{b, b}, {b + 1, b}, {b, b + 1}, {b + 1, b + 1}}}) {
                const double speed =
                    1.0 + static_cast<double>(uv[0]) * term_l + static_cast<double>(uv[1]) * term_r;
                min_speed = std::min(min_speed, speed);
            }
        }
    }

    if (min_speed <= 0.02)
        throw std::invalid_argument(
            "Strip width or tube count crosses the developable curve of regression (ds_{u,v} / ds <= 0)"
        );

    profile.min_speed = min_speed;
    profile.midspan_taper = std::exp(torsion_ratio * arc_angle / (Tolerance::PI * tan_a0));
    return profile;
}

inline CurvedCreaseTube::DeployedVault CurvedCreaseTube::compute_stage(
    const IntrinsicProfile& profile,
    double deployment,
    const Vector& shift
) const {

    const double alpha = deployment * opening;
    const double ca = std::cos(alpha);
    const double sa = std::sin(alpha);
    const double s2a = std::sin(2.0 * alpha);
    const double c2a = std::cos(2.0 * alpha);

    const double k_dep = profile.curvature * (std::cos(opening) / ca);
    const double tau_amp = torsion_ratio * profile.curvature * (std::tan(alpha) / std::tan(opening));
    const size_t stride = 16;
    const FrenetSamples fine = integrate_frenet(
        profile.arc_length, k_dep, tau_amp, profile.arc_length, stride * samples
    );

    std::vector<Point> p1(samples + 1);
    std::vector<Vector> tangents(samples + 1);
    std::vector<Vector> normals(samples + 1);
    std::vector<Vector> binormals(samples + 1);
    for (size_t j = 0; j <= samples; j++) {
        const size_t idx = j * stride;
        p1[j] = fine.points[idx];
        tangents[j] = fine.tangents[idx];
        normals[j] = fine.normals[idx];
        binormals[j] = fine.binormals[idx];
    }

    // Align reference chord horizontally along +x, centred on x = 0, with lowest support on z = 0
    const Vector chord = p1.back() - p1.front();
    const double yaw = -std::atan2(chord[1], chord[0]);
    const double cy = std::cos(yaw);
    const double sy = std::sin(yaw);
    for (size_t j = 0; j <= samples; j++) {
        p1[j] = Point(cy * p1[j][0] - sy * p1[j][1], sy * p1[j][0] + cy * p1[j][1], p1[j][2]);
        tangents[j] = Vector(cy * tangents[j][0] - sy * tangents[j][1], sy * tangents[j][0] + cy * tangents[j][1], tangents[j][2]);
        normals[j] = Vector(cy * normals[j][0] - sy * normals[j][1], sy * normals[j][0] + cy * normals[j][1], normals[j][2]);
        binormals[j] = Vector(cy * binormals[j][0] - sy * binormals[j][1], sy * binormals[j][0] + cy * binormals[j][1], binormals[j][2]);
    }
    const double mid_x = 0.5 * (p1.front()[0] + p1.back()[0]);
    const double mid_y = 0.5 * (p1.front()[1] + p1.back()[1]);
    const double floor_z = std::min(p1.front()[2], p1.back()[2]);
    for (size_t j = 0; j <= samples; j++)
        p1[j] = Point(p1[j][0] - mid_x + shift[0], p1[j][1] - mid_y + shift[1], p1[j][2] - floor_z + shift[2]);

    // Precompute ruling vectors r_L, r_R and strip normals n_L, n_R at every station
    std::vector<Vector> rl(samples + 1);
    std::vector<Vector> rr(samples + 1);
    std::vector<Vector> nl(samples + 1);
    std::vector<Vector> nr(samples + 1);
    for (size_t j = 0; j <= samples; j++) {
        const Vector el = normals[j] * ca + binormals[j] * sa;
        const Vector er = normals[j] * (-ca) + binormals[j] * sa;
        rl[j] = tangents[j] * profile.cos_beta[j] + el * profile.sin_beta[j];
        rr[j] = tangents[j] * profile.cos_beta[j] + er * profile.sin_beta[j];
        nl[j] = normals[j] * (-sa) + binormals[j] * ca;
        nr[j] = normals[j] * sa + binormals[j] * ca;
    }

    DeployedVault vault;
    vault.plates.reserve(4 * tubes);
    for (size_t b = 0; b < tubes; b++) {
        vault.plates.push_back(build_wall(
            profile, p1, tangents, rl, rr, profile.width_l, rl, nl, b, b, -0.5 * thickness,
            b == 0 ? -0.5 * thickness : 0.0, 0.5 * thickness, c2a, s2a, b, "bottom_left"
        ));
        vault.plates.push_back(build_wall(
            profile, p1, tangents, rl, rr, profile.width_r, rr, nr, b, b, 0.5 * thickness,
            0.5 * thickness, -0.5 * thickness, c2a, s2a, b, "top_left"
        ));
        vault.plates.push_back(build_wall(
            profile, p1, tangents, rl, rr, profile.width_r, rr, nr, b + 1, b, 0.5 * thickness,
            0.5 * thickness, -0.5 * thickness, c2a, s2a, b, "bottom_right"
        ));
        vault.plates.push_back(build_wall(
            profile, p1, tangents, rl, rr, profile.width_l, rl, nl, b, b + 1, -0.5 * thickness,
            -0.5 * thickness, b + 1 == tubes ? 0.5 * thickness : 0.0, c2a, s2a, b, "top_right"
        ));

        std::vector<Point> spine(samples + 1);
        std::vector<Point> ridge(samples + 1);
        std::vector<Point> keel(samples + 1);
        for (size_t j = 0; j <= samples; j++) {
            const double ul = static_cast<double>(b) * profile.length_l[j];
            const double u1l = static_cast<double>(b + 1) * profile.length_l[j];
            const double vr = static_cast<double>(b) * profile.length_r[j];
            const double v1r = static_cast<double>(b + 1) * profile.length_r[j];
            spine[j] = p1[j] + rl[j] * ul + rr[j] * vr;
            ridge[j] = p1[j] + rl[j] * ul + rr[j] * v1r;
            keel[j] = p1[j] + rl[j] * u1l + rr[j] * vr;
        }
        vault.creases.push_back(spine);
        vault.creases.push_back(ridge);
        vault.creases.push_back(keel);
    }
    std::vector<Point> end_spine(samples + 1);
    for (size_t j = 0; j <= samples; j++) {
        end_spine[j] = p1[j] + rl[j] * (static_cast<double>(tubes) * profile.length_l[j]) +
                       rr[j] * (static_cast<double>(tubes) * profile.length_r[j]);
    }
    vault.creases.push_back(end_spine);

    return vault;
}

inline CurvedCreaseTube::TubePlate CurvedCreaseTube::build_wall(
    const IntrinsicProfile& profile,
    const std::vector<Point>& p1,
    const std::vector<Vector>& tangents,
    const std::vector<Vector>& rl,
    const std::vector<Vector>& rr,
    const std::vector<double>& widths,
    const std::vector<Vector>& rulings,
    const std::vector<Vector>& wall_normals,
    size_t u,
    size_t v,
    double y_out,
    double start_shift,
    double end_shift,
    double cos_2a,
    double sin_2a,
    size_t tube_index,
    const char* suffix
) const {

    const double y_in = -y_out;
    TubePlate plate;
    plate.suffix = suffix;
    plate.tube_index = tube_index;
    plate.outer_a.resize(samples + 1);
    plate.outer_b.resize(samples + 1);
    plate.inner_a.resize(samples + 1);
    plate.inner_b.resize(samples + 1);

    for (size_t j = 0; j <= samples; j++) {
        const Point base = p1[j] + rl[j] * (static_cast<double>(u) * profile.length_l[j]) +
                           rr[j] * (static_cast<double>(v) * profile.length_r[j]);
        const double sb = profile.sin_beta[j];
        const double width = widths[j];
        const double xa_out = ((start_shift - y_out * cos_2a) / sin_2a) / sb;
        const double xb_out = (width + (end_shift - y_out * cos_2a) / sin_2a) / sb;
        const double xa_in = ((start_shift - y_in * cos_2a) / sin_2a) / sb;
        const double xb_in = (width + (end_shift - y_in * cos_2a) / sin_2a) / sb;
        plate.outer_a[j] = base + wall_normals[j] * y_out + rulings[j] * xa_out;
        plate.outer_b[j] = base + wall_normals[j] * y_out + rulings[j] * xb_out;
        plate.inner_a[j] = base + wall_normals[j] * y_in + rulings[j] * xa_in;
        plate.inner_b[j] = base + wall_normals[j] * y_in + rulings[j] * xb_in;
    }

    const size_t mid = samples / 2;
    if ((plate.outer_b[mid] - plate.outer_a[mid]).cross(tangents[mid]).dot(plate.outer_a[mid] - plate.inner_a[mid]) < 0.0) {
        std::swap(plate.outer_a, plate.inner_a);
        std::swap(plate.outer_b, plate.inner_b);
    }

    return plate;
}

inline std::vector<Development> CurvedCreaseTube::add_stage(
    const DeployedVault& stage,
    const std::string& group_name,
    const std::string& prefix,
    bool measure
) {

    const std::shared_ptr<TreeNode> plates_group = group_named(group_name);
    const std::shared_ptr<TreeNode> creases_group = group_named("creases");

    for (size_t c = 0; c < stage.creases.size(); c++) {
        Polyline curve(stage.creases[c]);
        curve.name = fmt::format("{}_crease_{}", prefix, c);
        add_polyline(curve, creases_group);
    }

    std::vector<Development> developments;
    developments.reserve(stage.plates.size());
    for (const TubePlate& plate : stage.plates) {
        const std::string plate_name = fmt::format("{}_{}_{}", prefix, plate.tube_index, plate.suffix);
        const std::shared_ptr<FaceCurvedElement> element = std::make_shared<FaceCurvedElement>(
            NurbsCurve::create_interpolated(plate.outer_a, CurveNurbsKnotStyle::Uniform),
            NurbsCurve::create_interpolated(plate.outer_b, CurveNurbsKnotStyle::Uniform),
            NurbsCurve::create_interpolated(plate.inner_a, CurveNurbsKnotStyle::Uniform),
            NurbsCurve::create_interpolated(plate.inner_b, CurveNurbsKnotStyle::Uniform),
            plate_name
        );
        add(element, plates_group);

        const Development development(*element, samples, fmt::format("flat_{}_{}", plate.tube_index, plate.suffix));
        if (measure)
            _distortion = std::max(_distortion, development.distortion);
        developments.push_back(development);
    }

    return developments;
}

inline void CurvedCreaseTube::add_flat(const std::vector<Development>& developments) {

    if (developments.empty())
        return;

    double low_x = std::numeric_limits<double>::infinity();
    double low_y = std::numeric_limits<double>::infinity();
    double high_y = -std::numeric_limits<double>::infinity();
    for (const std::shared_ptr<Element>& element : elements()) {
        for (const Point& corner : element->aabb().corners()) {
            low_x = std::min(low_x, corner[0]);
            low_y = std::min(low_y, corner[1]);
            high_y = std::max(high_y, corner[1]);
        }
    }

    double max_dx = 0.0;
    double max_dy = 0.0;
    for (const Development& dev : developments) {
        double min_px = std::numeric_limits<double>::infinity();
        double max_px = -std::numeric_limits<double>::infinity();
        double min_py = std::numeric_limits<double>::infinity();
        double max_py = -std::numeric_limits<double>::infinity();
        for (const Point& corner : dev.piece->aabb().corners()) {
            min_px = std::min(min_px, corner[0]);
            max_px = std::max(max_px, corner[0]);
            min_py = std::min(min_py, corner[1]);
            max_py = std::max(max_py, corner[1]);
        }
        max_dx = std::max(max_dx, max_px - min_px);
        max_dy = std::max(max_dy, max_py - min_py);
    }

    const std::shared_ptr<TreeNode> flat_group = group_named("flat");
    const double start_y = high_y + 0.35 * span;
    const double pitch_x = max_dx + 0.08 * span;
    const double pitch_y = max_dy + 0.06 * span;

    for (size_t i = 0; i < developments.size(); i++) {
        const size_t row = i / 4;
        const size_t col = i % 4;
        const Development& dev = developments[i];
        double min_px = std::numeric_limits<double>::infinity();
        double min_py = std::numeric_limits<double>::infinity();
        for (const Point& corner : dev.piece->aabb().corners()) {
            min_px = std::min(min_px, corner[0]);
            min_py = std::min(min_py, corner[1]);
        }
        const Vector shift(
            low_x + static_cast<double>(col) * pitch_x - min_px,
            start_y + static_cast<double>(row) * pitch_y - min_py,
            0.0
        );
        const std::shared_ptr<FaceCurvedElement> placed = std::make_shared<FaceCurvedElement>(*dev.piece);
        placed->name = dev.piece->name;
        placed->place(Xform::translation(shift[0], shift[1], shift[2]));
        add(placed, flat_group);
    }
}
