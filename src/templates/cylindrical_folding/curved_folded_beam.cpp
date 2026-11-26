#include "src/templates/cylindrical_folding/curved_folded_beam.h"

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CurvedFoldedBeam::CurvedFoldedBeam(const CurvedFoldedBeamGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // corner: the top right crease and the inner crease under it, the mitre between them
    const std::array<std::vector<Point>, 2> corner = compute_corner();

    // beams: every beam's four plates between its four corners, each a cylinder with its rulings straight across, and its webs
    add_beams(corner);

    // contacts: every two plates that share a face, found by the session's contact detection
    compute_face_contacts(0);

    // assembly: every beam from its bottom, its webs and sides and its top, the beams put side by side
    std::vector<AssemblyModule> beams;

    for (size_t b = 0; b < guide.count; b++)
        beams.push_back(
            AssemblyModule::raise(
                fmt::format("beam_{}", b),
                AssemblyModule::parts_of(
                    get_elements_numbered<FaceCurvedElement>(fmt::format("beam_{}", b))
                )
            )
        );

    assemblies = {AssemblyModule::module(name, beams)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results
// ═══════════════════════════════════════════════════════════════════════════
double CurvedFoldedBeam::distortion() const {
    return _distortion;
}

// ═══════════════════════════════════════════════════════════════════════════
// Corner
// ═══════════════════════════════════════════════════════════════════════════
std::array<std::vector<Point>, 2> CurvedFoldedBeam::compute_corner() const {

    const double half = 0.5 * guide.span;
    std::array<std::vector<Point>, 2> corner;

    for (size_t k = 0; k <= guide.samples; k++) {
        const double x =
            -half + guide.span * static_cast<double>(k) / static_cast<double>(guide.samples);
        const double side = profile(0.5 * guide.width, -guide.sag, x).first;
        const double top = profile(0.25 * guide.width, guide.rise, x).first;
        const double inner_side = inner_distance(0.5 * guide.width, -guide.sag, x);
        const double inner_top = inner_distance(0.25 * guide.width, guide.rise, x);
        corner[0].push_back(Point(x, side, top));
        corner[1].push_back(Point(x, inner_side, inner_top));
    }

    return corner;
}

std::pair<double, double> CurvedFoldedBeam::profile(double base, double change, double x) const {

    const double half = 0.5 * guide.span;
    const double radius = (half * half + change * change) / (2.0 * std::abs(change));
    const double root = std::sqrt(radius * radius - x * x);
    const double sign = change > 0.0 ? 1.0 : -1.0;

    return std::make_pair(base + change - sign * radius + sign * root, -sign * x / root);
}

std::pair<double, double>
CurvedFoldedBeam::inside_point(double base, double change, double u) const {

    const std::pair<double, double> value = profile(base, change, u);
    const double length = std::sqrt(1.0 + value.second * value.second);

    return std::make_pair(
        u + guide.thickness * value.second / length, value.first - guide.thickness / length
    );
}

double CurvedFoldedBeam::inner_distance(double base, double change, double x) const {

    double low = x - 2.0 * guide.thickness;
    double high = x + 2.0 * guide.thickness;

    for (int i = 0; i < 80; i++) {
        const double middle = 0.5 * (low + high);

        if (inside_point(base, change, middle).first < x)
            low = middle;
        else
            high = middle;
    }

    return inside_point(base, change, 0.5 * (low + high)).second;
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════
void CurvedFoldedBeam::add_beams(const std::array<std::vector<Point>, 2>& corner) {

    const NurbsCurve inner_crease =
        NurbsCurve::create_interpolated(corner[1], CurveNurbsKnotStyle::Uniform);

    for (size_t b = 0; b < guide.count; b++) {
        const std::shared_ptr<TreeNode> group = add_group(fmt::format("beam_{}", b));
        add_plates(corner, b, group);
        add_webs(inner_crease, b, group);
    }
}

void CurvedFoldedBeam::add_plates(
    const std::array<std::vector<Point>, 2>& corner,
    size_t b,
    const std::shared_ptr<TreeNode>& group
) {

    // the four corners as signs of y and z, and the plates between two of them: top, right, bottom, left
    const std::array<std::array<double, 2>, 4> corners = {
        {{1.0, 1.0}, {1.0, -1.0}, {-1.0, -1.0}, {-1.0, 1.0}}
    };
    const std::array<std::array<size_t, 2>, 4> plates = {{{3, 0}, {0, 1}, {1, 2}, {2, 3}}};
    const double shift = guide.width * static_cast<double>(b);

    for (size_t k = 0; k < 4; k++) {
        // outer_a, outer_b, inner_a, inner_b: the plate's two corners, the crease then the inner crease of each
        std::array<NurbsCurve, 4> curves;

        for (size_t e = 0; e < 4; e++) {
            const std::array<double, 2>& sign = corners[plates[k][e % 2]];
            std::vector<Point> points;

            for (const Point& point : corner[e / 2])
                points.push_back(Point(point[0], sign[0] * point[1] + shift, sign[1] * point[2]));

            curves[e] = NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform);
        }

        const std::shared_ptr<FaceCurvedElement> plate = std::make_shared<FaceCurvedElement>(
            curves[0], curves[1], curves[2], curves[3], fmt::format("beam_{}_{}", b, k)
        );
        add(plate, group);

        const Development development(curves[0], curves[1], guide.samples);
        _distortion = std::max(_distortion, development.distortion);
    }
}

void CurvedFoldedBeam::add_webs(
    const NurbsCurve& inner_crease, size_t b, const std::shared_ptr<TreeNode>& group
) {

    const double shift = guide.width * static_cast<double>(b);

    for (size_t w = 0; w < guide.webs; w++) {
        const double station = -0.5 * guide.span + guide.span * static_cast<double>(w + 1) /
                                                       static_cast<double>(guide.webs + 1);
        std::array<NurbsCurve, 4> curves;

        for (size_t e = 0; e < 4; e++) {
            const double x = station + (e < 2 ? -0.5 : 0.5) * guide.thickness;
            const double side = e % 2 == 0 ? 1.0 : -1.0;
            double low = inner_crease.domain_start();
            double high = inner_crease.domain_end();

            for (int i = 0; i < 80; i++) {
                const double middle = 0.5 * (low + high);

                if (inner_crease.point_at(middle)[0] < x)
                    low = middle;
                else
                    high = middle;
            }

            const Point point = inner_crease.point_at(0.5 * (low + high));
            const Point left(x, shift - point[1], side * point[2]);
            const Point right(x, shift + point[1], side * point[2]);
            curves[e] = NurbsCurve::create(false, 1, {left, right});
        }

        const std::shared_ptr<FaceCurvedElement> web = std::make_shared<FaceCurvedElement>(
            curves[0], curves[1], curves[2], curves[3], fmt::format("beam_{}_{}", b, 4 + w)
        );
        add(web, group);
    }
}

}
