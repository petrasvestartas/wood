#include "src/templates/cylindrical_folding/corner_fan.h"

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CornerFan::CornerFan(const CornerFanGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // tops: every fold's end on the square's two far sides, high and low in turn
    const std::vector<Point> tops = compute_tops();

    // creases: every fold a parabola from the corner, all lifted alike, so two neighbours differ by s times their tops' difference
    // and every strip is a cylinder of rulings parallel to its straight top edge
    const std::vector<NurbsCurve> creases = compute_creases(tops);

    // inners: at every fold the curve both strips' inner faces end on, the bisector of the two
    const std::vector<NurbsCurve> inners = compute_inners(tops);

    // strips: every cylinder between two folds, its inner face between their inner creases
    add_strips(creases, inners);

    // contacts: every two strips that share a fold, found by the session's contact detection
    compute_face_contacts(0);

    // assembly: the strips from one side of the square to the other
    assemblies = {AssemblyModule::module(
        name, AssemblyModule::parts_of(get_elements_numbered<FaceCurvedElement>("strip"))
    )};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results
// ═══════════════════════════════════════════════════════════════════════════
double CornerFan::distortion() const {
    return _distortion;
}

// ═══════════════════════════════════════════════════════════════════════════
// Folds
// ═══════════════════════════════════════════════════════════════════════════
std::vector<Point> CornerFan::compute_tops() const {

    // from the side y = 0 up the far side x = size to the far corner, then along the far side y = size to the side x = 0
    std::vector<Point> tops;
    const size_t half = guide.strips / 2;

    for (size_t k = 0; k <= guide.strips; k++) {
        const double z = k % 2 == 0 ? guide.height : guide.height - guide.drop;

        if (k <= half)
            tops.push_back(Point(
                guide.size, guide.size * static_cast<double>(k) / static_cast<double>(half), z
            ));
        else
            tops.push_back(Point(
                guide.size * static_cast<double>(guide.strips - k) / static_cast<double>(half),
                guide.size,
                z
            ));
    }

    return tops;
}

Point CornerFan::crease_point(const Point& top, double s) const {

    // the quadratic from the corner, the chord's middle lifted by lift and the top
    const Vector to_top(top[0], top[1], top[2]);
    const Vector control = to_top * 0.5 + Vector(0.0, 0.0, guide.lift);
    const Vector point = control * (2.0 * s * (1.0 - s)) + to_top * (s * s);

    return Point(point[0], point[1], point[2]);
}

Vector CornerFan::crease_tangent(const Point& top, double s) const {

    const Vector to_top(top[0], top[1], top[2]);
    const Vector control = to_top * 0.5 + Vector(0.0, 0.0, guide.lift);

    return control * (2.0 * (1.0 - 2.0 * s)) + to_top * (2.0 * s);
}

Vector CornerFan::strip_normal(const Vector& tangent, const Point& top, const Point& next) {

    // square to the strip's rulings, parallel to its top edge, and to the fold's tangent, turned under the sheet
    Vector normal = (next - top).cross(tangent).normalized();

    if (normal[2] > 0.0)
        normal = -normal;

    return normal;
}

// ═══════════════════════════════════════════════════════════════════════════
// Creases
// ═══════════════════════════════════════════════════════════════════════════
std::vector<NurbsCurve> CornerFan::compute_creases(const std::vector<Point>& tops) const {

    // the parabola cut at foot by de Casteljau, still a parabola of three control points
    std::vector<NurbsCurve> creases;

    for (const Point& top : tops) {
        const Vector to_top(top[0], top[1], top[2]);
        const Vector control = to_top * 0.5 + Vector(0.0, 0.0, guide.lift);
        const Vector between = control * (1.0 - guide.foot) + to_top * guide.foot;
        const std::vector<Point> controls = {
            crease_point(top, guide.foot), Point(between[0], between[1], between[2]), top
        };
        creases.push_back(NurbsCurve::create(false, 2, controls));
    }

    return creases;
}

Point CornerFan::inner_point(const std::vector<Point>& tops, size_t k, double s) const {

    const Point point = crease_point(tops[k], s);
    const Vector tangent = crease_tangent(tops[k], s);

    // a free fold: square under the one strip
    if (k == 0)
        return point + strip_normal(tangent, tops[0], tops[1]) * guide.thickness;

    if (k == guide.strips)
        return point + strip_normal(tangent, tops[k - 1], tops[k]) * guide.thickness;

    // an inner fold: a thickness under both strips, on their bisector square to the fold
    const Vector before = strip_normal(tangent, tops[k - 1], tops[k]);
    const Vector after = strip_normal(tangent, tops[k], tops[k + 1]);

    return point + (before + after) * (guide.thickness / (1.0 + before.dot(after)));
}

double CornerFan::inner_on(
    const std::vector<Point>& tops, size_t k, const Plane& plane, double start
) const {

    // the plane's side of the inner point changes within a tenth of the fold either side of start
    const Vector normal = plane.z_axis();
    double low = start - 0.1;
    double high = start + 0.1;
    const double sign = normal.dot(inner_point(tops, k, low) - plane.origin()) < 0.0 ? -1.0 : 1.0;

    for (int i = 0; i < 80; i++) {
        const double middle = 0.5 * (low + high);

        if (sign * normal.dot(inner_point(tops, k, middle) - plane.origin()) > 0.0)
            low = middle;
        else
            high = middle;
    }

    return 0.5 * (low + high);
}

std::vector<NurbsCurve> CornerFan::compute_inners(const std::vector<Point>& tops) const {

    // top ends: every inner crease cut by the square's far side its fold ends on, so every strip ends on a vertical plane
    std::vector<double> ends;

    for (size_t k = 0; k <= guide.strips; k++) {
        const Plane side = k <= guide.strips / 2
                               ? Plane::from_point_normal(tops[k], Vector(1.0, 0.0, 0.0))
                               : Plane::from_point_normal(tops[k], Vector(0.0, 1.0, 0.0));
        ends.push_back(inner_on(tops, k, side, 1.0));
    }

    // feet: every strip's foot a plane through its outer foot ruling and the inner foot of the fold before, so the next fold's
    // inner foot is found on it, strip by strip from the first free fold
    std::vector<double> feet = {guide.foot};

    for (size_t k = 0; k < guide.strips; k++) {
        const Point first = crease_point(tops[k], guide.foot);
        const Point second = crease_point(tops[k + 1], guide.foot);
        const Point inner = inner_point(tops, k, feet[k]);
        const Vector normal = (second - first).cross(inner - first).normalized();
        feet.push_back(inner_on(tops, k + 1, Plane::from_point_normal(first, normal), guide.foot));
    }

    // the inner creases between their feet and their ends
    std::vector<NurbsCurve> inners;

    for (size_t k = 0; k <= guide.strips; k++) {
        std::vector<Point> points;

        for (size_t i = 0; i <= guide.samples; i++) {
            const double s = feet[k] + (ends[k] - feet[k]) * static_cast<double>(i) /
                                           static_cast<double>(guide.samples);
            points.push_back(inner_point(tops, k, s));
        }

        inners.push_back(NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform));
    }

    return inners;
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════
void CornerFan::add_strips(
    const std::vector<NurbsCurve>& creases, const std::vector<NurbsCurve>& inners
) {

    const std::shared_ptr<TreeNode> group = add_group("strips");
    const Xform move = Xform::translation(guide.corner[0], guide.corner[1], guide.corner[2]);

    for (size_t k = 0; k < guide.strips; k++) {
        // the fan built at the origin, moved to its corner
        const std::shared_ptr<FaceCurvedElement> strip = std::make_shared<FaceCurvedElement>(
            creases[k].transformed(move),
            creases[k + 1].transformed(move),
            inners[k].transformed(move),
            inners[k + 1].transformed(move),
            fmt::format("strip_{}", k)
        );
        add(strip, group);

        // its development, the proof it is cut flat
        const Development development(creases[k], creases[k + 1], guide.samples);
        _distortion = std::max(_distortion, development.distortion);
    }
}

}
