#include "pch.h"
#include "wood_serialization.h"
#include "wood_interaction_contact_axis.h"
#include "interaction_contact_axis.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// ContactAxis - Constructors
// ═══════════════════════════════════════════════════════════════════════════

ContactAxis::ContactAxis(Line segment, double t_a, double t_b, int polyline_a, int segment_a, int polyline_b, int segment_b)
    : segment(std::move(segment)), t_a(t_a), t_b(t_b), polyline_a(polyline_a), segment_a(segment_a), polyline_b(polyline_b), segment_b(segment_b) {}

// ═══════════════════════════════════════════════════════════════════════════
// ContactAxis - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const ContactAxis& contact) { return os << contact.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// ContactAxis - Geometry
// ═══════════════════════════════════════════════════════════════════════════

ContactAxis ContactAxis::flipped() const {
    return ContactAxis(Line::from_points(segment.end(), segment.start()), t_b, t_a, polyline_b, segment_b, polyline_a, segment_a);
}

bool ContactAxis::coincides(const ContactAxis& other) const {
    return polyline_a == other.polyline_a && segment_a == other.segment_a && polyline_b == other.polyline_b && segment_b == other.segment_b;
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactAxis - JSON
// ═══════════════════════════════════════════════════════════════════════════

/// The protobuf message, printed.
nlohmann::ordered_json ContactAxis::jsondump() const {

    wood_proto::ContactAxis proto;
    proto.ParseFromString(pb_dumps());

    return json_of(proto);
}

/// The protobuf message, parsed.
ContactAxis ContactAxis::jsonload(const nlohmann::json& data) {

    wood_proto::ContactAxis proto;
    message_from_json(data, proto);

    return pb_loads(proto.SerializeAsString());
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactAxis - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string ContactAxis::pb_dumps() const {

    wood_proto::ContactAxis proto;
    proto.mutable_segment()->ParseFromString(segment.pb_dumps());
    proto.set_t_a(t_a);
    proto.set_t_b(t_b);
    proto.set_polyline_a(polyline_a);
    proto.set_segment_a(segment_a);
    proto.set_polyline_b(polyline_b);
    proto.set_segment_b(segment_b);

    return proto.SerializeAsString();
}

ContactAxis ContactAxis::pb_loads(const std::string& data) {

    wood_proto::ContactAxis proto;
    proto.ParseFromString(data);

    ContactAxis contact;
    if (proto.has_segment())
        contact.segment = Line::pb_loads(proto.segment().SerializeAsString());
    contact.t_a = proto.t_a();
    contact.t_b = proto.t_b();
    contact.polyline_a = proto.polyline_a();
    contact.segment_a = proto.segment_a();
    contact.polyline_b = proto.polyline_b();
    contact.segment_b = proto.segment_b();

    return contact;
}

// ═══════════════════════════════════════════════════════════════════════════
// ContactAxis - String
// ═══════════════════════════════════════════════════════════════════════════

std::string ContactAxis::str() const {
    return fmt::format("ContactAxis(a=({}, {}, {:.3f}), b=({}, {}, {:.3f}), length={:.3f})", polyline_a, segment_a, t_a, polyline_b, segment_b, t_b, segment.length());
}

} // namespace wood_session
