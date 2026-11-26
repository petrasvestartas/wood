#include "src/templates/developable_folding/crease_quadrants.h"

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CreaseQuadrants::CreaseQuadrants(const CreaseQuadrantsGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // frame: crease points, tangents, inward normals and osculating plane, uncurled to the deployment
    const Frame frame = compute_frame();

    // strips: per quadrant two strips of rulings r, -r, Mr or -Mr from the crease, without thickness, the quadrants side by side
    const std::array<std::array<Strip, 2>, 4> strips = compute_strips(frame);
    const AABB box = add_strips(strips);

    // sectors: per quadrant the sector angles around the crease's middle, 2 pi on a single sheet
    _sector_sums = compute_sector_sums(frame);

    // pattern: every strip developed flat under the model, the largest error the distortion
    std::vector<Development> pattern;
    for (const std::array<Strip, 2>& quadrant : strips)
        for (const Strip& strip : quadrant) {
            pattern.emplace_back(
                NurbsCurve::create_interpolated(strip.hinges, CurveNurbsKnotStyle::Uniform),
                NurbsCurve::create_interpolated(strip.ends, CurveNurbsKnotStyle::Uniform),
                guide.samples
            );
            _distortion = std::max(_distortion, pattern.back().distortion);
        }
    Development::add_pattern(*this, pattern, box, pattern.size(), 300.0);

    // materialization: the surfaces alone without a thickness
    if (guide.thickness == 0.0)
        return;

    // plates: every strip a plate of the thickness on its body side, hinged on its face at the crease (the axis shift) and mitered there
    add_plates(strips);

    // flat: every plate unrolled into its flat piece for cutting
    add_flat();

    // contacts: the shared miter faces at full deployment
    if (guide.deployment == 1.0)
        compute_face_contacts(0);

    // assembly: each quadrant on its own crease
    for (size_t quadrant = 1; quadrant <= 4; quadrant++)
        assemblies.push_back(
            AssemblyModule::module(
                fmt::format("quadrant_{}", quadrant),
                AssemblyModule::parts_of(
                    get_elements_numbered<FaceCurvedElement>(fmt::format("quadrant_{}", quadrant))
                )
            )
        );
}

// ═══════════════════════════════════════════════════════════════════════════
// Strips
// ═══════════════════════════════════════════════════════════════════════════
CreaseQuadrants::Frame CreaseQuadrants::compute_frame() const {

    const NurbsCurve& crease = guide.crease;
    const size_t samples = guide.samples;
    Frame frame;
    const double domain = crease.domain_end() - crease.domain_start();
    std::vector<Point> target_points;
    std::vector<Vector> target_tangents;

    for (size_t j = 0; j <= samples; j++) {
        const double t =
            crease.domain_start() + domain * static_cast<double>(j) / static_cast<double>(samples);
        target_points.push_back(crease.point_at(t));
        target_tangents.push_back(crease.tangent_at(t).normalized());
    }

    frame.normal = target_tangents.front().cross(target_tangents.back());
    for (const Vector& tangent : target_tangents)
        if (target_tangents.front().cross(tangent).magnitude() > frame.normal.magnitude())
            frame.normal = target_tangents.front().cross(tangent);

    frame.normal.normalize_self();
    const Vector x_axis = (target_points.back() - target_points.front()).normalized();
    frame.plane = Plane(target_points.front(), x_axis, frame.normal.cross(x_axis));

    if (guide.deployment == 1.0) {
        frame.points = target_points;
        frame.tangents = target_tangents;
    } else {
        // uncurl the crease by cos(alpha) = k_2d / k_3d (Fuchs and Tabachnikov 1999), on the crease's polyline
        const Vector u = frame.plane.x_axis().normalized();
        const Vector v = frame.plane.y_axis().normalized();
        std::vector<std::array<double, 2>> xy_target(samples + 1);
        for (size_t j = 0; j <= samples; j++) {
            const Vector offset = target_points[j] - frame.plane.origin();
            xy_target[j] = {offset.dot(u), offset.dot(v)};
        }

        std::vector<double> lengths(samples);
        std::vector<double> headings(samples);
        for (size_t j = 0; j < samples; j++) {
            const double dx = xy_target[j + 1][0] - xy_target[j][0];
            const double dy = xy_target[j + 1][1] - xy_target[j][1];
            lengths[j] = std::hypot(dx, dy);
            headings[j] = std::atan2(dy, dx);
        }

        const double ratio = std::cos(guide.opening) / std::cos(guide.deployment * guide.opening);
        const size_t middle = samples / 2;
        std::vector<double> deployed_headings(samples);
        deployed_headings[middle] = headings[middle];
        for (size_t j = middle; j + 1 < samples; j++) {
            const double delta = std::atan2(
                std::sin(headings[j + 1] - headings[j]), std::cos(headings[j + 1] - headings[j])
            );
            deployed_headings[j + 1] =
                deployed_headings[j] +
                2.0 * std::asin(std::clamp(ratio * std::sin(0.5 * delta), -1.0, 1.0));
        }
        for (size_t j = middle; j > 0; j--) {
            const double delta = std::atan2(
                std::sin(headings[j] - headings[j - 1]), std::cos(headings[j] - headings[j - 1])
            );
            deployed_headings[j - 1] =
                deployed_headings[j] -
                2.0 * std::asin(std::clamp(ratio * std::sin(0.5 * delta), -1.0, 1.0));
        }

        std::vector<std::array<double, 2>> xy(samples + 1);
        xy[middle] = xy_target[middle];
        for (size_t j = middle; j < samples; j++)
            xy[j + 1] = {
                xy[j][0] + lengths[j] * std::cos(deployed_headings[j]),
                xy[j][1] + lengths[j] * std::sin(deployed_headings[j]),
            };
        for (size_t j = middle; j > 0; j--)
            xy[j - 1] = {
                xy[j][0] - lengths[j - 1] * std::cos(deployed_headings[j - 1]),
                xy[j][1] - lengths[j - 1] * std::sin(deployed_headings[j - 1]),
            };

        for (size_t j = 0; j <= samples; j++)
            frame.points.push_back(frame.plane.origin() + u * xy[j][0] + v * xy[j][1]);

        const NurbsCurve rebuilt =
            NurbsCurve::create_interpolated(frame.points, CurveNurbsKnotStyle::Uniform);
        const double rebuilt_domain = rebuilt.domain_end() - rebuilt.domain_start();
        for (size_t j = 0; j <= samples; j++) {
            const double s = rebuilt.domain_start() +
                             rebuilt_domain * static_cast<double>(j) / static_cast<double>(samples);
            frame.tangents.push_back(rebuilt.tangent_at(s).normalized());
        }
    }

    for (const Vector& tangent : frame.tangents)
        frame.inward.push_back(frame.normal.cross(tangent).normalized());

    return frame;
}

std::array<std::array<CreaseQuadrants::Strip, 2>, 4>
CreaseQuadrants::compute_strips(const Frame& frame) const {

    const double alpha = guide.deployment * guide.opening;
    const double c_star = std::cos(guide.opening);
    const double s_star = std::sin(guide.opening);
    const double c = std::cos(alpha);
    const double s = std::sin(alpha);
    Vector chord = frame.points.back() - frame.points.front();
    const double step = chord.magnitude() + 3.0 * guide.width;
    chord.normalize_self();
    std::array<std::array<Strip, 2>, 4> strips;

    for (size_t q = 0; q < 4; q++) {
        const Vector shift = chord * (static_cast<double>(q) * step);
        const bool ccf = SIGNS[q][0][1] == SIGNS[q][1][1];

        for (size_t k = 0; k < 2; k++) {
            const std::array<double, 2>& signs = SIGNS[q][k];
            Strip& strip = strips[q][k];

            for (size_t j = 0; j < frame.points.size(); j++) {
                const Vector ruling_star =
                    (frame.inward[j] * (signs[0] * c_star) + frame.normal * (signs[1] * s_star))
                        .normalized();
                const Vector ruling =
                    (frame.inward[j] * (signs[0] * c) + frame.normal * (signs[1] * s)).normalized();

                // the body side: CCF (Q1, Q3) hinges on the outer fold face and opens into one flat sheet at w = 0,
                // CCU (Q2, Q4) hinges on the shared contact face and closes into a stack of two at w = 0
                const Vector side = frame.normal * (ccf ? signs[1] : -signs[1]);
                Vector body_normal_star = frame.tangents[j].cross(ruling_star).normalized();
                Vector body_normal = frame.tangents[j].cross(ruling).normalized();
                if (body_normal_star.dot(side) < 0.0)
                    body_normal_star = body_normal_star * -1.0;
                if (body_normal.dot(side) < 0.0)
                    body_normal = body_normal * -1.0;

                const Vector mitre = ccf ? frame.inward[j] : frame.normal;
                strip.hinges.push_back(frame.points[j] + shift);
                strip.rulings.push_back(ruling);
                strip.ends.push_back(frame.points[j] + shift + ruling * guide.width);
                strip.normals.push_back(body_normal);
                strip.bevels.push_back(-body_normal_star.dot(mitre) / ruling_star.dot(mitre));
            }
        }
    }

    return strips;
}

std::array<double, 4> CreaseQuadrants::compute_sector_sums(const Frame& frame) const {

    const size_t middle = guide.samples / 2;
    const Vector edge_prev = (frame.points[middle - 1] - frame.points[middle]).normalized();
    const Vector edge_next = (frame.points[middle + 1] - frame.points[middle]).normalized();
    std::array<double, 4> sums = {0.0, 0.0, 0.0, 0.0};

    for (size_t q = 0; q < 4; q++)
        for (const std::array<double, 2>& signs : SIGNS[q]) {
            const Vector ruling_star =
                (frame.inward[middle] * (signs[0] * std::cos(guide.opening)) +
                 frame.normal * (signs[1] * std::sin(guide.opening)))
                    .normalized();
            sums[q] += std::acos(std::clamp(edge_next.dot(ruling_star), -1.0, 1.0)) +
                       std::acos(std::clamp(edge_prev.dot(ruling_star), -1.0, 1.0));
        }

    return sums;
}

AABB CreaseQuadrants::add_strips(const std::array<std::array<Strip, 2>, 4>& strips) {

    const std::shared_ptr<TreeNode> group = group_named("strips");
    const std::shared_ptr<TreeNode> creases = group_named("creases");
    AABB box = AABB::empty();

    for (size_t q = 0; q < 4; q++) {
        for (size_t k = 0; k < 2; k++) {
            const Strip& strip = strips[q][k];
            const Mesh mesh = Development::strip(
                strip.hinges, strip.ends, fmt::format("quadrant_{}_strip_{}", q + 1, k)
            );
            box.union_with(AABB::from_mesh(mesh));
            add_mesh(mesh, group);
        }

        Polyline crease(strips[q][0].hinges);
        crease.name = fmt::format("crease_{}", q + 1);
        add_polyline(crease, creases);
    }

    return box;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plates
// ═══════════════════════════════════════════════════════════════════════════
void CreaseQuadrants::add_plates(const std::array<std::array<Strip, 2>, 4>& strips) {

    const double thickness = guide.thickness;

    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("quadrant_{}", q + 1));

        for (size_t k = 0; k < 2; k++) {
            const Strip& strip = strips[q][k];
            std::array<std::vector<Point>, 4> edges;

            for (size_t j = 0; j < strip.hinges.size(); j++) {
                const Point body = strip.hinges[j] + strip.normals[j] * thickness;
                edges[0].push_back(body + strip.rulings[j] * (thickness * strip.bevels[j]));
                edges[1].push_back(body + strip.rulings[j] * guide.width);
                edges[2].push_back(strip.hinges[j]);
                edges[3].push_back(strip.ends[j]);
            }

            NurbsCurve outer_a =
                NurbsCurve::create_interpolated(edges[0], CurveNurbsKnotStyle::Uniform);
            NurbsCurve outer_b =
                NurbsCurve::create_interpolated(edges[1], CurveNurbsKnotStyle::Uniform);
            NurbsCurve inner_a =
                NurbsCurve::create_interpolated(edges[2], CurveNurbsKnotStyle::Uniform);
            NurbsCurve inner_b =
                NurbsCurve::create_interpolated(edges[3], CurveNurbsKnotStyle::Uniform);

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

            add(std::make_shared<FaceCurvedElement>(
                    outer_a, outer_b, inner_a, inner_b, fmt::format("quadrant_{}_{}", q + 1, k)
                ),
                group);
        }
    }
}

void CreaseQuadrants::add_flat() {

    std::vector<Development> developments;

    for (size_t i = 0; i < 8; i++) {
        const std::string suffix = fmt::format("quadrant_{}_{}", i / 2 + 1, i % 2);
        developments.emplace_back(*get_element_by_name<FaceCurvedElement>(suffix), guide.samples, "flat_" + suffix);
    }

    Development::add_flat(*this, developments, 4, 300.0);
}

}
