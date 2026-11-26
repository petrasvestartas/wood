#pragma once
#include "wood_session.h"
#include "../development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// The four quadrants Q1-Q4 of a planar curved crease (CCF single-sheet folding vs CCU two-sheet mirror unfolding).
class CreaseQuadrants : public WoodSession {
public:
    const NurbsCurve crease; // Planar curved crease.
    const double opening; // radians, target reflection angle between rulings and osculating plane.
    const double width; // mm, strip width along its rulings.
    const double thickness; // mm, uniform strip thickness.
    const size_t samples; // Rulings each strip is interpolated through.
    const double deployment; // Normalized deployment fraction w = alpha / opening in [0, 1].

private:
    double _distortion = 0.0; // Largest relative diagonal error of any strip's flat development.
    std::array<double, 4> _sector_sums = {0.0, 0.0, 0.0, 0.0}; // Sum of four vertex sector angles per quadrant (radians).

public:
    /// The four quadrants of the crease, width wide strips, at deployment fraction w in [0, 1].
    CreaseQuadrants(
        const NurbsCurve& crease,
        double opening,
        double width,
        double thickness = 40.0,
        const std::string& name = "crease_quadrants",
        size_t samples = 48,
        double deployment = 1.0
    );

    /// Not copied, as a session is not.
    CreaseQuadrants(const CreaseQuadrants&) = delete;

    /// Not assigned, as it is not copied.
    CreaseQuadrants& operator=(const CreaseQuadrants&) = delete;

    /// Largest relative length error of any strip's flat development, 0 on exact cones/cylinders.
    double distortion() const {
        return _distortion;
    }

    /// Vertex sector angle sums (radians) for Q1..Q4: 2*pi on developable Q1/Q3, < 2*pi on synclastic Q2, > 2*pi on anticlastic Q4.
    const std::array<double, 4>& sector_sums() const {
        return _sector_sums;
    }

private:
    /// Sampled Frenet/osculating frame of the planar crease.
    struct Frame {
        std::vector<Point> points; // Sampled points on the crease.
        std::vector<Vector> tangents; // Unit tangents.
        std::vector<Vector> inward; // Unit normals in the crease plane toward its concave side.
        Plane plane; // Osculating plane of the crease.
        Vector normal; // Osculating plane unit normal.
    };

    /// Sampled frame of the crease, uncurled when deployment < 1.
    Frame compute_frame() const;

    /// Build quadrant q (1..4) shifted by shift, using axis-shift + miter lock and recording flat patterns.
    void add_quadrant(
        const Frame& frame,
        size_t quadrant,
        const std::array<std::array<double, 2>, 2>& signs,
        const Vector& shift,
        std::vector<Development>& developments
    );

    /// Place all flat 2D strip developments in group "flat" beside the 3D quadrants.
    void add_flat(const std::vector<Development>& developments);
};

inline CreaseQuadrants::CreaseQuadrants(
    const NurbsCurve& crease,
    double opening,
    double width,
    double thickness,
    const std::string& name,
    size_t samples,
    double deployment
)
    : WoodSession(name), crease(crease), opening(opening), width(width), thickness(thickness),
      samples(std::max<size_t>(samples, 4)), deployment(deployment) {

    if (width <= 0.0 || thickness <= 0.0 || opening <= 0.0 || opening >= 0.5 * Tolerance::PI ||
        deployment < 0.0 || deployment > 1.0)
        throw std::invalid_argument(
            "Crease quadrants need positive width and thickness, opening in (0, 90) degrees and deployment in [0, 1]"
        );

    // frame: crease points, tangents, inward normals and osculating plane
    const Frame frame = compute_frame();

    // quadrants: Q1 to Q4 side by side along the chord
    Vector chord = frame.points.back() - frame.points.front();
    const double step = chord.magnitude() + 3.0 * width;
    chord.normalize_self();

    std::vector<Development> developments;
    add_quadrant(frame, 1, {{{1.0, 1.0}, {-1.0, 1.0}}}, chord * 0.0, developments);
    add_quadrant(frame, 2, {{{1.0, 1.0}, {1.0, -1.0}}}, chord * step, developments);
    add_quadrant(frame, 3, {{{1.0, -1.0}, {-1.0, -1.0}}}, chord * (2.0 * step), developments);
    add_quadrant(frame, 4, {{{-1.0, -1.0}, {-1.0, 1.0}}}, chord * (3.0 * step), developments);

    // flat: 2D cutting patterns of all eight strips in the xy-plane
    add_flat(developments);

    // contacts: shared miter faces at full deployment
    if (std::abs(deployment - 1.0) < 1e-12)
        compute_face_contacts(0);

    // assembly: each quadrant on its own crease
    for (size_t quadrant = 1; quadrant <= 4; quadrant++)
        assemblies.push_back(AssemblyModule::module(
            fmt::format("quadrant_{}", quadrant),
            AssemblyModule::parts_of(
                get_elements_numbered<FaceCurvedElement>(fmt::format("quadrant_{}", quadrant))
            )
        ));
}

inline CreaseQuadrants::Frame CreaseQuadrants::compute_frame() const {

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

    if (frame.normal.magnitude() < 1e-9)
        throw std::invalid_argument("A straight crease has no concave side");

    frame.normal.normalize_self();
    const Vector x_axis = (target_points.back() - target_points.front()).normalized();
    frame.plane = Plane(target_points.front(), x_axis, frame.normal.cross(x_axis));

    if (std::abs(deployment - 1.0) < 1e-12) {
        frame.points = target_points;
        frame.tangents = target_tangents;
    } else {
        // uncurl crease according to cos(alpha) = k_2d / k_3d (Eq. 5 & 8)
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

        const double ratio = std::cos(opening) / std::cos(deployment * opening);
        const size_t middle = samples / 2;
        std::vector<double> deployed_headings(samples);
        deployed_headings[middle] = headings[middle];
        for (size_t j = middle; j + 1 < samples; j++) {
            const double delta = std::atan2(
                std::sin(headings[j + 1] - headings[j]),
                std::cos(headings[j + 1] - headings[j])
            );
            deployed_headings[j + 1] =
                deployed_headings[j] + 2.0 * std::asin(std::clamp(ratio * std::sin(0.5 * delta), -1.0, 1.0));
        }
        for (size_t j = middle; j > 0; j--) {
            const double delta = std::atan2(
                std::sin(headings[j] - headings[j - 1]),
                std::cos(headings[j] - headings[j - 1])
            );
            deployed_headings[j - 1] =
                deployed_headings[j] - 2.0 * std::asin(std::clamp(ratio * std::sin(0.5 * delta), -1.0, 1.0));
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

        const NurbsCurve rebuilt = NurbsCurve::create_interpolated(frame.points, CurveNurbsKnotStyle::Uniform);
        const double rebuilt_domain = rebuilt.domain_end() - rebuilt.domain_start();
        for (size_t j = 0; j <= samples; j++) {
            const double s =
                rebuilt.domain_start() + rebuilt_domain * static_cast<double>(j) / static_cast<double>(samples);
            frame.tangents.push_back(rebuilt.tangent_at(s).normalized());
        }
    }

    for (const Vector& tangent : frame.tangents)
        frame.inward.push_back(frame.normal.cross(tangent).normalized());

    return frame;
}

inline void CreaseQuadrants::add_quadrant(
    const Frame& frame,
    size_t quadrant,
    const std::array<std::array<double, 2>, 2>& signs,
    const Vector& shift,
    std::vector<Development>& developments
) {

    const std::shared_ptr<TreeNode> group = add_group(fmt::format("quadrant_{}", quadrant));
    const bool ccf = signs[0][1] == signs[1][1];
    const double alpha = deployment * opening;
    const double c_star = std::cos(opening);
    const double s_star = std::sin(opening);
    const double c = std::cos(alpha);
    const double s = std::sin(alpha);

    const size_t middle = samples / 2;
    const Vector edge_prev = (frame.points[middle - 1] - frame.points[middle]).normalized();
    const Vector edge_next = (frame.points[middle + 1] - frame.points[middle]).normalized();
    double angle_sum = 0.0;

    for (size_t k = 0; k < 2; k++) {
        // target miter bevel from axis-shifted hinge across uniform thickness t
        std::array<std::vector<Point>, 4> edges;
        std::vector<Point> neutral_a;
        std::vector<Point> neutral_b;

        for (size_t j = 0; j <= samples; j++) {
            const Point hinge = frame.points[j] + shift;
            const Vector ruling_star =
                (frame.inward[j] * (signs[k][0] * c_star) + frame.normal * (signs[k][1] * s_star)).normalized();
            const Vector ruling =
                (frame.inward[j] * (signs[k][0] * c) + frame.normal * (signs[k][1] * s)).normalized();

            if (j == middle)
                angle_sum += std::acos(std::clamp(edge_next.dot(ruling_star), -1.0, 1.0)) +
                             std::acos(std::clamp(edge_prev.dot(ruling_star), -1.0, 1.0));

            // axis-shift normal pointing from the shared hinge face into the strip's body:
            // - CCF (Q1/Q3): hinge on outer fold face (-signs[k][1]*normal), V-miter on inner fold face (+signs[k][1]*normal), opens flat into one sheet at w = 0
            // - CCU (Q2/Q4): hinge on shared contact face, V-miter away from contact plane (-signs[k][1]*normal), closes into a 2t stack at w = 0
            Vector body_normal_star = frame.tangents[j].cross(ruling_star).normalized();
            Vector body_normal = frame.tangents[j].cross(ruling).normalized();
            if (ccf) {
                if (body_normal_star.dot(frame.normal * signs[k][1]) < 0.0)
                    body_normal_star = body_normal_star * -1.0;
                if (body_normal.dot(frame.normal * signs[k][1]) < 0.0)
                    body_normal = body_normal * -1.0;
            } else {
                if (body_normal_star.dot(frame.normal * (-signs[k][1])) < 0.0)
                    body_normal_star = body_normal_star * -1.0;
                if (body_normal.dot(frame.normal * (-signs[k][1])) < 0.0)
                    body_normal = body_normal * -1.0;
            }

            const Vector mitre = ccf ? frame.inward[j] : frame.normal;
            const double bevel_reach = (-thickness * body_normal_star.dot(mitre)) / ruling_star.dot(mitre);

            const Point face_hinge_start = hinge;
            const Point face_hinge_end = hinge + ruling * width;
            const Point face_bevel_start = hinge + body_normal * thickness + ruling * bevel_reach;
            const Point face_bevel_end = hinge + body_normal * thickness + ruling * width;

            edges[0].push_back(face_bevel_start);
            edges[1].push_back(face_bevel_end);
            edges[2].push_back(face_hinge_start);
            edges[3].push_back(face_hinge_end);

            neutral_a.push_back(hinge + body_normal * (0.5 * thickness) + ruling * (0.5 * bevel_reach));
            neutral_b.push_back(hinge + body_normal * (0.5 * thickness) + ruling * width);
        }

        NurbsCurve outer_a = NurbsCurve::create_interpolated(edges[0], CurveNurbsKnotStyle::Uniform);
        NurbsCurve outer_b = NurbsCurve::create_interpolated(edges[1], CurveNurbsKnotStyle::Uniform);
        NurbsCurve inner_a = NurbsCurve::create_interpolated(edges[2], CurveNurbsKnotStyle::Uniform);
        NurbsCurve inner_b = NurbsCurve::create_interpolated(edges[3], CurveNurbsKnotStyle::Uniform);

        const double mid_t = 0.5 * (outer_a.domain_start() + outer_a.domain_end());
        const Vector tangent = outer_a.tangent_at(mid_t);
        const Vector across = outer_b.point_at(0.5 * (outer_b.domain_start() + outer_b.domain_end())) - outer_a.point_at(mid_t);
        const Vector up = outer_a.point_at(mid_t) - inner_a.point_at(0.5 * (inner_a.domain_start() + inner_a.domain_end()));
        if (across.cross(tangent).dot(up) < 0.0) {
            std::swap(outer_a, outer_b);
            std::swap(inner_a, inner_b);
        }

        const std::shared_ptr<FaceCurvedElement> strip = std::make_shared<FaceCurvedElement>(
            outer_a, outer_b, inner_a, inner_b, fmt::format("quadrant_{}_{}", quadrant, k)
        );
        add(strip, group);

        developments.emplace_back(*strip, samples, fmt::format("flat_quadrant_{}_{}", quadrant, k));
        _distortion = std::max(_distortion, developments.back().distortion);
    }

    _sector_sums[quadrant - 1] = angle_sum;

    std::vector<Point> points;
    for (const Point& point : frame.points)
        points.push_back(point + shift);

    Polyline drawn(points);
    drawn.name = fmt::format("crease_{}", quadrant);
    add_polyline(drawn, group_named("creases"));
}

inline void CreaseQuadrants::add_flat(const std::vector<Development>& developments) {

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
            flat_piece->name = fmt::format("flat_quadrant_{}_{}", i / 2 + 1, i % 2);
            flat_piece->place(shift);
            add(flat_piece, flat);
        }
        cursor_x += (high_x - low_x) + 300.0;
    }
}
