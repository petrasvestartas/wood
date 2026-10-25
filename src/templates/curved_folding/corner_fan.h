#pragma once
#include "wood_session.h"
#include "development.h"

using namespace session_cpp;
using namespace wood_session;

/// A fan of developable conical strips folded from one bottom corner of a box up to its two far top edges.
class CornerFan : public WoodSession {
public:
    const double width; // mm, the box along x.
    const double depth; // mm, the box along y.
    const double height; // mm, the box along z, the corner at the origin.
    const size_t strips; // Strips between the folds, dividing the two far top edges equally.
    const double rise; // mm, how far every strip's top parabola bulges at its middle, up, or down when negative.
    const double foot; // The share of the way up where the strips start, cut above the corner.
    const double thickness; // mm, every strip.
    const size_t samples; // Points every curve is interpolated through.

private:
    /// A strip's top edge: a parabola in the vertical plane through its two folds.
    struct Parabola {
        Point start; // The first fold.
        Vector chord; // From the first fold to the second.
        double bulge; // mm, the rise at the middle, negative bulging down.
    };

    /// The plane two strips meet on at a fold, with the normals of both, the same at a free fold.
    struct Mitre {
        Plane plane; // Through the fold, the bisector of the two strips or square to the one.
        Vector before; // The normal of the strip before the fold, along the fold.
        Vector after; // The normal of the strip after the fold, along the fold.
    };

    double _distortion = 0.0; // The largest relative length error of any strip's development.
    double _material = 1.0; // +1 or -1, the side of the sheet the thickness lies on.

public:
    /// The fan as the session named name.
    CornerFan(
        double width = 6000.0,
        double depth = 6000.0,
        double height = 4000.0,
        size_t strips = 6,
        double rise = 400.0,
        double foot = 0.15,
        double thickness = 40.0,
        const std::string& name = "corner_fan",
        size_t samples = 64
    );

    /// Not copied, as a session is not.
    CornerFan(const CornerFan&) = delete;

    /// Not assigned, as it is not copied.
    CornerFan& operator=(const CornerFan&) = delete;

    /// The largest relative length error of any strip's development, 0 for an exact cone.
    double distortion() const {
        return _distortion;
    }

private:
    /// The fold points dividing the two far top edges equally, from the edge x = width to the edge y = depth.
    std::vector<Point> compute_folds() const;

    /// Every strip's top parabola between two consecutive folds, all bulging the same way so every fold is a crease.
    std::vector<Parabola> compute_parabolas(const std::vector<Point>& folds) const;

    /// The mitre at every fold: the bisector of the two strips there, square to the strip at a free fold.
    std::vector<Mitre> compute_mitres(const std::vector<Point>& folds, const std::vector<Parabola>& parabolas) const;

    /// Every strip, strip_<k> in the group strips: the cone between the corner and its parabola, cut at foot.
    void add_strips(const std::vector<Parabola>& parabolas, const std::vector<Mitre>& mitres);

    /// The parabola's point at t, its first fold at 0 and its second at 1.
    static Point top_point(const Parabola& parabola, double t);

    /// The unit normal of the cone at t, to the side the thickness lies on.
    Vector normal(const Parabola& parabola, double t) const;

    /// The point on the inner face under the cone's point at t and share scale of the way up.
    Point inner_point(const Parabola& parabola, double t, double scale) const;

    /// The t near end where the inner point at scale lies on the mitre plane, by bisection.
    double inner_end(const Parabola& parabola, const Plane& mitre, double end, double scale) const;

    /// The point both strips' inner faces share on the mitre, in the section of the fold through its outer point.
    Point on_mitre(const Mitre& mitre, const Point& point) const;
};

inline CornerFan::CornerFan(
    double width,
    double depth,
    double height,
    size_t strips,
    double rise,
    double foot,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      width(width),
      depth(depth),
      height(height),
      strips(std::max<size_t>(strips, 1)),
      rise(rise),
      foot(foot),
      thickness(thickness),
      samples(std::max<size_t>(samples, 8)) {

    if (width <= 0.0 || depth <= 0.0 || height <= 0.0 || rise == 0.0 || foot <= 0.0 || foot >= 1.0 || thickness <= 0.0)
        throw std::invalid_argument("A corner fan needs a box, a rise, a foot between 0 and 1 and a thickness");

    // folds: the points the straight folds run to from the corner, dividing the two far top edges equally
    const std::vector<Point> folds = compute_folds();

    // parabolas: every strip's top edge bulging the same way, so two strips meet at an angle on every fold
    const std::vector<Parabola> parabolas = compute_parabolas(folds);

    // material: the thickness on the side of the sheet facing the box's middle
    const Point middle(0.5 * width, 0.5 * depth, 0.5 * height);
    const Point probe = top_point(parabolas[0], 0.5);

    if (normal(parabolas[0], 0.5).dot(middle - probe) < 0.0)
        _material = -1.0;

    // mitres: at every fold the plane two strips meet on, square to the strip at the two free folds
    const std::vector<Mitre> mitres = compute_mitres(folds, parabolas);

    // strips: every cone between the corner and its parabola, cut at foot, its inner face ending on the mitres
    add_strips(parabolas, mitres);

    // contacts: every two strips that share a mitre, found by the session's contact detection
    compute_face_contacts(0);
}

inline std::vector<Point> CornerFan::compute_folds() const {

    std::vector<Point> folds;
    const double total = width + depth;

    for (size_t k = 0; k <= strips; k++) {
        const double along = total * static_cast<double>(k) / static_cast<double>(strips);

        if (along <= depth)
            folds.push_back(Point(width, along, height));
        else
            folds.push_back(Point(width - (along - depth), depth, height));
    }

    return folds;
}

inline std::vector<CornerFan::Parabola> CornerFan::compute_parabolas(const std::vector<Point>& folds) const {

    std::vector<Parabola> parabolas;

    for (size_t k = 0; k < strips; k++)
        parabolas.push_back({folds[k], folds[k + 1] - folds[k], rise});

    return parabolas;
}

inline std::vector<CornerFan::Mitre> CornerFan::compute_mitres(const std::vector<Point>& folds, const std::vector<Parabola>& parabolas) const {

    std::vector<Mitre> mitres;
    const Point corner(0.0, 0.0, 0.0);

    for (size_t k = 0; k <= strips; k++) {
        const Vector fold = folds[k] - corner;
        const Vector before = k == 0 ? normal(parabolas[0], 0.0) : normal(parabolas[k - 1], 1.0);
        const Vector after = k == strips ? before : normal(parabolas[k], 0.0);

        // a free fold: the plane through the fold square to its strip
        if (k == 0 || k == strips) {
            mitres.push_back({Plane::from_point_normal(corner, fold.cross(before).normalized()), before, after});
            continue;
        }

        // an inner fold: the bisector of the two strips, where both inner faces lie a thickness off their cones
        mitres.push_back({Plane::from_point_normal(corner, (before - after).normalized()), before, after});
    }

    return mitres;
}

inline Point CornerFan::top_point(const Parabola& parabola, double t) {

    const Vector up(0.0, 0.0, 1.0);

    return parabola.start + parabola.chord * t + up * (4.0 * parabola.bulge * t * (1.0 - t));
}

inline Vector CornerFan::normal(const Parabola& parabola, double t) const {

    const Vector up(0.0, 0.0, 1.0);
    const Vector tangent = parabola.chord + up * (4.0 * parabola.bulge * (1.0 - 2.0 * t));
    const Point point = top_point(parabola, t);
    const Vector generator(point[0], point[1], point[2]);

    return tangent.cross(generator).normalized() * _material;
}

inline Point CornerFan::inner_point(const Parabola& parabola, double t, double scale) const {

    const Point point = top_point(parabola, t);

    return Point(scale * point[0], scale * point[1], scale * point[2]) + normal(parabola, t) * thickness;
}

inline double CornerFan::inner_end(const Parabola& parabola, const Plane& mitre, double end, double scale) const {

    // the mitre's side of the inner point changes between a fifth of the strip either side of its fold
    const Vector n = mitre.z_axis();
    double low = end - 0.2;
    double high = end + 0.2;
    const Point at_low = inner_point(parabola, low, scale);
    const double sign = n.dot(Vector(at_low[0], at_low[1], at_low[2])) < 0.0 ? -1.0 : 1.0;

    for (int i = 0; i < 80; i++) {
        const double middle = 0.5 * (low + high);
        const Point point = inner_point(parabola, middle, scale);

        if (sign * n.dot(Vector(point[0], point[1], point[2])) > 0.0)
            low = middle;
        else
            high = middle;
    }

    return 0.5 * (low + high);
}

inline Point CornerFan::on_mitre(const Mitre& mitre, const Point& point) const {

    // where the planes before . x = thickness, after . x = thickness and the fold's section through point meet
    const Vector along = mitre.before.cross(mitre.after).normalized();
    const Vector position(point[0], point[1], point[2]);
    const std::array<Vector, 3> normals = {mitre.before, mitre.after, along};
    const std::array<double, 3> values = {thickness, thickness, along.dot(position)};
    const double determinant = normals[0].dot(normals[1].cross(normals[2]));
    const Vector first = normals[1].cross(normals[2]) * values[0];
    const Vector second = normals[2].cross(normals[0]) * values[1];
    const Vector third = normals[0].cross(normals[1]) * values[2];
    const Vector solved = (first + second + third) * (1.0 / determinant);

    return Point(solved[0], solved[1], solved[2]);
}

inline void CornerFan::add_strips(const std::vector<Parabola>& parabolas, const std::vector<Mitre>& mitres) {

    const std::shared_ptr<TreeNode> group = add_group("strips");

    for (size_t k = 0; k < strips; k++) {
        const Parabola& parabola = parabolas[k];

        // the outer edges: the parabola at the top and its copy scaled to foot toward the corner, every ruling running to the corner
        std::array<std::vector<Point>, 4> points;

        for (size_t i = 0; i <= samples; i++) {
            const Point top = top_point(parabola, static_cast<double>(i) / static_cast<double>(samples));
            points[1].push_back(top);
            points[0].push_back(Point(foot * top[0], foot * top[1], foot * top[2]));
        }

        // the inner edges: the cone offset a thickness along its normal, run on or cut back to the mitres at both folds
        for (size_t e = 0; e < 2; e++) {
            const double scale = e == 0 ? foot : 1.0;
            const double first = inner_end(parabola, mitres[k].plane, 0.0, scale);
            const double last = inner_end(parabola, mitres[k + 1].plane, 1.0, scale);

            for (size_t i = 0; i <= samples; i++) {
                const double t = first + (last - first) * static_cast<double>(i) / static_cast<double>(samples);
                points[2 + e].push_back(inner_point(parabola, t, scale));
            }

            // at an inner fold both strips end on the line their inner faces share, in the section of the fold's outer
            // point, so their mitres are one face
            if (k > 0)
                points[2 + e].front() = on_mitre(mitres[k], points[e].front());

            if (k + 1 < strips)
                points[2 + e].back() = on_mitre(mitres[k + 1], points[e].back());
        }

        std::array<NurbsCurve, 4> curves;

        for (size_t c = 0; c < 4; c++)
            curves[c] = NurbsCurve::create_interpolated(points[c], CurveNurbsKnotStyle::Uniform);

        const std::shared_ptr<PlateCurved> strip = std::make_shared<PlateCurved>(
            curves[0],
            curves[1],
            curves[2],
            curves[3],
            fmt::format("strip_{}", k)
        );
        add(strip, group);

        // its development, the proof it is cut flat
        const Development development(curves[0], curves[1], samples);
        _distortion = std::max(_distortion, development.distortion);
    }
}
