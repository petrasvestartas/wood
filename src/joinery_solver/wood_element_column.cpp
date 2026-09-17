#include "wood_pch.h"
#include "wood_element_column.h"

namespace wood_session {

using session_cpp::Element;
using session_cpp::Line;
using session_cpp::Mesh;
using session_cpp::Point;
using session_cpp::Polyline;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Column::Column() : Element("column"), axis(Line::from_points(Point(0, 0, 0), Point(0, 0, 0))) {}

Column::Column(const Mesh& solid, const Line& axis, const Polyline& section, const std::string& name)
    : Element(solid, name), axis(axis), section(section) {}

// ═══════════════════════════════════════════════════════════════════════════
// Serialization
// ═══════════════════════════════════════════════════════════════════════════

std::string Column::element_data_dumps() const {
    nlohmann::ordered_json data{
        {"axis", axis.jsondump()},
        {"section", section.point_count() > 0 ? section.jsondump() : nlohmann::ordered_json(nullptr)},
        {"type", ELEMENT_TYPE},
    };
    return data.dump();
}

std::shared_ptr<Column> Column::from_element(const Element& e) {
    auto column = std::make_shared<Column>();
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

void Column::register_type() {
    Element::register_type(ELEMENT_TYPE, [](const std::string& data) -> std::shared_ptr<Element> {
        return from_element(Element::pb_loads(data));
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// Text
// ═══════════════════════════════════════════════════════════════════════════

std::string Column::str() const {
    std::ostringstream os;
    os << "Column(name=" << name << ", axis_length=" << axis.length() << ", section_pts=" << section.point_count() << ")";
    return os.str();
}

} // namespace wood_session
