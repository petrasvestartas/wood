#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_face_planar.h"
#include "wood_element_geometry.h"
#include "element_face_planar.pb.h"

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════
FacePlanarElement::FacePlanarElement() : FaceElement("plate") {
}

/// The normal of a bottom outline.
static Vector outline_normal(const Polyline& outline) {

    const Vector newell = compute_newell(outline.get_points()).normalized();
    Vector corners = Vector::average_normal(outline.get_points());
    if (corners.dot(newell) < 0.0)
        corners = corners * -1.0;

    return (corners - newell).magnitude() <= 1e-9 ? newell : corners;
}

/// A bad outline pair degrades to an empty element, which detection skips, rather than taking a dataset run down.
FacePlanarElement::FacePlanarElement(const Polyline& bot, const Polyline& top, const std::string& name)
    : FaceElement(name) {

    Polyline pp0 = bot;
    Polyline pp1 = top;
    if (pp0.point_count() < 3 || pp1.point_count() < 3) {
        std::cerr << fmt::format(
            "  WARNING: WoodElement built from outlines with {}/{} points (need >= 3 each) - "
            "element left empty.\n",
            pp0.point_count(),
            pp1.point_count()
        );
        return;
    }
    if (pp1.point_count() < pp0.point_count()) {
        std::cerr << fmt::format(
            "  WARNING: WoodElement top outline has {} points but bottom has {} - element left "
            "empty (side faces would index past the end).\n",
            pp1.point_count(),
            pp0.point_count()
        );
        return;
    }

    Vector normal = outline_normal(pp0);
    const Point c0 = pp0.center();
    const Point last_p1 = pp1[pp1.point_count() - 1];
    const double last_z = (last_p1 - c0).dot(normal);
    if (last_z > 0) {
        pp0.reverse();
        pp1.reverse();
        normal = outline_normal(pp0);
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
        const Vector sb1 = cgal_base1(n);
        const Vector sb2 = n.cross(sb1).normalized();
        planes[2 + j] = Plane(pp0[j + 1], sb1, sb2);
        polylines[2 + j] = Polyline({pp0[j], pp0[j + 1], pp1[j + 1], pp1[j], pp0[j]});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════
std::shared_ptr<FacePlanarElement> FacePlanarElement::from_rectangle(
    const Point& origin,
    const Vector& x_axis,
    const Vector& y_axis,
    double width,
    double height,
    double thickness,
    const std::string& name
) {
    const Polyline bottom = Polyline::rectangle(origin, x_axis, y_axis, width, height);
    return std::make_shared<FacePlanarElement>(
        bottom, bottom.translated(x_axis.cross(y_axis) * thickness), name
    );
}

std::shared_ptr<FacePlanarElement> FacePlanarElement::from_line(
    const Line& base, const Vector& up, double height, double thickness, const std::string& name
) {

    const Vector along = (base.end() - base.start()).normalized();
    const Vector upward = up.normalized();
    const Vector across = along.cross(upward).normalized();
    return from_rectangle(
        base.start() - across * (0.5 * thickness),
        along,
        upward,
        base.length(),
        height,
        thickness,
        name
    );
}

std::array<std::shared_ptr<FacePlanarElement>, 2>
FacePlanarElement::pair_in_plane(double angle_degrees, double length, double width, double thickness) {

    const double fold = (180.0 - angle_degrees) * Tolerance::PI / 180.0;
    const Point centre(length, 0.0, 0.5 * thickness);
    const std::array<Vector, 2> along = {
        Vector(1.0, 0.0, 0.0), Vector(std::cos(fold), 0.0, std::sin(fold))
    };
    const std::array<Vector, 2> up = {
        Vector(0.0, 0.0, 1.0), Vector(-std::sin(fold), 0.0, std::cos(fold))
    };
    const Vector miter = (along[0] + along[1]).normalized();
    const Vector across(0.0, width, 0.0);

    std::array<std::array<Polyline, 2>, 2> faces;
    for (int side = 0; side < 2; side++) {
        const double sign = side == 0 ? -1.0 : 1.0;
        for (int face = 0; face < 2; face++) {
            const Vector offset = up[side] * ((face == 0 ? -0.5 : 0.5) * thickness);
            const Point seam =
                centre + offset - along[side] * (offset.dot(miter) / along[side].dot(miter));
            const Point far = centre + offset + along[side] * (sign * length);
            faces[side][face] = Polyline({seam, far, far + across, seam + across, seam});
        }
    }

    return {
        std::make_shared<FacePlanarElement>(faces[0][0], faces[0][1], "left"),
        std::make_shared<FacePlanarElement>(faces[1][0], faces[1][1], "right"),
    };
}

std::array<std::shared_ptr<FacePlanarElement>, 2> FacePlanarElement::pair_folded(
    double angle_degrees, double length, double width, double height, double thickness
) {

    const double angle = angle_degrees * Tolerance::PI / 180.0;
    const Point outer(length, 0.0, 0.0);
    const Vector up(-std::cos(angle), 0.0, std::sin(angle));
    const Vector inward(-std::sin(angle), 0.0, -std::cos(angle));
    const Point inner =
        outer + inward * thickness + up * (thickness * (1.0 + std::cos(angle)) / std::sin(angle));
    const Vector along(0.0, width, 0.0);

    const Polyline floor_bottom(
        {Point(0.0, 0.0, 0.0),
         outer,
         outer + along,
         Point(0.0, 0.0, 0.0) + along,
         Point(0.0, 0.0, 0.0)}
    );
    const Polyline floor_top(
        {Point(0.0, 0.0, thickness),
         inner,
         inner + along,
         Point(0.0, 0.0, thickness) + along,
         Point(0.0, 0.0, thickness)}
    );
    const Point outer_end = outer + up * height;
    const Point inner_end = outer + inward * thickness + up * height;
    const Polyline wall_bottom({outer, outer + along, outer_end + along, outer_end, outer});
    const Polyline wall_top({inner, inner + along, inner_end + along, inner_end, inner});
    return {
        std::make_shared<FacePlanarElement>(floor_bottom, floor_top, "floor"),
        std::make_shared<FacePlanarElement>(wall_bottom, wall_top, "wall")
    };
}

std::array<std::shared_ptr<FacePlanarElement>, 2> FacePlanarElement::pair_top_side(
    double angle_degrees, double base_size, double upright_size, double thickness
) {

    const double tilt = (90.0 - angle_degrees) * Tolerance::PI / 180.0;
    const Vector foot(0.0, upright_size, 0.0);
    const Vector rise(upright_size * std::tan(tilt), 0.0, upright_size);
    const Point corner(0.5 * (base_size - thickness), 0.5 * (base_size - upright_size), thickness);
    const Polyline face({corner, corner + foot, corner + foot + rise, corner + rise, corner});

    const std::shared_ptr<FacePlanarElement> base = from_rectangle(
        Point(0.0, 0.0, 0.0),
        Vector(1.0, 0.0, 0.0),
        Vector(0.0, 1.0, 0.0),
        base_size,
        base_size,
        thickness,
        "base"
    );
    const std::shared_ptr<FacePlanarElement> upright = std::make_shared<FacePlanarElement>(
        face, face.transformed(Xform::translation(thickness, 0.0, 0.0)), "upright"
    );
    return {base, upright};
}

std::array<std::shared_ptr<FacePlanarElement>, 2>
FacePlanarElement::pair_crossing(double angle_degrees, double length, double height, double thickness) {

    const double angle = angle_degrees * Tolerance::PI / 180.0;
    const Vector along(std::cos(angle), std::sin(angle), 0.0);
    const Vector side = along.cross(Vector(0.0, 0.0, 1.0));
    const Point centre(0.5 * length, 0.0, 0.0);
    const Point origin = centre - along * (0.5 * length) - side * (0.5 * thickness);
    const std::shared_ptr<FacePlanarElement> first = from_rectangle(
        Point(0.0, 0.5 * thickness, 0.0),
        Vector(1.0, 0.0, 0.0),
        Vector(0.0, 0.0, 1.0),
        length,
        height,
        thickness,
        "first"
    );
    const std::shared_ptr<FacePlanarElement> second =
        from_rectangle(origin, along, Vector(0.0, 0.0, 1.0), length, height, thickness, "second");
    return {first, second};
}

std::array<std::shared_ptr<FacePlanarElement>, 2>
FacePlanarElement::pair_stacked(double twist_degrees, double length, double width, double thickness) {

    const double twist = twist_degrees * Tolerance::PI / 180.0;
    const Vector x(std::cos(twist), std::sin(twist), 0.0);
    const Vector y(-std::sin(twist), std::cos(twist), 0.0);
    const Point centre(0.55 * length, 0.6 * width, thickness);
    const std::shared_ptr<FacePlanarElement> lower = from_rectangle(
        Point(0.0, 0.0, 0.0),
        Vector(1.0, 0.0, 0.0),
        Vector(0.0, 1.0, 0.0),
        length,
        width,
        thickness,
        "lower"
    );
    const std::shared_ptr<FacePlanarElement> upper = from_rectangle(
        centre - x * (0.5 * length) - y * (0.5 * width), x, y, length, width, thickness, "upper"
    );
    return {lower, upper};
}

std::array<std::shared_ptr<FacePlanarElement>, 2>
FacePlanarElement::pair_rotated(double twist_degrees, double length, double width, double thickness) {

    const double twist = twist_degrees * Tolerance::PI / 180.0;
    const Vector twisted_y(0.0, std::cos(twist), std::sin(twist));
    const Vector twisted_z(0.0, -std::sin(twist), std::cos(twist));

    const Point seam_centre(length, width * 0.5, thickness * 0.5);
    const Point right_origin =
        seam_centre - twisted_y * (width * 0.5) - twisted_z * (thickness * 0.5);

    const std::shared_ptr<FacePlanarElement> left = from_rectangle(
        Point(0.0, 0.0, 0.0),
        Vector(1.0, 0.0, 0.0),
        Vector(0.0, 1.0, 0.0),
        length,
        width,
        thickness,
        "left"
    );
    const std::shared_ptr<FacePlanarElement> right = from_rectangle(
        right_origin, Vector(1.0, 0.0, 0.0), twisted_y, length, width, thickness, "right"
    );
    return {left, right};
}

std::vector<std::shared_ptr<FacePlanarElement>> FacePlanarElement::row_between(
    const std::array<Polyline, 2>& bottom,
    const std::array<Polyline, 2>& top,
    const std::string& name
) {

    std::vector<std::shared_ptr<FacePlanarElement>> plates;
    const size_t min_pts = std::min(
        {bottom[0].point_count(),
         bottom[1].point_count(),
         top[0].point_count(),
         top[1].point_count()}
    );
    if (min_pts < 2)
        return plates;

    const size_t segments = min_pts - 1;

    for (size_t i = 0; i < segments; i++) {
        const Polyline below = Polyline({bottom[0].get_point(i),
                                         bottom[0].get_point(i + 1),
                                         bottom[1].get_point(i + 1),
                                         bottom[1].get_point(i)})
                                   .closed();
        const Polyline above = Polyline({top[0].get_point(i),
                                         top[0].get_point(i + 1),
                                         top[1].get_point(i + 1),
                                         top[1].get_point(i)})
                                   .closed();
        plates.push_back(std::make_shared<FacePlanarElement>(below, above, fmt::format("{}_{}", name, i)));
    }

    return plates;
}

std::shared_ptr<FacePlanarElement> FacePlanarElement::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::optional<Polyline> bottom;
    std::optional<Polyline> top;
    bool reversed = false;
    std::vector<InteractionFeatureSolid> saved_cuts;
    if (!bytes.empty() && bytes.front() == '{') {
        try {
            const nlohmann::json payload = nlohmann::json::parse(bytes);
            if (payload.contains("bottom") && !payload["bottom"].is_null() &&
                payload.contains("top") && !payload["top"].is_null()) {
                if (const std::string type = payload.value("type", "");
                    type == "FacePlanarElement" || type == "Plate") {
                    wood_proto::FacePlanarElement proto;
                    message_from_json(payload, proto);
                    bottom = Polyline::pb_loads(proto.bottom().SerializeAsString());
                    top = Polyline::pb_loads(proto.top().SerializeAsString());
                    for (const auto& cut : proto.solid_features())
                        saved_cuts.push_back(
                            InteractionFeatureSolid::pb_loads(cut.SerializeAsString())
                        );
                } else {
                    bottom = Polyline::jsonload(payload["bottom"]);
                    top = Polyline::jsonload(payload["top"]);
                }
            }
            reversed = payload.value("reversed", false);
        } catch (const std::exception&) {
        }
    } else {
        wood_proto::FacePlanarElement proto;
        if (proto.ParseFromString(bytes) && proto.has_bottom() && proto.has_top()) {
            bottom = Polyline::pb_loads(proto.bottom().SerializeAsString());
            top = Polyline::pb_loads(proto.top().SerializeAsString());
        }
        reversed = proto.reversed();
        for (const auto& cut : proto.solid_features())
            saved_cuts.push_back(InteractionFeatureSolid::pb_loads(cut.SerializeAsString()));
    }

    std::shared_ptr<FacePlanarElement> plate =
        bottom.has_value() ? std::make_shared<FacePlanarElement>(*bottom, *top) : std::make_shared<FacePlanarElement>();
    static_cast<Element&>(*plate) = std::move(e);
    plate->reversed = reversed;
    plate->solid_features = std::move(saved_cuts);
    plate->_geometry_synced = true;

    FacePlanarElement& out = *plate;
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
            out.features.bottom.insert(
                out.features.bottom.end(), f.outlines.begin(), f.outlines.end()
            );
        if (face == 1)
            out.features.top.insert(out.features.top.end(), f.outlines.begin(), f.outlines.end());
    }

    return plate;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& FacePlanarElement::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        _element_geometry_mesh =
            polylines.size() < 2 ? Mesh() : Mesh::loft({polylines[0]}, {polylines[1]});
    }

    return *_element_geometry_mesh;
}

const BRep& FacePlanarElement::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        _element_geometry_brep =
            polylines.size() < 2 ? BRep() : brep_between_loops({polylines[0]}, {polylines[1]});
    }

    return *_element_geometry_brep;
}

Mesh FacePlanarElement::trimmed_mesh() const {

    if (features.top.empty() || features.bottom.size() != features.top.size() || planes.empty())
        return stock_mesh();

    // out of the bottom: the bottom plane's normal, turned away from the top loop when the solver swapped the planes
    Vector out = planes[0].z_axis();
    if ((features.top[0].center() - features.bottom[0].center()).dot(out) > 0.0)
        out = out * -1.0;

    // the caps: the bottom and top loops triangulated, the first the outline and the rest its holes, every triangle facing out
    std::vector<std::vector<Point>> polygons;
    for (int face = 0; face < 2; face++) {
        std::vector<std::vector<Point>> loops;
        for (const Polyline& loop : face == 0 ? features.bottom : features.top)
            loops.push_back(loop.get_points());

        const Mesh cap = Mesh::from_polygon_with_holes(loops);
        const Vector normal = face == 0 ? out : out * -1.0;
        for (const auto& [key, triangles] : cap.get_triangulation())
            for (const std::array<size_t, 3>& triangle : triangles) {
                std::vector<Point> corners = {
                    *cap.vertex_point(triangle[0]),
                    *cap.vertex_point(triangle[1]),
                    *cap.vertex_point(triangle[2])
                };
                if ((corners[1] - corners[0]).cross(corners[2] - corners[0]).dot(normal) < 0.0)
                    std::swap(corners[1], corners[2]);
                polygons.push_back(corners);
            }
    }

    // the walls: a quad from every edge of a bottom loop to the same edge of its top loop, run against the cap along that edge
    for (size_t k = 0; k < features.bottom.size(); k++) {
        const Polyline& bottom = features.bottom[k];
        const Polyline& top = features.top[k];
        if (bottom.point_count() != top.point_count())
            return stock_mesh();

        const bool with_cap = (k == 0) == (compute_newell(bottom.get_points()).dot(out) > 0.0);
        for (size_t i = 0; i + 1 < bottom.point_count(); i++)
            if (with_cap)
                polygons.push_back({bottom[i + 1], bottom[i], top[i], top[i + 1]});
            else
                polygons.push_back({bottom[i], bottom[i + 1], top[i + 1], top[i]});
    }

    // one solid: the caps and walls welded at their shared corners
    const Mesh solid = Mesh::from_polylines(polygons);
    if (manifold_solid(solid))
        return solid;

    // a cap's triangulation lost a corner: the loft as before when it closes, else the stock plate
    const Mesh lofted = Mesh::loft(features.bottom, features.top);

    return manifold_solid(lofted) ? lofted : stock_mesh();
}

BRep FacePlanarElement::trimmed_brep() const {
    return features.top.empty() ? element_geometry_brep()
                                : brep_between_loops(features.bottom, features.top);
}

void FacePlanarElement::swap_planes() {

    if (planes.size() > 1)
        std::swap(planes[0], planes[1]);
    reset();
    invalidate_geometry();
}

std::shared_ptr<FacePlanarElement> FacePlanarElement::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<FacePlanarElement> plate = std::make_shared<FacePlanarElement>();
    plate->name = name;
    plate->guid() = guid();
    plate->polylines = transformed_list(polylines, xform);
    for (const auto& cut : solid_features)
        plate->solid_features.push_back(cut.transformed(xform));
    plate->planes = transformed_list(planes, xform);
    plate->thickness = thickness;
    plate->reversed = reversed;
    plate->features = {
        transformed_list(features.top, xform), transformed_list(features.bottom, xform)
    };
    plate->feature_types = feature_types;
    plate->set_features(transformed_features(_features, xform));
    plate->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return plate;
}

void FacePlanarElement::place(const Xform& xform) {

    Element::place(xform);
    polylines = transformed_list(polylines, xform);
    for (auto& cut : solid_features)
        cut = cut.transformed(xform);
    planes = transformed_list(planes, xform);
    features = {transformed_list(features.top, xform), transformed_list(features.bottom, xform)};

    _element_geometry_mesh.reset();
    _model_mesh_cache.reset();
    _element_geometry_brep.reset();
    _model_brep_cache.reset();
}

void FacePlanarElement::compute_geometry_mesh_impl() {

    if (polylines.size() > 1) {
        set_geometry(model_geometry_mesh());
    }
    compute_geometry_features();
}

void FacePlanarElement::compute_geometry_brep_impl() {

    if (polylines.size() > 1) {
        set_geometry(model_geometry_brep());
    }
    compute_geometry_features();
}

void FacePlanarElement::compute_geometry_features() {
    set_dimensions(nominal_dimensions());

    std::vector<ElementFeature> next = face_features();
    for (size_t face = 0; face < std::min<size_t>(2, polylines.size()); face++)
        next.push_back(polyline_feature("outline", polylines[face], static_cast<int>(face)));
    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

Vector FacePlanarElement::nominal_dimensions() const {

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
std::vector<ElementFeature> FacePlanarElement::face_features() const {

    static const std::vector<Polyline> none;
    std::vector<ElementFeature> out;
    const size_t face_count = std::max(feature_types.size(), size_t{2});
    for (size_t face = 0; face < face_count; face++) {

        const int type = face < feature_types.size() ? feature_types[face] : -1;
        const std::vector<Polyline>& outlines =
            face == 0 ? features.bottom : (face == 1 ? features.top : none);
        if (type < 0 && outlines.empty())
            continue;

        const std::string feature_type = type >= 0 ? "joint_type_" + std::to_string(type) : "cut";
        out.emplace_back(
            feature_type, static_cast<int>(face), outlines, "face_" + std::to_string(face)
        );
    }

    return out;
}

int FacePlanarElement::given_face(int face) const {

    if (!reversed)
        return face;
    if (face < 2)
        return 1 - face;

    const int side_count = static_cast<int>(planes.size()) - 2;
    return 2 + (side_count - 1 - (face - 2));
}

Vector FacePlanarElement::insertion_vector(int face) const {

    const std::vector<Vector>& vectors = insertion_vectors();
    const int count = static_cast<int>(vectors.size());
    if (face < 0 || face >= count)
        return Vector(0.0, 0.0, 0.0);

    return vectors[reversed && face >= 2 ? count + 1 - face : face];
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════
AABB FacePlanarElement::aabb(double inflate) const {

    std::vector<Point> points;
    for (const Polyline& outline : polylines) {
        const std::vector<Point> vertices = outline.get_points();
        points.insert(points.end(), vertices.begin(), vertices.end());
    }

    return AABB::from_points(points, inflate);
}

nlohmann::ordered_json FacePlanarElement::element_data_jsondump() const {

    wood_proto::FacePlanarElement proto;
    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Failed to parse FacePlanarElement protobuf data");

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════
std::string FacePlanarElement::element_data_dumps() const {

    wood_proto::FacePlanarElement proto;
    if (polylines.size() > 0)
        if (!proto.mutable_bottom()->ParseFromString(polylines[0].pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");
    if (polylines.size() > 1)
        if (!proto.mutable_top()->ParseFromString(polylines[1].pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");
    proto.set_reversed(reversed);

    for (const auto& cut : solid_features)
        if (!proto.add_solid_features()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid solid cut");
    return proto.SerializeAsString();
}

/// The element factory of a serialized plate: the protobuf bytes decoded as an Element and promoted to a FacePlanarElement.
static std::shared_ptr<Element> plate_from_protobuf(const std::string& data) {
    return FacePlanarElement::from_element(Element::pb_loads(data));
}

void FacePlanarElement::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), plate_from_protobuf);
    for (const std::string_view legacy : LEGACY_ELEMENT_TYPES)
        Element::register_type(std::string(legacy), plate_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════
std::string FacePlanarElement::str() const {

    std::ostringstream os;
    os << "FacePlanarElement(name=" << name << ", polylines=" << polylines.size() << ", thickness=" << thickness
       << ", is_visible=" << std::boolalpha << is_visible << ", is_locked=" << is_locked << ")";

    return os.str();
}

std::string FacePlanarElement::repr() const {

    std::ostringstream os;
    os << "FacePlanarElement(name=" << name << ", polylines=" << polylines.size()
       << ", planes=" << planes.size() << ", reversed=" << (reversed ? "true" : "false")
       << ", thickness=" << thickness << ", features=" << face_features().size() << ")";

    return os.str();
}

std::optional<Plane> FacePlanarElement::base_plane() const {

    if (polylines.size() < 2 || planes.empty() || polylines[0].point_count() < 2)
        return std::nullopt;

    const Polyline& bottom = polylines[0];

    return frame_along(
        bottom.get_point(0), bottom.get_point(1) - bottom.get_point(0), -planes[0].z_axis()
    );
}

}
