#include "pch.h"
#include "wood_interaction_feature_plate.h"
#include "interaction_feature_plate.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// FeaturePlate - Constructors
// ═══════════════════════════════════════════════════════════════════════════

FeaturePlate::FeaturePlate()
    : joint_lines{Line::from_points(Point(0, 0, 0), Point(0, 0, 0)), Line::from_points(Point(0, 0, 0), Point(0, 0, 0))} {}

// ═══════════════════════════════════════════════════════════════════════════
// FeaturePlate - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const FeaturePlate& feature) { return os << feature.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// FeaturePlate - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// Rings as an array of kernel polylines.
static nlohmann::ordered_json rings_json(const std::vector<Polyline>& rings) {

    nlohmann::ordered_json array = nlohmann::ordered_json::array();
    for (const Polyline& ring : rings)
        array.push_back(ring.jsondump());

    return array;
}

/// Rings read back from an array of kernel polylines.
static std::vector<Polyline> rings_from_json(const nlohmann::json& data) {

    std::vector<Polyline> rings;
    for (const nlohmann::json& ring : data)
        rings.push_back(Polyline::jsonload(ring));

    return rings;
}

nlohmann::ordered_json FeaturePlate::jsondump() const {

    using nlohmann::ordered_json;

    ordered_json volumes = ordered_json::array();
    for (const std::optional<Polyline>& v : joint_volumes)
        volumes.push_back(v.has_value() ? v->jsondump() : ordered_json(nullptr));

    ordered_json seq = ordered_json::array();
    for (const std::vector<std::array<int, 4>>& group : linked_joints_seq) {
        ordered_json g = ordered_json::array();
        for (const std::array<int, 4>& q : group)
            g.push_back({q[0], q[1], q[2], q[3]});
        seq.push_back(g);
    }

    return ordered_json{
        {"type", "FeaturePlate"},
        {"joint_type", joint_type},
        {"name", name},
        {"cross_faces", {cross_faces[0], cross_faces[1]}},
        {"joint_lines", {joint_lines[0].jsondump(), joint_lines[1].jsondump()}},
        {"joint_volumes", volumes},
        {"male_outlines", {rings_json(male_outlines[0]), rings_json(male_outlines[1])}},
        {"female_outlines", {rings_json(female_outlines[0]), rings_json(female_outlines[1])}},
        {"male_cut_types", {male_cut_types[0], male_cut_types[1]}},
        {"female_cut_types", {female_cut_types[0], female_cut_types[1]}},
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
    };
}

FeaturePlate FeaturePlate::jsonload(const nlohmann::json& data) {

    FeaturePlate j;
    j.joint_type = data.value("joint_type", 0);
    j.name = data.value("name", std::string());
    if (data.contains("cross_faces"))
        j.cross_faces = {data["cross_faces"][0], data["cross_faces"][1]};

    if (data.contains("joint_lines")) {
        j.joint_lines[0] = Line::jsonload(data["joint_lines"][0]);
        j.joint_lines[1] = Line::jsonload(data["joint_lines"][1]);
    }

    if (data.contains("joint_volumes")) {
        size_t k = 0;
        for (const nlohmann::json& v : data["joint_volumes"]) {
            if (k >= 4)
                break;
            if (!v.is_null())
                j.joint_volumes[k] = Polyline::jsonload(v);
            ++k;
        }
    }

    for (int face = 0; face < 2; ++face) {
        if (data.contains("male_outlines"))
            j.male_outlines[face] = rings_from_json(data["male_outlines"][face]);
        if (data.contains("female_outlines"))
            j.female_outlines[face] = rings_from_json(data["female_outlines"][face]);
        if (data.contains("male_cut_types"))
            j.male_cut_types[face] = data["male_cut_types"][face].get<std::vector<int>>();
        if (data.contains("female_cut_types"))
            j.female_cut_types[face] = data["female_cut_types"][face].get<std::vector<int>>();
    }

    j.divisions = data.value("divisions", 1);
    j.shift = data.value("shift", 0.5);
    j.length = data.value("length", 0.0);
    j.division_length = data.value("division_length", 0.0);
    if (data.contains("scale"))
        j.scale = {data["scale"][0], data["scale"][1], data["scale"][2]};
    j.unit_scale = data.value("unit_scale", false);
    j.unit_scale_distance = data.value("unit_scale_distance", 0.0);

    if (data.contains("linked_joints"))
        j.linked_joints = data["linked_joints"].get<std::vector<int>>();

    if (data.contains("linked_joints_seq")) {
        for (const nlohmann::json& group : data["linked_joints_seq"]) {
            std::vector<std::array<int, 4>> g;
            for (const nlohmann::json& q : group)
                g.push_back({q[0], q[1], q[2], q[3]});
            j.linked_joints_seq.push_back(std::move(g));
        }
    }

    j.link = data.value("link", false);
    j.no_orient = data.value("no_orient", false);

    return j;
}

// ═══════════════════════════════════════════════════════════════════════════
// FeaturePlate - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

/// Rings into a PolylineList.
static void rings_pb(const std::vector<Polyline>& rings, wood_proto::PolylineList* list) {
    for (const Polyline& ring : rings)
        list->add_items()->ParseFromString(ring.pb_dumps());
}

/// Rings read back from a PolylineList.
static std::vector<Polyline> rings_from_pb(const wood_proto::PolylineList& list) {

    std::vector<Polyline> rings;
    for (const session_proto::Polyline& ring : list.items())
        rings.push_back(Polyline::pb_loads(ring.SerializeAsString()));

    return rings;
}

std::string FeaturePlate::pb_dumps() const {

    wood_proto::FeaturePlate proto;
    proto.set_joint_type(joint_type);
    proto.set_name(name);
    for (int k = 0; k < 2; ++k) {
        proto.add_cross_faces(cross_faces[k]);
        proto.add_joint_lines()->ParseFromString(joint_lines[k].pb_dumps());
        rings_pb(male_outlines[k], proto.add_male_outlines());
        rings_pb(female_outlines[k], proto.add_female_outlines());
        proto.add_male_cut_types()->mutable_values()->Add(male_cut_types[k].begin(), male_cut_types[k].end());
        proto.add_female_cut_types()->mutable_values()->Add(female_cut_types[k].begin(), female_cut_types[k].end());
    }

    for (const std::optional<Polyline>& volume : joint_volumes) {
        session_proto::Polyline* slot = proto.add_joint_volumes();
        if (volume.has_value())
            slot->ParseFromString(volume->pb_dumps());
    }

    proto.set_divisions(divisions);
    proto.set_shift(shift);
    proto.set_length(length);
    proto.set_division_length(division_length);
    proto.mutable_scale()->Add(scale.begin(), scale.end());
    proto.set_unit_scale(unit_scale);
    proto.set_unit_scale_distance(unit_scale_distance);
    proto.mutable_linked_joints()->Add(linked_joints.begin(), linked_joints.end());
    for (const std::vector<std::array<int, 4>>& group : linked_joints_seq) {
        wood_proto::IntList* list = proto.add_linked_joints_seq();
        for (const std::array<int, 4>& q : group)
            list->mutable_values()->Add(q.begin(), q.end());
    }
    proto.set_link(link);
    proto.set_no_orient(no_orient);

    return proto.SerializeAsString();
}

FeaturePlate FeaturePlate::pb_loads(const std::string& data) {

    wood_proto::FeaturePlate proto;
    proto.ParseFromString(data);

    FeaturePlate j;
    j.joint_type = proto.joint_type();
    j.name = proto.name();
    for (int k = 0; k < 2; ++k) {
        if (k < proto.cross_faces_size())
            j.cross_faces[k] = proto.cross_faces(k);
        if (k < proto.joint_lines_size())
            j.joint_lines[k] = Line::pb_loads(proto.joint_lines(k).SerializeAsString());
        if (k < proto.male_outlines_size())
            j.male_outlines[k] = rings_from_pb(proto.male_outlines(k));
        if (k < proto.female_outlines_size())
            j.female_outlines[k] = rings_from_pb(proto.female_outlines(k));
        if (k < proto.male_cut_types_size())
            j.male_cut_types[k].assign(proto.male_cut_types(k).values().begin(), proto.male_cut_types(k).values().end());
        if (k < proto.female_cut_types_size())
            j.female_cut_types[k].assign(proto.female_cut_types(k).values().begin(), proto.female_cut_types(k).values().end());
    }

    for (int k = 0; k < 4 && k < proto.joint_volumes_size(); ++k)
        if (proto.joint_volumes(k).coords_size() > 0)
            j.joint_volumes[k] = Polyline::pb_loads(proto.joint_volumes(k).SerializeAsString());

    j.divisions = proto.divisions();
    j.shift = proto.shift();
    j.length = proto.length();
    j.division_length = proto.division_length();
    for (int k = 0; k < 3 && k < proto.scale_size(); ++k)
        j.scale[k] = proto.scale(k);
    j.unit_scale = proto.unit_scale();
    j.unit_scale_distance = proto.unit_scale_distance();
    j.linked_joints.assign(proto.linked_joints().begin(), proto.linked_joints().end());
    for (const wood_proto::IntList& list : proto.linked_joints_seq()) {
        std::vector<std::array<int, 4>> group;
        for (int q = 0; q + 3 < list.values_size(); q += 4)
            group.push_back({list.values(q), list.values(q + 1), list.values(q + 2), list.values(q + 3)});
        j.linked_joints_seq.push_back(std::move(group));
    }
    j.link = proto.link();
    j.no_orient = proto.no_orient();

    return j;
}

// ═══════════════════════════════════════════════════════════════════════════
// FeaturePlate - String
// ═══════════════════════════════════════════════════════════════════════════

std::string FeaturePlate::str() const {
    return fmt::format("FeaturePlate(type={}, name={}, divisions={})", joint_type, name.empty() ? "-" : name, divisions);
}

} // namespace wood_session
