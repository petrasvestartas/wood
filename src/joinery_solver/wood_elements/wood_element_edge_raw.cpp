#include "pch.h"
#include "wood_serialization.h"
#include "wood_config.h"
#include "wood_element_edge_raw.h"
#include "wood_element_geometry.h"
#include "element_edge_raw.pb.h"
#include "../src/file_xyz.h"

namespace wood_session {

using namespace session_cpp;

namespace {

/// Builds a closed ring of `chords` vertices of `radius` in the xy plane.
Polyline circle_ring(double radius, double chord_tolerance) {

    const int chords = circle_segments(radius, chord_tolerance);
    std::vector<Point> ring;
    ring.reserve(chords + 1);
    for (int i = 0; i < chords; i++) {
        const double turn = Tolerance::TWO_PI * i / chords;
        ring.emplace_back(radius * std::cos(turn), radius * std::sin(turn), 0.0);
    }
    ring.push_back(ring.front());

    return Polyline(ring);
}

/// Reads polylines of whitespace-separated x y z triples, one polyline per line.
std::vector<Polyline> read_axes_file(const std::filesystem::path& path) {

    std::vector<Polyline> polylines;
    if (!std::filesystem::exists(path))
        return polylines;

    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::vector<Point> points;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        while (row >> x >> y >> z)
            points.emplace_back(x, y, z);
        if (points.size() >= 2)
            polylines.emplace_back(points);
    }

    return polylines;
}

/// Axial span along `z_axis` of a stock mesh relative to `origin`.
double mesh_span_along(const Mesh& mesh, const Point& origin, const Vector& z_axis) {

    const Vector z = z_axis.normalized();
    double low = std::numeric_limits<double>::max();
    double high = std::numeric_limits<double>::lowest();
    for (const Point& point : mesh.to_vertices_and_faces().first) {
        const double along = (point - origin).dot(z);
        low = std::min(low, along);
        high = std::max(high, along);
    }

    return high > low ? high - low : 0.0;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════
EdgeRawElement::EdgeRawElement() : EdgeStraightElement() {
    name = "edge_raw";
}

EdgeRawElement::EdgeRawElement(
    const Line& axis_line,
    double radius,
    const Mesh& mesh,
    const PointCloud& pointcloud,
    const Xform& xform,
    const std::string& name
)
    : EdgeStraightElement(), raw_mesh(mesh), raw_pointcloud(pointcloud), raw_xform(xform) {

    this->name = name;
    const Plane start_frame = Plane::from_point_normal(axis_line.start(), axis_line.to_vector());
    axis = Polyline({axis_line.start(), axis_line.end()});
    radii = {radius};
    directions = {start_frame.y_axis()};
    profile = {circle_ring(radius, 0.2)};
    round_radius = radius;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════
std::shared_ptr<EdgeRawElement> EdgeRawElement::round(
    const Line& axis_line, double radius, double chord_tolerance, const std::string& name
) {

    const Plane start_frame = Plane::from_point_normal(axis_line.start(), axis_line.to_vector());
    std::shared_ptr<EdgeRawElement> edge = std::make_shared<EdgeRawElement>();
    edge->name = name;
    edge->axis = Polyline({axis_line.start(), axis_line.end()});
    edge->radii = {radius};
    edge->directions = {start_frame.y_axis()};
    edge->profile = {circle_ring(radius, chord_tolerance)};
    edge->round_radius = radius;

    return edge;
}

std::shared_ptr<EdgeRawElement> EdgeRawElement::from_stock(
    const Line& axis_line,
    double radius,
    const std::string& stock_name,
    double roll_degrees,
    bool center_along_axis,
    const std::string& name
) {

    std::shared_ptr<EdgeRawElement> edge = round(axis_line, radius, 0.2, name);
    edge->attach_stock(stock_name, roll_degrees, center_along_axis);

    return edge;
}

std::shared_ptr<EdgeRawElement> EdgeRawElement::from_element(Element element) {

    const std::string bytes = element.element_data_dumps();
    std::shared_ptr<EdgeRawElement> raw = std::make_shared<EdgeRawElement>();
    static_cast<Element&>(*raw) = std::move(element);

    wood_proto::EdgeRawElement proto;
    if (!proto.ParseFromString(bytes))
        return raw;

    if (proto.has_base()) {
        const wood_proto::EdgeStraightElement& base = proto.base();
        if (base.has_axis())
            raw->axis = Polyline::pb_loads(base.axis().SerializeAsString());
        raw->radii.assign(base.radii().begin(), base.radii().end());
        for (const session_proto::Vector& direction : base.directions())
            raw->directions.push_back(Vector::pb_loads(direction.SerializeAsString()));
        raw->allowed_type = base.allowed_type();
        raw->extension = base.extension();
        raw->round_radius = base.round_radius();
        for (const session_proto::Plane& cut : base.cuts())
            raw->cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));
        for (const session_proto::Polyline& ring : base.profile())
            raw->profile.push_back(Polyline::pb_loads(ring.SerializeAsString()));
        for (const wood_proto::InteractionFeatureSolid& cut : base.solid_features())
            raw->solid_features.push_back(InteractionFeatureSolid::pb_loads(cut.SerializeAsString()));
        for (const wood_proto::InteractionFeaturePlane& feature : base.plane_features())
            raw->plane_features.push_back(
                InteractionFeaturePlane::pb_loads(feature.SerializeAsString())
            );
    }

    if (proto.has_raw_mesh())
        raw->raw_mesh = Mesh::from_proto(proto.raw_mesh());
    if (proto.has_raw_pointcloud())
        raw->raw_pointcloud = PointCloud::from_proto(proto.raw_pointcloud());
    if (proto.has_raw_xform())
        raw->raw_xform = Xform::from_proto(proto.raw_xform());
    for (const session_proto::Polyline& branch : proto.branch_axes())
        raw->branch_axes.push_back(Polyline::from_proto(branch));
    raw->use_raw_mesh = proto.use_raw_mesh();
    raw->show_pointcloud = proto.show_pointcloud();

    return raw;
}

// ═══════════════════════════════════════════════════════════════════════════
// Raw stock attachment
// ═══════════════════════════════════════════════════════════════════════════
bool EdgeRawElement::has_raw_mesh() const {
    return raw_mesh.number_of_faces() > 0;
}

bool EdgeRawElement::has_raw_pointcloud() const {
    return !raw_pointcloud.is_empty();
}

bool EdgeRawElement::using_raw_mesh() const {
    return use_raw_mesh && has_raw_mesh();
}

Mesh EdgeRawElement::placed_raw_mesh() const {
    return raw_xform.is_identity() ? raw_mesh : raw_mesh.transformed(raw_xform);
}

PointCloud EdgeRawElement::placed_raw_pointcloud() const {
    return raw_xform.is_identity() ? raw_pointcloud : raw_pointcloud.transformed(raw_xform);
}

std::vector<Polyline> EdgeRawElement::placed_branch_axes() const {
    return raw_xform.is_identity() ? branch_axes : transformed_list(branch_axes, raw_xform);
}

Xform EdgeRawElement::align_stock(
    const Plane& stock_plane, double stock_length, double roll_degrees
) const {

    const std::optional<Plane> target = base_plane();
    if (!target)
        return Xform::identity();

    const Xform to_stock = Xform::frame_to_world(
        stock_plane.origin(), stock_plane.x_axis(), stock_plane.y_axis(), stock_plane.z_axis()
    );
    const std::optional<Xform> from_stock = to_stock.inverse();
    if (!from_stock)
        return Xform::identity();

    const double shift_z = stock_length > 0.0 ? 0.5 * (chord().length() - stock_length) : 0.0;
    const Point shifted_origin = target->origin() + target->z_axis() * shift_z;
    const Xform roll = std::abs(roll_degrees) > 0.0
                           ? Xform::rotation(Vector::z_axis(), roll_degrees, true)
                           : Xform::identity();
    const Xform to_world = Xform::frame_to_world(
        shifted_origin, target->x_axis(), target->y_axis(), target->z_axis()
    );

    return to_world * roll * (*from_stock);
}

void EdgeRawElement::attach_raw(const Mesh& mesh, const PointCloud& pointcloud, const Xform& xform) {

    raw_mesh = mesh;
    raw_pointcloud = pointcloud;
    raw_xform = xform;
    use_raw_mesh = mesh.number_of_faces() > 0;
    invalidate_geometry();
}

void EdgeRawElement::attach_stock(
    const std::string& stock_name, double roll_degrees, bool center_along_axis
) {

    const std::filesystem::path raw_dir = config::session_data_dir() / "raw";
    const std::filesystem::path mesh_path = raw_dir / (stock_name + ".obj");
    if (!std::filesystem::exists(mesh_path))
        throw std::runtime_error("EdgeRawElement::attach_stock: missing " + mesh_path.string());

    const Mesh loaded_mesh = file_obj::read_file_obj(mesh_path.string());

    PointCloud loaded_cloud;
    const std::filesystem::path pb_cloud = raw_dir / (stock_name + "_cloud.pb");
    const std::filesystem::path xyz_cloud = raw_dir / (stock_name + ".xyz");
    if (std::filesystem::exists(pb_cloud))
        loaded_cloud = PointCloud::pb_load(pb_cloud.string());
    else if (std::filesystem::exists(xyz_cloud))
        loaded_cloud = file_xyz::read_xyz(xyz_cloud.string());
    if (!loaded_cloud.is_empty() && loaded_cloud.point_size <= 1.0)
        loaded_cloud.point_size = 3.5;

    const std::vector<Polyline> axes_in_stock =
        read_axes_file(raw_dir / (stock_name + "_axes.txt"));
    branch_axes.clear();
    if (axes_in_stock.size() > 1)
        branch_axes.assign(axes_in_stock.begin() + 1, axes_in_stock.end());

    const double span =
        center_along_axis ? mesh_span_along(loaded_mesh, Point(0.0, 0.0, 0.0), Vector::z_axis())
                          : 0.0;
    const Xform placement = align_stock(Plane::xy_plane(), span, roll_degrees);
    attach_raw(loaded_mesh, loaded_cloud, placement);
}

double EdgeRawElement::raw_overshoot(double margin) const {

    if (!has_raw_mesh() || axis.point_count() < 2)
        return margin;

    const Line line = chord();
    const Vector along = line.to_vector().normalized();
    double max_radial = 0.0;
    for (const Point& point : placed_raw_mesh().to_vertices_and_faces().first) {
        const Vector offset = point - line.start();
        const Vector radial = offset - along * offset.dot(along);
        max_radial = std::max(max_radial, radial.magnitude());
    }

    return std::max(0.0, max_radial - radius(0)) + margin;
}

double EdgeRawElement::raw_end_extension(double margin) const {

    if (!has_raw_mesh() || axis.point_count() < 2)
        return extension + margin;

    const Line line = chord();
    const double span = line.length();
    const Vector along = line.to_vector().normalized();
    double before = 0.0;
    double after = 0.0;
    for (const Point& point : placed_raw_mesh().to_vertices_and_faces().first) {
        const double z = (point - line.start()).dot(along);
        if (z < 0.0)
            before = std::max(before, -z);
        if (z > span)
            after = std::max(after, z - span);
    }

    return std::max(extension, std::max(before, after)) + margin;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& EdgeRawElement::element_geometry_mesh() const {

    if (!using_raw_mesh())
        return EdgeStraightElement::element_geometry_mesh();

    if (!_element_geometry_mesh)
        _element_geometry_mesh =
            cuts.empty() ? placed_raw_mesh() : cut_mesh(placed_raw_mesh(), cuts);

    return *_element_geometry_mesh;
}

const BRep& EdgeRawElement::element_geometry_brep() const {

    if (!using_raw_mesh())
        return EdgeStraightElement::element_geometry_brep();

    if (!_element_geometry_brep)
        _element_geometry_brep = mesh_brep(element_geometry_mesh());

    return *_element_geometry_brep;
}

const BRep& EdgeRawElement::model_geometry_brep() const {

    if (!using_raw_mesh())
        return EdgeStraightElement::model_geometry_brep();

    return WoodElement::model_geometry_brep();
}

std::shared_ptr<EdgeRawElement> EdgeRawElement::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<EdgeRawElement> copy = std::make_shared<EdgeRawElement>(*this);
    copy->guid() = guid();
    copy->place(xform);

    return copy;
}

void EdgeRawElement::place(const Xform& xform) {

    EdgeStraightElement::place(xform);
    raw_xform = xform * raw_xform;
}

void EdgeRawElement::compute_geometry_mesh_impl() {

    if (using_raw_mesh() || !sections().empty())
        set_geometry(model_geometry_mesh());

    compute_geometry_features();
}

void EdgeRawElement::compute_geometry_brep_impl() {

    if (using_raw_mesh() || !sections().empty())
        set_geometry(model_geometry_brep());

    compute_geometry_features();
}

void EdgeRawElement::compute_geometry_features() {

    const std::pair<Polyline, std::vector<Polyline>> trimmed =
        trim_to_cuts(axis, sections(), cuts);

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", trimmed.first));

    for (const Polyline& branch : placed_branch_axes())
        if (branch.point_count() > 1)
            next.push_back(polyline_feature("axis", branch));

    if (!using_raw_mesh())
        for (const Polyline& ring : trimmed.second)
            if (ring.point_count() > 0)
                next.push_back(polyline_feature("section", ring));

    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON & Protobuf
// ═══════════════════════════════════════════════════════════════════════════
nlohmann::ordered_json EdgeRawElement::element_data_jsondump() const {

    wood_proto::EdgeRawElement proto;
    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Failed to parse EdgeRawElement protobuf data");

    return json_of(proto);
}

std::string EdgeRawElement::element_data_dumps() const {

    wood_proto::EdgeRawElement proto;
    if (!proto.mutable_base()->ParseFromString(EdgeStraightElement::element_data_dumps()))
        throw std::runtime_error("Failed to serialize EdgeRawElement base data");
    if (has_raw_mesh())
        *proto.mutable_raw_mesh() = raw_mesh.to_proto();
    if (has_raw_pointcloud())
        *proto.mutable_raw_pointcloud() = raw_pointcloud.to_proto();
    *proto.mutable_raw_xform() = raw_xform.to_proto();
    for (const Polyline& branch : branch_axes)
        *proto.add_branch_axes() = branch.to_proto();
    proto.set_use_raw_mesh(use_raw_mesh);
    proto.set_show_pointcloud(show_pointcloud);

    return proto.SerializeAsString();
}

static std::shared_ptr<Element> edge_raw_from_protobuf(const std::string& data) {
    return EdgeRawElement::from_element(Element::pb_loads(data));
}

void EdgeRawElement::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), edge_raw_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════
std::string EdgeRawElement::str() const {

    std::ostringstream os;
    os << "EdgeRawElement(name=" << name << ", segments=" << axis.segment_count()
       << ", raw_faces=" << raw_mesh.number_of_faces()
       << ", raw_points=" << raw_pointcloud.point_count() << ", use_raw_mesh=" << std::boolalpha
       << use_raw_mesh << ", is_visible=" << is_visible << ")";

    return os.str();
}

} // namespace wood_session
