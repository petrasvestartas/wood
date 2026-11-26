#include "src/templates/cylindrical_folding/cylinder_vault.h"

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CylinderVault::CylinderVault(const CylinderVaultGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // segments: every cylinder with its circle, its generatrices and its footprint
    const std::vector<Segment> segments = compute_segments();

    // plan: the footprints on the ground
    add_plan(segments);

    // panels: every segment one curved plate between the planes its generatrices run between
    const std::vector<Development> developments = add_panels(segments);

    // walls: a flat panel under every side the cloister vault stands on
    if (guide.wall > 0.0)
        add_walls(segments);

    // flat: the cutting pattern beside the model, every panel unfolded
    add_flat(developments, 0.25 * guide.edge);

    // contacts: every two panels that share a face, found by the session's contact detection
    compute_face_contacts(0);

    // assembly: the walls stand, the lamellas lowered on them
    std::vector<AssemblyModule> whole =
        AssemblyModule::parts_of(get_elements_numbered<FaceCurvedElement>("wall"));
    const std::vector<AssemblyModule> lamellas =
        AssemblyModule::parts_of(get_elements_numbered<FaceCurvedElement>("lamella"));
    whole.insert(whole.end(), lamellas.begin(), lamellas.end());
    assemblies = {AssemblyModule::module(name, whole)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results
// ═══════════════════════════════════════════════════════════════════════════
double CylinderVault::distortion() const {
    return _distortion;
}

double CylinderVault::twist() const {
    return _twist;
}

// ═══════════════════════════════════════════════════════════════════════════
// Segments
// ═══════════════════════════════════════════════════════════════════════════
Point CylinderVault::corner(size_t i) const {

    const double circumradius =
        0.5 * guide.edge / std::sin(Tolerance::PI / static_cast<double>(guide.sides));
    const double angle = 2.0 * Tolerance::PI * static_cast<double>(i % guide.sides) /
                         static_cast<double>(guide.sides);

    return Point(circumradius * std::cos(angle), circumradius * std::sin(angle), 0.0);
}

Plane CylinderVault::wall_plane(const Point& from, const Point& to, const Point& inside) {

    Vector normal = (to - from).cross(Vector(0.0, 0.0, 1.0));
    normal.normalize_self();

    if (normal.dot(inside - from) < 0.0)
        normal = -normal;

    return Plane::from_point_normal(from, normal);
}

std::vector<CylinderVault::Segment> CylinderVault::compute_segments() const {

    std::vector<Segment> segments;
    const Point centre(0.0, 0.0, 0.0);
    const double apothem =
        0.5 * guide.edge / std::tan(Tolerance::PI / static_cast<double>(guide.sides));

    for (size_t i = 0; i < guide.sides; i++) {
        const Point from = corner(i);
        const Point to = corner(i + 1);
        const Point middle = Point::mid_point(from, to);
        const Vector along = (to - from).normalized();
        const Vector outward = (middle - centre).normalized();
        Segment segment;

        if (!guide.cloister) {
            segment.axis = -outward;
            segment.centre =
                middle -
                Vector(
                    0.0,
                    0.0,
                    std::sqrt(
                        std::max(0.0, guide.radius * guide.radius - 0.25 * guide.edge * guide.edge)
                    )
                );
            segment.x = along;
            segment.y = Vector(0.0, 0.0, 1.0);
            segment.start = -std::asin(0.5 * guide.edge / guide.radius);
            segment.end = -segment.start;
        } else {
            const double peak =
                guide.radius -
                std::sqrt(std::max(0.0, guide.radius * guide.radius - apothem * apothem));
            segment.axis = along;
            segment.centre = centre + Vector(0.0, 0.0, guide.wall + peak - guide.radius);
            segment.x = outward;
            segment.y = Vector(0.0, 0.0, 1.0);
            segment.start = 0.0;
            segment.end = std::atan2(apothem, guide.radius - peak);
        }

        segment.radius = guide.radius;
        segment.corners = {from, to, centre};
        segment.from = from;
        segment.to = to;
        const size_t count = segment.corners.size();
        const Point inside = Point::centroid(segment.corners);

        for (size_t c = 0; c < count; c++)
            segment.footprint.push_back(
                wall_plane(segment.corners[c], segment.corners[(c + 1) % count], inside)
            );

        segments.push_back(segment);
    }

    return segments;
}

// ═══════════════════════════════════════════════════════════════════════════
// Generatrices and meridians
// ═══════════════════════════════════════════════════════════════════════════
Point CylinderVault::on_circle(const Segment& segment, double radius, double angle) {

    return segment.centre + segment.x * (radius * std::sin(angle)) +
           segment.y * (radius * std::cos(angle));
}

std::array<std::pair<double, size_t>, 2>
CylinderVault::clip(const Segment& segment, double radius, double angle) const {

    // the generatrix through the circle point clipped by every plane of the footprint: the latest entry and the earliest exit
    const Point point = on_circle(segment, radius, angle);
    std::pair<double, size_t> entry = {-std::numeric_limits<double>::infinity(), 0};
    std::pair<double, size_t> exit = {std::numeric_limits<double>::infinity(), 0};

    for (size_t p = 0; p < segment.footprint.size(); p++) {
        const Plane& plane = segment.footprint[p];
        const double speed = plane.z_axis().dot(segment.axis);
        const double distance = plane.z_axis().dot(plane.origin() - point);

        if (std::abs(speed) < 1e-12)
            continue;

        const double s = distance / speed;

        if (speed > 0.0 && s > entry.first)
            entry = {s, p};

        if (speed < 0.0 && s < exit.first)
            exit = {s, p};
    }

    return {entry, exit};
}

double CylinderVault::generatrix_length(const Segment& segment, double radius, double angle) const {

    const std::array<std::pair<double, size_t>, 2> ends = clip(segment, radius, angle);

    return ends[1].first - ends[0].first;
}

std::pair<size_t, size_t>
CylinderVault::generatrix_planes(const Segment& segment, double radius, double angle) const {

    const std::array<std::pair<double, size_t>, 2> ends = clip(segment, radius, angle);

    return std::make_pair(ends[0].second, ends[1].second);
}

double
CylinderVault::drawn_in(const Segment& segment, double radius, double end, double other) const {

    double short_angle = end;
    double long_angle = other;

    for (int step = 0; step < 60; step++) {
        const double middle = 0.5 * (short_angle + long_angle);

        if (generatrix_length(segment, radius, middle) < 1.0)
            short_angle = middle;
        else
            long_angle = middle;
    }

    return long_angle;
}

std::vector<double> CylinderVault::compute_spans(const Segment& segment, double radius) const {

    double first = segment.start;
    double last = segment.end;

    if (generatrix_length(segment, radius, first) < 1.0)
        first = drawn_in(segment, radius, first, last);

    if (generatrix_length(segment, radius, last) < 1.0)
        last = drawn_in(segment, radius, last, first);

    std::vector<double> spans = {first};
    const size_t fine = 1024;

    for (size_t k = 0; k < fine; k++) {
        double low = first + (last - first) * static_cast<double>(k) / static_cast<double>(fine);
        double high =
            first + (last - first) * static_cast<double>(k + 1) / static_cast<double>(fine);
        const std::pair<size_t, size_t> at_low = generatrix_planes(segment, radius, low);

        if (at_low == generatrix_planes(segment, radius, high))
            continue;

        for (int step = 0; step < 50; step++) {
            const double middle = 0.5 * (low + high);

            if (generatrix_planes(segment, radius, middle) == at_low)
                low = middle;
            else
                high = middle;
        }

        if (0.5 * (low + high) - spans.back() > 1e-6)
            spans.push_back(0.5 * (low + high));
    }

    if (last - spans.back() > 1e-6)
        spans.push_back(last);

    return spans;
}

NurbsCurve CylinderVault::compute_meridian(
    const Segment& segment, const std::vector<double>& spans, double radius, size_t end
) const {

    std::vector<Point> points;
    std::vector<double> weights;
    std::vector<double> knots = {0.0};
    std::vector<int> mults = {3};

    for (size_t s = 0; s + 1 < spans.size(); s++) {
        const double from = spans[s];
        const double to = spans[s + 1];
        const double middle = 0.5 * (from + to);
        const size_t plane = clip(segment, radius, middle)[end].second;
        const Xform onto = Xform::project_to_plane_by_axis(segment.footprint[plane], segment.axis);
        const Point a = on_circle(segment, radius, from);
        const Point b = on_circle(segment, radius, middle);
        const Point c = on_circle(segment, radius, to);
        const NurbsCurve arc = Primitives::arc(a, b, c).transformed(onto);

        for (int cv = s == 0 ? 0 : 1; cv < 3; cv++) {
            points.push_back(arc.get_cv(cv));
            weights.push_back(arc.weight(cv));
        }

        knots.push_back(static_cast<double>(s + 1) / static_cast<double>(spans.size() - 1));
        mults.push_back(s + 2 < spans.size() ? 2 : 3);
    }

    return NurbsCurve::create_from_parameters(points, weights, knots, mults, 2);
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════
void CylinderVault::add_plan(const std::vector<Segment>& segments) {

    const std::shared_ptr<TreeNode> plan = add_group("plan");

    for (size_t i = 0; i < segments.size(); i++) {
        std::vector<Point> loop = segments[i].corners;
        loop.push_back(loop.front());
        Polyline footprint(loop);
        footprint.name = fmt::format("footprint_{}", i);
        add_polyline(footprint, plan);
    }
}

std::vector<Development> CylinderVault::add_panels(const std::vector<Segment>& segments) {

    const std::shared_ptr<TreeNode> group = add_group("lamellas");
    std::vector<Development> developments;

    for (size_t i = 0; i < segments.size(); i++) {
        const Segment& segment = segments[i];
        const double inner = segment.radius - guide.thickness;
        const std::vector<double> outer_spans = compute_spans(segment, segment.radius);
        const std::vector<double> inner_spans = compute_spans(segment, inner);
        const NurbsCurve outer_a = compute_meridian(segment, outer_spans, segment.radius, 0);
        const NurbsCurve outer_b = compute_meridian(segment, outer_spans, segment.radius, 1);
        const NurbsCurve inner_a = compute_meridian(segment, inner_spans, inner, 0);
        const NurbsCurve inner_b = compute_meridian(segment, inner_spans, inner, 1);
        const std::shared_ptr<FaceCurvedElement> panel = std::make_shared<FaceCurvedElement>(
            outer_a, outer_b, inner_a, inner_b, fmt::format("lamella_{}", i)
        );
        add(panel, group);

        developments.emplace_back(outer_a, outer_b, 96);
        _distortion = std::max(_distortion, developments.back().distortion);
        _twist = std::max(_twist, developments.back().twist);
    }

    return developments;
}

void CylinderVault::add_walls(const std::vector<Segment>& segments) {

    const std::shared_ptr<TreeNode> group = add_group("walls");

    const double circumradius = corner(0).distance(Point(0.0, 0.0, 0.0));
    const double inset = 1.0 - guide.thickness /
                                   std::cos(Tolerance::PI / static_cast<double>(guide.sides)) /
                                   circumradius;

    for (size_t i = 0; i < segments.size(); i++) {
        // a flat panel under the side: its outer face the side's plane from the ground to the wall's top, its inner face the thickness inward
        const Segment& segment = segments[i];
        const Point from(segment.from[0] * inset, segment.from[1] * inset, 0.0);
        const Point to(segment.to[0] * inset, segment.to[1] * inset, 0.0);
        const Vector top(0.0, 0.0, guide.wall);
        const NurbsCurve outer_a = NurbsCurve::create(false, 1, {segment.from, segment.to});
        const NurbsCurve outer_b =
            NurbsCurve::create(false, 1, {segment.from + top, segment.to + top});
        const NurbsCurve inner_a = NurbsCurve::create(false, 1, {from, to});
        const NurbsCurve inner_b = NurbsCurve::create(false, 1, {from + top, to + top});
        const std::shared_ptr<FaceCurvedElement> panel = std::make_shared<FaceCurvedElement>(
            outer_a, outer_b, inner_a, inner_b, fmt::format("wall_{}", i)
        );
        add(panel, group);
    }
}

void CylinderVault::add_flat(const std::vector<Development>& developments, double beside) {

    const std::shared_ptr<TreeNode> flat = add_group("flat");

    double largest = -std::numeric_limits<double>::infinity();
    double tallest = 0.0;

    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (const std::shared_ptr<FaceCurvedElement> panel =
                std::dynamic_pointer_cast<FaceCurvedElement>(element))
            for (const NurbsCurve* curve : {&panel->outer_a, &panel->outer_b})
                for (size_t k = 0; k <= 16; k++) {
                    const double domain = curve->domain_end() - curve->domain_start();
                    const double t = curve->domain_start() + domain * static_cast<double>(k) / 16.0;
                    largest = std::max(largest, curve->point_at(t)[0]);
                }

    for (const Development& development : developments)
        for (size_t k = 0; k < development.outline.point_count(); k++)
            tallest = std::max(tallest, development.outline.get_point(k)[1]);

    for (size_t i = 0; i < developments.size(); i++) {
        const Xform place =
            Xform::translation(largest + beside, static_cast<double>(i) * (tallest + beside), 0.0);
        Polyline outline = developments[i].outline.transformed(place);
        outline.name = fmt::format("flat_{}", i);
        add_polyline(outline, flat);
    }
}

}
