#include "pch.h"
#include "wood_serialization.h"
#include "wood_config.h"
#include "wood_element_edge_raw.h"
#include "wood_element_geometry.h"
#include "element_edge_raw.pb.h"
#include "../src/file_xyz.h"

namespace wood_session {

using namespace laika_cpp;

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

struct StockData {
    Mesh mesh;
    PointCloud cloud;
    std::vector<Polyline> axes;
};

/// Loads `<stock_name>.obj`, optional `_cloud.pb` / `.xyz` and optional `_axes.txt` from `data/raw/`.
StockData load_stock_data(const std::string& stock_name) {

    const std::filesystem::path raw_dir = config::session_data_dir() / "raw";
    const std::filesystem::path mesh_path = raw_dir / (stock_name + ".obj");
    if (!std::filesystem::exists(mesh_path))
        throw std::runtime_error("EdgeRawElement: missing " + mesh_path.string());

    StockData stock;
    stock.mesh = file_obj::read_file_obj(mesh_path.string());

    const std::filesystem::path pb_cloud = raw_dir / (stock_name + "_cloud.pb");
    const std::filesystem::path xyz_cloud = raw_dir / (stock_name + ".xyz");
    if (std::filesystem::exists(pb_cloud))
        stock.cloud = PointCloud::pb_load(pb_cloud.string());
    else if (std::filesystem::exists(xyz_cloud))
        stock.cloud = file_xyz::read_xyz(xyz_cloud.string());
    if (!stock.cloud.is_empty() && stock.cloud.point_size <= 1.0)
        stock.cloud.point_size = 3.5;

    stock.axes = read_axes_file(raw_dir / (stock_name + "_axes.txt"));

    return stock;
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

std::shared_ptr<EdgeRawElement> EdgeRawElement::from_registered_stock(
    const std::string& stock_name,
    const Xform& xform,
    double radius,
    const std::optional<Line>& proxy_axis,
    const std::string& name
) {

    const StockData stock = load_stock_data(stock_name);
    Line axis_line = Line::from_points(
        Point(0.0, 0.0, 0.0).transformed(xform), Point(0.0, 0.0, 1000.0).transformed(xform)
    );
    if (proxy_axis) {
        axis_line = *proxy_axis;
    } else if (!stock.axes.empty()) {
        const Point start = stock.axes.front().get_point(0).transformed(xform);
        const std::vector<Point> upper =
            (stock.axes.size() > 1 ? stock.axes[1] : stock.axes[0]).get_points();
        axis_line = Line::from_points(start, upper.back().transformed(xform));
    }

    std::shared_ptr<EdgeRawElement> edge = round(axis_line, radius, 0.2, name);
    if (stock.axes.size() > 1)
        edge->branch_axes.assign(stock.axes.begin() + 1, stock.axes.end());
    edge->attach_raw(stock.mesh, stock.cloud, xform);

    return edge;
}

std::shared_ptr<EdgeRawElement> EdgeRawElement::from_bent_stock(
    const Polyline& curve_axis,
    double radius,
    const std::string& stock_name,
    double roll_degrees,
    const std::string& name
) {

    const std::vector<Point> pts = curve_axis.get_points();
    if (pts.size() < 2)
        throw std::invalid_argument("EdgeRawElement::from_bent_stock requires at least 2 axis points");
    if (pts.size() == 2)
        return from_stock(Line::from_points(pts[0], pts[1]), radius, stock_name, roll_degrees, true, name);

    const StockData stock = load_stock_data(stock_name);
    const size_t n_pts = pts.size();
    const size_t n_seg = n_pts - 1;

    std::vector<double> cum(n_pts, 0.0);
    std::vector<Vector> seg_tan(n_seg);
    for (size_t i = 0; i < n_seg; i++) {
        const Vector d = pts[i + 1] - pts[i];
        cum[i + 1] = cum[i] + d.magnitude();
        seg_tan[i] = d.normalized();
    }
    const double curve_len = cum.back();

    std::vector<Vector> vtx_tan(n_pts);
    vtx_tan[0] = seg_tan[0];
    vtx_tan[n_pts - 1] = seg_tan[n_seg - 1];
    for (size_t i = 1; i + 1 < n_pts; i++)
        vtx_tan[i] = (seg_tan[i - 1] + seg_tan[i]).normalized();

    const Plane frame0 = Plane::from_point_normal(pts[0], vtx_tan[0]);
    const double rad = roll_degrees * Tolerance::PI / 180.0;
    Vector x0 = frame0.x_axis() * std::cos(rad) + frame0.y_axis() * std::sin(rad);
    Vector y0 = vtx_tan[0].cross(x0).normalized();

    std::vector<Vector> vtx_x(n_pts), vtx_y(n_pts);
    vtx_x[0] = x0;
    vtx_y[0] = y0;
    for (size_t i = 1; i < n_pts; i++) {
        Vector xi = vtx_x[i - 1] - vtx_tan[i] * vtx_x[i - 1].dot(vtx_tan[i]);
        if (xi.magnitude() < 1e-9)
            xi = Plane::from_point_normal(pts[i], vtx_tan[i]).x_axis();
        else
            xi = xi.normalized();
        vtx_x[i] = xi;
        vtx_y[i] = vtx_tan[i].cross(xi).normalized();
    }

    const auto [src_verts, src_faces] = stock.mesh.to_vertices_and_faces();
    double z_low = std::numeric_limits<double>::max();
    double z_high = std::numeric_limits<double>::lowest();
    for (const Point& p : src_verts) {
        z_low = std::min(z_low, p[2]);
        z_high = std::max(z_high, p[2]);
    }
    const double z_mid = 0.5 * (z_low + z_high);

    const auto warp_point = [&](const Point& p) -> Point {
        const double s = 0.5 * curve_len + (p[2] - z_mid);
        if (s <= 0.0)
            return pts[0] + vtx_tan[0] * s + vtx_x[0] * p[0] + vtx_y[0] * p[1];
        if (s >= curve_len)
            return pts.back() + vtx_tan.back() * (s - curve_len) + vtx_x.back() * p[0]
                   + vtx_y.back() * p[1];
        const auto it = std::upper_bound(cum.begin(), cum.end(), s);
        const size_t idx = std::min<size_t>(
            n_seg - 1, static_cast<size_t>(std::max<std::ptrdiff_t>(0, (it - cum.begin()) - 1))
        );
        const double seg_len = std::max(1e-9, cum[idx + 1] - cum[idx]);
        const double t = (s - cum[idx]) / seg_len;
        const Point c = pts[idx] + (pts[idx + 1] - pts[idx]) * t;
        const Vector tz = (vtx_tan[idx] * (1.0 - t) + vtx_tan[idx + 1] * t).normalized();
        Vector tx = vtx_x[idx] * (1.0 - t) + vtx_x[idx + 1] * t;
        tx = (tx - tz * tx.dot(tz)).normalized();
        const Vector ty = tz.cross(tx).normalized();
        return c + tx * p[0] + ty * p[1];
    };

    std::vector<Point> bent_verts;
    bent_verts.reserve(src_verts.size());
    for (const Point& p : src_verts)
        bent_verts.push_back(warp_point(p));
    std::vector<std::vector<size_t>> tri_faces;
    tri_faces.reserve(src_faces.size() * 2);
    for (const std::vector<size_t>& face : src_faces) {
        for (size_t k = 1; k + 1 < face.size(); k++)
            tri_faces.push_back({face[0], face[k], face[k + 1]});
    }
    const Mesh bent_mesh = Mesh::from_vertices_and_faces(bent_verts, tri_faces);

    PointCloud bent_cloud;
    if (!stock.cloud.is_empty()) {
        std::vector<Point> cpts = stock.cloud.get_points();
        for (Point& cp : cpts)
            cp = warp_point(cp);
        bent_cloud = PointCloud(cpts, stock.cloud.get_normals(), stock.cloud.get_colors());
        bent_cloud.point_size = stock.cloud.point_size;
    }

    std::shared_ptr<EdgeRawElement> edge = std::make_shared<EdgeRawElement>();
    edge->name = name;
    edge->axis = curve_axis;
    edge->radii.assign(n_seg, radius);
    edge->directions.reserve(n_seg);
    for (size_t i = 0; i < n_seg; i++) {
        Vector dir = vtx_y[i] - seg_tan[i] * vtx_y[i].dot(seg_tan[i]);
        edge->directions.push_back(
            dir.magnitude() > 1e-9 ? dir.normalized()
                                   : Plane::from_point_normal(pts[i], seg_tan[i]).y_axis()
        );
    }
    edge->profile = {circle_ring(radius, 0.2)};
    edge->round_radius = radius;
    edge->attach_raw(bent_mesh, bent_cloud, Xform::identity());

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
        for (const laika_proto::Vector& direction : base.directions())
            raw->directions.push_back(Vector::pb_loads(direction.SerializeAsString()));
        raw->allowed_type = base.allowed_type();
        raw->extension = base.extension();
        raw->round_radius = base.round_radius();
        for (const laika_proto::Plane& cut : base.cuts())
            raw->cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));
        for (const laika_proto::Polyline& ring : base.profile())
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
    for (const laika_proto::Polyline& branch : proto.branch_axes())
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

std::shared_ptr<EdgeRawElement> EdgeRawElement::branch_proxy(
    double branch_radius, const std::optional<Line>& proxy_axis, size_t branch_index
) const {

    Line line;
    if (proxy_axis) {
        line = *proxy_axis;
    } else {
        const std::vector<Polyline> branches = placed_branch_axes();
        if (branches.empty())
            throw std::out_of_range(name + ": has no secondary branch_axes");
        const std::vector<Point> points =
            branches[std::min(branch_index, branches.size() - 1)].get_points();
        line = Line::from_points(points.front(), points.back());
    }

    const double limb_radius = branch_radius > 0.0 ? branch_radius : 0.85 * radius(0);
    std::shared_ptr<EdgeRawElement> proxy = round(line, limb_radius, 0.2, name);
    proxy->guid() = guid();
    proxy->raw_mesh = raw_mesh;
    proxy->raw_xform = raw_xform;
    proxy->use_raw_mesh = false;

    return proxy;
}

std::shared_ptr<EdgeRawElement>
EdgeRawElement::segment_proxy(const Point& near_point, double half_span) const {

    const std::vector<Point> pts = axis.get_points();
    if (pts.size() < 2)
        throw std::out_of_range(name + ": axis has fewer than 2 points");

    double best_d2 = std::numeric_limits<double>::max();
    Point best_pt = pts[0];
    Vector best_tan = (pts[1] - pts[0]).normalized();
    for (size_t i = 0; i + 1 < pts.size(); i++) {
        const Vector seg = pts[i + 1] - pts[i];
        const double len2 = seg.dot(seg);
        if (len2 < 1e-12)
            continue;
        const double t = std::clamp((near_point - pts[i]).dot(seg) / len2, 0.0, 1.0);
        const Point proj = pts[i] + seg * t;
        const Vector diff = near_point - proj;
        const double d2 = diff.dot(diff);
        if (d2 < best_d2) {
            best_d2 = d2;
            best_pt = proj;
            best_tan = seg.normalized();
        }
    }

    const Line local_line =
        Line::from_points(best_pt - best_tan * half_span, best_pt + best_tan * half_span);
    std::shared_ptr<EdgeRawElement> proxy = round(local_line, radius(0), 0.2, name);
    proxy->guid() = guid();
    proxy->raw_mesh = raw_mesh;
    proxy->raw_xform = raw_xform;
    proxy->use_raw_mesh = false;

    return proxy;
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

    const StockData stock = load_stock_data(stock_name);
    branch_axes.clear();
    if (stock.axes.size() > 1)
        branch_axes.assign(stock.axes.begin() + 1, stock.axes.end());

    const double span =
        center_along_axis ? mesh_span_along(stock.mesh, Point(0.0, 0.0, 0.0), Vector::z_axis())
                          : 0.0;
    const Xform placement = align_stock(Plane::xy_plane(), span, roll_degrees);
    attach_raw(stock.mesh, stock.cloud, placement);
}

double EdgeRawElement::raw_overshoot(double margin) const {

    if (!has_raw_mesh() || axis.point_count() < 2)
        return margin;

    const Line line = chord();
    const double span = line.length();
    const Vector along = line.to_vector().normalized();
    const double nominal = radius(0);
    const double limb_cap = std::max(nominal * 2.5, nominal + 95.0);
    double max_radial = 0.0;
    for (const Point& point : placed_raw_mesh().to_vertices_and_faces().first) {
        const Vector offset = point - line.start();
        const double z = offset.dot(along);
        if (z < -limb_cap || z > span + limb_cap)
            continue;
        const double radial = (offset - along * z).magnitude();
        if (radial <= limb_cap)
            max_radial = std::max(max_radial, radial);
    }

    return std::max(0.0, max_radial - nominal) + margin;
}

double EdgeRawElement::raw_end_extension(double margin) const {

    if (!has_raw_mesh() || axis.point_count() < 2)
        return extension + margin;

    const Line line = chord();
    const double span = line.length();
    const Vector along = line.to_vector().normalized();
    const double nominal = radius(0);
    const double limb_cap = std::max(nominal * 2.5, nominal + 95.0);
    double before = 0.0;
    double after = 0.0;
    for (const Point& point : placed_raw_mesh().to_vertices_and_faces().first) {
        const Vector offset = point - line.start();
        if ((offset - along * offset.dot(along)).magnitude() > limb_cap)
            continue;
        const double z = offset.dot(along);
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

    if (!using_raw_mesh() && solid_features.empty() && cuts.empty())
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
