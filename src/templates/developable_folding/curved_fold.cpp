#include "src/templates/developable_folding/curved_fold.h"

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CurvedFold::CurvedFold(const CurvedFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // rings: every ring's star of ridge and valley points on the outer surface
    const std::vector<std::vector<Point>> stars = compute_rings();

    // meridians: every ridge and valley a NURBS curve through the rings
    const std::vector<NurbsCurve> meridians = add_meridians(stars);

    // strips: every fold strip the ruled surface between two meridians, without thickness
    const AABB box = add_strips(meridians);

    // pattern: every strip developed flat under the roof, the largest error the distortion
    std::vector<Development> pattern;
    for (size_t j = 0; j < meridians.size(); j++) {
        pattern.emplace_back(meridians[j], meridians[(j + 1) % meridians.size()], guide.samples);
        _distortion = std::max(_distortion, pattern.back().distortion);
    }
    Development::add_pattern(*this, pattern, box, 6, 400.0);

    // materialization: the surface alone without a thickness
    if (guide.thickness == 0.0)
        return;

    // inner rings: the same points on the inner faces, where the neighbouring strips offset by the thickness meet
    const std::vector<std::vector<Point>> inner_stars = compute_inner_rings(stars);

    // lamellas: every fold strip one curved plate under its outer surface, its inner face between the inner meridians, capped at both ends
    add_lamellas(meridians, inner_stars);

    // flat: every lamella unrolled into its flat piece for cutting
    add_flat();

    // contacts: every two lamellas that share a face, found by the session's contact detection
    compute_face_contacts(0);

    // assembly: two neighbours on their crease make one fold, the folds put side by side
    std::vector<std::shared_ptr<FaceCurvedElement>> placed =
        get_elements_numbered<FaceCurvedElement>("lamella");
    std::vector<size_t> keys;
    for (size_t i = 0; i < placed.size(); i++)
        keys.push_back(i / 2);
    assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("fold", placed, keys))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Surface
// ═══════════════════════════════════════════════════════════════════════════
std::vector<std::vector<Point>> CurvedFold::compute_rings() const {

    const size_t count = 2 * guide.sectors;
    std::vector<std::vector<Point>> stars;

    for (size_t k = 0; k < guide.rings; k++) {
        const double t = static_cast<double>(k) / static_cast<double>(guide.rings - 1);
        const double ring_radius =
            guide.top_radius + (guide.radius - guide.top_radius) * (1.0 - t) * (1.0 - t);
        const double ring_height = guide.eave + (guide.height - guide.eave) * t;
        const double fade = ring_radius / guide.radius;
        std::vector<Point> star;

        for (size_t j = 0; j < count; j++) {
            const double angle =
                2.0 * Tolerance::PI * static_cast<double>(j) / static_cast<double>(count);
            const bool ridge = j % 2 == 0;
            const double r = ridge ? ring_radius : ring_radius * (1.0 - guide.inset);
            const double z = ring_height + (ridge ? guide.gable * fade * (1.0 - t * t * t) : 0.0);
            star.push_back(Point(r * std::cos(angle), r * std::sin(angle), z));
        }

        stars.push_back(star);
    }

    return stars;
}

std::vector<NurbsCurve> CurvedFold::add_meridians(const std::vector<std::vector<Point>>& stars) {

    const std::shared_ptr<TreeNode> group = add_group("meridians");
    std::vector<NurbsCurve> meridians;

    for (size_t j = 0; j < 2 * guide.sectors; j++) {
        std::vector<Point> points;
        for (const std::vector<Point>& star : stars)
            points.push_back(star[j]);

        meridians.push_back(NurbsCurve::create_interpolated(points));
        add_polyline(Polyline(points), group);
    }

    return meridians;
}

AABB CurvedFold::add_strips(const std::vector<NurbsCurve>& meridians) {

    const std::shared_ptr<TreeNode> group = add_group("strips");
    AABB box = AABB::empty();

    for (size_t j = 0; j < meridians.size(); j++) {
        const NurbsCurve& a = meridians[j];
        const NurbsCurve& b = meridians[(j + 1) % meridians.size()];
        std::vector<Point> side_a;
        std::vector<Point> side_b;
        for (size_t k = 0; k <= guide.samples; k++) {
            const double frac = static_cast<double>(k) / static_cast<double>(guide.samples);
            side_a.push_back(
                a.point_at(a.domain_start() + (a.domain_end() - a.domain_start()) * frac)
            );
            side_b.push_back(
                b.point_at(b.domain_start() + (b.domain_end() - b.domain_start()) * frac)
            );
        }

        const Mesh strip = Development::strip(side_a, side_b, fmt::format("strip_{}", j));
        box.union_with(AABB::from_mesh(strip));
        add_mesh(strip, group);
    }

    return box;
}

// ═══════════════════════════════════════════════════════════════════════════
// Lamellas
// ═══════════════════════════════════════════════════════════════════════════
std::vector<std::vector<Point>>
CurvedFold::compute_inner_rings(const std::vector<std::vector<Point>>& stars) const {

    const size_t count = 2 * guide.sectors;
    std::vector<std::vector<Point>> inner;

    for (size_t k = 0; k < stars.size(); k++) {
        const size_t band = std::min(k, stars.size() - 2);
        const std::vector<Point>& low = stars[band];
        const std::vector<Point>& high = stars[band + 1];
        std::vector<Point> ring;

        for (size_t j = 0; j < count; j++) {
            std::array<Plane, 2> offsets;

            for (size_t side = 0; side < 2; side++) {
                const size_t a = (j + count - 1 + side) % count;
                const size_t b = (a + 1) % count;
                Vector normal = (high[a] - low[a]).cross(low[b] - low[a]);
                normal.normalize_self();
                offsets[side] = Plane::from_point_normal(low[a] + normal * guide.thickness, normal);
            }

            const std::optional<Line> fold = Intersection::plane_plane(offsets[0], offsets[1]);
            const Plane level = Plane::from_point_normal(stars[k][j], Vector(0.0, 0.0, 1.0));
            const std::optional<Point> point =
                fold ? Intersection::line_plane(*fold, level, false) : std::nullopt;
            ring.push_back(point.value_or(stars[k][j]));
        }

        inner.push_back(ring);
    }

    return inner;
}

void CurvedFold::add_lamellas(
    const std::vector<NurbsCurve>& meridians, const std::vector<std::vector<Point>>& inner_stars
) {

    const size_t count = meridians.size();
    const std::shared_ptr<TreeNode> lamellas = add_group("lamellas");
    std::vector<NurbsCurve> inner;

    for (size_t j = 0; j < count; j++) {
        std::vector<Point> points;
        for (const std::vector<Point>& star : inner_stars)
            points.push_back(star[j]);
        inner.push_back(NurbsCurve::create_interpolated(points));
    }

    for (size_t j = 0; j < count; j++) {
        const size_t next = (j + 1) % count;
        NurbsCurve outer_a = meridians[j];
        NurbsCurve outer_b = meridians[next];
        NurbsCurve inner_a = inner[j];
        NurbsCurve inner_b = inner[next];

        const double mid_t = 0.5 * (outer_a.domain_start() + outer_a.domain_end());
        const Vector tangent = outer_a.tangent_at(mid_t);
        const Vector across =
            outer_b.point_at(0.5 * (outer_b.domain_start() + outer_b.domain_end())) -
            outer_a.point_at(mid_t);
        const Vector up = outer_a.point_at(mid_t) -
                          inner_a.point_at(0.5 * (inner_a.domain_start() + inner_a.domain_end()));
        if (across.cross(tangent).dot(up) < 0.0) {
            std::swap(outer_a, outer_b);
            std::swap(inner_a, inner_b);
        }

        add(std::make_shared<FaceCurvedElement>(
                outer_a, outer_b, inner_a, inner_b, fmt::format("lamella_{}", j)
            ),
            lamellas);
    }
}

void CurvedFold::add_flat() {

    std::vector<Development> developments;

    for (size_t j = 0; j < 2 * guide.sectors; j++)
        developments.emplace_back(
            *get_element_by_name<FaceCurvedElement>(fmt::format("lamella_{}", j)), guide.samples, fmt::format("flat_{}", j)
        );

    Development::add_flat(*this, developments, 6, 400.0);
}

}
