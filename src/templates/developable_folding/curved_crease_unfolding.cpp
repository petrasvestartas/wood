#include "src/templates/developable_folding/curved_crease_unfolding.h"

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CurvedCreaseUnfolding::CurvedCreaseUnfolding(
    const CurvedCreaseUnfoldingGuide& guide, const std::string& name
)
    : WoodSession(name), guide(guide) {

    // start: the plane the given crease lies on
    const size_t start = compute_start();

    // target: the mid-surface strips at full opening, traced from the crease plane by plane, without thickness
    const std::vector<TargetStep> target = compute_target(start);

    // stages: every deployment's strips rebuilt isometrically from the uncurled crease, hinged on their shared creases
    const std::vector<DeployedStage> strips =
        compute_stages(start, target, compute_reaches(target, 0.0));

    // layout: the stages side by side along the unfolding direction, every strip a quad mesh and every crease a polyline
    const std::vector<Vector> shifts = compute_shifts(start, strips);
    const AABB box = add_strips(strips, shifts);

    // developments: every strip of every stage developed flat, the largest error the distortion, the target's feet the drift
    const std::vector<std::vector<Development>> developments = compute_developments(strips);
    for (const std::vector<Development>& stage : developments)
        for (const Development& development : stage)
            _distortion = std::max(_distortion, development.distortion);
    _drift = compute_drift(strips);

    // pattern: the 2D crease pattern of the first stage under the model
    Development::add_pattern(*this, developments[0], box, developments[0].size(), 300.0);

    // materialization: the surfaces alone without a thickness
    if (guide.thickness == 0.0)
        return;

    // plates: every strip a plate, hinged on its faces (the axis shift of paper 5.3) and mitered on the planes
    const std::vector<DeployedStage> plates =
        compute_stages(start, target, compute_reaches(target, guide.thickness));
    add_plates(plates, shifts);

    // flat: every plate of the first stage unrolled into its flat piece for cutting
    add_flat();

    // contacts: the shared miter faces of a fully unfolded stage
    if (std::find(guide.deployments.begin(), guide.deployments.end(), 1.0) !=
        guide.deployments.end())
        compute_face_contacts(0);

    // assembly: pairs of neighbours on their crease at full opening, plate by plate when partly unfolded
    add_assembly();
}

// ═══════════════════════════════════════════════════════════════════════════
// Target
// ═══════════════════════════════════════════════════════════════════════════
size_t CurvedCreaseUnfolding::compute_start() const {

    const NurbsCurve& crease = guide.crease;
    const std::array<Point, 3> points = {
        crease.point_at(crease.domain_start()),
        crease.point_at(0.5 * (crease.domain_start() + crease.domain_end())),
        crease.point_at(crease.domain_end()),
    };
    size_t start = 0;
    double nearest = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < guide.planes.size(); i++) {
        const Vector normal = guide.planes[i].z_axis().normalized();
        double distance = 0.0;

        for (const Point& point : points)
            distance = std::max(distance, std::abs((point - guide.planes[i].origin()).dot(normal)));

        if (distance < nearest) {
            nearest = distance;
            start = i;
        }
    }

    return start;
}

std::vector<CurvedCreaseUnfolding::TargetStep>
CurvedCreaseUnfolding::compute_target(size_t start) const {

    const std::vector<Plane>& planes = guide.planes;
    const size_t samples = guide.samples;
    const Vector normal = planes[start].z_axis().normalized();
    const Plane& neighbour = planes[start + 1 < planes.size() ? start + 1 : start - 1];
    const double domain = guide.crease.domain_end() - guide.crease.domain_start();
    std::vector<Point> points;
    std::vector<Vector> tangents;

    for (size_t j = 0; j <= samples; j++) {
        const double t = guide.crease.domain_start() +
                         domain * static_cast<double>(j) / static_cast<double>(samples);
        points.push_back(guide.crease.point_at(t));
        tangents.push_back(guide.crease.tangent_at(t).normalized());
    }

    // concave side of the given crease
    double turning = 0.0;
    for (size_t j = 0; j < samples; j++)
        turning += tangents[j].cross(tangents[j + 1]).dot(normal);

    const double c = std::cos(guide.opening);
    const double s = std::sin(guide.opening);
    const size_t middle = samples / 2;
    const Vector inward_mid =
        (turning >= 0.0 ? normal.cross(tangents[middle]) : tangents[middle].cross(normal))
            .normalized();
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

void CurvedCreaseUnfolding::trace_target_side(
    std::vector<Point> neutral,
    std::vector<Vector> rulings,
    size_t from,
    int step,
    std::vector<TargetStep>& steps
) const {

    const std::vector<Plane>& planes = guide.planes;
    const size_t samples = guide.samples;
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
        info.rulings = rulings;

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
            const double reach = cut(neutral[j], rulings[j], planes[next]);
            info.neutral_reach.push_back(reach);
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

std::vector<CurvedCreaseUnfolding::FaceReaches> CurvedCreaseUnfolding::compute_reaches(
    const std::vector<TargetStep>& target, double thickness
) const {

    const double half = 0.5 * thickness;
    std::vector<FaceReaches> reaches;

    for (const TargetStep& info : target) {
        const std::vector<Vector> tangents = sample_tangents(info.neutral_start);
        FaceReaches reach;
        reach.half = half;

        for (size_t j = 0; j < info.rulings.size(); j++) {
            Vector strip_normal = tangents[j].cross(info.rulings[j]).normalized();
            if (strip_normal.dot(info.plane_normal) < 0.0)
                strip_normal = strip_normal * -1.0;

            const Point minus_pt = info.neutral_start[j] - strip_normal * half;
            const Point plus_pt = info.neutral_start[j] + strip_normal * half;
            reach.minus_start.push_back(cut(minus_pt, info.rulings[j], guide.planes[info.from]));
            reach.minus_end.push_back(cut(minus_pt, info.rulings[j], guide.planes[info.to]));
            reach.plus_start.push_back(cut(plus_pt, info.rulings[j], guide.planes[info.from]));
            reach.plus_end.push_back(cut(plus_pt, info.rulings[j], guide.planes[info.to]));
        }

        reaches.push_back(reach);
    }

    return reaches;
}

// ═══════════════════════════════════════════════════════════════════════════
// Deployment
// ═══════════════════════════════════════════════════════════════════════════
std::vector<CurvedCreaseUnfolding::DeployedStage> CurvedCreaseUnfolding::compute_stages(
    size_t start, const std::vector<TargetStep>& target, const std::vector<FaceReaches>& reaches
) const {

    std::vector<DeployedStage> stages;

    for (const double fraction : guide.deployments)
        stages.push_back(compute_deployed(start, target, reaches, fraction));

    return stages;
}

CurvedCreaseUnfolding::DeployedStage CurvedCreaseUnfolding::compute_deployed(
    size_t start,
    const std::vector<TargetStep>& target,
    const std::vector<FaceReaches>& reaches,
    double fraction
) const {

    const size_t samples = guide.samples;
    const size_t plate_count = guide.planes.size() - 1;
    DeployedStage stage;
    stage.plates.resize(plate_count);
    stage.start_index = start;

    const TargetStep& seed = target[start < plate_count ? start : start - 1];
    const std::vector<Point>& start_target =
        seed.from == start ? seed.neutral_start : seed.neutral_end;
    stage.start_crease = uncurl_crease(start_target, guide.planes[start], fraction);

    // primary direction first to establish the shared hinge at start
    const int primary = start < plate_count ? 1 : -1;
    std::vector<Point> shared_start_hinge;

    for (int pass = 0; pass < 2; pass++) {
        const int step = pass == 0 ? primary : -primary;
        if ((step > 0 && start >= plate_count) || (step < 0 && start == 0))
            continue;

        const size_t first_idx =
            std::min(start, static_cast<size_t>(static_cast<long>(start) + step));
        Vector osculating = target[first_idx].plane_normal;
        if (fraction == 0.0 && pass == 1)
            osculating =
                target[std::min(start, static_cast<size_t>(static_cast<long>(start) + primary))]
                    .plane_normal *
                -1.0;

        std::vector<Point> current_hinge = shared_start_hinge;
        std::vector<Point> hint_curve = stage.start_crease;

        for (size_t k = start; step > 0 ? k + 1 < guide.planes.size() : k > 0; k += step) {
            const size_t next = static_cast<size_t>(static_cast<long>(k) + step);
            const size_t idx = std::min(k, next);
            const TargetStep& info = target[idx];
            const FaceReaches& reach = reaches[idx];
            const double half = reach.half;

            std::vector<Point> neutral_a(samples + 1);
            std::vector<Vector> rulings;
            std::vector<Vector> normals;

            if (fraction == 1.0) {
                neutral_a = info.neutral_start;
                osculating = info.plane_normal;
                compute_strip_frame(neutral_a, osculating, info, 1.0, rulings, normals);
                if (k == start && pass == 0) {
                    shared_start_hinge.resize(samples + 1);
                    for (size_t j = 0; j <= samples; j++)
                        shared_start_hinge[j] =
                            (neutral_a[j] - normals[j] * half) + rulings[j] * reach.minus_start[j];
                }
            } else if (k == start && pass == 0) {
                neutral_a = stage.start_crease;
                compute_strip_frame(neutral_a, osculating, info, fraction, rulings, normals);
                shared_start_hinge.resize(samples + 1);
                for (size_t j = 0; j <= samples; j++)
                    shared_start_hinge[j] =
                        (neutral_a[j] - normals[j] * half) + rulings[j] * reach.minus_start[j];
            } else {
                compute_strip_frame(hint_curve, osculating, info, fraction, rulings, normals);
                for (size_t j = 0; j <= samples; j++)
                    neutral_a[j] =
                        current_hinge[j] + normals[j] * half - rulings[j] * reach.minus_start[j];
                compute_strip_frame(neutral_a, osculating, info, fraction, rulings, normals);
                for (size_t j = 0; j <= samples; j++)
                    neutral_a[j] =
                        current_hinge[j] + normals[j] * half - rulings[j] * reach.minus_start[j];
            }

            DeployedPlate plate;
            plate.to_index = next;
            plate.neutral_a = neutral_a;
            for (size_t j = 0; j <= samples; j++) {
                const Point minus_pt = neutral_a[j] - normals[j] * half;
                const Point plus_pt = neutral_a[j] + normals[j] * half;
                plate.neutral_b.push_back(neutral_a[j] + rulings[j] * info.neutral_reach[j]);
                plate.minus_a.push_back(minus_pt + rulings[j] * reach.minus_start[j]);
                plate.minus_b.push_back(minus_pt + rulings[j] * reach.minus_end[j]);
                plate.plus_a.push_back(plus_pt + rulings[j] * reach.plus_start[j]);
                plate.plus_b.push_back(plus_pt + rulings[j] * reach.plus_end[j]);
            }
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

std::vector<Point> CurvedCreaseUnfolding::uncurl_crease(
    const std::vector<Point>& points, const Plane& plane, double fraction
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

    const double ratio = std::cos(guide.opening) / std::cos(fraction * guide.opening);
    std::vector<double> turns(count - 1);
    for (size_t j = 0; j + 1 < count; j++) {
        const double delta = std::atan2(
            std::sin(headings[j + 1] - headings[j]), std::cos(headings[j + 1] - headings[j])
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

void CurvedCreaseUnfolding::compute_strip_frame(
    const std::vector<Point>& curve,
    const Vector& plane_normal,
    const TargetStep& info,
    double fraction,
    std::vector<Vector>& rulings,
    std::vector<Vector>& normals
) const {

    const size_t samples = guide.samples;
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
        const Vector ruling =
            (tangents[j] * info.cos_gamma[j] +
             (across * (info.inward_sign * ca) + plane_normal * sa) * info.sin_gamma[j])
                .normalized();
        Vector strip_normal = tangents[j].cross(ruling).normalized();
        if (strip_normal.dot(plane_normal) < 0.0)
            strip_normal = strip_normal * -1.0;

        rulings[j] = ruling;
        normals[j] = strip_normal;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Layout and measurements
// ═══════════════════════════════════════════════════════════════════════════
std::vector<Vector> CurvedCreaseUnfolding::compute_shifts(
    size_t start, const std::vector<DeployedStage>& stages
) const {

    const std::vector<Plane>& planes = guide.planes;
    const bool sequence = stages.size() > 1;
    Vector axis = planes[start].z_axis().normalized();
    if (start + 1 < planes.size() &&
        (planes[start + 1].origin() - planes[start].origin()).magnitude() > 1e-6)
        axis = (planes[start + 1].origin() - planes[start].origin()).normalized();

    const NurbsCurve& crease = guide.crease;
    const double gap =
        0.35 *
        crease.point_at(crease.domain_start()).distance(crease.point_at(crease.domain_end()));
    double cursor = 0.0;
    std::vector<Vector> shifts;

    for (const DeployedStage& stage : stages) {
        double low = std::numeric_limits<double>::infinity();
        double high = -std::numeric_limits<double>::infinity();
        for (const DeployedPlate& plate : stage.plates)
            for (const std::vector<Point>* edge : {&plate.neutral_a, &plate.neutral_b})
                for (const Point& point : *edge) {
                    low = std::min(low, (point - planes[start].origin()).dot(axis));
                    high = std::max(high, (point - planes[start].origin()).dot(axis));
                }

        shifts.push_back(sequence ? axis * (cursor - low) : Vector(0.0, 0.0, 0.0));
        cursor += (high - low) + gap;
    }

    return shifts;
}

std::vector<std::vector<Development>>
CurvedCreaseUnfolding::compute_developments(const std::vector<DeployedStage>& stages) const {

    std::vector<std::vector<Development>> developments;

    for (const DeployedStage& stage : stages) {
        std::vector<Development> flat;
        for (const DeployedPlate& plate : stage.plates)
            flat.emplace_back(
                NurbsCurve::create_interpolated(plate.neutral_a, CurveNurbsKnotStyle::Uniform),
                NurbsCurve::create_interpolated(plate.neutral_b, CurveNurbsKnotStyle::Uniform),
                guide.samples
            );
        developments.push_back(flat);
    }

    return developments;
}

double CurvedCreaseUnfolding::compute_drift(const std::vector<DeployedStage>& stages) const {

    const double start_height = guide.crease.point_at(guide.crease.domain_start())[2];
    const double end_height = guide.crease.point_at(guide.crease.domain_end())[2];
    const double floor = std::min(start_height, end_height);
    double drift = 0.0;

    for (size_t s = 0; s < stages.size(); s++) {
        if (stages.size() > 1 && guide.deployments[s] != 1.0)
            continue;

        for (const DeployedPlate& plate : stages[s].plates) {
            if (start_height < floor + 1.0)
                drift = std::max(drift, std::abs(plate.neutral_b.front()[2] - floor));
            if (end_height < floor + 1.0)
                drift = std::max(drift, std::abs(plate.neutral_b.back()[2] - floor));
        }
    }

    return drift;
}

// ═══════════════════════════════════════════════════════════════════════════
// Strips and plates
// ═══════════════════════════════════════════════════════════════════════════
AABB CurvedCreaseUnfolding::add_strips(
    const std::vector<DeployedStage>& stages, const std::vector<Vector>& shifts
) {

    const std::shared_ptr<TreeNode> creases = group_named("creases");
    AABB box = AABB::empty();

    for (size_t s = 0; s < stages.size(); s++) {
        const std::string prefix = stages.size() > 1 ? fmt::format("stage_{}_", s) : "";
        const std::shared_ptr<TreeNode> group = group_named(prefix + "strips");
        const Xform shift = Xform::translation(shifts[s][0], shifts[s][1], shifts[s][2]);

        Polyline start_line = Polyline(stages[s].start_crease).transformed(shift);
        start_line.name = fmt::format("{}crease_{}", prefix, stages[s].start_index);
        add_polyline(start_line, creases);

        for (size_t i = 0; i < stages[s].plates.size(); i++) {
            const DeployedPlate& plate = stages[s].plates[i];
            const Mesh strip =
                Development::strip(
                    plate.neutral_a, plate.neutral_b, fmt::format("{}strip_{}", prefix, i)
                )
                    .transformed(shift);
            box.union_with(AABB::from_mesh(strip));
            add_mesh(strip, group);

            Polyline crease = Polyline(plate.crease_to).transformed(shift);
            crease.name = fmt::format("{}crease_{}", prefix, plate.to_index);
            add_polyline(crease, creases);
        }
    }

    return box;
}

void CurvedCreaseUnfolding::add_plates(
    const std::vector<DeployedStage>& stages, const std::vector<Vector>& shifts
) {

    for (size_t s = 0; s < stages.size(); s++) {
        const std::array<std::string, 2> names = stage_names(s);
        const std::shared_ptr<TreeNode> group = group_named(names[0]);
        const Xform shift = Xform::translation(shifts[s][0], shifts[s][1], shifts[s][2]);

        for (size_t i = 0; i < stages[s].plates.size(); i++) {
            const DeployedPlate& data = stages[s].plates[i];
            NurbsCurve outer_a =
                NurbsCurve::create_interpolated(data.plus_a, CurveNurbsKnotStyle::Uniform)
                    .transformed(shift);
            NurbsCurve outer_b =
                NurbsCurve::create_interpolated(data.plus_b, CurveNurbsKnotStyle::Uniform)
                    .transformed(shift);
            NurbsCurve inner_a =
                NurbsCurve::create_interpolated(data.minus_a, CurveNurbsKnotStyle::Uniform)
                    .transformed(shift);
            NurbsCurve inner_b =
                NurbsCurve::create_interpolated(data.minus_b, CurveNurbsKnotStyle::Uniform)
                    .transformed(shift);

            // outward closed_mesh winding: ((outer_b - outer_a) x tangent) . (outer_a - inner_a) > 0
            const double mid_t = 0.5 * (outer_a.domain_start() + outer_a.domain_end());
            const Vector tangent = outer_a.tangent_at(mid_t);
            const Vector across =
                outer_b.point_at(0.5 * (outer_b.domain_start() + outer_b.domain_end())) -
                outer_a.point_at(mid_t);
            const Vector up =
                outer_a.point_at(mid_t) -
                inner_a.point_at(0.5 * (inner_a.domain_start() + inner_a.domain_end()));
            if (across.cross(tangent).dot(up) < 0.0) {
                std::swap(outer_a, outer_b);
                std::swap(inner_a, inner_b);
            }

            const std::shared_ptr<FaceCurvedElement> plate = std::make_shared<FaceCurvedElement>(
                outer_a, outer_b, inner_a, inner_b, fmt::format("{}_{}", names[1], i)
            );
            add(plate, group);
        }
    }
}

void CurvedCreaseUnfolding::add_flat() {

    const std::shared_ptr<TreeNode> flat = group_named("flat");
    double max_x = -std::numeric_limits<double>::infinity();
    for (const std::shared_ptr<Element>& element : elements())
        for (const Point& corner : element->aabb().corners())
            max_x = std::max(max_x, corner[0]);

    double cursor_x = max_x + 1200.0;

    for (size_t i = 0; i + 1 < guide.planes.size(); i++) {
        const std::shared_ptr<FaceCurvedElement> plate =
            get_element_by_name<FaceCurvedElement>(fmt::format("{}_{}", stage_names(0)[1], i));
        const Development development(*plate, guide.samples, fmt::format("flat_{}", i));
        const AABB bounds = AABB::merge(
            AABB::from_polyline(development.bottom), AABB::from_polyline(development.top)
        );
        const std::shared_ptr<FaceCurvedElement> piece =
            std::make_shared<FaceCurvedElement>(*development.piece);
        piece->name = fmt::format("flat_{}", i);
        piece->place(
            Xform::translation(cursor_x - bounds.min_point()[0], -bounds.min_point()[1], 0.0)
        );
        add(piece, flat);
        cursor_x += bounds.max_point()[0] - bounds.min_point()[0] + 300.0;
    }
}

void CurvedCreaseUnfolding::add_assembly() {

    for (size_t s = 0; s < guide.deployments.size(); s++) {
        const std::array<std::string, 2> names = stage_names(s);
        std::vector<std::shared_ptr<FaceCurvedElement>> placed;
        std::vector<size_t> keys;
        for (size_t i = 0; i + 1 < guide.planes.size(); i++) {
            placed.push_back(
                get_element_by_name<FaceCurvedElement>(fmt::format("{}_{}", names[1], i))
            );
            keys.push_back(i / 2);
        }

        const std::string module = guide.deployments.size() > 1 ? names[0] : name;
        if (guide.deployments[s] == 1.0)
            assemblies.push_back(
                AssemblyModule::module(module, AssemblyModule::modules_of("fold", placed, keys))
            );
        else
            assemblies.push_back(AssemblyModule::module(module, AssemblyModule::parts_of(placed)));
    }
}

std::array<std::string, 2> CurvedCreaseUnfolding::stage_names(size_t s) const {

    if (guide.deployments.size() > 1)
        return {fmt::format("stage_{}", s), fmt::format("stage_{}", s)};

    return {"plates", "plate"};
}

// ═══════════════════════════════════════════════════════════════════════════
// Vectors
// ═══════════════════════════════════════════════════════════════════════════
std::vector<Vector> CurvedCreaseUnfolding::sample_tangents(const std::vector<Point>& points) {

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

Vector CurvedCreaseUnfolding::reflected(const Vector& vector, const Vector& normal) {
    return vector - normal * (2.0 * vector.dot(normal));
}

double CurvedCreaseUnfolding::cut(const Point& point, const Vector& direction, const Plane& plane) {

    const Vector normal = plane.z_axis();
    const double along = direction.dot(normal);

    if (std::abs(along) < 1e-12)
        return std::numeric_limits<double>::quiet_NaN();

    return (plane.origin() - point).dot(normal) / along;
}

}
