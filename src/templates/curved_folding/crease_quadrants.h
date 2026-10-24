#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// The four quadrants of one planar curved crease.
class CreaseQuadrants : public WoodSession {
public:
    const NurbsCurve crease; // A planar curved crease.
    const double opening; // radians, the angle between the rulings and the crease plane.
    const double width; // mm, every strip along its rulings.
    const double thickness; // mm, every strip.
    const size_t samples; // Rulings each strip is interpolated through.

private:
    double _distortion = 0.0; // The largest relative length error of any strip's development.

public:
    /// The four quadrants of the crease, width wide strips, as the session named name.
    CreaseQuadrants(
        const NurbsCurve& crease,
        double opening,
        double width,
        double thickness = 40.0,
        const std::string& name = "crease_quadrants",
        size_t samples = 48
    );

    /// Not copied, as a session is not.
    CreaseQuadrants(const CreaseQuadrants&) = delete;

    /// Not assigned, as it is not copied.
    CreaseQuadrants& operator=(const CreaseQuadrants&) = delete;

    /// The largest relative length error of any strip's development, the proof every strip is cut flat.
    double distortion() const {
        return _distortion;
    }

private:
    /// The crease sampled with its plane's normal.
    struct Frame {
        std::vector<Point> points; // The crease's samples.
        std::vector<Vector> tangents; // Its unit tangents.
        std::vector<Vector> inward; // Its unit normals in its plane toward its concave side.
        Vector normal; // The plane's unit normal.
    };

    /// The crease's frame.
    Frame compute_frame() const;

    /// One quadrant moved by shift, its strips quadrant_<q>_<k> with rulings signs[k] = {across, along the normal}, and its crease.
    void add_quadrant(
        const Frame& frame,
        size_t quadrant,
        const std::array<std::array<double, 2>, 2>& signs,
        const Vector& shift
    );
};

inline CreaseQuadrants::CreaseQuadrants(
    const NurbsCurve& crease,
    double opening,
    double width,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name), crease(crease), opening(opening), width(width), thickness(thickness),
      samples(std::max<size_t>(samples, 4)) {

    if (width <= 0.0 || thickness <= 0.0 || opening <= 0.0 || opening >= 0.5 * Tolerance::PI)
        throw std::invalid_argument(
            "Crease quadrants need a width, a thickness and an opening between 0 and 90 degrees"
        );

    // frame: the crease's points, tangents, inward normals and plane
    const Frame frame = compute_frame();

    // quadrants: Q1 to Q4 side by side along the chord, each one the chord and two widths beyond the one before
    Vector chord = frame.points.back() - frame.points.front();
    const double step = chord.magnitude() + 3.0 * width;
    chord.normalize_self();

    add_quadrant(frame, 1, {{{1.0, 1.0}, {-1.0, 1.0}}}, chord * 0.0);
    add_quadrant(frame, 2, {{{1.0, 1.0}, {1.0, -1.0}}}, chord * step);
    add_quadrant(frame, 3, {{{1.0, -1.0}, {-1.0, -1.0}}}, chord * (2.0 * step));
    add_quadrant(frame, 4, {{{-1.0, -1.0}, {-1.0, 1.0}}}, chord * (3.0 * step));

    // contacts: every two strips that share a face, found by the session's contact detection
    compute_face_contacts(0);
}

inline CreaseQuadrants::Frame CreaseQuadrants::compute_frame() const {

    Frame frame;
    const double domain = crease.domain_end() - crease.domain_start();

    for (size_t j = 0; j <= samples; j++) {
        const double t =
            crease.domain_start() + domain * static_cast<double>(j) / static_cast<double>(samples);
        frame.points.push_back(crease.point_at(t));
        frame.tangents.push_back(crease.tangent_at(t).normalized());
    }

    frame.normal = frame.tangents.front().cross(frame.tangents.back());

    for (const Vector& tangent : frame.tangents)
        if (frame.tangents.front().cross(tangent).magnitude() > frame.normal.magnitude())
            frame.normal = frame.tangents.front().cross(tangent);

    if (frame.normal.magnitude() < 1e-9)
        throw std::invalid_argument("A straight crease has no concave side");

    frame.normal.normalize_self();

    for (const Vector& tangent : frame.tangents)
        frame.inward.push_back(frame.normal.cross(tangent).normalized());

    return frame;
}

inline void CreaseQuadrants::add_quadrant(
    const Frame& frame,
    size_t quadrant,
    const std::array<std::array<double, 2>, 2>& signs,
    const Vector& shift
) {

    const std::shared_ptr<TreeNode> group = add_group(fmt::format("quadrant_{}", quadrant));
    const bool folded = signs[0][1] == signs[1][1];
    const double half = 0.5 * thickness;
    const double c = std::cos(opening);
    const double s = std::sin(opening);

    for (size_t k = 0; k < 2; k++) {
        // the four edges: each face's rulings from the mitre, the crease plane or the plane through the crease square to it
        std::array<std::vector<Point>, 4> edges;

        for (size_t j = 0; j <= samples; j++) {
            const Point point = frame.points[j] + shift;
            const Vector ruling =
                (frame.inward[j] * (signs[k][0] * c) + frame.normal * (signs[k][1] * s))
                    .normalized();
            const Vector normal = frame.tangents[j].cross(ruling).normalized();
            const Vector mitre = folded ? frame.inward[j] : frame.normal;

            for (size_t face = 0; face < 2; face++) {
                const Point start = point + normal * (face == 0 ? half : -half);
                const double reach = (point - start).dot(mitre) / ruling.dot(mitre);
                edges[2 * face].push_back(start + ruling * reach);
                edges[2 * face + 1].push_back(start + ruling * width);
            }
        }

        const NurbsCurve outer_a =
            NurbsCurve::create_interpolated(edges[0], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve outer_b =
            NurbsCurve::create_interpolated(edges[1], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve inner_a =
            NurbsCurve::create_interpolated(edges[2], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve inner_b =
            NurbsCurve::create_interpolated(edges[3], CurveNurbsKnotStyle::Uniform);
        const std::shared_ptr<PlateCurved> strip = std::make_shared<PlateCurved>(
            outer_a, outer_b, inner_a, inner_b, fmt::format("quadrant_{}_{}", quadrant, k)
        );
        add(strip, group);

        const Development development(outer_a, outer_b, samples);
        _distortion = std::max(_distortion, development.distortion);
    }

    std::vector<Point> points;

    for (const Point& point : frame.points)
        points.push_back(point + shift);

    Polyline drawn(points);
    drawn.name = fmt::format("crease_{}", quadrant);
    add_polyline(drawn, group_named("creases"));
}
