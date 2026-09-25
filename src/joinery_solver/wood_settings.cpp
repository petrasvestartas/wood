#include "pch.h"
#include "wood_serialization.h"
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

/// The protobuf message, printed.
nlohmann::ordered_json Settings::jsondump() const {

    wood_proto::Settings proto;
    if (!proto.ParseFromString(pb_dumps()))
        throw std::runtime_error("Failed to parse Settings protobuf data");

    return json_of(proto);
}

/// The protobuf message, parsed.
Settings Settings::jsonload(const nlohmann::json& data) {

    wood_proto::Settings proto;
    message_from_json(data, proto);

    return pb_loads(proto.SerializeAsString());
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
            if (!entry->add_male()->ParseFromString(outline.pb_dumps()))
                throw std::runtime_error("Failed to parse Polyline protobuf data");
        for (const Polyline& outline : outlines[1])
            if (!entry->add_female()->ParseFromString(outline.pb_dumps()))
                throw std::runtime_error("Failed to parse Polyline protobuf data");
    }

    return proto.SerializeAsString();
}

Settings Settings::pb_loads(const std::string& data) {

    wood_proto::Settings proto;
    if (!proto.ParseFromString(data))
        throw std::runtime_error("Failed to parse Settings protobuf data");

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
