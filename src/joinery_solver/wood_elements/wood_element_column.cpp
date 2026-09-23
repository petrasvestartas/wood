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
    for (const session_proto::Plane& cut : proto.cuts())
        column->cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));

    return column;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

const ElementGeometry& Column::element_geometry(bool mesh_or_brep) const {

    std::optional<ElementGeometry>& cache = mesh_or_brep ? _element_geometry_mesh : _element_geometry_brep;
    if (!cache)
        cache = compute_element_geometry(mesh_or_brep);

    return *cache;
}

ElementGeometry Column::compute_element_geometry(bool mesh_or_brep) const {

    if (section.point_count() < 3 || axis.length() <= 0.0)
        return mesh_or_brep ? ElementGeometry(Mesh()) : ElementGeometry(BRep());

    if (mesh_or_brep)
        return Mesh::loft({section}, {section.translated(axis.to_vector())}, true);

    return brep_sections({section, section.translated(axis.to_vector())});
}

const ElementGeometry& Column::model_geometry(bool mesh_or_brep) const {

    std::optional<ElementGeometry>& cache = mesh_or_brep ? _model_geometry_mesh : _model_geometry_brep;
    if (!cache)
        cache = compute_model_geometry(mesh_or_brep);

    return *cache;
}

ElementGeometry Column::compute_model_geometry(bool mesh_or_brep) const {
    return cut_geometry(element_geometry(mesh_or_brep), cuts);
}

void Column::invalidate_geometry() {
    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_mesh.reset();
    _model_geometry_brep.reset();
    _geometry_synced = false;
}

std::shared_ptr<Column> Column::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<Column> column = std::make_shared<Column>(axis.transformed(xform), section.transformed(xform), name);
    column->guid() = guid();
    column->cuts = transformed_list(cuts, xform);
    column->set_features(transformed_features(_features, xform));
    column->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return column;
}

void Column::place(const Xform& xform) {

    Element::place(xform);
    axis.transform(xform);
    section.transform(xform);
    cuts = transformed_list(cuts, xform);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_mesh.reset();
    _model_geometry_brep.reset();
}

void Column::compute_geometry_impl(bool mesh_or_brep) {

    if (section.point_count() >= 3 && axis.length() > 0.0) {
        set_geometry(model_geometry(mesh_or_brep));
        _element_geometry_mesh.reset();
        _element_geometry_brep.reset();
        _model_geometry_mesh.reset();
        _model_geometry_brep.reset();
    }

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", Polyline({axis.start(), axis.end()})));
    if (section.point_count() > 0)
        next.push_back(polyline_feature("section", section));
    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
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
    for (const Plane& cut : cuts)
        proto.add_cuts()->ParseFromString(cut.pb_dumps());

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
