#include "wood_element.h"
#include <cstdio>

#include "../src/element.h"
#include "../src/line.h"
#include "../src/plane.h"
#include "../src/point.h"
#include "../src/polyline.h"
#include "../src/vector.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace wood_session {

using session_cpp::Element;
using session_cpp::ElementFeature;
using session_cpp::Line;
using session_cpp::Mesh;
using session_cpp::Plane;
using session_cpp::Point;
using session_cpp::Polyline;
using session_cpp::Vector;

namespace {

// Drop a closing vertex that repeats the first one (to 1e-6), the same test the
// (bottom, top) constructor and loft_mesh apply.
void strip_closing(std::vector<Point>& v) {
    if (v.size() > 3) {
        const Point& f = v.front();
        const Point& l = v.back();
        if (std::abs(f[0]-l[0]) < 1e-6 && std::abs(f[1]-l[1]) < 1e-6 &&
            std::abs(f[2]-l[2]) < 1e-6) { v.pop_back(); }
    }
}

// The one thing composition cannot reach. Element leaves its type name and payload to two
// virtuals so that a domain package can write a derived element; everything else about it
// is public. The copy handed to a Session is therefore this: a session_cpp::Element that
// answers those two virtuals, and nothing more. WoodElement / BlockElement own a plain
// Element and only produce one of these on the way out.
class TaggedElement final : public Element {
public:
    TaggedElement(const Element& base, std::string type, std::string data)
        : Element(base), _type(std::move(type)), _data(std::move(data)) {
        // Element's copy constructor mints a fresh guid; this copy IS the plate, so it
        // keeps the plate's identity.
        guid() = base.guid();
    }
    std::string element_type_name() const override { return _type; }
    std::string element_data_dumps() const override { return _data; }
private:
    std::string _type;
    std::string _data;
};

void write_binary(const std::string& filename, const std::string& data) {
    std::ofstream file(filename, std::ios::binary);
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::string read_binary(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// WoodJoint
// ═══════════════════════════════════════════════════════════════════════════

WoodJoint::WoodJoint()
    : el_ids{0, 0}
    , face_ids{ {{0,0}}, {{0,0}} }
    , joint_type{0}
    , joint_area{session_cpp::Polyline(std::vector<session_cpp::Point>{})}
    , joint_lines{
        session_cpp::Line::from_points(session_cpp::Point(0,0,0), session_cpp::Point(0,0,0)),
        session_cpp::Line::from_points(session_cpp::Point(0,0,0), session_cpp::Point(0,0,0)),
      }
    , divisions{1}
    , shift{0.5}
    , length{0}
    , division_length{0.0}
    , scale{1.0, 1.0, 1.0}
    , unit_scale{false}
    , unit_scale_distance{0.0}
    , link{false}
    , no_orient{false}
    , dbg_coplanar{0}
    , dbg_boolean{0}
{}

void WoodJoint::sync_features() {
    for (int side = 0; side < 2; ++side) {
        ElementFeature& f = element_features[side];
        f.feature_type = "joint";
        f.name = name.empty() ? "joint_" + std::to_string(joint_type) : name;
        f.face_index = side == 0 ? face_ids.first[0] : face_ids.second[0];
        const auto& outlines = side == 0 ? m_outlines : f_outlines;
        f.outlines.clear();
        f.outlines.reserve(outlines[0].size() + outlines[1].size());
        for (int face = 0; face < 2; ++face) {
            f.outlines.insert(f.outlines.end(), outlines[face].begin(), outlines[face].end());
        }
    }
}

std::array<ElementFeature, 2> WoodJoint::to_features() const {
    // Copy the joint's solver fields into a scratch joint and sync that, so a const joint
    // can be read without a stale element_features slipping through. Identity is copied
    // explicitly because ElementFeature's copy constructor mints a new guid.
    WoodJoint scratch = *this;
    scratch.sync_features();
    std::array<ElementFeature, 2> out = std::move(scratch.element_features);
    out[0].guid() = element_features[0].guid();
    out[1].guid() = element_features[1].guid();
    return out;
}

nlohmann::ordered_json WoodJoint::jsondump() const {
    using nlohmann::ordered_json;
    auto polylines = [](const std::vector<Polyline>& v) {
        ordered_json a = ordered_json::array();
        for (const Polyline& p : v) { a.push_back(p.jsondump()); }
        return a;
    };
    ordered_json volumes = ordered_json::array();
    for (const auto& v : joint_volumes_pair_a_pair_b) {
        volumes.push_back(v.has_value() ? v->jsondump() : ordered_json(nullptr));
    }
    ordered_json seq = ordered_json::array();
    for (const auto& group : linked_joints_seq) {
        ordered_json g = ordered_json::array();
        for (const auto& q : group) { g.push_back({q[0], q[1], q[2], q[3]}); }
        seq.push_back(g);
    }
    std::array<ElementFeature, 2> feats = to_features();
    return ordered_json{
        {"type", "WoodJoint"},
        {"el_ids", {el_ids.first, el_ids.second}},
        {"face_ids", {{face_ids.first[0], face_ids.first[1]}, {face_ids.second[0], face_ids.second[1]}}},
        {"joint_type", joint_type},
        {"name", name},
        {"joint_area", joint_area.jsondump()},
        {"joint_lines", {joint_lines[0].jsondump(), joint_lines[1].jsondump()}},
        {"joint_volumes", volumes},
        {"m_outlines", {polylines(m_outlines[0]), polylines(m_outlines[1])}},
        {"f_outlines", {polylines(f_outlines[0]), polylines(f_outlines[1])}},
        {"m_cut_types", {m_cut_types[0], m_cut_types[1]}},
        {"f_cut_types", {f_cut_types[0], f_cut_types[1]}},
        {"divisions", divisions},
        {"shift", shift},
        {"length", length},
        {"division_length", division_length},
        {"scale", {scale[0], scale[1], scale[2]}},
        {"unit_scale", unit_scale},
        {"unit_scale_distance", unit_scale_distance},
        {"linked_joints", linked_joints},
        {"linked_joints_seq", seq},
        {"link", link},
        {"no_orient", no_orient},
        {"element_features", {feats[0].jsondump(), feats[1].jsondump()}},
    };
}

WoodJoint WoodJoint::jsonload(const nlohmann::json& data) {
    WoodJoint j;
    auto polylines = [](const nlohmann::json& a) {
        std::vector<Polyline> v;
        for (const auto& p : a) { v.push_back(Polyline::jsonload(p)); }
        return v;
    };
    if (data.contains("el_ids")) { j.el_ids = {data["el_ids"][0], data["el_ids"][1]}; }
    if (data.contains("face_ids")) {
        j.face_ids.first  = {data["face_ids"][0][0], data["face_ids"][0][1]};
        j.face_ids.second = {data["face_ids"][1][0], data["face_ids"][1][1]};
    }
    j.joint_type = data.value("joint_type", 0);
    j.name       = data.value("name", std::string());
    if (data.contains("joint_area")) { j.joint_area = Polyline::jsonload(data["joint_area"]); }
    if (data.contains("joint_lines")) {
        j.joint_lines[0] = Line::jsonload(data["joint_lines"][0]);
        j.joint_lines[1] = Line::jsonload(data["joint_lines"][1]);
    }
    if (data.contains("joint_volumes")) {
        size_t k = 0;
        for (const auto& v : data["joint_volumes"]) {
            if (k >= 4) { break; }
            if (!v.is_null()) { j.joint_volumes_pair_a_pair_b[k] = Polyline::jsonload(v); }
            ++k;
        }
    }
    for (int face = 0; face < 2; ++face) {
        if (data.contains("m_outlines"))  { j.m_outlines[face]  = polylines(data["m_outlines"][face]); }
        if (data.contains("f_outlines"))  { j.f_outlines[face]  = polylines(data["f_outlines"][face]); }
        if (data.contains("m_cut_types")) { j.m_cut_types[face] = data["m_cut_types"][face].get<std::vector<int>>(); }
        if (data.contains("f_cut_types")) { j.f_cut_types[face] = data["f_cut_types"][face].get<std::vector<int>>(); }
    }
    j.divisions       = data.value("divisions", 1);
    j.shift           = data.value("shift", 0.5);
    j.length          = data.value("length", 0.0);
    j.division_length = data.value("division_length", 0.0);
    if (data.contains("scale")) { j.scale = {data["scale"][0], data["scale"][1], data["scale"][2]}; }
    j.unit_scale          = data.value("unit_scale", false);
    j.unit_scale_distance = data.value("unit_scale_distance", 0.0);
    if (data.contains("linked_joints")) { j.linked_joints = data["linked_joints"].get<std::vector<int>>(); }
    if (data.contains("linked_joints_seq")) {
        for (const auto& group : data["linked_joints_seq"]) {
            std::vector<std::array<int, 4>> g;
            for (const auto& q : group) { g.push_back({q[0], q[1], q[2], q[3]}); }
            j.linked_joints_seq.push_back(std::move(g));
        }
    }
    j.link      = data.value("link", false);
    j.no_orient = data.value("no_orient", false);
    // Identity comes back with the features; their content is re-derived from the solver
    // fields above so the two halves cannot disagree.
    if (data.contains("element_features")) {
        for (int side = 0; side < 2 && side < (int)data["element_features"].size(); ++side) {
            ElementFeature f = ElementFeature::jsonload(data["element_features"][side]);
            j.element_features[side] = std::move(f);
        }
    }
    j.sync_features();
    return j;
}

std::string WoodJoint::file_json_dumps() const { return jsondump().dump(); }
WoodJoint WoodJoint::file_json_loads(const std::string& json_string) {
    return jsonload(nlohmann::ordered_json::parse(json_string));
}
void WoodJoint::file_json_dump(const std::string& filename) const {
    std::ofstream file(filename);
    file << jsondump().dump(2);
}
WoodJoint WoodJoint::file_json_load(const std::string& filename) {
    std::ifstream file(filename);
    return jsonload(nlohmann::json::parse(file));
}

std::string WoodJoint::str() const {
    std::ostringstream os;
    os << "WoodJoint(type=" << joint_type
       << ", elements=(" << el_ids.first << "," << el_ids.second << ")"
       << ", faces=(" << face_ids.first[0] << "," << face_ids.second[0] << ")"
       << ", name=" << (name.empty() ? "-" : name) << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodJoint& j) { return os << j.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// BlockElement
// ═══════════════════════════════════════════════════════════════════════════

BlockElement::BlockElement() : element("block") {}

BlockElement::BlockElement(const std::vector<Polyline>& loops) : element("block") {
    polylines.reserve(loops.size());
    planes.reserve(loops.size());
    for (const Polyline& loop : loops) {
        std::vector<Point> pts = loop.get_points();
        strip_closing(pts);
        if (pts.size() < 3) { continue; }
        polylines.push_back(loop);
        // from_point_normal takes non-const refs, so both need to be lvalues.
        Point  origin = Point::centroid(pts);
        Vector normal = Vector::average_normal(pts);
        planes.push_back(Plane::from_point_normal(origin, normal));
    }
}

Mesh BlockElement::mesh() const {
    std::vector<Point> verts;
    std::vector<std::vector<size_t>> faces;
    faces.reserve(polylines.size());
    for (const Polyline& loop : polylines) {
        std::vector<Point> pts = loop.get_points();
        strip_closing(pts);
        if (pts.size() < 3) { continue; }
        std::vector<size_t> face(pts.size());
        for (size_t k = 0; k < pts.size(); ++k) {
            face[k] = verts.size();
            verts.push_back(pts[k]);
        }
        faces.push_back(std::move(face));
    }
    if (faces.empty()) { return Mesh{}; }
    return Mesh::from_vertices_and_faces(verts, faces);
}

void BlockElement::sync_element() {
    element.set_geometry(mesh());
}

std::shared_ptr<Element> BlockElement::to_element() const {
    auto out = std::make_shared<TaggedElement>(element, ELEMENT_TYPE, std::string());
    out->set_geometry(mesh());
    return out;
}

BlockElement BlockElement::from_element(const Element& e) {
    std::vector<Polyline> loops;
    if (const Mesh* m = std::get_if<Mesh>(&e.geometry())) {
        auto [verts, faces] = m->to_vertices_and_faces();
        loops.reserve(faces.size());
        for (const auto& face : faces) {
            if (face.size() < 3) { continue; }
            std::vector<Point> pts;
            pts.reserve(face.size() + 1);
            for (size_t v : face) { pts.push_back(verts[v]); }
            pts.push_back(pts.front());
            loops.emplace_back(pts);
        }
    } else {
        fprintf(stderr, "  WARNING: BlockElement::from_element: element '%s' carries %s, not a "
                        "Mesh - block left empty.\n", e.name.c_str(), e.geometry_type_name().c_str());
        fflush(stderr);
    }
    BlockElement out(loops);
    // Same object, same identity: the copy assignment mints a guid, so put the original back.
    out.element = e;
    out.element.guid() = e.guid();
    return out;
}

nlohmann::ordered_json BlockElement::jsondump() const { return to_element()->jsondump(); }
BlockElement BlockElement::jsonload(const nlohmann::json& data) { return from_element(Element::jsonload(data)); }
std::string BlockElement::file_json_dumps() const { return jsondump().dump(); }
BlockElement BlockElement::file_json_loads(const std::string& json_string) {
    return jsonload(nlohmann::ordered_json::parse(json_string));
}
void BlockElement::file_json_dump(const std::string& filename) const {
    std::ofstream file(filename);
    file << jsondump().dump(2);
}
BlockElement BlockElement::file_json_load(const std::string& filename) {
    std::ifstream file(filename);
    return jsonload(nlohmann::json::parse(file));
}
std::string BlockElement::pb_dumps() const { return to_element()->pb_dumps(); }
BlockElement BlockElement::pb_loads(const std::string& data) { return from_element(Element::pb_loads(data)); }
void BlockElement::pb_dump(const std::string& filename) const { write_binary(filename, pb_dumps()); }
BlockElement BlockElement::pb_load(const std::string& filename) { return pb_loads(read_binary(filename)); }

std::string BlockElement::str() const {
    std::ostringstream os;
    os << "BlockElement(name=" << element.name << ", loops=" << polylines.size() << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const BlockElement& e) { return os << e.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// WoodElement
// ═══════════════════════════════════════════════════════════════════════════

WoodElement::WoodElement()
    : element("plate")
    , reversed{false}
    , thickness{0.0}
{}

WoodElement::WoodElement(const Polyline& bot, const Polyline& top)
    : element("plate")
    , reversed{false}
    , thickness{0.0}
{
    std::vector<Point> pp0 = bot.get_points();
    std::vector<Point> pp1 = top.get_points();

    // Outline sizes are INPUT (OBJ curves paired blindly, or Python lists),
    // not an invariant. Empty outlines reached average_normal's
    // front()/back() as UB; a top outline shorter than the bottom read
    // pp1[j+1] past the end in the side loop - heap OOB producing garbage
    // side planes or a crash. Degrade to an empty element (detection skips
    // it) instead: throwing would take down whole-dataset runs for one bad
    // pair.
    if (pp0.size() < 3 || pp1.size() < 3) {
        fprintf(stderr,
                "  WARNING: WoodElement built from outlines with %zu/%zu points "
                "(need >= 3 each) - element left empty.\n",
                pp0.size(), pp1.size());
        fflush(stderr);
        return;
    }
    if (pp1.size() < pp0.size()) {
        fprintf(stderr,
                "  WARNING: WoodElement top outline has %zu points but bottom has "
                "%zu - element left empty (side faces would index past the end).\n",
                pp1.size(), pp0.size());
        fflush(stderr);
        return;
    }

    Vector normal = Vector::average_normal(pp0);
    auto pp0_open = pp0;
    strip_closing(pp0_open);
    Point c0 = Point::centroid(pp0_open);
    Point last_p1 = pp1.back();
    double last_z = (last_p1[0]-c0[0])*normal[0]
                  + (last_p1[1]-c0[1])*normal[1]
                  + (last_p1[2]-c0[2])*normal[2];
    if (last_z > 0) {
        std::reverse(pp0.begin(), pp0.end());
        std::reverse(pp1.begin(), pp1.end());
        normal = Vector::average_normal(pp0);
        reversed = true;
    }

    size_t n_sides = pp0.size() > 1 ? pp0.size() - 1 : 0;

    polylines.resize(2 + n_sides, Polyline(std::vector<Point>{}));
    polylines[0] = Polyline(pp0);
    polylines[1] = Polyline(pp1);

    auto pp0_stripped = pp0;
    auto pp1_stripped = pp1;
    strip_closing(pp0_stripped);
    strip_closing(pp1_stripped);
    Point cen0 = Point::centroid(pp0_stripped);
    Point cen1 = Point::centroid(pp1_stripped);
    planes.resize(2 + n_sides);
    Vector neg_normal(-normal[0],-normal[1],-normal[2]);
    planes[0] = Plane::from_point_normal(cen0, normal);
    planes[1] = Plane::from_point_normal(cen1, neg_normal);
    thickness = Point::distance(cen0, planes[1].project(cen0));

    for (size_t j = 0; j < n_sides; j++) {
        double ax = pp0[j][0]-pp0[j+1][0];
        double ay = pp0[j][1]-pp0[j+1][1];
        double az = pp0[j][2]-pp0[j+1][2];
        double bx = pp1[j+1][0]-pp0[j+1][0];
        double by = pp1[j+1][1]-pp0[j+1][1];
        double bz = pp1[j+1][2]-pp0[j+1][2];
        double nx = ay*bz-az*by;
        double ny = az*bx-ax*bz;
        double nz = ax*by-ay*bx;
        Point side_origin = pp0[j+1];
        double anx = std::abs(nx), any = std::abs(ny), anz = std::abs(nz);
        Vector sb1;
        if (anx < 1e-12) {
            sb1 = Vector(1,0,0);
        } else if (any < 1e-12) {
            sb1 = Vector(0,1,0);
        } else if (anz < 1e-12) {
            sb1 = Vector(0,0,1);
        } else if (anx<=any && anx<=anz) {
            sb1 = Vector(0,-nz,ny);
        } else if (any<=anx && any<=anz) {
            sb1 = Vector(-nz,0,nx);
        } else {
            sb1 = Vector(-ny,nx,0);
        }
        Vector snv(nx,ny,nz);
        Vector sb2 = snv.cross(sb1);
        sb1.normalize_self();
        sb2.normalize_self();
        snv.normalize_self();
        planes[2+j] = Plane(side_origin, sb1, sb2, snv);
        polylines[2+j] = Polyline(std::vector<Point>{
            pp0[j], pp0[j+1], pp1[j+1], pp1[j], pp0[j]});
    }
}

session_cpp::Mesh WoodElement::loft_mesh() const {
    auto strip = [](const Polyline& pl) -> std::vector<Point> {
        std::vector<Point> pts = pl.get_points();
        size_t n = pts.size();
        if (n >= 2) {
            const auto& f = pts.front();
            const auto& l = pts.back();
            if (std::abs(f[0]-l[0]) < 1e-6 &&
                std::abs(f[1]-l[1]) < 1e-6 &&
                std::abs(f[2]-l[2]) < 1e-6) {
                pts.pop_back();
            }
        }
        return pts;
    };

    if (polylines.size() < 2) return session_cpp::Mesh{};
    std::vector<Point> bot = strip(polylines[0]);
    std::vector<Point> top = strip(polylines[1]);
    if (bot.size() != top.size()) {
        // Truncating to the shorter ring shifted every side quad - a
        // plausible-looking but wrong solid. Fail visibly instead.
        fprintf(stderr,
                "  WARNING: loft_mesh outlines have %zu vs %zu points - "
                "returning empty mesh.\n", bot.size(), top.size());
        fflush(stderr);
        return session_cpp::Mesh{};
    }
    size_t n = bot.size();
    if (n < 3) return session_cpp::Mesh{};

    std::vector<Point> verts;
    verts.reserve(2 * n);
    for (size_t i = 0; i < n; ++i) {
        verts.push_back(bot[i]);
    }
    for (size_t i = 0; i < n; ++i) {
        verts.push_back(top[i]);
    }

    std::vector<std::vector<size_t>> faces;
    faces.reserve(2 + n);

    // WoodElement orientation ensures polylines[0] winding is already outward for bottom —
    // use forward order. Top cap is reversed so its normal points outward away from bottom.
    std::vector<size_t> bot_cap(n);
    for (size_t i = 0; i < n; ++i) {
        bot_cap[i] = i;
    }
    faces.push_back(bot_cap);

    std::vector<size_t> top_cap(n);
    for (size_t i = 0; i < n; ++i) {
        top_cap[i] = n + (n - 1 - i);
    }
    faces.push_back(top_cap);

    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        faces.push_back({i, n + i, n + j, j});
    }

    return session_cpp::Mesh::from_vertices_and_faces(verts, faces);
}

Vector WoodElement::nominal_dimensions() const {
    if (polylines.empty() || planes.empty()) {
        return Vector(0.0, 0.0, thickness);
    }
    const Plane& frame = planes[0];
    const Point& origin = frame.origin();
    const Vector& ex = frame.x_axis();
    const Vector& ey = frame.y_axis();

    bool first = true;
    double min_u = 0.0, max_u = 0.0, min_v = 0.0, max_v = 0.0;
    for (const auto& polyline : polylines) {
        for (const auto& pt : polyline.get_points()) {
            Vector d(pt[0] - origin[0], pt[1] - origin[1], pt[2] - origin[2]);
            double u = d[0]*ex[0] + d[1]*ex[1] + d[2]*ex[2];
            double v = d[0]*ey[0] + d[1]*ey[1] + d[2]*ey[2];
            if (first) { min_u = max_u = u; min_v = max_v = v; first = false; }
            else {
                min_u = std::min(min_u, u); max_u = std::max(max_u, u);
                min_v = std::min(min_v, v); max_v = std::max(max_v, v);
            }
        }
    }
    return Vector(max_u - min_u, max_v - min_v, thickness);
}

std::vector<ElementFeature> WoodElement::face_features() const {
    std::vector<ElementFeature> out;

    // -1 is wood's "no joint assigned"; a face with neither a type nor an outline is not a
    // feature and must not be written as an empty one.
    auto joint_type_of = [this](size_t face) -> int {
        return face < joint_types.size() ? joint_types[face] : -1;
    };
    auto outlines_of = [this](size_t face) -> const std::vector<Polyline>& {
        static const std::vector<Polyline> none;
        if (face == 0) return features.bottom;
        if (face == 1) return features.top;
        return none;
    };

    size_t face_count = std::max(joint_types.size(), size_t{2});
    for (size_t face = 0; face < face_count; face++) {
        int type = joint_type_of(face);
        const std::vector<Polyline>& outlines = outlines_of(face);
        if (type < 0 && outlines.empty()) { continue; }
        std::string feature_type = type >= 0 ? "joint_type_" + std::to_string(type) : "cut";
        out.emplace_back(feature_type, static_cast<int>(face), outlines,
                         "face_" + std::to_string(face));
    }
    return out;
}

namespace {

// The wood-only state a plate cannot be rebuilt without. Everything else on the Element -
// mesh, insertion vectors, dimensions, face features - is derived from these two outlines
// plus fields Element already has a home for. Text JSON rather than a second proto message:
// element_data is opaque bytes by design, and three languages can read this without a
// schema change in any of them.
std::string wood_payload(const WoodElement& we) {
    nlohmann::ordered_json j{
        {"type", WoodElement::ELEMENT_TYPE},
        {"bottom", we.polylines.size() > 0 ? we.polylines[0].jsondump() : nlohmann::ordered_json(nullptr)},
        {"top",    we.polylines.size() > 1 ? we.polylines[1].jsondump() : nlohmann::ordered_json(nullptr)},
        {"reversed", we.reversed},
    };
    return j.dump();
}

void fill_kernel(const WoodElement& we, Element& out) {
    out.set_geometry(we.loft_mesh());
    out.set_insertion_vectors(we.insertion_vectors);
    out.set_dimensions(we.nominal_dimensions());
    out.set_features(we.face_features());
}

}  // namespace

void WoodElement::sync_element() {
    fill_kernel(*this, element);
}

std::shared_ptr<Element> WoodElement::to_element() const {
    auto out = std::make_shared<TaggedElement>(element, ELEMENT_TYPE, wood_payload(*this));
    fill_kernel(*this, *out);
    return out;
}

WoodElement WoodElement::from_element(const Element& e) {
    WoodElement out;
    out.element = e;
    out.element.guid() = e.guid();

    if (e.element_type_name() != ELEMENT_TYPE) {
        fprintf(stderr, "  WARNING: WoodElement::from_element: element '%s' has element_type "
                        "'%s', not '%s' - element left empty.\n",
                e.name.c_str(), e.element_type_name().c_str(), ELEMENT_TYPE);
        fflush(stderr);
        return out;
    }
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(e.element_data_dumps());
    } catch (const std::exception& ex) {
        fprintf(stderr, "  WARNING: WoodElement::from_element: element '%s' carries an "
                        "unreadable payload (%s) - element left empty.\n", e.name.c_str(), ex.what());
        fflush(stderr);
        return out;
    }
    if (!payload.contains("bottom") || payload["bottom"].is_null() ||
        !payload.contains("top")    || payload["top"].is_null()) {
        return out;   // written from an empty plate; comes back as one
    }

    // The outlines were stored already oriented, so the constructor's orientation test is a
    // no-op and `reversed` has to be restored from the payload rather than re-derived.
    Polyline bottom = Polyline::jsonload(payload["bottom"]);
    Polyline top    = Polyline::jsonload(payload["top"]);
    out = WoodElement(bottom, top);
    out.element = e;
    out.element.guid() = e.guid();
    out.reversed = payload.value("reversed", false);
    out.insertion_vectors = e.insertion_vectors();

    // face_features() in reverse: "joint_type_<code>" restores joint_types[face], and the
    // outlines of faces 0 / 1 restore the merged bottom / top. Detected joints ("joint") are
    // left where they are - without the solver fields they cannot become a WoodJoint again,
    // and they stay on `element` for anyone who wants to read them. Trailing -1 entries of
    // joint_types are not features and do not come back; the solver treats a missing face
    // as -1 anyway.
    static const std::string prefix = "joint_type_";
    for (const ElementFeature& f : e.features()) {
        if (f.face_index < 0) { continue; }
        const size_t face = static_cast<size_t>(f.face_index);
        if (f.feature_type.compare(0, prefix.size(), prefix) == 0) {
            try {
                int code = std::stoi(f.feature_type.substr(prefix.size()));
                if (out.joint_types.size() <= face) { out.joint_types.resize(face + 1, -1); }
                out.joint_types[face] = code;
            } catch (const std::exception&) { /* not a code: ignore */ }
        } else if (f.feature_type != "cut") {
            continue;
        }
        if (face == 0) { out.features.bottom.insert(out.features.bottom.end(), f.outlines.begin(), f.outlines.end()); }
        if (face == 1) { out.features.top.insert(out.features.top.end(), f.outlines.begin(), f.outlines.end()); }
    }
    return out;
}

nlohmann::ordered_json WoodElement::jsondump() const { return to_element()->jsondump(); }
WoodElement WoodElement::jsonload(const nlohmann::json& data) { return from_element(Element::jsonload(data)); }
std::string WoodElement::file_json_dumps() const { return jsondump().dump(); }
WoodElement WoodElement::file_json_loads(const std::string& json_string) {
    return jsonload(nlohmann::ordered_json::parse(json_string));
}
void WoodElement::file_json_dump(const std::string& filename) const {
    std::ofstream file(filename);
    file << jsondump().dump(2);
}
WoodElement WoodElement::file_json_load(const std::string& filename) {
    std::ifstream file(filename);
    return jsonload(nlohmann::json::parse(file));
}
std::string WoodElement::pb_dumps() const { return to_element()->pb_dumps(); }
WoodElement WoodElement::pb_loads(const std::string& data) { return from_element(Element::pb_loads(data)); }
void WoodElement::pb_dump(const std::string& filename) const { write_binary(filename, pb_dumps()); }
WoodElement WoodElement::pb_load(const std::string& filename) { return pb_loads(read_binary(filename)); }

std::string WoodElement::str() const {
    std::ostringstream os;
    os << "WoodElement(name=" << element.name << ", polylines=" << polylines.size()
       << ", thickness=" << thickness << ")";
    return os.str();
}
std::string WoodElement::repr() const {
    std::ostringstream os;
    os << "WoodElement(name=" << element.name
       << ", polylines=" << polylines.size()
       << ", planes=" << planes.size()
       << ", reversed=" << (reversed ? "true" : "false")
       << ", thickness=" << thickness
       << ", features=" << face_features().size() << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodElement& e) {
    return os << e.str();
}

} // namespace wood_session
