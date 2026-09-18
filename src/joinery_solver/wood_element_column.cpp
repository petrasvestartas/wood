#include "pch.h"
#include "wood_element_column.h"

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Column::Column() : Element("column"), axis(Line::from_points(Point(0, 0, 0), Point(0, 0, 0))) {}

Column::Column(const Mesh& solid, const Line& axis, const Polyline& section, const std::string& name)
    : Element(solid, name), axis(axis), section(section) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Column> Column::from_element(const Element& e) {

    std::shared_ptr<Column> column = std::make_shared<Column>();
    static_cast<Element&>(*column) = e;
    column->guid() = e.guid();

    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(e.element_data_dumps());
    } catch (const std::exception&) {
        return column;
    }

    if (payload.contains("axis") && !payload["axis"].is_null())
        column->axis = Line::jsonload(payload["axis"]);
    if (payload.contains("section") && !payload["section"].is_null())
        column->section = Polyline::jsonload(payload["section"]);

    return column;
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

AABB Column::aabb(double inflate) const {

    if (const Mesh* solid = std::get_if<Mesh>(&geometry()))
        if (solid->number_of_vertices() > 0)
            return AABB::from_mesh(*solid, inflate);

    std::vector<Point> points = section.get_points();
    points.push_back(axis.start());
    points.push_back(axis.end());

    return AABB::from_points(points, inflate);
}

std::string Column::element_data_dumps() const {
    nlohmann::ordered_json data{
        {"axis", axis.jsondump()},
        {"section", section.point_count() > 0 ? section.jsondump() : nlohmann::ordered_json(nullptr)},
        {"type", std::string(ELEMENT_TYPE)},
    };
    return data.dump();
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

/// The element factory of a serialized column: the protobuf bytes decoded as an Element and promoted to a Column.
static std::shared_ptr<Element> column_from_protobuf(const std::string& data) {
    return Column::from_element(Element::pb_loads(data));
}

void Column::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), column_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string Column::str() const {

    std::ostringstream os;
    os << "Column(name=" << name << ", axis_length=" << axis.length() << ", section_pts=" << section.point_count() << ")";

    return os.str();
}

} // namespace wood_session
