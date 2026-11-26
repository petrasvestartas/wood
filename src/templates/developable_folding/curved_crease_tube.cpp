#include "src/templates/developable_folding/curved_crease_tube.h"

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CurvedCreaseTube::CurvedCreaseTube(const CurvedCreaseTubeGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // profile: the space curve's coplanar-support root and the alpha-invariant strip widths and sector angles
    const IntrinsicProfile profile = compute_profile();
    _min_speed = profile.min_speed;
    _midspan_taper = profile.midspan_taper;

    // stages: every deployment's walls and creases on the deployed space curve, without thickness
    std::vector<DeployedVault> stages;
    for (const double deployment : guide.deployments)
        stages.push_back(compute_stage(profile, deployment));

    // layout: the stages side by side along y, every wall a quad mesh and every crease a polyline
    const std::vector<Vector> shifts = compute_shifts(stages);
    const AABB box = add_walls(stages, shifts);

    // pattern: every wall developed flat, the first stage's under the model, the largest error at the target the distortion
    std::vector<Development> pattern;
    for (size_t s = 0; s < stages.size(); s++)
        for (const Wall& wall : stages[s].walls) {
            const Development development(
                NurbsCurve::create_interpolated(wall.start, CurveNurbsKnotStyle::Uniform),
                NurbsCurve::create_interpolated(wall.end, CurveNurbsKnotStyle::Uniform),
                guide.samples
            );
            if (stages.size() == 1 || guide.deployments[s] == 1.0)
                _distortion = std::max(_distortion, development.distortion);
            if (s == 0)
                pattern.push_back(development);
        }
    Development::add_pattern(*this, pattern, box, 4, 0.06 * guide.span);

    // materialization: the surfaces alone without a thickness
    if (guide.thickness == 0.0)
        return;

    // plates: every wall a plate of the thickness centred on it, mitered where the walls meet on the creases
    add_plates(stages, shifts);

    // flat: every plate of the first stage unrolled into its flat piece for cutting
    add_flat();

    // contacts: shared bevel and spine faces across every tube
    compute_face_contacts(0);

    // assembly: four walls per tube, then the tubes side by side
    add_assembly();
}

// ═══════════════════════════════════════════════════════════════════════════
// Space curve and profile
// ═══════════════════════════════════════════════════════════════════════════
CurvedCreaseTube::FrenetSamples CurvedCreaseTube::integrate_frenet(
    double length, double curvature, double torsion_amplitude, double period_length, size_t steps
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
            next_t =
                tangent * ca + axis.cross(tangent) * sa + axis * (axis.dot(tangent) * (1.0 - ca));
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

CurvedCreaseTube::IntrinsicProfile CurvedCreaseTube::compute_profile() const {

    // Solve T(0.5 * s_0) . T(0) = 0 for dimensionless total curvature arc_angle = kappa_0 * s_0 >= pi
    double arc_angle = Tolerance::PI;
    if (std::abs(guide.torsion_ratio) > 1e-12) {
        double lo = Tolerance::PI;
        double hi = 5.0;
        for (int iter = 0; iter < 60; iter++) {
            const double mid = 0.5 * (lo + hi);
            const FrenetSamples half =
                integrate_frenet(0.5 * mid, 1.0, guide.torsion_ratio, mid, 256);
            if (half.tangents.back()[2] > 0.0)
                lo = mid;
            else
                hi = mid;
        }
        arc_angle = 0.5 * (lo + hi);
    }

    const size_t fine_steps = 16 * guide.samples;
    const FrenetSamples unit_arch =
        integrate_frenet(arc_angle, 1.0, guide.torsion_ratio, arc_angle, fine_steps);
    const Vector unit_chord = unit_arch.points.back() - unit_arch.points.front();
    const double chord_xy = std::hypot(unit_chord[0], unit_chord[1]);
    const double scale = guide.span / chord_xy;

    IntrinsicProfile profile;
    profile.arc_angle = arc_angle;
    profile.arc_length = arc_angle * scale;
    profile.curvature = 1.0 / scale;
    profile.cos_beta.resize(guide.samples + 1);
    profile.sin_beta.resize(guide.samples + 1);
    profile.width_l.resize(guide.samples + 1);
    profile.width_r.resize(guide.samples + 1);
    profile.length_l.resize(guide.samples + 1);
    profile.length_r.resize(guide.samples + 1);

    const double k0 = profile.curvature;
    const double s0 = profile.arc_length;
    const double k2d = k0 * std::cos(guide.opening);
    const double sin_a0 = std::sin(guide.opening);
    const double tan_a0 = std::tan(guide.opening);
    double min_speed = std::numeric_limits<double>::infinity();

    for (size_t j = 0; j <= guide.samples; j++) {
        const double s = s0 * static_cast<double>(j) / static_cast<double>(guide.samples);
        const double phase = 2.0 * Tolerance::PI * s / s0;
        const double tau0 = guide.torsion_ratio * k0 * std::sin(phase);
        const double dtau0 =
            guide.torsion_ratio * k0 * (2.0 * Tolerance::PI / s0) * std::cos(phase);

        const double cot_b = tau0 / (k0 * sin_a0);
        const double dcot_b = dtau0 / (k0 * sin_a0);
        const double beta = std::atan2(1.0, cot_b);
        const double cb = std::cos(beta);
        const double sb = std::sin(beta);

        const double phi = (guide.torsion_ratio * arc_angle / (2.0 * Tolerance::PI * tan_a0)) *
                           (1.0 - std::cos(phase));
        const double wl = guide.width_left * std::exp(-phi);
        const double wr = guide.width_right * std::exp(phi);
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
        for (size_t b = 0; b < guide.tubes; b++) {
            for (const std::array<size_t, 2>& uv : std::array<std::array<size_t, 2>, 4>{
                     {{b, b}, {b + 1, b}, {b, b + 1}, {b + 1, b + 1}}
                 }) {
                const double speed =
                    1.0 + static_cast<double>(uv[0]) * term_l + static_cast<double>(uv[1]) * term_r;
                min_speed = std::min(min_speed, speed);
            }
        }
    }

    profile.min_speed = min_speed;
    profile.midspan_taper = std::exp(guide.torsion_ratio * arc_angle / (Tolerance::PI * tan_a0));
    return profile;
}

// ═══════════════════════════════════════════════════════════════════════════
// Walls
// ═══════════════════════════════════════════════════════════════════════════
CurvedCreaseTube::DeployedVault
CurvedCreaseTube::compute_stage(const IntrinsicProfile& profile, double deployment) const {

    const size_t samples = guide.samples;
    const size_t tubes = guide.tubes;
    const double alpha = deployment * guide.opening;
    const double ca = std::cos(alpha);
    const double sa = std::sin(alpha);

    const double k_dep = profile.curvature * (std::cos(guide.opening) / ca);
    const double tau_amp =
        guide.torsion_ratio * profile.curvature * (std::tan(alpha) / std::tan(guide.opening));
    const size_t stride = 16;
    const FrenetSamples fine =
        integrate_frenet(profile.arc_length, k_dep, tau_amp, profile.arc_length, stride * samples);

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

    // the reference chord horizontal along +x, centred on x = 0, the lowest support on z = 0
    const Vector chord = p1.back() - p1.front();
    const double yaw = -std::atan2(chord[1], chord[0]);
    const double cy = std::cos(yaw);
    const double sy = std::sin(yaw);
    for (size_t j = 0; j <= samples; j++) {
        p1[j] = Point(cy * p1[j][0] - sy * p1[j][1], sy * p1[j][0] + cy * p1[j][1], p1[j][2]);
        tangents[j] = Vector(
            cy * tangents[j][0] - sy * tangents[j][1],
            sy * tangents[j][0] + cy * tangents[j][1],
            tangents[j][2]
        );
        normals[j] = Vector(
            cy * normals[j][0] - sy * normals[j][1],
            sy * normals[j][0] + cy * normals[j][1],
            normals[j][2]
        );
        binormals[j] = Vector(
            cy * binormals[j][0] - sy * binormals[j][1],
            sy * binormals[j][0] + cy * binormals[j][1],
            binormals[j][2]
        );
    }
    const double mid_x = 0.5 * (p1.front()[0] + p1.back()[0]);
    const double mid_y = 0.5 * (p1.front()[1] + p1.back()[1]);
    const double floor_z = std::min(p1.front()[2], p1.back()[2]);
    for (size_t j = 0; j <= samples; j++)
        p1[j] = Point(p1[j][0] - mid_x, p1[j][1] - mid_y, p1[j][2] - floor_z);

    // ruling vectors r_L, r_R and wall normals n_L, n_R at every station
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

    // per tube four walls, their faces and miters to come on sides, and its spine, ridge and keel
    DeployedVault vault;
    vault.cos_2a = std::cos(2.0 * alpha);
    vault.sin_2a = std::sin(2.0 * alpha);
    for (size_t b = 0; b < tubes; b++) {
        vault.walls.push_back(compute_wall(
            profile,
            p1,
            tangents,
            rl,
            rr,
            profile.width_l,
            rl,
            nl,
            b,
            b,
            {-1.0, b == 0 ? -1.0 : 0.0, 1.0},
            b,
            "bottom_left"
        ));
        vault.walls.push_back(compute_wall(
            profile,
            p1,
            tangents,
            rl,
            rr,
            profile.width_r,
            rr,
            nr,
            b,
            b,
            {1.0, 1.0, -1.0},
            b,
            "top_left"
        ));
        vault.walls.push_back(compute_wall(
            profile,
            p1,
            tangents,
            rl,
            rr,
            profile.width_r,
            rr,
            nr,
            b + 1,
            b,
            {1.0, 1.0, -1.0},
            b,
            "bottom_right"
        ));
        vault.walls.push_back(compute_wall(
            profile,
            p1,
            tangents,
            rl,
            rr,
            profile.width_l,
            rl,
            nl,
            b,
            b + 1,
            {-1.0, -1.0, b + 1 == tubes ? 1.0 : 0.0},
            b,
            "top_right"
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
    for (size_t j = 0; j <= samples; j++)
        end_spine[j] = p1[j] + rl[j] * (static_cast<double>(tubes) * profile.length_l[j]) +
                       rr[j] * (static_cast<double>(tubes) * profile.length_r[j]);
    vault.creases.push_back(end_spine);

    return vault;
}

CurvedCreaseTube::Wall CurvedCreaseTube::compute_wall(
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
    const std::array<double, 3>& sides,
    size_t tube,
    const char* suffix
) const {

    Wall wall;
    wall.suffix = suffix;
    wall.tube = tube;
    wall.rulings = rulings;
    wall.normals = wall_normals;
    wall.widths = widths;
    wall.sin_beta = profile.sin_beta;
    wall.tangent = tangents[guide.samples / 2];
    wall.sides = sides;

    for (size_t j = 0; j <= guide.samples; j++) {
        const Point base = p1[j] + rl[j] * (static_cast<double>(u) * profile.length_l[j]) +
                           rr[j] * (static_cast<double>(v) * profile.length_r[j]);
        wall.start.push_back(base);
        wall.end.push_back(base + rulings[j] * (widths[j] / profile.sin_beta[j]));
    }

    return wall;
}

// ═══════════════════════════════════════════════════════════════════════════
// Layout
// ═══════════════════════════════════════════════════════════════════════════
std::vector<Vector>
CurvedCreaseTube::compute_shifts(const std::vector<DeployedVault>& stages) const {

    const bool sequence = stages.size() > 1;
    const double gap = 0.28 * guide.span;
    double cursor = 0.0;
    std::vector<Vector> shifts;

    for (const DeployedVault& stage : stages) {
        double low_y = std::numeric_limits<double>::infinity();
        double high_y = -std::numeric_limits<double>::infinity();
        for (const Wall& wall : stage.walls)
            for (const std::vector<Point>* edge : {&wall.start, &wall.end})
                for (const Point& point : *edge) {
                    low_y = std::min(low_y, point[1]);
                    high_y = std::max(high_y, point[1]);
                }

        shifts.push_back(sequence ? Vector(0.0, cursor - low_y, 0.0) : Vector(0.0, 0.0, 0.0));
        cursor += (high_y - low_y) + gap;
    }

    return shifts;
}

AABB CurvedCreaseTube::add_walls(
    const std::vector<DeployedVault>& stages, const std::vector<Vector>& shifts
) {

    const std::shared_ptr<TreeNode> creases = group_named("creases");
    AABB box = AABB::empty();

    for (size_t s = 0; s < stages.size(); s++) {
        const std::string stage = stages.size() > 1 ? fmt::format("stage_{}_", s) : "";
        const std::shared_ptr<TreeNode> group = group_named(stage + "walls");
        const Xform shift = Xform::translation(shifts[s][0], shifts[s][1], shifts[s][2]);

        for (size_t c = 0; c < stages[s].creases.size(); c++) {
            Polyline curve = Polyline(stages[s].creases[c]).transformed(shift);
            curve.name = fmt::format("{}_crease_{}", prefix(s), c);
            add_polyline(curve, creases);
        }

        for (const Wall& wall : stages[s].walls) {
            const Mesh mesh =
                Development::strip(
                    wall.start, wall.end, fmt::format("{}wall_{}_{}", stage, wall.tube, wall.suffix)
                )
                    .transformed(shift);
            box.union_with(AABB::from_mesh(mesh));
            add_mesh(mesh, group);
        }
    }

    return box;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plates
// ═══════════════════════════════════════════════════════════════════════════
void CurvedCreaseTube::add_plates(
    const std::vector<DeployedVault>& stages, const std::vector<Vector>& shifts
) {

    const double half = 0.5 * guide.thickness;
    const size_t mid = guide.samples / 2;

    for (size_t s = 0; s < stages.size(); s++) {
        const std::shared_ptr<TreeNode> group =
            group_named(stages.size() > 1 ? fmt::format("stage_{}", s) : "plates");
        const double cos_2a = stages[s].cos_2a;
        const double sin_2a = stages[s].sin_2a;

        for (const Wall& wall : stages[s].walls) {
            const double y_out = wall.sides[0] * half;
            const double y_in = -y_out;
            const double start_shift = wall.sides[1] * half;
            const double end_shift = wall.sides[2] * half;
            std::array<std::vector<Point>, 4> edges;

            for (size_t j = 0; j <= guide.samples; j++) {
                const Point base = wall.start[j] + shifts[s];
                const double sb = wall.sin_beta[j];
                const double xa_out = ((start_shift - y_out * cos_2a) / sin_2a) / sb;
                const double xb_out = (wall.widths[j] + (end_shift - y_out * cos_2a) / sin_2a) / sb;
                const double xa_in = ((start_shift - y_in * cos_2a) / sin_2a) / sb;
                const double xb_in = (wall.widths[j] + (end_shift - y_in * cos_2a) / sin_2a) / sb;
                edges[0].push_back(base + wall.normals[j] * y_out + wall.rulings[j] * xa_out);
                edges[1].push_back(base + wall.normals[j] * y_out + wall.rulings[j] * xb_out);
                edges[2].push_back(base + wall.normals[j] * y_in + wall.rulings[j] * xa_in);
                edges[3].push_back(base + wall.normals[j] * y_in + wall.rulings[j] * xb_in);
            }

            // outward closed_mesh winding: the outer face first
            if ((edges[1][mid] - edges[0][mid])
                    .cross(wall.tangent)
                    .dot(edges[0][mid] - edges[2][mid]) < 0.0) {
                std::swap(edges[0], edges[2]);
                std::swap(edges[1], edges[3]);
            }

            add(std::make_shared<FaceCurvedElement>(
                    NurbsCurve::create_interpolated(edges[0], CurveNurbsKnotStyle::Uniform),
                    NurbsCurve::create_interpolated(edges[1], CurveNurbsKnotStyle::Uniform),
                    NurbsCurve::create_interpolated(edges[2], CurveNurbsKnotStyle::Uniform),
                    NurbsCurve::create_interpolated(edges[3], CurveNurbsKnotStyle::Uniform),
                    fmt::format("{}_{}_{}", prefix(s), wall.tube, wall.suffix)
                ),
                group);
        }
    }
}

void CurvedCreaseTube::add_flat() {

    const std::array<const char*, 4> order = {
        "bottom_left", "top_left", "bottom_right", "top_right"
    };
    double low_x = std::numeric_limits<double>::infinity();
    double low_y = std::numeric_limits<double>::infinity();
    double high_y = -std::numeric_limits<double>::infinity();
    for (const std::shared_ptr<Element>& element : elements())
        for (const Point& corner : element->aabb().corners()) {
            low_x = std::min(low_x, corner[0]);
            low_y = std::min(low_y, corner[1]);
            high_y = std::max(high_y, corner[1]);
        }

    std::vector<std::shared_ptr<FaceCurvedElement>> pieces;
    double max_dx = 0.0;
    double max_dy = 0.0;
    for (size_t b = 0; b < guide.tubes; b++)
        for (const char* suffix : order) {
            const std::shared_ptr<FaceCurvedElement> plate = get_element_by_name<FaceCurvedElement>(
                fmt::format("{}_{}_{}", prefix(0), b, suffix)
            );
            const Development development(
                *plate, guide.samples, fmt::format("flat_{}_{}", b, suffix)
            );
            pieces.push_back(development.piece);
            const OBB bounds = development.piece->aabb();
            max_dx = std::max(max_dx, bounds.max_point()[0] - bounds.min_point()[0]);
            max_dy = std::max(max_dy, bounds.max_point()[1] - bounds.min_point()[1]);
        }

    const std::shared_ptr<TreeNode> flat = group_named("flat");
    const double start_y = high_y + 0.35 * guide.span;
    const double pitch_x = max_dx + 0.08 * guide.span;
    const double pitch_y = max_dy + 0.06 * guide.span;

    for (size_t i = 0; i < pieces.size(); i++) {
        const OBB bounds = pieces[i]->aabb();
        const std::shared_ptr<FaceCurvedElement> piece =
            std::make_shared<FaceCurvedElement>(*pieces[i]);
        piece->name = pieces[i]->name;
        piece->place(
            Xform::translation(
                low_x + static_cast<double>(i % 4) * pitch_x - bounds.min_point()[0],
                start_y + static_cast<double>(i / 4) * pitch_y - bounds.min_point()[1],
                0.0
            )
        );
        add(piece, flat);
    }
}

void CurvedCreaseTube::add_assembly() {

    const std::array<const char*, 4> order = {
        "bottom_left", "top_left", "bottom_right", "top_right"
    };

    for (size_t s = 0; s < guide.deployments.size(); s++) {
        std::vector<std::shared_ptr<FaceCurvedElement>> placed;
        std::vector<size_t> keys;
        for (size_t b = 0; b < guide.tubes; b++)
            for (const char* suffix : order) {
                placed.push_back(
                    get_element_by_name<FaceCurvedElement>(
                        fmt::format("{}_{}_{}", prefix(s), b, suffix)
                    )
                );
                keys.push_back(b);
            }

        const std::string module = guide.deployments.size() > 1 ? fmt::format("stage_{}", s) : name;
        if (guide.tubes > 1)
            assemblies.push_back(
                AssemblyModule::module(module, AssemblyModule::modules_of("tube", placed, keys))
            );
        else
            assemblies.push_back(AssemblyModule::module(module, AssemblyModule::parts_of(placed)));
    }
}

std::string CurvedCreaseTube::prefix(size_t s) const {
    return guide.deployments.size() > 1 ? fmt::format("stage_{}_tube", s) : "tube";
}

}
