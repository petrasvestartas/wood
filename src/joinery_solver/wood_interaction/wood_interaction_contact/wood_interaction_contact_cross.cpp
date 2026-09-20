#include "pch.h"
#include "wood_serialization.h"
#include "wood_interaction_contact_cross.h"
#include "interaction_contact_cross.pb.h"
using namespace session_cpp;

namespace wood_session {

WoodSession& ContactCross::session() const {

    if (!_session)
        throw std::logic_error("ContactCross::session: the record is not in a scene");

    return *_session;
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactCross - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const ContactCross& contact) { return os << contact.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// ContactCross - Geometry
// ═══════════════════════════════════════════════════════════════════════════

ContactCross ContactCross::flipped() const {

    ContactCross out = *this;
    std::swap(out.faces_a, out.faces_b);
    std::swap(out.lines[0], out.lines[1]);

    return out;
}

bool ContactCross::coincides(const ContactCross& other) const {
    return faces_a == other.faces_a && faces_b == other.faces_b;
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactCross - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// The protobuf message, printed.
nlohmann::ordered_json ContactCross::jsondump() const {

    wood_proto::ContactCross proto;
    proto.ParseFromString(pb_dumps());

    return json_of(proto);
}

/// The protobuf message, parsed.
ContactCross ContactCross::jsonload(const nlohmann::json& data) {

    wood_proto::ContactCross proto;
    message_from_json(data, proto);

    return pb_loads(proto.SerializeAsString());
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactCross - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string ContactCross::pb_dumps() const {

    wood_proto::ContactCross proto;
    for (int k = 0; k < 2; ++k) {
        proto.add_faces_a(faces_a[k]);
        proto.add_faces_b(faces_b[k]);
        proto.add_lines()->ParseFromString(lines[k].pb_dumps());
        proto.add_volumes()->ParseFromString(volumes[k].pb_dumps());
    }
    proto.mutable_polygon()->ParseFromString(polygon.pb_dumps());

    return proto.SerializeAsString();
}

ContactCross ContactCross::pb_loads(const std::string& data) {

    wood_proto::ContactCross proto;
    proto.ParseFromString(data);

    ContactCross contact;
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

// ═══════════════════════════════════════════════════════════════════════════
// ContactCross - String
// ═══════════════════════════════════════════════════════════════════════════

std::string ContactCross::str() const {
    return fmt::format("ContactCross(faces_a=({}, {}), faces_b=({}, {}), points={})", faces_a[0], faces_a[1], faces_b[0], faces_b[1], polygon.point_count());
}

} // namespace wood_session
