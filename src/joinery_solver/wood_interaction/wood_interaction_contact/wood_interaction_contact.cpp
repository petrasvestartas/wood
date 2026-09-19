#include "pch.h"
#include "wood_interaction_contact.h"
#include "interaction_contact.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - Constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionContact::InteractionContact(ContactFace face) : data(std::move(face)) {}

InteractionContact::InteractionContact(ContactAxis axis) : data(std::move(axis)) {}

InteractionContact::InteractionContact(ContactCross cross) : data(std::move(cross)) {}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const InteractionContact& contact) { return os << contact.str(); }

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
    std::visit([&out](const auto& kind) { out.data = kind.flipped(); }, data);

    return out;
}

bool InteractionContact::coincides(const InteractionContact& other) const {

    if (data.index() != other.data.index())
        return false;

    return std::visit([&other](const auto& kind) { return kind.coincides(std::get<std::decay_t<decltype(kind)>>(other.data)); }, data);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContact - JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json InteractionContact::jsondump() const {
    return nlohmann::ordered_json{
        {"type", "InteractionContact"},
        {"guid", guid},
        {"kind", std::string(kind())},
        {"data", std::visit([](const auto& kind) { return kind.jsondump(); }, data)},
    };
}

InteractionContact InteractionContact::jsonload(const nlohmann::json& data) {

    InteractionContact contact;
    contact.guid = data.value("guid", std::string());
    const std::string kind = data.value("kind", std::string("face"));
    const nlohmann::json& body = data.contains("data") ? data["data"] : nlohmann::json::object();
    if (kind == "axis")
        contact.data = ContactAxis::jsonload(body);
    else if (kind == "cross")
        contact.data = ContactCross::jsonload(body);
    else
        contact.data = ContactFace::jsonload(body);

    return contact;
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
