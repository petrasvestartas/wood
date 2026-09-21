#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_column.h"
#include "wood_element_geometry.h"
#include "element_column.pb.h"

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Column::Column() : Element("column"), axis(Line::from_points(Point(0, 0, 0), Point(0, 0, 0))) {}

Column::Column(const Line& axis, const Polyline& section, const std::string& name)
    : Element(name), axis(axis), section(section) {}

Column::Column(const Mesh& solid, const Line& axis, const Polyline& section, const std::string& name)
    : Element(solid, name), axis(axis), section(section) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Column> Column::from_element(const Element& e) {

    std::shared_ptr<Column> column = std::make_shared<Column>();
    static_cast<Element&>(*column) = e;
    column->guid() = e.guid();

    const std::string bytes = e.element_data_dumps();
    if (!bytes.empty() && bytes.front() == '{') {
        try {
            const nlohmann::json payload = nlohmann::json::parse(bytes);
            if (payload.contains("axis") && !payload["axis"].is_null())
                column->axis = Line::jsonload(payload["axis"]);
            if (payload.contains("section") && !payload["section"].is_null())
                column->section = Polyline::jsonload(payload["section"]);
        } catch (const std::exception&) {
        }
        return column;
    }

    wood_proto::Column proto;
    if (!proto.ParseFromString(bytes))
        return column;

    if (proto.has_axis())
        column->axis = Line::pb_loads(proto.axis().SerializeAsString());
    if (proto.has_section())
        column->section = Polyline::pb_loads(proto.section().SerializeAsString());

    return column;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

void Column::invalidate_geometry() {
    _geometry_synced = false;
}

void Column::compute_geometry() {

    if (section.point_count() >= 3 && axis.length() > 0.0)
        set_geometry(Mesh::loft({section}, {section.translated(axis.to_vector())}, true));

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", Polyline({axis.start(), axis.end()})));
    if (section.point_count() > 0)
        next.push_back(polyline_feature("section", section));
    if (has_geometry())
        next.push_back(centroid_feature(*this));
    for (ElementFeature& joint : joint_features(*this))
        next.push_back(std::move(joint));

    set_features(std::move(next));
    _geometry_synced = true;
}

AABB Column::aabb(double inflate) const {

    if (const Mesh* solid = std::get_if<Mesh>(&geometry()))
        if (solid->number_of_vertices() > 0)
            return AABB::from_mesh(*solid, inflate);

    std::vector<Point> points = section.get_points();
    points.push_back(axis.start());
    points.push_back(axis.end());

    return AABB::from_points(points, inflate);
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json Column::element_data_jsondump() const {

    wood_proto::Column proto;
    proto.ParseFromString(element_data_dumps());

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Column::element_data_dumps() const {

    wood_proto::Column proto;
    proto.mutable_axis()->ParseFromString(axis.pb_dumps());
    if (section.point_count() > 0)
        proto.mutable_section()->ParseFromString(section.pb_dumps());

    return proto.SerializeAsString();
}

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
