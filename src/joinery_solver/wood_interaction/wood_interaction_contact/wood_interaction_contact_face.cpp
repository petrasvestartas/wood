#include "pch.h"
#include "wood_serialization.h"
#include "wood_interaction_contact_face.h"
#include "interaction_contact_face.pb.h"
using namespace session_cpp;

namespace wood_session {

WoodSession& ContactFace::session() const {

    if (!_session)
        throw std::logic_error("ContactFace::session: the record is not in a scene");

    return *_session;
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactFace - Constructors
// ═══════════════════════════════════════════════════════════════════════════

ContactFace::ContactFace(int face_a, int face_b, ContactType type, Polyline polygon)
    : face_a(face_a), face_b(face_b), type(type), polygon(std::move(polygon)) {}

// ═══════════════════════════════════════════════════════════════════════════
// ContactFace - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const ContactFace& contact) {
    return os << contact.str();
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactFace - Geometry
// ═══════════════════════════════════════════════════════════════════════════

ContactFace ContactFace::flipped() const {
    return ContactFace(face_b, face_a, type, polygon);
}

bool ContactFace::coincides(const ContactFace& other) const {
    return face_a == other.face_a && face_b == other.face_b && type == other.type;
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactFace - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// The protobuf message, printed.
nlohmann::ordered_json ContactFace::jsondump() const {

    wood_proto::ContactFace proto;
    proto.ParseFromString(pb_dumps());

    return json_of(proto);
}

/// The protobuf message, parsed.
ContactFace ContactFace::jsonload(const nlohmann::json& data) {

    wood_proto::ContactFace proto;
    message_from_json(data, proto);

    return pb_loads(proto.SerializeAsString());
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactFace - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string ContactFace::pb_dumps() const {

    wood_proto::ContactFace proto;
    proto.set_face_a(face_a);
    proto.set_face_b(face_b);
    proto.set_type(static_cast<int>(type));
    proto.mutable_polygon()->ParseFromString(polygon.pb_dumps());

    return proto.SerializeAsString();
}

ContactFace ContactFace::pb_loads(const std::string& data) {

    wood_proto::ContactFace proto;
    proto.ParseFromString(data);

    ContactFace contact;
    contact.face_a = proto.face_a();
    contact.face_b = proto.face_b();
    contact.type = static_cast<ContactType>(proto.type());
    if (proto.has_polygon())
        contact.polygon = Polyline::pb_loads(proto.polygon().SerializeAsString());

    return contact;
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactFace - String
// ═══════════════════════════════════════════════════════════════════════════

std::string ContactFace::str() const {
    return fmt::format("ContactFace(face_a={}, face_b={}, type={}, points={})", face_a, face_b, static_cast<int>(type), polygon.point_count());
}

} // namespace wood_session
