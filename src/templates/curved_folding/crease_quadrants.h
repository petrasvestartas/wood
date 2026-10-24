#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// The four quadrants of one planar curved crease: a developable through a planar crease has its rulings r on one side of the crease plane and -r on the other, and their
/// mirror images Mr and -Mr in the plane. Two strips joined along the crease take one ruling each:
///
/// | quadrant | rulings | kind | the two strips |
/// | --- | --- | --- | --- |
/// | Q1 | r, -Mr | curved-crease folding | one sheet folded, both strips on the plane's front, the crease convex for one and concave for the other |
/// | Q2 | r, Mr | curved-crease unfolding | two mirror sheets, the crease convex for both: synclastic, the sector angles summing to less than 360 |
/// | Q3 | Mr, -r | curved-crease folding | one sheet folded the other way, both strips behind the plane |
/// | Q4 | -r, -Mr | curved-crease unfolding | two mirror sheets, the crease concave for both: anticlastic, the sector angles summing to more than 360 |
///
/// r leaves the crease perpendicular to it, at the opening to its plane, toward the crease's concave side (the ultra-special case of a
/// planar crease with a constant reflection angle: every strip a cone on a circle). Each strip is a PlateCurved width wide: its faces half
/// the thickness off the mid-surface, its far end square, its crease side mitred where it meets the other strip, the crease plane for an
/// unfolding (the two strips are each other's mirror image there) and the plane through the crease square to it for a folding (the two
/// rulings are symmetric about it).
///
/// Fields: the crease, the opening, the width, the thickness and the sampling; the quadrants side by side along the crease's chord.
class CreaseQuadrants : public WoodSession {
public:
    const NurbsCurve crease; // A planar curved crease.
    const double opening; // radians, the angle between the rulings and the crease plane.
    const double width; // mm, every strip along its rulings.
    const double thickness; // mm, every strip.
    const size_t samples; // Rulings each strip is interpolated through.

    /// The four quadrants of the crease, width wide strips, as the session named name.
    CreaseQuadrants(const NurbsCurve& crease, double opening, double width, double thickness = 40.0, const std::string& name = "crease_quadrants", size_t samples = 48);

    /// Not copied, as a session is not.
    CreaseQuadrants(const CreaseQuadrants&) = delete;

    /// Not assigned, as it is not copied.
    CreaseQuadrants& operator=(const CreaseQuadrants&) = delete;

    /// The largest relative length error of any strip's development, the proof every strip is cut flat.
    double distortion() const {
        return _distortion;
    }

private:
    double _distortion = 0.0;

    /// The crease sampled with its unit tangents and its unit normals in its plane toward its concave side, and the plane's unit normal.
    struct Frame {
        std::vector<Point> points;
        std::vector<Vector> tangents;
        std::vector<Vector> inward;
        Vector normal;
    };

    /// The crease's frame.
    Frame compute_frame() const;

    /// One quadrant, its two strips quadrant_<q>_<k> in the group quadrant_<q>, its crease beside them, moved by shift: the strips' rulings
    /// signs[k] = {across, along the normal}, folded when the two share the normal's sign.
    void add_quadrant(const Frame& frame, size_t quadrant, const std::array<std::array<double, 2>, 2>& signs, const Vector& shift);
};

inline CreaseQuadrants::CreaseQuadrants(const NurbsCurve& crease, double opening, double width, double thickness, const std::string& name, size_t samples)
    : WoodSession(name),
      crease(crease),
      opening(opening),
      width(width),
      thickness(thickness),
      samples(std::max<size_t>(samples, 4)) {

    if (width <= 0.0 || thickness <= 0.0 || opening <= 0.0 || opening >= 0.5 * Tolerance::PI)
        throw std::invalid_argument("Crease quadrants need a width, a thickness and an opening between 0 and 90 degrees");

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

    for (size_t j = 0; j <= samples; j++) {
        const double t = crease.domain_start() + (crease.domain_end() - crease.domain_start()) * static_cast<double>(j) / static_cast<double>(samples);
        frame.points.push_back(crease.point_at(t));
        frame.tangents.push_back(crease.tangent_at(t).normalized());
    }

    // the plane's normal from the two most different tangents, turned so the tangent turns toward normal x tangent
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

inline void CreaseQuadrants::add_quadrant(const Frame& frame, size_t quadrant, const std::array<std::array<double, 2>, 2>& signs, const Vector& shift) {

    const std::shared_ptr<TreeNode> group = add_group(fmt::format("quadrant_{}", quadrant));
    const bool folded = signs[0][1] == signs[1][1];
    const double half = 0.5 * thickness;
    const double c = std::cos(opening);
    const double s = std::sin(opening);

    for (size_t k = 0; k < 2; k++) {
        std::array<std::vector<Point>, 4> edges;

        for (size_t j = 0; j <= samples; j++) {
            // the ruling and the face normal; the mitre the crease plane, or the plane through the crease square to it
            const Point point = frame.points[j] + shift;
            const Vector ruling = (frame.inward[j] * (signs[k][0] * c) + frame.normal * (signs[k][1] * s)).normalized();
            const Vector normal = frame.tangents[j].cross(ruling).normalized();
            const Vector mitre = folded ? frame.inward[j] : frame.normal;

            for (size_t face = 0; face < 2; face++) {
                const Point start = point + normal * (face == 0 ? half : -half);
                const double reach = (point - start).dot(mitre) / ruling.dot(mitre);
                edges[2 * face].push_back(start + ruling * reach);
                edges[2 * face + 1].push_back(start + ruling * width);
            }
        }

        const NurbsCurve outer_a = NurbsCurve::create_interpolated(edges[0], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve outer_b = NurbsCurve::create_interpolated(edges[1], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve inner_a = NurbsCurve::create_interpolated(edges[2], CurveNurbsKnotStyle::Uniform);
        const NurbsCurve inner_b = NurbsCurve::create_interpolated(edges[3], CurveNurbsKnotStyle::Uniform);
        add(std::make_shared<PlateCurved>(outer_a, outer_b, inner_a, inner_b, fmt::format("quadrant_{}_{}", quadrant, k)), group);

        // its development, the proof it is cut flat
        const Development development(outer_a, outer_b, samples);
        _distortion = std::max(_distortion, development.distortion);
    }

    // the crease
    std::vector<Point> points;

    for (const Point& point : frame.points)
        points.push_back(point + shift);

    Polyline drawn(points);
    drawn.name = fmt::format("crease_{}", quadrant);
    add_polyline(drawn, group_named("creases"));
}
