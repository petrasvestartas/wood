#include "wood_pch.h"
#include "wood_element_plate.h"

namespace wood_session {

using session_cpp::Element;
using session_cpp::ElementFeature;
using session_cpp::Mesh;
using session_cpp::Plane;
using session_cpp::Point;
using session_cpp::Polyline;
using session_cpp::Vector;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Plate::Plate() : Element("plate") {}

/// A bad outline pair degrades to an empty element, which detection skips, rather than taking a dataset run down.
Plate::Plate(const Polyline& bot, const Polyline& top, const std::string& name) : Element(name) {
    Polyline pp0 = bot;
    Polyline pp1 = top;
    if (pp0.point_count() < 3 || pp1.point_count() < 3) {
        fmt::print(stderr, "  WARNING: WoodElement built from outlines with {}/{} points (need >= 3 each) - element left empty.\n", pp0.point_count(), pp1.point_count());
        return;
    }
    if (pp1.point_count() < pp0.point_count()) {
        fmt::print(stderr, "  WARNING: WoodElement top outline has {} points but bottom has {} - element left empty (side faces would index past the end).\n", pp1.point_count(), pp0.point_count());
        return;
    }

    Vector normal = Vector::average_normal(pp0);
    const Point c0 = pp0.center();
    const Point last_p1 = pp1[pp1.point_count() - 1];
    const double last_z = (last_p1 - c0).dot(normal);
    if (last_z > 0) {
        pp0.reverse();
        pp1.reverse();
        normal = Vector::average_normal(pp0);
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
    compute_geometry();
}

// ═══════════════════════════════════════════════════════════════════════════
// Computation
// ═══════════════════════════════════════════════════════════════════════════

void Plate::compute_geometry() {
    if (polylines.size() > 1) {
        const std::vector<Polyline> bottom_outlines{polylines[0]};
        const std::vector<Polyline> top_outlines{polylines[1]};
        set_geometry(features.top.empty() ? Mesh::loft(bottom_outlines, top_outlines) : Mesh::loft(features.bottom, features.top));
    }
    set_dimensions(nominal_dimensions());
    set_features(face_features());
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
    const size_t face_count = std::max(joint_types.size(), size_t{2});
    for (size_t face = 0; face < face_count; face++) {
        const int type = face < joint_types.size() ? joint_types[face] : -1;
        const std::vector<Polyline>& outlines = face == 0 ? features.bottom : (face == 1 ? features.top : none);
        if (type < 0 && outlines.empty())
            continue;
        const std::string feature_type = type >= 0 ? "joint_type_" + std::to_string(type) : "cut";
        out.emplace_back(feature_type, static_cast<int>(face), outlines, "face_" + std::to_string(face));
    }
    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
// Serialization
// ═══════════════════════════════════════════════════════════════════════════

std::string Plate::element_data_dumps() const {
    nlohmann::ordered_json data{
        {"bottom", polylines.size() > 0 ? polylines[0].jsondump() : nlohmann::ordered_json(nullptr)},
        {"reversed", reversed},
        {"top", polylines.size() > 1 ? polylines[1].jsondump() : nlohmann::ordered_json(nullptr)},
        {"type", ELEMENT_TYPE},
    };
    return data.dump();
}

std::shared_ptr<Plate> Plate::from_element(const Element& e) {
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(e.element_data_dumps());
    } catch (const std::exception&) {
        payload = nlohmann::json::object();
    }
    const bool outlined = payload.contains("bottom") && !payload["bottom"].is_null() && payload.contains("top") && !payload["top"].is_null();
    std::shared_ptr<Plate> plate = outlined
        ? std::make_shared<Plate>(Polyline::jsonload(payload["bottom"]), Polyline::jsonload(payload["top"]))
        : std::make_shared<Plate>();
    static_cast<Element&>(*plate) = e;
    plate->guid() = e.guid();
    plate->reversed = payload.value("reversed", false);
    Plate& out = *plate;
    static const std::string prefix = "joint_type_";
    for (const ElementFeature& f : e.features()) {
        if (f.face_index < 0)
            continue;
        const size_t face = static_cast<size_t>(f.face_index);
        if (f.feature_type.compare(0, prefix.size(), prefix) == 0) {
            try {
                const int code = std::stoi(f.feature_type.substr(prefix.size()));
                if (out.joint_types.size() <= face)
                    out.joint_types.resize(face + 1, -1);
                out.joint_types[face] = code;
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

void Plate::register_type() {
    const Element::Factory factory = [](const std::string& data) -> std::shared_ptr<Element> {
        return from_element(Element::pb_loads(data));
    };
    Element::register_type(ELEMENT_TYPE, factory);
    Element::register_type(LEGACY_ELEMENT_TYPE, factory);
}

// ═══════════════════════════════════════════════════════════════════════════
// Text
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
