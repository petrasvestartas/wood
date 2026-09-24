#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_column.h"
#include "wood_element_geometry.h"
#include "wood_profile.h"
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

/// The profile x axis in world space: world x projected perpendicular to the axis (world y when the axis is along x), turned by rotation degrees about the axis.
static Vector profile_x(const Line& axis, double rotation) {

    const Vector along = axis.to_vector().normalized();
    Vector x = Vector::x_axis() - along * along.dot(Vector::x_axis());
    if (x.magnitude() < Tolerance::RELATIVE)
        x = Vector::y_axis() - along * along.dot(Vector::y_axis());

    return x.normalized().transformed(Xform::rotation(along, rotation, true));
}

/// The rotation that puts the profile x axis on x_world about the axis: its signed angle in degrees from the unturned profile x.
static double compute_rotation(const Line& axis, const Vector& x_world) {

    const Vector along = axis.to_vector().normalized();
    const Vector base = profile_x(axis, 0.0);

    return std::atan2(base.cross(x_world).dot(along), base.dot(x_world)) * Tolerance::TO_DEGREES;
}

/// Every profile loop placed at the axis base, x along profile_x and y along the axis cross it.
static std::vector<Polyline> placed_profile(const Line& axis, const std::vector<Polyline>& profile, double rotation) {

    const Vector along = axis.to_vector().normalized();
    const Vector x = profile_x(axis, rotation);
    const Vector y = along.cross(x);

    std::vector<Polyline> placed;
    for (const Polyline& ring : profile) {
        std::vector<Point> points;
        for (const Point& point : ring.get_points())
            points.push_back(axis.start() + x * point[0] + y * point[1]);
        placed.emplace_back(points);
    }

    return placed;
}

Column::Column(const Line& axis, const std::vector<Polyline>& profile, double rotation, const std::string& name)
    : Element(name), axis(axis), profile(profile), rotation(rotation) {

    if (!profile.empty() && axis.length() > 0.0)
        section = placed_profile(axis, profile, rotation)[0];
}

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
    for (const session_proto::Polyline& ring : proto.profile())
        column->profile.push_back(Polyline::pb_loads(ring.SerializeAsString()));
    column->rotation = proto.rotation();

    return column;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

/// The bottom loops of the solid: the placed profile when it has holes, else the section alone.
static std::vector<Polyline> bottom_loops(const Column& column) {
    return column.profile.size() > 1 ? placed_profile(column.axis, column.profile, column.rotation) : std::vector<Polyline>{column.section};
}

/// The loops moved to the axis top.
static std::vector<Polyline> top_loops(const Column& column, const std::vector<Polyline>& bottom) {

    std::vector<Polyline> top;
    for (const Polyline& ring : bottom)
        top.push_back(ring.translated(column.axis.to_vector()));

    return top;
}

const Mesh& Column::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        const std::vector<Polyline> bottom = bottom_loops(*this);
        _element_geometry_mesh = section.point_count() < 3 || axis.length() <= 0.0
            ? Mesh() : Mesh::loft(bottom, top_loops(*this, bottom), true);
    }

    return *_element_geometry_mesh;
}

const BRep& Column::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        const std::vector<Polyline> bottom = bottom_loops(*this);
        _element_geometry_brep = section.point_count() < 3 || axis.length() <= 0.0
            ? BRep() : brep_between_loops(bottom, top_loops(*this, bottom));
    }

    return *_element_geometry_brep;
}

const Mesh& Column::model_geometry_mesh() const {

    if (!_model_geometry_mesh) {
        _model_geometry_mesh = cut_mesh(element_geometry_mesh(), cuts);
    }

    return *_model_geometry_mesh;
}

const BRep& Column::model_geometry_brep() const {

    if (!_model_geometry_brep) {
        _model_geometry_brep = cut_brep(element_geometry_brep(), cuts);
    }

    return *_model_geometry_brep;
}

std::vector<Plane> Column::compute_planes() const {
    return face_planes(model_geometry_mesh());
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
    column->profile = profile;
    column->rotation = profile.empty() ? rotation : compute_rotation(column->axis, profile_x(axis, rotation).transformed(xform));
    column->set_features(transformed_features(_features, xform));
    column->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return column;
}

void Column::place(const Xform& xform) {

    const Vector x_world = profile_x(axis, rotation).transformed(xform);
    Element::place(xform);
    axis.transform(xform);
    section.transform(xform);
    cuts = transformed_list(cuts, xform);
    if (!profile.empty())
        rotation = compute_rotation(axis, x_world);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_mesh.reset();
    _model_geometry_brep.reset();
}

void Column::compute_geometry_mesh_impl() {

    if (section.point_count() >= 3 && axis.length() > 0.0) {
        set_geometry(model_geometry_mesh());
    }
    compute_geometry_features();
}

void Column::compute_geometry_brep_impl() {

    if (section.point_count() >= 3 && axis.length() > 0.0) {
        set_geometry(model_geometry_brep());
    }
    compute_geometry_features();
}

void Column::compute_geometry_features() {

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", Polyline({axis.start(), axis.end()})));
    if (section.point_count() > 0)
        next.push_back(polyline_feature("section", section));
    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

AABB Column::aabb(double inflate) const {

    const Mesh& solid = geometry_mesh();
    if (solid.number_of_vertices() > 0)
        return AABB::from_mesh(solid, inflate);

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
    for (const Polyline& ring : profile)
        proto.add_profile()->ParseFromString(ring.pb_dumps());
    proto.set_rotation(rotation);

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
