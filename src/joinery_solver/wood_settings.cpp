#include "pch.h"
#include "wood_settings.h"
#include "settings.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Settings - Geometry
// ═══════════════════════════════════════════════════════════════════════════

const std::array<std::vector<Polyline>, 2>& Settings::custom(const std::string& family) const {

    static const std::array<std::vector<Polyline>, 2> none;
    const auto found = custom_joints.find(family);

    return found == custom_joints.end() ? none : found->second;
}

// ═══════════════════════════════════════════════════════════════════════════
// Settings - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// Outlines as an array of kernel polylines.
static nlohmann::ordered_json outlines_json(const std::vector<Polyline>& outlines) {

    nlohmann::ordered_json array = nlohmann::ordered_json::array();
    for (const Polyline& outline : outlines)
        array.push_back(outline.jsondump());

    return array;
}

/// Outlines read back from an array of kernel polylines.
static std::vector<Polyline> outlines_from_json(const nlohmann::json& data) {

    std::vector<Polyline> outlines;
    for (const nlohmann::json& outline : data)
        outlines.push_back(Polyline::jsonload(outline));

    return outlines;
}

nlohmann::ordered_json Settings::jsondump() const {

    nlohmann::ordered_json custom = nlohmann::ordered_json::array();
    for (const auto& [family, outlines] : custom_joints)
        custom.push_back({{"family", family}, {"male", outlines_json(outlines[0])}, {"female", outlines_json(outlines[1])}});

    return nlohmann::ordered_json{
        {"type", "Settings"},
        {"search_type", static_cast<int>(search_type)},
        {"joint_parameters", joint_parameters},
        {"joint_volume_extension", joint_volume_extension},
        {"joint_scale", {joint_scale[0], joint_scale[1], joint_scale[2]}},
        {"dihedral_angle", dihedral_angle},
        {"all_treated_as_rotated", all_treated_as_rotated},
        {"rotated_joint_as_average", rotated_joint_as_average},
        {"distance", distance},
        {"distance_squared", distance_squared},
        {"angle", angle},
        {"duplicate_points_tolerance", duplicate_points_tolerance},
        {"limit_min_joint_length", limit_min_joint_length},
        {"clipper_scale", clipper_scale},
        {"clipper_area", clipper_area},
        {"beams", beams},
        {"custom_joints", custom},
    };
}

Settings Settings::jsonload(const nlohmann::json& data) {

    Settings s;
    s.search_type = static_cast<SearchType>(data.value("search_type", 0));
    if (data.contains("joint_parameters"))
        s.joint_parameters = data["joint_parameters"].get<std::vector<double>>();
    if (data.contains("joint_volume_extension"))
        s.joint_volume_extension = data["joint_volume_extension"].get<std::vector<double>>();
    if (data.contains("joint_scale"))
        s.joint_scale = {data["joint_scale"][0], data["joint_scale"][1], data["joint_scale"][2]};
    s.dihedral_angle = data.value("dihedral_angle", s.dihedral_angle);
    s.all_treated_as_rotated = data.value("all_treated_as_rotated", false);
    s.rotated_joint_as_average = data.value("rotated_joint_as_average", false);
    s.distance = data.value("distance", s.distance);
    s.distance_squared = data.value("distance_squared", s.distance_squared);
    s.angle = data.value("angle", s.angle);
    s.duplicate_points_tolerance = data.value("duplicate_points_tolerance", 0.0);
    s.limit_min_joint_length = data.value("limit_min_joint_length", 0.0);
    s.clipper_scale = data.value("clipper_scale", s.clipper_scale);
    s.clipper_area = data.value("clipper_area", s.clipper_area);
    if (data.contains("beams"))
        s.beams = data["beams"].get<std::vector<double>>();
    if (data.contains("custom_joints"))
        for (const nlohmann::json& entry : data["custom_joints"])
            s.custom_joints[entry.value("family", std::string())] = {outlines_from_json(entry["male"]), outlines_from_json(entry["female"])};

    return s;
}

// ═══════════════════════════════════════════════════════════════════════════
// Settings - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Settings::pb_dumps() const {

    wood_proto::Settings proto;
    proto.set_search_type(static_cast<int>(search_type));
    proto.mutable_joint_parameters()->Add(joint_parameters.begin(), joint_parameters.end());
    proto.mutable_joint_volume_extension()->Add(joint_volume_extension.begin(), joint_volume_extension.end());
    proto.mutable_joint_scale()->Add(joint_scale.begin(), joint_scale.end());
    proto.set_dihedral_angle(dihedral_angle);
    proto.set_all_treated_as_rotated(all_treated_as_rotated);
    proto.set_rotated_joint_as_average(rotated_joint_as_average);
    proto.set_distance(distance);
    proto.set_distance_squared(distance_squared);
    proto.set_angle(angle);
    proto.set_duplicate_points_tolerance(duplicate_points_tolerance);
    proto.set_limit_min_joint_length(limit_min_joint_length);
    proto.set_clipper_scale(clipper_scale);
    proto.set_clipper_area(clipper_area);
    proto.mutable_beams()->Add(beams.begin(), beams.end());
    for (const auto& [family, outlines] : custom_joints) {
        wood_proto::CustomJoint* entry = proto.add_custom_joints();
        entry->set_family(family);
        for (const Polyline& outline : outlines[0])
            entry->add_male()->ParseFromString(outline.pb_dumps());
        for (const Polyline& outline : outlines[1])
            entry->add_female()->ParseFromString(outline.pb_dumps());
    }

    return proto.SerializeAsString();
}

Settings Settings::pb_loads(const std::string& data) {

    wood_proto::Settings proto;
    proto.ParseFromString(data);

    Settings s;
    s.search_type = static_cast<SearchType>(proto.search_type());
    if (proto.joint_parameters_size() > 0)
        s.joint_parameters.assign(proto.joint_parameters().begin(), proto.joint_parameters().end());
    if (proto.joint_volume_extension_size() > 0)
        s.joint_volume_extension.assign(proto.joint_volume_extension().begin(), proto.joint_volume_extension().end());
    for (int k = 0; k < 3 && k < proto.joint_scale_size(); ++k)
        s.joint_scale[k] = proto.joint_scale(k);
    s.dihedral_angle = proto.dihedral_angle();
    s.all_treated_as_rotated = proto.all_treated_as_rotated();
    s.rotated_joint_as_average = proto.rotated_joint_as_average();
    s.distance = proto.distance();
    s.distance_squared = proto.distance_squared();
    s.angle = proto.angle();
    s.duplicate_points_tolerance = proto.duplicate_points_tolerance();
    s.limit_min_joint_length = proto.limit_min_joint_length();
    s.clipper_scale = proto.clipper_scale();
    s.clipper_area = proto.clipper_area();
    s.beams.assign(proto.beams().begin(), proto.beams().end());
    for (const wood_proto::CustomJoint& entry : proto.custom_joints()) {
        std::array<std::vector<Polyline>, 2>& outlines = s.custom_joints[entry.family()];
        for (const session_proto::Polyline& outline : entry.male())
            outlines[0].push_back(Polyline::pb_loads(outline.SerializeAsString()));
        for (const session_proto::Polyline& outline : entry.female())
            outlines[1].push_back(Polyline::pb_loads(outline.SerializeAsString()));
    }

    return s;
}

} // namespace wood_session
