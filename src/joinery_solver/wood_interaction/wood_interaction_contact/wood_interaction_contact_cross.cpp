#include "pch.h"
#include "wood_interaction_contact_cross.h"
#include "interaction_contact_cross.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactCross - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionContactCross::kind() const {
    return "cross";
}

std::shared_ptr<InteractionContact> InteractionContactCross::flipped() const {

    std::shared_ptr<InteractionContactCross> out = std::make_shared<InteractionContactCross>(*this);
    std::swap(out->faces_a, out->faces_b);
    std::swap(out->lines[0], out->lines[1]);

    return out;
}

bool InteractionContactCross::coincides(const InteractionContact& other) const {

    const InteractionContactCross* cross = dynamic_cast<const InteractionContactCross*>(&other);

    return cross && faces_a == cross->faces_a && faces_b == cross->faces_b;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactCross - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionContactCross::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionContactCross::interaction_data_dumps() const {

    wood_proto::InteractionContactCross proto;

    for (int k = 0; k < 2; ++k) {
        proto.add_faces_a(faces_a[k]);
        proto.add_faces_b(faces_b[k]);
        proto.add_lines()->ParseFromString(lines[k].pb_dumps());
        proto.add_volumes()->ParseFromString(volumes[k].pb_dumps());
    }

    proto.mutable_polygon()->ParseFromString(polygon.pb_dumps());

    return proto.SerializeAsString();
}

InteractionContactCross InteractionContactCross::interaction_data_loads(const std::string& data) {

    wood_proto::InteractionContactCross proto;
    proto.ParseFromString(data);

    InteractionContactCross contact;

    for (int k = 0; k < 2; ++k) {

        if (k < proto.faces_a_size())
            contact.faces_a[k] = proto.faces_a(k);

        if (k < proto.faces_b_size())
            contact.faces_b[k] = proto.faces_b(k);

        if (k < proto.lines_size())
            contact.lines[k] = Polyline::pb_loads(proto.lines(k).SerializeAsString());

        if (k < proto.volumes_size())
            contact.volumes[k] = Polyline::pb_loads(proto.volumes(k).SerializeAsString());
    }

    if (proto.has_polygon())
        contact.polygon = Polyline::pb_loads(proto.polygon().SerializeAsString());

    return contact;
}

std::shared_ptr<Interaction> InteractionContactCross::clone() const {
    return std::make_shared<InteractionContactCross>(*this);
}

/// The registered factory: interaction_data bytes to a cross contact.
static std::shared_ptr<Interaction> contact_cross_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionContactCross>(InteractionContactCross::interaction_data_loads(data));
}

void InteractionContactCross::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), contact_cross_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactCross - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionContactCross::str() const {
    return fmt::format("InteractionContactCross(faces_a=({}, {}), faces_b=({}, {}), points={})", faces_a[0], faces_a[1], faces_b[0], faces_b[1], polygon.point_count());
}

} // namespace wood_session
