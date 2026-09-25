#include "pch.h"
#include "wood_interaction_feature_plate.h"
#include "interaction_feature_plate.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlate - Protobuf helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Rings into a PolylineList.
static void rings_pb(const std::vector<Polyline>& rings, wood_proto::PolylineList* list) {
    for (const Polyline& ring : rings)
        if (!list->add_items()->ParseFromString(ring.pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");
}

/// Rings read back from a PolylineList.
static std::vector<Polyline> rings_from_pb(const wood_proto::PolylineList& list) {

    std::vector<Polyline> rings;

    for (const session_proto::Polyline& ring : list.items())
        rings.push_back(Polyline::pb_loads(ring.SerializeAsString()));

    return rings;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlate - Constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionFeaturePlate::InteractionFeaturePlate()
    : joint_lines{Line::from_points(Point(0, 0, 0), Point(0, 0, 0)), Line::from_points(Point(0, 0, 0), Point(0, 0, 0))} {}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlate - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionFeaturePlate::kind() const {
    return "plate";
}

const std::string& InteractionFeaturePlate::feature_guid(int side) const {

    std::string& id = feature_guids[side];

    if (id.empty())
        id = ::guid();

    return id;
}

void InteractionFeaturePlate::sync_features() {
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
std::array<ElementFeature, 2> InteractionFeaturePlate::to_features() const {

    InteractionFeaturePlate scratch = *this;
    scratch.feature_guids = {feature_guid(0), feature_guid(1)};
    scratch.sync_features();

    return std::move(scratch.element_features);
}

std::shared_ptr<InteractionContact> InteractionFeaturePlate::to_contact() const {

    if (joint_type != 30)
        return std::make_shared<InteractionContactFace>(contact);

    std::shared_ptr<InteractionContactCross> crossing = std::make_shared<InteractionContactCross>();
    crossing->faces_a = {contact.face_a, cross_faces[0]};
    crossing->faces_b = {contact.face_b, cross_faces[1]};
    crossing->polygon = contact.polygon;

    for (int k = 0; k < 2; ++k) {

        crossing->lines[k] = Polyline({joint_lines[k].start(), joint_lines[k].end()});

        if (joint_volumes[k].has_value())
            crossing->volumes[k] = *joint_volumes[k];
    }

    return crossing;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlate - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeaturePlate::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionFeaturePlate::interaction_data_dumps() const {

    wood_proto::InteractionFeaturePlate proto;
    proto.set_contact_guid(contact_guid);
    proto.set_element_a(element_a);
    proto.set_element_b(element_b);
    if (!proto.mutable_contact()->ParseFromString(contact.interaction_data_dumps()))
        throw std::runtime_error("Failed to parse InteractionContactFace protobuf data");
    proto.set_joint_type(joint_type);

    for (int k = 0; k < 2; ++k) {
        proto.add_cross_faces(cross_faces[k]);
        if (!proto.add_joint_lines()->ParseFromString(joint_lines[k].pb_dumps()))
            throw std::runtime_error("Failed to parse Line protobuf data");
        rings_pb(male_outlines[k], proto.add_male_outlines());
        rings_pb(female_outlines[k], proto.add_female_outlines());
        proto.add_male_fabrication_types()->mutable_values()->Add(male_fabrication_types[k].begin(), male_fabrication_types[k].end());
        proto.add_female_fabrication_types()->mutable_values()->Add(female_fabrication_types[k].begin(), female_fabrication_types[k].end());
    }

    for (const std::optional<Polyline>& volume : joint_volumes) {

        session_proto::Polyline* slot = proto.add_joint_volumes();

        if (volume.has_value())
            if (!slot->ParseFromString(volume->pb_dumps()))
                throw std::runtime_error("Failed to parse Polyline protobuf data");
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
        if (!proto.add_element_features()->ParseFromString(feature.pb_dumps()))
            throw std::runtime_error("Failed to parse ElementFeature protobuf data");

    return proto.SerializeAsString();
}

InteractionFeaturePlate InteractionFeaturePlate::interaction_data_loads(const std::string& data) {

    wood_proto::InteractionFeaturePlate proto;
    if (!proto.ParseFromString(data))
        throw std::runtime_error("Failed to parse InteractionFeaturePlate protobuf data");

    InteractionFeaturePlate j;
    j.contact_guid = proto.contact_guid();
    j.element_a = proto.element_a();
    j.element_b = proto.element_b();

    if (proto.has_contact())
        j.contact = InteractionContactFace::interaction_data_loads(proto.contact().SerializeAsString());

    j.joint_type = proto.joint_type();

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

std::shared_ptr<Interaction> InteractionFeaturePlate::clone() const {
    return std::make_shared<InteractionFeaturePlate>(*this);
}

/// The registered factory: interaction_data bytes to a plate joint.
static std::shared_ptr<Interaction> feature_plate_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionFeaturePlate>(InteractionFeaturePlate::interaction_data_loads(data));
}

void InteractionFeaturePlate::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), feature_plate_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeaturePlate - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeaturePlate::str() const {
    return fmt::format("InteractionFeaturePlate(type={}, elements=({},{}), faces=({},{}), name={})", joint_type, element_a, element_b, contact.face_a, contact.face_b, name.empty() ? "-" : name);
}

} // namespace wood_session
