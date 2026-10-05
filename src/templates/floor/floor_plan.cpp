#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

// ═══════════════════════════════════════════════════════════════════════════
// Parameters
// ═══════════════════════════════════════════════════════════════════════════

double FloorParameters::static_h() const {
    return height - rise;
}

// ═══════════════════════════════════════════════════════════════════════════
// Guide
// ═══════════════════════════════════════════════════════════════════════════

FloorGuide FloorGuide::rectangle(double half_x, double half_y, const FloorParameters& parameters) {
    return FloorGuide({Point(-half_x, -half_y, 0.0), Point(half_x, -half_y, 0.0), Point(half_x, half_y, 0.0), Point(-half_x, half_y, 0.0)}, parameters);
}

Point FloorGuide::midpoint(size_t k) const {
    return Line::from_points(corners[k % 4], corners[(k + 1) % 4]).center();
}

double FloorGuide::corner_angle(size_t k) const {
    return (corners[(k + 1) % 4] - corners[k % 4]).angle(corners[(k + 3) % 4] - corners[k % 4], false);
}

double FloorGuide::oculus_corner_angle(size_t k) const {
    return (oculus_corners[(k + 3) % 4] - oculus_corners[k % 4]).angle(oculus_corners[(k + 1) % 4] - oculus_corners[k % 4], false);
}

double FloorGuide::oculus_seam_angle(size_t k) const {
    return (midpoint(k) - centre).angle(oculus_corners[(k + 1) % 4] - oculus_corners[k % 4], false);
}

/// One quarter family as the guide draws it: its plan quads and face planes in member order, its colour, and the index of its first rib parabola, -1 for none.
struct DrawnFamily {
    Family family;
    const std::vector<Polyline>& quads;
    const std::vector<std::array<Plane, 2>>& planes;
    Color color;
    int parabola;
};

void FloorGuide::draw() {

    for (size_t q = 0; q < 4; q++) {
        const std::string suffix = fmt::format("_{}", q);
        const QuarterGeometry& drawn = geometry[q];
        const std::shared_ptr<TreeNode> quarter = wood_floor::add_group(*this, "quarter" + suffix, nullptr);
        const std::shared_ptr<TreeNode> plan = wood_floor::add_group(*this, "plan" + suffix, quarter);

        const auto line = [this](Polyline polyline, const std::string& name, const Color& color, double width, const std::shared_ptr<TreeNode>& group) {
            polyline.name = name;
            polyline.linecolor = color;
            polyline.width = width;
            add_polyline(polyline, group);
        };

        line(Polyline(drawn.polygon).closed(), "polygon" + suffix, Color::black(), 3.0, plan);
        line(Polyline(columns[q].head).closed(), "column_head" + suffix, Color::black(), 3.0, plan);
        Point corner = oculus_corners[q];
        corner.name = "oculus_corner" + suffix;
        corner.width = 10.0;
        add_point(corner, plan);

        const std::array<DrawnFamily, 5> families = {{
            {Family::outer_ribs, drawn.quads.outer_ribs, drawn.planes.outer_ribs, Color(0.85f, 0.33f, 0.10f, 1.0f, "outer_ribs"), 0},
            {Family::inner_ribs, drawn.quads.inner_ribs, drawn.planes.inner_ribs, Color(0.93f, 0.69f, 0.13f, 1.0f, "inner_ribs"), 2},
            {Family::inner_beams, drawn.quads.inner_beams, drawn.planes.inner_beams, Color(0.13f, 0.55f, 0.45f, 1.0f, "inner_beams"), -1},
            {Family::wedges, drawn.quads.wedges, drawn.planes.wedges, Color(0.49f, 0.18f, 0.56f, 1.0f, "wedges"), -1},
            {Family::tsections, drawn.quads.tsections, drawn.planes.tsections, Color(0.47f, 0.67f, 0.19f, 1.0f, "tsections"), -1},
        }};

        for (const DrawnFamily& family : families) {
            const std::string& name = FAMILY_NAMES[static_cast<size_t>(family.family)];
            const std::shared_ptr<TreeNode> group = wood_floor::add_group(*this, name + suffix, quarter);

            for (size_t i = 0; i < family.quads.size(); i++) {
                const std::shared_ptr<TreeNode> member = wood_floor::add_group(*this, fmt::format("{}_{}{}", name, i, suffix), group);
                line(family.quads[i].closed(), "quad", family.color, 2.0, member);

                for (size_t side = 0; side < 2; side++) {
                    Plane face = family.planes[i][side];
                    face.name = fmt::format("face_{}", side);
                    face.linecolor = family.color;
                    add_plane(face, member);
                }

                if (family.parabola < 0)
                    continue;

                const std::array<Polyline, 3>& parabola = drawn.parabolas[static_cast<size_t>(family.parabola) + i];
                line(parabola[0], "soffit", family.color, 2.0, member);
                line(parabola[1], "tsections_top", family.color, 1.0, member);
                line(parabola[2], "beds_top", family.color, 1.0, member);
            }
        }
    }
}

namespace geometry {

std::string invalid(const FloorGuide& guide) {

    const std::array<Point, 4>& corners = guide.corners;

    for (size_t k = 0; k < 4; k++) {
        const Vector after = corners[(k + 1) % 4] - corners[k];
        const Vector before = corners[(k + 3) % 4] - corners[k];
        const double turn = after.cross(corners[(k + 2) % 4] - corners[(k + 1) % 4])[2];

        if (std::abs(corners[k][2]) > 0.0)
            return fmt::format("corner {} is not at z 0", k);

        if (turn <= 0.0)
            return fmt::format("the corners are not counter-clockwise and convex at corner {}", (k + 1) % 4);

        if (after.magnitude() <= 0.0 || before.magnitude() <= 0.0)
            return fmt::format("corner {} repeats its neighbour", k);

        const double along = (guide.oculus_corners[k] - guide.centre).dot((guide.midpoint(k) - guide.centre).normalized());

        if (along <= 0.0 || along >= (guide.midpoint(k) - guide.centre).magnitude())
            return fmt::format("oculus corner {} is not between the centre and the midpoint of edge {}", k, k);
    }

    for (size_t k = 0; k < 4; k++)
        if (std::sin(guide.oculus_corner_angle(k) * M_PI / 180.0) < std::sin(guide.oculus_seam_angle(k) * M_PI / 180.0))
            return fmt::format("the ring beam leaves quarter {}'s oculus beam face uncovered at oculus corner {}: corner angle {:.3f}, seam angle {:.3f} degrees", (k + 1) % 4, k, guide.oculus_corner_angle(k), guide.oculus_seam_angle(k));

    return "";
}

}

}
