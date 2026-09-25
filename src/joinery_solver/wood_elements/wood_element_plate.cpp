#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_plate.h"
#include "wood_element_geometry.h"
#include "element_plate.pb.h"

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Plate::Plate() : Element("plate") {}

/// A bad outline pair degrades to an empty element, which detection skips, rather than taking a dataset run down.
Plate::Plate(const Polyline& bot, const Polyline& top, const std::string& name) : Element(name) {

    Polyline pp0 = bot;
    Polyline pp1 = top;
    if (pp0.point_count() < 3 || pp1.point_count() < 3) {
        std::cerr << fmt::format("  WARNING: WoodElement built from outlines with {}/{} points (need >= 3 each) - element left empty.\n", pp0.point_count(), pp1.point_count());
        return;
    }
    if (pp1.point_count() < pp0.point_count()) {
        std::cerr << fmt::format("  WARNING: WoodElement top outline has {} points but bottom has {} - element left empty (side faces would index past the end).\n", pp1.point_count(), pp0.point_count());
        return;
    }

    Vector normal = compute_newell(pp0.get_points());
    const Point c0 = pp0.center();
    const Point last_p1 = pp1[pp1.point_count() - 1];
    const double last_z = (last_p1 - c0).dot(normal);
    if (last_z > 0) {
        pp0.reverse();
        pp1.reverse();
        normal = compute_newell(pp0.get_points());
        reversed = true;
    }

    const size_t n_sides = pp0.point_count() > 1 ? pp0.point_count() - 1 : 0;

    polylines.resize(2 + n_sides, Polyline());
    polylines[0] = pp0;
    polylines[1] = pp1;

    const Point cen0 = pp0.center();
    const Point cen1 = pp1.center();
    planes.resize(2 + n_sides);
    planes[0] = Plane::from_point_normal(cen0, normal);
    planes[1] = Plane::from_point_normal(cen1, -normal);
    thickness = Point::distance(cen0, planes[1].project(cen0));

    for (size_t j = 0; j < n_sides; j++) {
        const Vector n = (pp0[j] - pp0[j + 1]).cross(pp1[j + 1] - pp0[j + 1]);
        const double anx = std::abs(n[0]);
        const double any = std::abs(n[1]);
        const double anz = std::abs(n[2]);
        Vector sb1;
        if (anx < 1e-12)
            sb1 = Vector(1, 0, 0);
        else if (any < 1e-12)
            sb1 = Vector(0, 1, 0);
        else if (anz < 1e-12)
            sb1 = Vector(0, 0, 1);
        else if (anx <= any && anx <= anz)
            sb1 = Vector(0, -n[2], n[1]);
        else if (any <= anx && any <= anz)
            sb1 = Vector(-n[2], 0, n[0]);
        else
            sb1 = Vector(-n[1], n[0], 0);

        Vector sb2 = n.cross(sb1);
        sb1.normalize_self();
        sb2.normalize_self();
        planes[2 + j] = Plane(pp0[j + 1], sb1, sb2);
        polylines[2 + j] = Polyline({pp0[j], pp0[j + 1], pp1[j + 1], pp1[j], pp0[j]});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Plate> Plate::from_rectangle(const Point& origin, const Vector& x_axis, const Vector& y_axis, double width, double height, const Vector& thickness, const std::string& name) {
    const Polyline bottom = Polyline::rectangle(origin, x_axis, y_axis, width, height);
    return std::make_shared<Plate>(bottom, bottom.translated(thickness), name);
}

std::shared_ptr<Plate> Plate::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::optional<Polyline> bottom;
    std::optional<Polyline> top;
    bool reversed = false;
    if (!bytes.empty() && bytes.front() == '{') {
        try {
            const nlohmann::json payload = nlohmann::json::parse(bytes);
            if (payload.contains("bottom") && !payload["bottom"].is_null() && payload.contains("top") && !payload["top"].is_null()) {
                bottom = Polyline::jsonload(payload["bottom"]);
                top = Polyline::jsonload(payload["top"]);
            }
            reversed = payload.value("reversed", false);
        } catch (const std::exception&) {
        }
    } else {
        wood_proto::Plate proto;
        if (proto.ParseFromString(bytes) && proto.has_bottom() && proto.has_top()) {
            bottom = Polyline::pb_loads(proto.bottom().SerializeAsString());
            top = Polyline::pb_loads(proto.top().SerializeAsString());
        }
        reversed = proto.reversed();
    }

    std::shared_ptr<Plate> plate = bottom.has_value() ? std::make_shared<Plate>(*bottom, *top) : std::make_shared<Plate>();
    static_cast<Element&>(*plate) = std::move(e);
    plate->reversed = reversed;
    plate->_geometry_synced = true;

    Plate& out = *plate;
    static const std::string prefix = "joint_type_";
    for (const ElementFeature& f : out.Element::features()) {

        if (f.face_index < 0)
            continue;

        const size_t face = static_cast<size_t>(f.face_index);
        if (f.feature_type.compare(0, prefix.size(), prefix) == 0) {
            try {
                const int code = std::stoi(f.feature_type.substr(prefix.size()));
                if (out.feature_types.size() <= face)
                    out.feature_types.resize(face + 1, -1);
                out.feature_types[face] = code;
            } catch (const std::exception&) {
            }
        } else if (f.feature_type != "cut") {
            continue;
        }

        if (face == 0)
            out.features.bottom.insert(out.features.bottom.end(), f.outlines.begin(), f.outlines.end());
        if (face == 1)
            out.features.top.insert(out.features.top.end(), f.outlines.begin(), f.outlines.end());
    }

    return plate;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

const Mesh& Plate::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        _element_geometry_mesh = polylines.size() < 2 ? Mesh() : Mesh::loft({polylines[0]}, {polylines[1]});
    }

    return *_element_geometry_mesh;
}

const BRep& Plate::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        _element_geometry_brep = polylines.size() < 2 ? BRep() : brep_between_loops({polylines[0]}, {polylines[1]});
    }

    return *_element_geometry_brep;
}

const Mesh& Plate::model_geometry_mesh() const {

    if (!_model_geometry_mesh) {
        _model_geometry_mesh = features.top.empty()
            ? element_geometry_mesh() : Mesh::loft(features.bottom, features.top);
    }

    return *_model_geometry_mesh;
}

const BRep& Plate::model_geometry_brep() const {

    if (!_model_geometry_brep) {
        _model_geometry_brep = features.top.empty()
            ? element_geometry_brep() : brep_between_loops(features.bottom, features.top);
    }

    return *_model_geometry_brep;
}

void Plate::flip() {

    if (polylines.size() > 1)
        std::swap(polylines[0], polylines[1]);
    if (planes.size() > 1)
        std::swap(planes[0], planes[1]);
    reset();
    invalidate_geometry();
}

void Plate::invalidate_geometry() {
    _element_geometry_mesh.reset();
    _model_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_brep.reset();
    _geometry_synced = false;
}

std::shared_ptr<Plate> Plate::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<Plate> plate = std::make_shared<Plate>();
    plate->name = name;
    plate->guid() = guid();
    plate->polylines = transformed_list(polylines, xform);
    plate->planes = transformed_list(planes, xform);
    plate->thickness = thickness;
    plate->reversed = reversed;
    plate->features = {transformed_list(features.top, xform), transformed_list(features.bottom, xform)};
    plate->feature_types = feature_types;
    plate->set_features(transformed_features(_features, xform));
    plate->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return plate;
}

void Plate::place(const Xform& xform) {

    Element::place(xform);
    polylines = transformed_list(polylines, xform);
    planes = transformed_list(planes, xform);
    features = {transformed_list(features.top, xform), transformed_list(features.bottom, xform)};

    _element_geometry_mesh.reset();
    _model_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_brep.reset();
}

void Plate::compute_geometry_mesh_impl() {

    if (polylines.size() > 1) {
        set_geometry(model_geometry_mesh());
    }
    compute_geometry_features();
}

void Plate::compute_geometry_brep_impl() {

    if (polylines.size() > 1) {
        set_geometry(model_geometry_brep());
    }
    compute_geometry_features();
}

void Plate::compute_geometry_features() {
    set_dimensions(nominal_dimensions());

    std::vector<ElementFeature> next = face_features();
    for (size_t face = 0; face < std::min<size_t>(2, polylines.size()); face++)
        next.push_back(polyline_feature("outline", polylines[face], static_cast<int>(face)));
    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

Vector Plate::nominal_dimensions() const {

    if (polylines.empty() || planes.empty())
        return Vector(0.0, 0.0, thickness);

    const Plane& frame = planes[0];
    const Point& origin = frame.origin();
    const Vector& ex = frame.x_axis();
    const Vector& ey = frame.y_axis();

    bool first = true;
    double min_u = 0.0;
    double max_u = 0.0;
    double min_v = 0.0;
    double max_v = 0.0;
    for (const Polyline& polyline : polylines) {
        for (size_t k = 0; k < polyline.point_count(); k++) {

            const Vector d = polyline.get_point(k) - origin;
            const double u = d.dot(ex);
            const double v = d.dot(ey);
            if (first) {
                min_u = u;
                max_u = u;
                min_v = v;
                max_v = v;
                first = false;
                continue;
            }

            min_u = std::min(min_u, u);
            max_u = std::max(max_u, u);
            min_v = std::min(min_v, v);
            max_v = std::max(max_v, v);
        }
    }

    return Vector(max_u - min_u, max_v - min_v, thickness);
}

/// -1 is "no joint assigned"; a face with neither a type nor an outline is not a feature.
std::vector<ElementFeature> Plate::face_features() const {

    static const std::vector<Polyline> none;
    std::vector<ElementFeature> out;
    const size_t face_count = std::max(feature_types.size(), size_t{2});
    for (size_t face = 0; face < face_count; face++) {

        const int type = face < feature_types.size() ? feature_types[face] : -1;
        const std::vector<Polyline>& outlines = face == 0 ? features.bottom : (face == 1 ? features.top : none);
        if (type < 0 && outlines.empty())
            continue;

        const std::string feature_type = type >= 0 ? "joint_type_" + std::to_string(type) : "cut";
        out.emplace_back(feature_type, static_cast<int>(face), outlines, "face_" + std::to_string(face));
    }

    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

AABB Plate::aabb(double inflate) const {

    std::vector<Point> points;
    for (const Polyline& outline : polylines) {
        const std::vector<Point> vertices = outline.get_points();
        points.insert(points.end(), vertices.begin(), vertices.end());
    }

    return AABB::from_points(points, inflate);
}

nlohmann::ordered_json Plate::element_data_jsondump() const {

    wood_proto::Plate proto;
    proto.ParseFromString(element_data_dumps());

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Plate::element_data_dumps() const {

    wood_proto::Plate proto;
    if (polylines.size() > 0)
        proto.mutable_bottom()->ParseFromString(polylines[0].pb_dumps());
    if (polylines.size() > 1)
        proto.mutable_top()->ParseFromString(polylines[1].pb_dumps());
    proto.set_reversed(reversed);

    return proto.SerializeAsString();
}

/// The element factory of a serialized plate: the protobuf bytes decoded as an Element and promoted to a Plate.
static std::shared_ptr<Element> plate_from_protobuf(const std::string& data) {
    return Plate::from_element(Element::pb_loads(data));
}

void Plate::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), plate_from_protobuf);
    Element::register_type(std::string(LEGACY_ELEMENT_TYPE), plate_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string Plate::str() const {

    std::ostringstream os;
    os << "Plate(name=" << name << ", polylines=" << polylines.size() << ", thickness=" << thickness << ")";

    return os.str();
}

std::string Plate::repr() const {

    std::ostringstream os;
    os << "Plate(name=" << name << ", polylines=" << polylines.size() << ", planes=" << planes.size()
       << ", reversed=" << (reversed ? "true" : "false") << ", thickness=" << thickness
       << ", features=" << face_features().size() << ")";

    return os.str();
}

}  // namespace wood_session
