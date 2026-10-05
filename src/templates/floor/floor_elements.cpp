#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

/// The point at z.
static Point at_level(const Point& point, double z) {
    return Point(point[0], point[1], z);
}

/// The closed square from the corner over side along both frame axes, at z.
static Polyline square(const ColumnCorner& corner, double side, double z) {

    const Point& o = corner.corner;
    const Vector x = corner.x_axis * side;
    const Vector y = corner.y_axis * side;

    return Polyline({at_level(o, z), at_level(o + x, z), at_level(o + x + y, z), at_level(o + y, z)}).closed();
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
        const Point& far_low = bottom[2 + i];
        Point high = at_level(low, 0.0);
        Point far_high = at_level(far_low, 0.0);

        if (i == 0) {
            high = top[1];
            far_high = bottom[1];
        } else if (i + 1 == stations) {
            high = top[0];
            far_high = bottom[0];
        }

        sections.push_back(Polyline({low, high, far_high, far_low}).closed());
    }

    const Line axis = Line::from_points(Line::from_points(top[1], bottom[1]).center(), Line::from_points(top[0], bottom[0]).center());

    return std::make_shared<wood_session::BeamVariable>(axis, sections, name);
}

std::shared_ptr<wood_session::BeamVariable> to_beam(const Outline& outline, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name) {

    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const Polyline first = Polyline({top[start[0]], top[start[1]], bottom[start[1]], bottom[start[0]]}).closed();
    const Polyline last = Polyline({top[end[0]], top[end[1]], bottom[end[1]], bottom[end[0]]}).closed();
    const Point a = Point::centroid({top[start[0]], top[start[1]], bottom[start[1]], bottom[start[0]]});
    const Point b = Point::centroid({top[end[0]], top[end[1]], bottom[end[1]], bottom[end[0]]});

    return std::make_shared<wood_session::BeamVariable>(Line::from_points(a, b), std::vector<Polyline>{first, last}, name);
}

std::shared_ptr<wood_session::Plate> to_plate(const Outline& outline, const std::string& name) {
    return std::make_shared<wood_session::Plate>(outline.bottom, outline.top, name);
}

std::shared_ptr<wood_session::Support> to_support(const ColumnCorner& corner) {
    return std::make_shared<wood_session::Support>(corner.support_plane, "support");
}

std::shared_ptr<wood_session::Column> to_column(const ColumnCorner& corner, const FloorSizes& sizes, const wood_session::Support& support) {

    const Point foot = support.column_foot();
    const double side = sizes.column_head;
    const double head = side + sizes.column_head_chamfer;
    const Line axis = Line::from_points(foot, Point(foot[0], foot[1], sizes.bay_height));

    std::shared_ptr<wood_session::Column> column = std::make_shared<wood_session::Column>(axis, square(corner, side, foot[2]), "column");
    column->head = square(corner, head, foot[2]);
    column->head_height = sizes.column_head_depth;

    return column;
}

std::vector<std::shared_ptr<wood_session::Joint>> to_column_cutters(const Quarter& quarter, const wood_session::Column& column) {

    const Xform lift = Xform::translation(0.0, 0.0, quarter.sizes().bay_height);
    std::vector<std::shared_ptr<wood_session::Joint>> cutters;

    for (const Outline& outline : quarter.column_cutters()) {
        const Mesh solid = to_plate(outline, "column_cutter")->element_geometry_mesh().transformed(lift);
        std::shared_ptr<wood_session::Joint> cutter = std::make_shared<wood_session::Joint>(solid, wood_session::SolidOperation::difference);
        cutter->name = "column_cutter";
        cutter->targets = {column.guid()};
        cutters.push_back(cutter);
    }

    return cutters;
}

double outline_thickness(const Outline& outline) {
    return (area_centroid(outline.top) - area_centroid(outline.bottom)).magnitude();
}

}
