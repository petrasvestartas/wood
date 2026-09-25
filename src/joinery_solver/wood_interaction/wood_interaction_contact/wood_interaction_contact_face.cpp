#include "pch.h"
#include "wood_interaction_contact_face.h"
#include "interaction_contact_face.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactFace - Constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionContactFace::InteractionContactFace(int face_a, int face_b, ContactType type, Polyline polygon)
    : face_a(face_a), face_b(face_b), type(type), polygon(std::move(polygon)) {}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactFace - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionContactFace::kind() const {
    return "face";
}

std::shared_ptr<InteractionContact> InteractionContactFace::flipped() const {

    std::shared_ptr<InteractionContactFace> out = std::make_shared<InteractionContactFace>(*this);
    std::swap(out->face_a, out->face_b);

    return out;
}

bool InteractionContactFace::coincides(const InteractionContact& other) const {

    const InteractionContactFace* face = dynamic_cast<const InteractionContactFace*>(&other);

    return face && face_a == face->face_a && face_b == face->face_b && type == face->type;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactFace - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionContactFace::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionContactFace::interaction_data_dumps() const {

    wood_proto::InteractionContactFace proto;
    proto.set_face_a(face_a);
    proto.set_face_b(face_b);
    proto.set_type(static_cast<int>(type));
    proto.mutable_polygon()->ParseFromString(polygon.pb_dumps());

    return proto.SerializeAsString();
}

InteractionContactFace InteractionContactFace::interaction_data_loads(const std::string& data) {

    wood_proto::InteractionContactFace proto;
    proto.ParseFromString(data);

    InteractionContactFace contact;
    contact.face_a = proto.face_a();
    contact.face_b = proto.face_b();
    contact.type = static_cast<ContactType>(proto.type());

    if (proto.has_polygon())
        contact.polygon = Polyline::pb_loads(proto.polygon().SerializeAsString());

    return contact;
}

std::shared_ptr<Interaction> InteractionContactFace::clone() const {
    return std::make_shared<InteractionContactFace>(*this);
}

/// The registered factory: interaction_data bytes to a face contact.
static std::shared_ptr<Interaction> contact_face_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionContactFace>(InteractionContactFace::interaction_data_loads(data));
}

void InteractionContactFace::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), contact_face_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactFace - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionContactFace::str() const {
    return fmt::format("InteractionContactFace(face_a={}, face_b={}, type={}, points={})", face_a, face_b, static_cast<int>(type), polygon.point_count());
}

} // namespace wood_session
