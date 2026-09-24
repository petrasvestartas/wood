#include "pch.h"
#include "wood_serialization.h"
#include "wood_interaction_contact.h"
#include "interaction_contact.pb.h"
using namespace session_cpp;

namespace wood_session {

WoodSession& InteractionContact::session() const {

    if (!_session)
        throw std::logic_error("InteractionContact::session: the record is not in a scene");

    return *_session;
}

void InteractionContact::set_session(WoodSession* scene) {

    _session = scene;

    if (ContactFace* kind = std::get_if<ContactFace>(&data))
        kind->_session = scene;
    else if (ContactAxis* kind = std::get_if<ContactAxis>(&data))
        kind->_session = scene;
    else if (ContactCross* kind = std::get_if<ContactCross>(&data))
        kind->_session = scene;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - Constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionContact::InteractionContact(ContactFace face) : data(std::move(face)) {}

InteractionContact::InteractionContact(ContactAxis axis) : data(std::move(axis)) {}

InteractionContact::InteractionContact(ContactCross cross) : data(std::move(cross)) {}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const InteractionContact& contact) {
    return os << contact.str();
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionContact::kind() const {

    if (face())
        return "face";
    if (axis())
        return "axis";

    return "cross";
}

InteractionContact InteractionContact::flipped() const {

    InteractionContact out;
    out.guid = guid;

    if (const ContactFace* kind = face())
        out.data = kind->flipped();
    else if (const ContactAxis* kind = axis())
        out.data = kind->flipped();
    else if (const ContactCross* kind = cross())
        out.data = kind->flipped();

    return out;
}

bool InteractionContact::coincides(const InteractionContact& other) const {

    if (data.index() != other.data.index())
        return false;

    if (const ContactFace* kind = face())
        return kind->coincides(*other.face());
    if (const ContactAxis* kind = axis())
        return kind->coincides(*other.axis());

    return cross()->coincides(*other.cross());
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// The protobuf message, printed.
nlohmann::ordered_json InteractionContact::jsondump() const {

    wood_proto::InteractionContact proto;
    proto.ParseFromString(pb_dumps());

    return json_of(proto);
}

/// The protobuf message, parsed.
InteractionContact InteractionContact::jsonload(const nlohmann::json& data) {

    wood_proto::InteractionContact proto;
    message_from_json(data, proto);

    return pb_loads(proto.SerializeAsString());
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionContact::pb_dumps() const {

    wood_proto::InteractionContact proto;
    proto.set_guid(guid);
    if (const ContactFace* kind = face())
        proto.mutable_face()->ParseFromString(kind->pb_dumps());
    if (const ContactAxis* kind = axis())
        proto.mutable_axis()->ParseFromString(kind->pb_dumps());
    if (const ContactCross* kind = cross())
        proto.mutable_cross()->ParseFromString(kind->pb_dumps());

    return proto.SerializeAsString();
}

InteractionContact InteractionContact::pb_loads(const std::string& data) {

    wood_proto::InteractionContact proto;
    proto.ParseFromString(data);

    InteractionContact contact;
    contact.guid = proto.guid();
    if (proto.has_axis())
        contact.data = ContactAxis::pb_loads(proto.axis().SerializeAsString());
    else if (proto.has_cross())
        contact.data = ContactCross::pb_loads(proto.cross().SerializeAsString());
    else
        contact.data = ContactFace::pb_loads(proto.face().SerializeAsString());

    return contact;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionContact::str() const {
    return fmt::format("InteractionContact(guid={}, kind={})", guid, kind());
}

} // namespace wood_session
