#include "wood_pch.h"
#include "wood_joint.h"
#include "wood_session.h"
#include "wood_cut.h"
using namespace session_cpp;
using wood_session::WoodJoint;
using wood_session::Plate;

/// The joint library is header-only and static; it lands in this TU's anonymous namespace.
namespace {
#include "wood_cut.h"
#include "joints/ss_e_r_0.h"
}

namespace wood_session {

constexpr bool TRACE = false;

// ═══════════════════════════════════════════════════════════════════════════
// Contacts and joints
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// A ring as bare coordinates: a joint's rings are geometry, not objects, so no guid, colour or name.
nlohmann::ordered_json to_coords(const Polyline& ring) {
    nlohmann::ordered_json coords = nlohmann::ordered_json::array();
    for (size_t i = 0; i < ring.point_count(); i++) {
        const Point point = ring.get_point(i);
        coords.push_back(point[0]);
        coords.push_back(point[1]);
        coords.push_back(point[2]);
    }
    return coords;
}

Polyline from_coords(const nlohmann::json& data) {
    std::vector<Point> points;
    points.reserve(data.size() / 3);
    for (size_t i = 0; i + 2 < data.size(); i += 3)
        points.emplace_back(data[i].get<double>(), data[i + 1].get<double>(), data[i + 2].get<double>());
    return Polyline(points);
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// FaceContact
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json FaceContact::jsondump() const {
    return nlohmann::ordered_json{
        {"face_a", face_a},
        {"face_b", face_b},
        {"type", static_cast<int>(type)},
        {"area", to_coords(area)},
    };
}

FaceContact FaceContact::jsonload(const nlohmann::json& data) {
    FaceContact contact;
    contact.face_a = data.value("face_a", 0);
    contact.face_b = data.value("face_b", 0);
    contact.type   = static_cast<ContactType>(data.value("type", -1));
    if (data.contains("area"))
        contact.area = from_coords(data["area"]);
    return contact;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodJoint
// ═══════════════════════════════════════════════════════════════════════════

WoodJoint::WoodJoint()
    : joint_type{0}
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

const std::string& WoodJoint::feature_guid(int side) const {
    std::string& id = feature_guids[side];
    if (id.empty())
        id = ::guid();
    return id;
}

void WoodJoint::sync_features() {
    for (int side = 0; side < 2; ++side) {
        ElementFeature& f = element_features[side];
        f.guid() = feature_guid(side);
        f.feature_type = "joint";
        f.name = name.empty() ? "joint_" + std::to_string(joint_type) : name;
        f.face_index = side == 0 ? contact.face_a : contact.face_b;
        const auto& outlines = side == 0 ? m_outlines : f_outlines;
        f.outlines.clear();
        f.outlines.reserve(outlines[0].size() + outlines[1].size());
        for (int face = 0; face < 2; ++face)
            f.outlines.insert(f.outlines.end(), outlines[face].begin(), outlines[face].end());
    }
}

/// Syncs a scratch copy that carries this joint's feature guids, so a const joint reads fresh and keeps its identity.
std::array<ElementFeature, 2> WoodJoint::to_features() const {
    WoodJoint scratch = *this;
    scratch.feature_guids = {feature_guid(0), feature_guid(1)};
    scratch.sync_features();
    return std::move(scratch.element_features);
}

nlohmann::ordered_json WoodJoint::jsondump() const {
    using nlohmann::ordered_json;
    auto rings = [](const std::vector<Polyline>& v) {
        ordered_json a = ordered_json::array();
        for (const Polyline& ring : v)
            a.push_back(to_coords(ring));
        return a;
    };
    ordered_json volumes = ordered_json::array();
    for (const auto& v : joint_volumes_pair_a_pair_b)
        volumes.push_back(v.has_value() ? to_coords(*v) : ordered_json(nullptr));
    ordered_json seq = ordered_json::array();
    for (const auto& group : linked_joints_seq) {
        ordered_json g = ordered_json::array();
        for (const auto& q : group)
            g.push_back({q[0], q[1], q[2], q[3]});
        seq.push_back(g);
    }
    return ordered_json{
        {"type", "WoodJoint"},
        {"el_ids", {element_a, element_b}},
        {"face_ids", {{contact.face_a, cross_faces[0]}, {contact.face_b, cross_faces[1]}}},
        {"contact_type", static_cast<int>(contact.type)},
        {"joint_type", joint_type},
        {"name", name},
        {"joint_area", to_coords(contact.area)},
        {"joint_lines", {to_coords(Polyline({joint_lines[0].start(), joint_lines[0].end()})),
                         to_coords(Polyline({joint_lines[1].start(), joint_lines[1].end()}))}},
        {"joint_volumes", volumes},
        {"m_outlines", {rings(m_outlines[0]), rings(m_outlines[1])}},
        {"f_outlines", {rings(f_outlines[0]), rings(f_outlines[1])}},
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
        {"feature_guids", {feature_guid(0), feature_guid(1)}},
    };
}

WoodJoint WoodJoint::jsonload(const nlohmann::json& data) {
    WoodJoint j;
    auto rings = [](const nlohmann::json& a) {
        std::vector<Polyline> v;
        for (const auto& ring : a)
            v.push_back(from_coords(ring));
        return v;
    };
    auto line = [](const nlohmann::json& a) {
        const Polyline ring = from_coords(a);
        return ring.point_count() >= 2 ? Line::from_points(ring.get_point(0), ring.get_point(1))
                                       : Line::from_points(Point(0, 0, 0), Point(0, 0, 0));
    };
    if (data.contains("el_ids")) {
        j.element_a = data["el_ids"][0];
        j.element_b = data["el_ids"][1];
    }
    if (data.contains("face_ids")) {
        j.contact.face_a = data["face_ids"][0][0];
        j.contact.face_b = data["face_ids"][1][0];
        j.cross_faces    = {data["face_ids"][0][1], data["face_ids"][1][1]};
    }
    j.contact.type = static_cast<ContactType>(data.value("contact_type", -1));
    j.joint_type = data.value("joint_type", 0);
    j.name       = data.value("name", std::string());
    if (data.contains("joint_area"))
        j.contact.area = from_coords(data["joint_area"]);
    if (data.contains("joint_lines")) {
        j.joint_lines[0] = line(data["joint_lines"][0]);
        j.joint_lines[1] = line(data["joint_lines"][1]);
    }
    if (data.contains("joint_volumes")) {
        size_t k = 0;
        for (const auto& v : data["joint_volumes"]) {
            if (k >= 4)
                break;
            if (!v.is_null())
                j.joint_volumes_pair_a_pair_b[k] = from_coords(v);
            ++k;
        }
    }
    for (int face = 0; face < 2; ++face) {
        if (data.contains("m_outlines"))
            j.m_outlines[face] = rings(data["m_outlines"][face]);
        if (data.contains("f_outlines"))
            j.f_outlines[face] = rings(data["f_outlines"][face]);
        if (data.contains("m_cut_types"))
            j.m_cut_types[face] = data["m_cut_types"][face].get<std::vector<int>>();
        if (data.contains("f_cut_types"))
            j.f_cut_types[face] = data["f_cut_types"][face].get<std::vector<int>>();
    }
    j.divisions       = data.value("divisions", 1);
    j.shift           = data.value("shift", 0.5);
    j.length          = data.value("length", 0.0);
    j.division_length = data.value("division_length", 0.0);
    if (data.contains("scale"))
        j.scale = {data["scale"][0], data["scale"][1], data["scale"][2]};
    j.unit_scale          = data.value("unit_scale", false);
    j.unit_scale_distance = data.value("unit_scale_distance", 0.0);
    if (data.contains("linked_joints"))
        j.linked_joints = data["linked_joints"].get<std::vector<int>>();
    if (data.contains("linked_joints_seq")) {
        for (const auto& group : data["linked_joints_seq"]) {
            std::vector<std::array<int, 4>> g;
            for (const auto& q : group)
                g.push_back({q[0], q[1], q[2], q[3]});
            j.linked_joints_seq.push_back(std::move(g));
        }
    }
    j.link      = data.value("link", false);
    j.no_orient = data.value("no_orient", false);
    if (data.contains("feature_guids")) {
        for (int side = 0; side < 2 && side < (int)data["feature_guids"].size(); ++side)
            j.feature_guids[side] = data["feature_guids"][side].get<std::string>();
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
       << ", elements=(" << element_a << "," << element_b << ")"
       << ", faces=(" << contact.face_a << "," << contact.face_b << ")"
       << ", name=" << (name.empty() ? "-" : name) << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodJoint& j) { return os << j.str(); }

std::array<double, 3> joint_volume_extension(const std::vector<double>& extension, int joint_type) {
    const size_t triples = extension.size() / 3;
    if (triples == 0)
        return {0.0, 0.0, 0.0};
    const size_t klass = joint_type == 20 ? 1 : joint_type == 40 ? 2 : joint_type == 30 ? 3 : 0;
    const size_t at = std::min(klass, triples - 1) * 3;
    return {extension[at], extension[at + 1], extension[at + 2]};
}

int index_of(const std::vector<std::shared_ptr<Plate>>& elements, const std::string& guid) {
    for (size_t i = 0; i < elements.size(); ++i)
        if (elements[i]->guid() == guid)
            return static_cast<int>(i);
    return -1;
}

// ═══════════════════════════════════════════════════════════════════════════
// Joint construction
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// Slide a volume pair to its midpoint, then apart by unit_scale_distance along the joint line.
void move_pair(WoodJoint& joint, Polyline& a, Polyline& b) {
    if (joint.unit_scale_distance == 0.0) {
        const Vector edge = a.get_point(2) - a.get_point(1);
        const double raw = std::sqrt(edge.magnitude_squared());
        // +1e-6: a near-integer edge length floors to the integer it means.
        joint.unit_scale_distance = std::floor(raw + 1e-6);
    }
    const Vector seg = b.get_point(0) - a.get_point(0);
    const Vector vec = seg * 0.5;
    const double len = std::sqrt(seg.magnitude_squared());
    if (len < 1e-12)
        return;
    const double s = (joint.unit_scale_distance * 0.5) / len;
    const Vector unit = seg * s;
    a.translate(vec);
    b.translate(-vec);
    a.translate(-unit);
    b.translate(unit);
}

}  // namespace

void apply_unit_scale(WoodJoint& joint) {
    auto& vols = joint.joint_volumes_pair_a_pair_b;
    if (TRACE) {
        std::ofstream out("apply_unit_scale.txt", std::ios::app);
        out << "apply_unit_scale: joint v0=" << joint.element_a << " v1=" << joint.element_b
            << " unit_scale=" << joint.unit_scale << " usd=" << joint.unit_scale_distance << "\n";
        for (int i = 0; i < 4; i++) {
            out << "  BEFORE vols[" << i << "] ";
            if (!vols[i].has_value()) {
                out << "nullopt\n";
                continue;
            }
            for (size_t k = 0; k < vols[i]->point_count(); k++) {
                const Point p = vols[i]->get_point(k);
                out << "(" << p[0] << "," << p[1] << "," << p[2] << ") ";
            }
            out << "\n";
        }
    }
    if (!joint.unit_scale)
        return;
    if (!vols[0].has_value() || !vols[1].has_value())
        return;
    move_pair(joint, *vols[0], *vols[1]);
    if (vols[2].has_value() && vols[3].has_value())
        move_pair(joint, *vols[2], *vols[3]);
}

/// Rescale the volumes along the joint line, then map the unit-cube outlines onto them by change of basis.
void joint_orient_to_connection_area(WoodJoint& joint) {
    auto& vols = joint.joint_volumes_pair_a_pair_b;
    if (!vols[0].has_value() || !vols[1].has_value())
        return;
    apply_unit_scale(joint);
    const Xform xf0 = Xform::from_change_of_basis(*vols[0], *vols[1]);
    const Xform xf1 = (vols[2].has_value() && vols[3].has_value())
        ? Xform::from_change_of_basis(*vols[2], *vols[3])
        : xf0;
    for (int face = 0; face < 2; face++) {
        for (auto& pl : joint.m_outlines[face])
            pl.transform(xf0);
        for (auto& pl : joint.f_outlines[face])
            pl.transform(xf1);
    }
}

namespace {

bool compute_linked_outline(
    std::array<std::vector<Point>, 2>& current,
    const std::array<std::vector<Point>, 2>& linked,
    const std::array<int, 4>& sequence
) {
    if (sequence == std::array<int, 4>{0, 0, 0, 0})
        return true;
    if (sequence[0] < 0 || sequence[2] < 0 || sequence[1] <= 0 || sequence[3] <= 0)
        return false;
    const size_t start = sequence[0];
    const size_t step = sequence[1];
    const size_t offset = sequence[2];
    const size_t stride = sequence[3];
    if (start > current[0].size())
        return false;
    const size_t count = start < current[0].size() - start ? current[0].size() - 2 * start : 0;
    const size_t iterations = count / step + (count % step != 0);
    for (size_t i = 0; i < 2; ++i) {
        if (start > current[i].size() || offset > linked[i].size())
            return false;
        if (iterations > (current[i].size() - start) / step ||
            iterations > (linked[i].size() - offset) / stride)
            return false;
    }
    std::array<std::vector<Point>, 2> result;
    for (size_t i = 0; i < 2; ++i) {
        result[i].insert(result[i].end(), current[i].begin(), current[i].begin() + start);
        size_t position = start;
        size_t index = offset;
        for (size_t j = 0; j < iterations; ++j) {
            result[i].insert(result[i].end(), current[i].begin() + position, current[i].begin() + position + step / 2);
            result[i].insert(result[i].end(), linked[i].begin() + index, linked[i].begin() + index + stride);
            result[i].insert(result[i].end(), current[i].begin() + position + step / 2, current[i].begin() + position + step);
            position += step;
            index += stride;
        }
        result[i].insert(result[i].end(), current[i].end() - start, current[i].end());
    }
    current = std::move(result);
    return true;
}

}  // namespace

void merge_linked_joints(WoodJoint& joint, std::vector<WoodJoint>& all_joints) {
    if (joint.linked_joints_seq.size() != joint.linked_joints.size())
        return;
    for (size_t i = 0; i < joint.linked_joints.size(); ++i) {
        const int index = joint.linked_joints[i];
        if (index < 0 || static_cast<size_t>(index) >= all_joints.size())
            continue;
        WoodJoint& linked = all_joints[index];
        if (&linked == &joint)
            continue;
        const bool male = joint.element_a == linked.element_a;
        const bool side = i == 1 ? !male : male;
        auto& current = male ? joint.m_outlines : joint.f_outlines;
        auto& next = side ? linked.m_outlines : linked.f_outlines;
        if (current[0].empty() || current[1].empty() || next[0].empty() || next[1].empty())
            continue;
        if (current[0].size() % 2 != 0 || joint.linked_joints_seq[i].size() != current[0].size() / 2)
            continue;
        std::array<std::vector<Point>, 2> points{current[0][0].get_points(), current[1][0].get_points()};
        const std::array<std::vector<Point>, 2> addition{next[0][0].get_points(), next[1][0].get_points()};
        bool valid = true;
        for (const auto& sequence : joint.linked_joints_seq[i]) {
            if (!compute_linked_outline(points, addition, sequence)) {
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;
        current[0][0] = Polyline(points[0]);
        current[1][0] = Polyline(points[1]);
        next[0].clear();
        next[1].clear();
    }
}

void joint_get_divisions(WoodJoint& joint, double division_distance) {
    joint.division_length = division_distance;
    const double length = joint.joint_lines[0].squared_length();
    if (!std::isfinite(length) || length <= 1e-10)
        return;
    joint.length = std::sqrt(length);
    joint.divisions = division_distance > 0.0
        ? static_cast<int>(std::clamp(std::ceil(joint.length / division_distance), 1.0, 100.0))
        : 1;
}

// ═══════════════════════════════════════════════════════════════════════════
// Side removal
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// Convexity of every corner of a closed polygon, tested against its normal.
std::vector<bool> convex_corners(const Polyline& pl, const Vector& normal) {
    size_t n = pl.point_count();
    if (n > 1 && (pl.get_point(0) - pl.get_point(n - 1)).magnitude_squared() < 1e-10)
        --n;
    std::vector<bool> conv;
    conv.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        const size_t prev = i == 0 ? n - 1 : i - 1;
        const size_t next = i + 1 == n ? 0 : i + 1;
        const Point pi = pl.get_point(i);
        Vector d0 = pi - pl.get_point(prev);
        d0.normalize_self();
        Vector d1 = pl.get_point(next) - pi;
        d1.normalize_self();
        conv.push_back(d0.cross(d1).dot(normal) >= 0.0);
    }
    return conv;
}

/// Slide edge `edge_id` of a closed polyline: its start back by s0, its end forward by s1, closing vertex kept in sync.
void extend_edge(Polyline& pl, size_t edge_id, double s0, double s1) {
    if (s0 == 0.0 && s1 == 0.0)
        return;
    const size_t n = pl.point_count();
    if (edge_id + 1 >= n)
        return;
    const Point a = pl.get_point(edge_id);
    const Point b = pl.get_point(edge_id + 1);
    const Vector d = b - a;
    const double len = std::sqrt(d.magnitude_squared());
    if (len < 1e-12)
        return;
    const Vector u = d / len;
    std::vector<Point> pts = pl.get_points();
    pts[edge_id] = a - u * s0;
    pts[edge_id + 1] = b + u * s1;
    if (edge_id == 0)
        pts.back() = pts.front();
    else if (edge_id + 1 == n - 1)
        pts.front() = pts.back();
    pl = Polyline(pts);
}

}  // namespace

/// Four side-face rectangles, widened at convex corners and pushed along the face normals; no orient.
void side_removal_ss_e_r_1_port(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    joint.name = "side_removal";
    joint.no_orient = true;

    std::swap(joint.element_a, joint.element_b);
    std::swap(joint.contact.face_a, joint.contact.face_b);
    std::swap(joint.cross_faces[0], joint.cross_faces[1]);
    std::swap(joint.joint_lines[0], joint.joint_lines[1]);

    const int v0 = index_of(elements, joint.element_a);
    const int v1 = index_of(elements, joint.element_b);
    const int f0_0 = joint.contact.face_a;
    const int f1_0 = joint.contact.face_b;

    if (v0 < 0 || v0 >= (int)elements.size() || v1 < 0 || v1 >= (int)elements.size()) {
        ss_e_r_0(joint);
        return;
    }
    if (f0_0 < 0 || f0_0 >= (int)elements[v0]->planes.size() ||
        f1_0 < 0 || f1_0 >= (int)elements[v1]->planes.size() ||
        f0_0 >= (int)elements[v0]->polylines.size() ||
        f1_0 >= (int)elements[v1]->polylines.size()) {
        ss_e_r_0(joint);
        return;
    }

    Vector n0 = elements[v0]->planes[f0_0].z_axis();
    n0.normalize_self();
    Vector n1 = elements[v1]->planes[f1_0].z_axis();
    n1.normalize_self();

    const double s2 = joint.scale[2];
    const Vector f0_0_normal = n0 * s2;
    const Vector f1_0_normal = n1 * (s2 + 2.0);
    const Vector f0_1_normal = n0 * (s2 + 2.0 + joint.shift);

    Polyline pline0 = elements[v0]->polylines[f0_0];
    Polyline pline1 = elements[v1]->polylines[f1_0];

    if (pline0.point_count() == 5 && pline1.point_count() == 5) {
        Vector norm0 = elements[v0]->planes[0].z_axis();
        norm0.normalize_self();
        Vector norm1 = elements[v1]->planes[0].z_axis();
        norm1.normalize_self();
        const std::vector<bool> cc0 = convex_corners(elements[v0]->polylines[0], norm0);
        const std::vector<bool> cc1 = convex_corners(elements[v1]->polylines[0], norm1);
        const double sc0 = joint.scale[0];
        if (!cc0.empty()) {
            const int a_idx = f0_0 - 2;
            const int b_idx = (a_idx + 1) % (int)cc0.size();
            const double sc0_0 = (a_idx >= 0 && a_idx < (int)cc0.size() && cc0[a_idx]) ? sc0 : 0.0;
            const double sc0_1 = (b_idx >= 0 && b_idx < (int)cc0.size() && cc0[b_idx]) ? sc0 : 0.0;
            extend_edge(pline0, 0, sc0_0, sc0_1);
            extend_edge(pline0, 2, sc0_1, sc0_0);
        }
        if (!cc1.empty()) {
            const int a_idx = f1_0 - 2;
            const int b_idx = (a_idx + 1) % (int)cc1.size();
            const double sc1_0 = (a_idx >= 0 && a_idx < (int)cc1.size() && cc1[a_idx]) ? sc0 : 0.0;
            const double sc1_1 = (b_idx >= 0 && b_idx < (int)cc1.size() && cc1[b_idx]) ? sc0 : 0.0;
            extend_edge(pline1, 0, sc1_0, sc1_1);
            extend_edge(pline1, 2, sc1_1, sc1_0);
        }
        const double sv = joint.scale[1];
        extend_edge(pline0, 1, sv, sv);
        extend_edge(pline0, 3, sv, sv);
        extend_edge(pline1, 1, sv, sv);
        extend_edge(pline1, 3, sv, sv);
    }

    const Polyline pline0_moved0 = pline0.translated(f0_0_normal);
    const Polyline pline0_moved1 = pline0.translated(f0_1_normal);
    const Polyline pline1_moved  = pline1.translated(f1_0_normal);

    if (!(joint.shift > 0.0)) {
        joint.m_outlines[0] = { pline0,        pline0 };
        joint.m_outlines[1] = { pline0_moved0, pline0_moved0 };
        joint.f_outlines[0] = { pline1,        pline1 };
        joint.f_outlines[1] = { pline1_moved,  pline1_moved };
        joint.m_cut_types[0] = { wood_cut::mill_project, wood_cut::mill_project };
        joint.m_cut_types[1] = { wood_cut::mill_project, wood_cut::mill_project };
        joint.f_cut_types[0] = { wood_cut::mill_project, wood_cut::mill_project };
        joint.f_cut_types[1] = { wood_cut::mill_project, wood_cut::mill_project };
        return;
    }

    joint.m_outlines[0] = { pline0_moved0, pline0_moved0, pline0, pline0 };
    joint.m_outlines[1] = { pline0_moved1, pline0_moved1, pline0_moved0, pline0_moved0 };
    joint.f_outlines[0] = { pline1,        pline1 };
    joint.f_outlines[1] = { pline1_moved,  pline1_moved };
    joint.m_cut_types[0] = { wood_cut::mill_project, wood_cut::mill_project,
                             wood_cut::mill_project, wood_cut::mill_project };
    joint.m_cut_types[1] = { wood_cut::mill_project, wood_cut::mill_project,
                             wood_cut::mill_project, wood_cut::mill_project };
    joint.f_cut_types[0] = { wood_cut::mill_project, wood_cut::mill_project };
    joint.f_cut_types[1] = { wood_cut::mill_project, wood_cut::mill_project };
}

/// side_removal_ss_e_r_1_port with the merge branch forced off unless merge_with_joint.
void side_removal(WoodJoint& joint,
                  const std::vector<std::shared_ptr<Plate>>& elements,
                  bool merge_with_joint) {
    const double saved_shift = joint.shift;
    if (!merge_with_joint)
        joint.shift = 0.0;
    side_removal_ss_e_r_1_port(joint, elements);
    if (!merge_with_joint)
        joint.shift = saved_shift;
}

// ═══════════════════════════════════════════════════════════════════════════
// Top-to-top drills
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// Both plates found, a first joint volume with 3+ points, an area with min_area+ points.
bool drill_ready(
    const WoodJoint& joint,
    const std::vector<std::shared_ptr<Plate>>& elements,
    int& v0,
    int& v1,
    size_t min_area
) {
    v0 = index_of(elements, joint.element_a);
    v1 = index_of(elements, joint.element_b);
    if (v0 < 0 || v0 >= (int)elements.size() || v1 < 0 || v1 >= (int)elements.size())
        return false;
    if (!joint.joint_volumes_pair_a_pair_b[0])
        return false;
    if (joint.joint_volumes_pair_a_pair_b[0]->point_count() < 3)
        return false;
    return joint.contact.area.point_count() >= min_area;
}

/// Centroid over every vertex, the closing duplicate included (Polyline::center() drops it).
Point area_centroid(const Polyline& area) {
    double sx = 0;
    double sy = 0;
    double sz = 0;
    for (size_t k = 0; k < area.point_count(); k++) {
        const Point p = area[k];
        sx += p[0];
        sy += p[1];
        sz += p[2];
    }
    const double n = static_cast<double>(area.point_count());
    return Point(sx / n, sy / n, sz / n);
}

/// dir0: the first volume's [1]->[2] edge, unit, times plate v0's thickness; dir1: the reverse times v1's.
void drill_axes(const WoodJoint& joint, double t0, double t1, Vector& dir0, Vector& dir1) {
    const Polyline& jv0 = *joint.joint_volumes_pair_a_pair_b[0];
    dir0 = jv0.get_point(1) - jv0.get_point(2);
    dir0.normalize_self();
    dir1 = -dir0;
    dir0 = dir0 * t0;
    dir1 = dir1 * t1;
}

/// One two-point drill line per point on every face, twice per face as the merge expects.
void emit_drills(WoodJoint& joint, const std::vector<Point>& points, const Vector& dir0, const Vector& dir1) {
    for (int f = 0; f < 2; f++) {
        joint.m_outlines[f].clear();
        joint.f_outlines[f].clear();
        joint.m_cut_types[f].clear();
        joint.f_cut_types[f].clear();
        joint.m_outlines[f].reserve(points.size() * 2);
        joint.f_outlines[f].reserve(points.size() * 2);
        joint.m_cut_types[f].reserve(points.size() * 2);
        joint.f_cut_types[f].reserve(points.size() * 2);
    }
    for (const Point& pt : points) {
        const Polyline line0({pt, pt + dir0});
        const Polyline line1({pt, pt + dir1});
        for (int f = 0; f < 2; f++) {
            joint.f_outlines[f].push_back(line0);
            joint.f_outlines[f].push_back(line0);
            joint.m_outlines[f].push_back(line1);
            joint.m_outlines[f].push_back(line1);
            joint.m_cut_types[f].push_back(wood_cut::drill);
            joint.m_cut_types[f].push_back(wood_cut::drill);
            joint.f_cut_types[f].push_back(wood_cut::drill);
            joint.f_cut_types[f].push_back(wood_cut::drill);
        }
    }
}

/// The area offset inward by shift, divided every division_distance; the last vertex too when the ring is open.
std::vector<Point> offset_boundary_points(
    const Polyline& area,
    double shift,
    double division_distance,
    double open_tolerance
) {
    Polyline poly = area;
    Point origin;
    Plane plane;
    poly.get_fast_plane(origin, plane);
    const double offset_distance = -shift;
    Intersection::offset_in_3d(poly, plane, offset_distance);
    std::vector<Point> points;
    for (size_t i = 0; i + 1 < poly.point_count(); i++) {
        const double seg_len = Point::distance(poly[i], poly[i + 1]);
        const int divisions = (int)std::min(100.0, seg_len / division_distance);
        const std::vector<Point> dp = Polyline::interpolate_points(poly[i], poly[i + 1], divisions, 2);
        points.insert(points.end(), dp.begin(), dp.end());
    }
    if (poly.point_count() > 0) {
        const Vector gap = poly[0] - poly[poly.point_count() - 1];
        if (gap.magnitude_squared() > open_tolerance)
            points.push_back(poly[poly.point_count() - 1]);
    }
    return points;
}

/// One drill through the area centroid.
void centroid_drill(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    int v0;
    int v1;
    if (!drill_ready(joint, elements, v0, v1, 3))
        return;
    Vector dir0;
    Vector dir1;
    drill_axes(joint, elements[v0]->thickness, elements[v1]->thickness, dir0, dir1);
    emit_drills(joint, {area_centroid(joint.contact.area)}, dir0, dir1);
}

/// Drills along the offset area boundary.
void boundary_drill(
    WoodJoint& joint,
    const std::vector<std::shared_ptr<Plate>>& elements,
    double division_distance,
    double open_tolerance
) {
    int v0;
    int v1;
    if (!drill_ready(joint, elements, v0, v1, 4))
        return;
    if (division_distance <= 0.0)
        return;
    const std::vector<Point> points = offset_boundary_points(joint.contact.area, joint.shift, division_distance, open_tolerance);
    Vector dir0;
    Vector dir1;
    drill_axes(joint, elements[v0]->thickness, elements[v1]->thickness, dir0, dir1);
    emit_drills(joint, points, dir0, dir1);
}

}  // namespace

/// Single centroid drill.
void tt_e_p_0(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    joint.name = "tt_e_p_0";
    joint.no_orient = true;
    centroid_drill(joint, elements);
}

/// Single drill at the visual centre, approximated by the centroid.
void tt_e_p_1(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    joint.name = "tt_e_p_1";
    joint.no_orient = true;
    centroid_drill(joint, elements);
}

/// division_length drills on a circle of radius shift around the centroid, in the area plane.
void tt_e_p_2(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    joint.name = "tt_e_p_2";
    joint.no_orient = true;
    int v0;
    int v1;
    if (!drill_ready(joint, elements, v0, v1, 3))
        return;

    const double radius = joint.shift;
    const int n_pts = std::max(1, std::min(100, (int)joint.division_length));
    const Point center = area_centroid(joint.contact.area);
    Point origin;
    Plane plane;
    joint.contact.area.get_fast_plane(origin, plane);
    Vector zp = plane.z_axis();
    zp.normalize_self();
    Vector xp = plane.x_axis();
    xp.normalize_self();
    Vector yp = zp.cross(xp);
    yp.normalize_self();

    std::vector<Point> points;
    if (radius < 1e-9 || n_pts <= 1) {
        points.push_back(center);
    } else {
        for (int i = 0; i < n_pts; ++i) {
            const double angle = 2.0 * Tolerance::PI * i / n_pts;
            const double cx = std::cos(angle) * radius;
            const double cy = std::sin(angle) * radius;
            points.push_back(center + xp * cx + yp * cy);
        }
    }

    Vector dir0;
    Vector dir1;
    drill_axes(joint, elements[v0]->thickness, elements[v1]->thickness, dir0, dir1);
    emit_drills(joint, points, dir0, dir1);
}

/// Drill grid along the offset area boundary; open rings by the runtime DISTANCE_SQUARED.
void tt_e_p_3(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    joint.name = "tt_e_p_3";
    joint.no_orient = true;
    boundary_drill(joint, elements, joint.division_length, wood_session::globals::DISTANCE_SQUARED);
}

/// Boundary drills, open rings by a fixed 0.01.
void tt_e_p_4(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    joint.name = "tt_e_p_4";
    joint.no_orient = true;
    boundary_drill(joint, elements, joint.division_length, 0.01);
}

/// Boundary drills with the division length taken absolute.
void tt_e_p_5(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    joint.name = "tt_e_p_5";
    joint.no_orient = true;
    boundary_drill(joint, elements, std::abs(joint.division_length), 0.01);
}

} // namespace wood_session
