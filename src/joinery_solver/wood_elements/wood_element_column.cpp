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

/// The point at (a, b) in the corner frame, moved along the frame normal to the axis base: one expression for the shaft and the glued blocks, so the faces they share coincide exactly.
static Point corner_point(const Line& axis, const Plane& corner, double a, double b) {
    return corner.origin() + corner.z_axis() * corner.signed_distance(axis.start()) + corner.x_axis() * a + corner.y_axis() * b;
}

/// The closed rectangle from (a0, b0) to (a1, b1) in the corner frame at the axis base.
static Polyline corner_rectangle(const Line& axis, const Plane& corner, double a0, double b0, double a1, double b1) {
    return Polyline({corner_point(axis, corner, a0, b0), corner_point(axis, corner, a1, b0), corner_point(axis, corner, a1, b1), corner_point(axis, corner, a0, b1)}).closed();
}

std::shared_ptr<Column> Column::square(const Line& axis, const Plane& corner, double side, const std::string& name) {
    return std::make_shared<Column>(axis, corner_rectangle(axis, corner, 0.0, 0.0, side, side), name);
}

std::shared_ptr<Column> Column::glued_head(const Line& axis, const Plane& corner, double side, double head_side, double head_height, const std::string& name) {

    std::shared_ptr<Column> column = square(axis, corner, side, name);

    // a block over the top head_height, its top lifted by the same axis vector as the shaft's
    const Vector under = axis.to_vector() * ((axis.length() - head_height) / axis.length());
    const auto block = [&axis, &corner, &under](double a0, double b0, double a1, double b1) {
        const Polyline base = corner_rectangle(axis, corner, a0, b0, a1, b1);
        return Mesh::loft({base.translated(under)}, {base.translated(axis.to_vector())}, true);
    };

    column->solid_features.push_back(SolidCut::unite(block(0.0, side, head_side, head_side)));
    column->solid_features.push_back(SolidCut::unite(block(side, 0.0, head_side, side)));

    return column;
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

    for (const wood_proto::SolidCut& feature : proto.solid_features())
        column->solid_features.push_back(SolidCut::pb_loads(feature.SerializeAsString()));

    for (const wood_proto::SolidCut& cut : proto.solid_cuts())
        column->solid_cuts.push_back(SolidCut::pb_loads(cut.SerializeAsString()));

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

/// The mesh in the frame to_local maps into, every coordinate on a 1e-6 mm grid there: the shaft and a block glued to it then share their planes exactly.
static Mesh snapped(const Mesh& mesh, const Xform& to_local) {

    std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.transformed(to_local).to_vertices_and_faces();

    for (Point& point : data.first)
        point = Point(std::round(point[0] * 1e6) / 1e6, std::round(point[1] * 1e6) / 1e6, std::round(point[2] * 1e6) / 1e6);

    return Mesh::from_vertices_and_faces(data.first, data.second);
}

/// The number of glued blocks among the column's solid features.
static size_t glued_count(const Column& column) {
    return std::count_if(column.solid_features.begin(), column.solid_features.end(), [](const SolidCut& feature) { return feature.operation == SolidOperation::unite; });
}

/// The swept shaft with the glued blocks united, in the column's own frame on a 1e-6 mm grid, where a block and the shaft meet on exactly one plane; the stock the removals cut.
static Mesh glued(const Column& column, const Mesh& shaft) {

    if (glued_count(column) == 0)
        return shaft;

    const Vector z = column.axis.to_vector().normalized();
    const Vector x = (column.section.get_point(1) - column.section.get_point(0)).normalized();
    const Xform to_world = Xform::frame_to_world(column.section.get_point(0), x, z.cross(x), z);
    const std::optional<Xform> to_local = to_world.inverse();

    if (!to_local)
        return shaft;

    std::vector<SolidCut> blocks;

    for (const SolidCut& feature : column.solid_features)
        if (feature.operation == SolidOperation::unite) {
            blocks.push_back(feature);
            blocks.back().mesh = snapped(feature.mesh, *to_local);
        }

    return apply_solid_cuts(snapped(shaft, *to_local), blocks).transformed(to_world);
}

const Mesh& Column::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        const std::vector<Polyline> bottom = bottom_loops(*this);

        if (section.point_count() < 3 || axis.length() <= 0.0)
            _element_geometry_mesh = Mesh();
        else
            _element_geometry_mesh = glued(*this, Mesh::loft(bottom, top_loops(*this, bottom), true));
    }

    return *_element_geometry_mesh;
}

const BRep& Column::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        const std::vector<Polyline> bottom = bottom_loops(*this);

        if (section.point_count() < 3 || axis.length() <= 0.0)
            _element_geometry_brep = BRep();
        else if (glued_count(*this) > 0)
            _element_geometry_brep = mesh_brep(element_geometry_mesh());
        else
            _element_geometry_brep = brep_between_loops(bottom, top_loops(*this, bottom));
    }

    return *_element_geometry_brep;
}


Mesh Column::trimmed_mesh() const {

    std::vector<SolidCut> removals;

    for (const SolidCut& feature : solid_features)
        if (feature.operation != SolidOperation::unite)
            removals.push_back(feature);

    return apply_solid_cuts(cut_mesh(element_geometry_mesh(), cuts), removals);
}

BRep Column::trimmed_brep() const {
    return solid_features.size() == glued_count(*this) ? cut_brep(element_geometry_brep(), cuts) : mesh_brep(trimmed_mesh());
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

    for (const SolidCut& feature : solid_features)
        column->solid_features.push_back(feature.transformed(xform));

    for (const SolidCut& cut : solid_cuts)
        column->solid_cuts.push_back(cut.transformed(xform));

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

    for (SolidCut& feature : solid_features)
        feature = feature.transformed(xform);

    for (SolidCut& cut : solid_cuts)
        cut = cut.transformed(xform);

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

    for (const SolidCut& feature : solid_features)
        if (feature.operation == SolidOperation::difference && feature.mesh.number_of_faces() > 0)
            next.push_back(ElementFeature("cut", -1, feature.mesh.face_outlines(), "cut"));

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

    for (const SolidCut& feature : solid_features)
        if (!proto.add_solid_features()->ParseFromString(feature.pb_dumps()))
            throw std::runtime_error("Invalid solid feature");

    for (const SolidCut& cut : solid_cuts)
        if (!proto.add_solid_cuts()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid solid cut");

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
