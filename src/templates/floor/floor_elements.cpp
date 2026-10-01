#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

/// The closed quad of four points.
Polyline section(const Point& a, const Point& b, const Point& c, const Point& d) {
    return Polyline({a, b, c, d, a});
}

/// The middle of two points.
Point middle(const Point& a, const Point& b) {
    return a + (b - a) * 0.5;
}

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<wood_session::BeamVariable> to_rib(const Outline& outline, const std::string& name) {

    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const size_t stations = top.size() - 3;
    std::vector<Polyline> sections;

    for (size_t i = 0; i < stations; i++) {
        const Point& low = top[2 + i];
        const Vector across = bottom[2 + i] - low;
        Point high(low[0], low[1], 0.0);

        if (i == 0)
            high = top[1];
        else if (i + 1 == stations)
            high = top[0];

        sections.push_back(section(low, high, high + across, low + across));
    }

    const Line axis = Line::from_points(middle(top[1], bottom[1]), middle(top[0], bottom[0]));

    return std::make_shared<wood_session::BeamVariable>(axis, sections, name);
}

std::shared_ptr<wood_session::BeamVariable> to_beam(const Outline& outline, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name) {

    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const Polyline first = section(top[start[0]], top[start[1]], bottom[start[1]], bottom[start[0]]);
    const Polyline last = section(top[end[0]], top[end[1]], bottom[end[1]], bottom[end[0]]);
    const Point a = Point::centroid({top[start[0]], top[start[1]], bottom[start[1]], bottom[start[0]]});
    const Point b = Point::centroid({top[end[0]], top[end[1]], bottom[end[1]], bottom[end[0]]});

    return std::make_shared<wood_session::BeamVariable>(Line::from_points(a, b), std::vector<Polyline>{first, last}, name);
}

std::shared_ptr<wood_session::Plate> to_plate(const Outline& outline, const std::string& name) {
    return std::make_shared<wood_session::Plate>(outline.bottom, outline.top, name);
}

/// The closed square from corner over the sides x and y, at z.
Polyline square(const Point& corner, double x, double y, double z) {
    return section(Point(corner[0], corner[1], z), Point(corner[0] + x, corner[1], z), Point(corner[0] + x, corner[1] + y, z), Point(corner[0], corner[1] + y, z));
}

std::shared_ptr<wood_session::Column> to_column(const FloorGuide& guide, double support_height) {

    const Point corner = guide.quarter_polygon()[0];
    const Point centre = guide.corner_point_column(guide.size_column_head);
    const double side = guide.size_column_head;
    const double head = side + guide.size_column_head_chamfer;
    const Line axis = Line::from_points(Point(centre[0], centre[1], support_height), Point(centre[0], centre[1], guide.bay_height));

    std::shared_ptr<wood_session::Column> column = std::make_shared<wood_session::Column>(axis, square(corner, side, side, support_height), "column");
    column->head = square(corner, head, head, support_height);
    column->head_height = std::abs(guide.column_head_lowest_height);

    return column;
}

std::vector<std::shared_ptr<wood_session::Joint>> to_column_cutters(const FloorGuide& guide, const wood_session::Column& column) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    std::vector<std::shared_ptr<wood_session::Joint>> cutters;

    for (const Outline& outline : guide.column_cutters()) {
        const Mesh solid = to_plate(outline, "column_cutter")->element_geometry_mesh().transformed(lift);
        std::shared_ptr<wood_session::Joint> cutter = std::make_shared<wood_session::Joint>(solid, wood_session::SolidOperation::difference);
        cutter->name = "column_cutter";
        cutter->targets = {column.guid()};
        cutters.push_back(cutter);
    }

    return cutters;
}

}
