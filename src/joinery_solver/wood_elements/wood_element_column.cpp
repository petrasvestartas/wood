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

Column::Column() : WoodElement("column"), axis(Line::from_points(Point(0, 0, 0), Point(0, 0, 0))) {}

Column::Column(const Line& axis, const Polyline& section, const std::string& name)
    : WoodElement(name), axis(axis), section(section) {}

Column::Column(const Mesh& solid, const Line& axis, const Polyline& section, const std::string& name)
    : WoodElement(solid, name), axis(axis), section(section) {}

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
    : WoodElement(name), axis(axis), profile(profile), rotation(rotation) {

    if (!profile.empty() && axis.length() > 0.0)
        section = placed_profile(axis, profile, rotation)[0];
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

/// The point at (a, b) in the corner frame, moved along the frame normal to the axis base.
static Point corner_point(const Line& axis, const Plane& corner, double a, double b) {

    const Point base = corner.origin() + corner.z_axis() * corner.signed_distance(axis.start());

    return base + corner.x_axis() * a + corner.y_axis() * b;
}

/// The closed rectangle from (a0, b0) to (a1, b1) in the corner frame at the axis base.
static Polyline corner_rectangle(const Line& axis, const Plane& corner, double a0, double b0, double a1, double b1) {
    return Polyline({
        corner_point(axis, corner, a0, b0),
        corner_point(axis, corner, a1, b0),
        corner_point(axis, corner, a1, b1),
        corner_point(axis, corner, a0, b1),
    }).closed();
}

std::shared_ptr<Column> Column::square(const Line& axis, const Plane& corner, double side, const std::string& name) {
    return std::make_shared<Column>(axis, corner_rectangle(axis, corner, 0.0, 0.0, side, side), name);
}

std::shared_ptr<Column> Column::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<Column> column = std::make_shared<Column>();
    static_cast<Element&>(*column) = std::move(e);

    if (!bytes.empty() && bytes.front() == '{') {
        const nlohmann::json payload = nlohmann::json::parse(bytes, nullptr, false);

        if (payload.is_discarded())
            throw std::runtime_error("Invalid column JSON");

        if (payload.contains("axis") && !payload["axis"].is_null())
            column->axis = Line::jsonload(payload["axis"]);

        if (payload.contains("section") && !payload["section"].is_null())
            column->section = Polyline::jsonload(payload["section"]);

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

    for (const wood_proto::InteractionFeatureSolid& cut : proto.solid_features())
        column->solid_features.push_back(InteractionFeatureSolid::pb_loads(cut.SerializeAsString()));

    for (const wood_proto::InteractionFeaturePlane& feature : proto.plane_features())
        column->plane_features.push_back(InteractionFeaturePlane::pb_loads(feature.SerializeAsString()));

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

        if (section.point_count() < 3 || axis.length() <= 0.0)
            _element_geometry_mesh = Mesh();
        else
            _element_geometry_mesh = Mesh::loft(bottom, top_loops(*this, bottom), true);
    }

    return *_element_geometry_mesh;
}

const BRep& Column::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        const std::vector<Polyline> bottom = bottom_loops(*this);

        if (section.point_count() < 3 || axis.length() <= 0.0)
            _element_geometry_brep = BRep();
        else
            _element_geometry_brep = brep_between_loops(bottom, top_loops(*this, bottom));
    }

    return *_element_geometry_brep;
}

Mesh Column::trimmed_mesh() const {
    return cut_mesh(stock_mesh(), cuts);
}

BRep Column::trimmed_brep() const {
    return cut_brep(element_geometry_brep(), cuts);
}

Plane Column::frame() const {

    if (section.point_count() < 3 || axis.length() <= 0.0)
        return WoodElement::frame();

    const Vector z = axis.to_vector().normalized();
    const Vector x = (section.get_point(1) - section.get_point(0)).normalized();

    return Plane::from_frame(section.get_point(0), x, z.cross(x), z);
}

std::vector<Plane> Column::compute_planes() const {
    return face_planes(model_geometry_mesh());
}

std::shared_ptr<Column> Column::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<Column> column = std::make_shared<Column>(axis.transformed(xform), section.transformed(xform), name);
    column->guid() = guid();
    column->cuts = transformed_list(cuts, xform);

    for (const InteractionFeatureSolid& cut : solid_features)
        column->solid_features.push_back(cut.transformed(xform));

    for (const InteractionFeaturePlane& feature : plane_features)
        column->plane_features.push_back(feature.transformed(xform));

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

    for (InteractionFeatureSolid& cut : solid_features)
        cut = cut.transformed(xform);

    for (InteractionFeaturePlane& feature : plane_features)
        feature = feature.transformed(xform);

    if (!profile.empty())
        rotation = compute_rotation(axis, x_world);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_mesh_cache.reset();
    _model_brep_cache.reset();
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

    std::vector<Polyline> ends;
    if (section.point_count() > 0)
        ends = {section, section.translated(axis.to_vector())};

    const std::pair<Polyline, std::vector<Polyline>> trimmed = trim_to_cuts(Polyline({axis.start(), axis.end()}), ends, cuts);

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", trimmed.first));

    if (!trimmed.second.empty() && trimmed.second.front().point_count() > 0)
        next.push_back(polyline_feature("section", trimmed.second.front()));

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
    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Failed to parse Column protobuf data");

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Column::element_data_dumps() const {

    wood_proto::Column proto;
    if (!proto.mutable_axis()->ParseFromString(axis.pb_dumps()))
        throw std::runtime_error("Failed to parse Line protobuf data");
    if (section.point_count() > 0)
        if (!proto.mutable_section()->ParseFromString(section.pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");
    for (const Plane& cut : cuts)
        if (!proto.add_cuts()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Failed to parse Plane protobuf data");
    for (const Polyline& ring : profile)
        if (!proto.add_profile()->ParseFromString(ring.pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");
    proto.set_rotation(rotation);

    for (const InteractionFeatureSolid& cut : solid_features)
        if (!proto.add_solid_features()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid solid cut");

    for (const InteractionFeaturePlane& feature : plane_features)
        if (!proto.add_plane_features()->ParseFromString(feature.pb_dumps()))
            throw std::runtime_error("Invalid plane feature");

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
    os << "Column(name=" << name << ", axis_length=" << axis.length() << ", section_pts=" << section.point_count() << ", is_visible=" << std::boolalpha << is_visible << ", is_locked=" << is_locked << ")";

    return os.str();
}

} // namespace wood_session
