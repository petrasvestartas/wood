#include "pch.h"
#include "wood_serialization.h"
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
// FeaturePlate - Geometry
// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════
// FeaturePlate - Geometry
// ═══════════════════════════════════════════════════════════════════════════

const std::string& FeaturePlate::feature_guid(int side) const {

    std::string& id = feature_guids[side];
    if (id.empty())
        id = ::guid();

    return id;
}

void FeaturePlate::sync_features() {
    for (int side = 0; side < 2; ++side) {
        ElementFeature& f = element_features[side];
        f.guid() = feature_guid(side);
        f.feature_type = "joint";
        f.name = name.empty() ? "joint_" + std::to_string(joint_type) : name;
        f.face_index = side == 0 ? contact.face_a : contact.face_b;

        const std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? male_outlines : female_outlines;
        f.outlines.clear();
        f.outlines.reserve(outlines[0].size() + outlines[1].size());
        for (int face = 0; face < 2; ++face)
            f.outlines.insert(f.outlines.end(), outlines[face].begin(), outlines[face].end());
    }
}

/// Syncs a scratch copy that carries this joint's feature guids, so a const joint reads fresh and keeps its identity.
std::array<ElementFeature, 2> FeaturePlate::to_features() const {

    FeaturePlate scratch = *this;
    scratch.feature_guids = {feature_guid(0), feature_guid(1)};
    scratch.sync_features();

    return std::move(scratch.element_features);
}

// ═══════════════════════════════════════════════════════════════════════════
// FeaturePlate - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// The protobuf message, printed.
nlohmann::ordered_json FeaturePlate::jsondump() const {

    wood_proto::FeaturePlate proto;
    proto.ParseFromString(pb_dumps());

    return json_of(proto);
}

/// The protobuf message, parsed.
FeaturePlate FeaturePlate::jsonload(const nlohmann::json& data) {

    wood_proto::FeaturePlate proto;
    message_from_json(data, proto);

    return pb_loads(proto.SerializeAsString());
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
    proto.set_guid(guid);
    proto.set_element_a(element_a);
    proto.set_element_b(element_b);
    proto.mutable_contact()->ParseFromString(contact.pb_dumps());
    proto.set_joint_type(joint_type);
    proto.set_name(name);
    for (int k = 0; k < 2; ++k) {
        proto.add_cross_faces(cross_faces[k]);
        proto.add_joint_lines()->ParseFromString(joint_lines[k].pb_dumps());
        rings_pb(male_outlines[k], proto.add_male_outlines());
        rings_pb(female_outlines[k], proto.add_female_outlines());
        proto.add_male_fabrication_types()->mutable_values()->Add(male_fabrication_types[k].begin(), male_fabrication_types[k].end());
        proto.add_female_fabrication_types()->mutable_values()->Add(female_fabrication_types[k].begin(), female_fabrication_types[k].end());
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
    for (const std::string& linked : linked_joints)
        proto.add_linked_joints(linked);
    for (const std::vector<std::array<int, 4>>& group : linked_joints_seq) {
        wood_proto::IntList* list = proto.add_linked_joints_seq();
        for (const std::array<int, 4>& q : group)
            list->mutable_values()->Add(q.begin(), q.end());
    }
    proto.set_link(link);
    proto.set_no_orient(no_orient);
    for (const ElementFeature& feature : to_features())
        proto.add_element_features()->ParseFromString(feature.pb_dumps());

    return proto.SerializeAsString();
}

FeaturePlate FeaturePlate::pb_loads(const std::string& data) {

    wood_proto::FeaturePlate proto;
    proto.ParseFromString(data);

    FeaturePlate j;
    j.guid = proto.guid();
    j.element_a = proto.element_a();
    j.element_b = proto.element_b();
    if (proto.has_contact())
        j.contact = ContactFace::pb_loads(proto.contact().SerializeAsString());
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
        if (k < proto.male_fabrication_types_size())
            j.male_fabrication_types[k].assign(proto.male_fabrication_types(k).values().begin(), proto.male_fabrication_types(k).values().end());
        if (k < proto.female_fabrication_types_size())
            j.female_fabrication_types[k].assign(proto.female_fabrication_types(k).values().begin(), proto.female_fabrication_types(k).values().end());
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
    for (int k = 0; k < 2 && k < proto.element_features_size(); ++k) {
        j.element_features[k] = ElementFeature::pb_loads(proto.element_features(k).SerializeAsString());
        j.feature_guids[k] = proto.element_features(k).guid();
    }

    return j;
}

// ═══════════════════════════════════════════════════════════════════════════
// FeaturePlate - String
// ═══════════════════════════════════════════════════════════════════════════

std::string FeaturePlate::str() const {
    return fmt::format("FeaturePlate(type={}, elements=({},{}), faces=({},{}), name={})", joint_type, element_a, element_b, contact.face_a, contact.face_b, name.empty() ? "-" : name);
}

} // namespace wood_session
