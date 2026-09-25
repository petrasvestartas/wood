#include "pch.h"
#include "wood_interaction_contact_axis.h"
#include "interaction_contact_axis.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactAxis - Constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionContactAxis::InteractionContactAxis(Line segment, double t_a, double t_b, int polyline_a, int segment_a, int polyline_b, int segment_b)
    : segment(std::move(segment)), t_a(t_a), t_b(t_b), polyline_a(polyline_a), segment_a(segment_a), polyline_b(polyline_b), segment_b(segment_b) {}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactAxis - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionContactAxis::kind() const {
    return "axis";
}

std::shared_ptr<InteractionContact> InteractionContactAxis::flipped() const {

    std::shared_ptr<InteractionContactAxis> out = std::make_shared<InteractionContactAxis>(*this);
    out->segment = Line::from_points(segment.end(), segment.start());
    out->t_a = t_b;
    out->t_b = t_a;
    out->polyline_a = polyline_b;
    out->segment_a = segment_b;
    out->polyline_b = polyline_a;
    out->segment_b = segment_a;

    return out;
}

bool InteractionContactAxis::coincides(const InteractionContact& other) const {

    const InteractionContactAxis* axis = dynamic_cast<const InteractionContactAxis*>(&other);

    return axis && polyline_a == axis->polyline_a && segment_a == axis->segment_a && polyline_b == axis->polyline_b && segment_b == axis->segment_b;
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactAxis - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionContactAxis::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionContactAxis::interaction_data_dumps() const {

    wood_proto::InteractionContactAxis proto;
    if (!proto.mutable_segment()->ParseFromString(segment.pb_dumps()))
        throw std::runtime_error("Failed to parse Line protobuf data");
    proto.set_t_a(t_a);
    proto.set_t_b(t_b);
    proto.set_polyline_a(polyline_a);
    proto.set_segment_a(segment_a);
    proto.set_polyline_b(polyline_b);
    proto.set_segment_b(segment_b);

    return proto.SerializeAsString();
}

InteractionContactAxis InteractionContactAxis::interaction_data_loads(const std::string& data) {

    wood_proto::InteractionContactAxis proto;
    if (!proto.ParseFromString(data))
        throw std::runtime_error("Failed to parse InteractionContactAxis protobuf data");

    InteractionContactAxis contact;

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

std::shared_ptr<Interaction> InteractionContactAxis::clone() const {
    return std::make_shared<InteractionContactAxis>(*this);
}

/// The registered factory: interaction_data bytes to an axis contact.
static std::shared_ptr<Interaction> contact_axis_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionContactAxis>(InteractionContactAxis::interaction_data_loads(data));
}

void InteractionContactAxis::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), contact_axis_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactAxis - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionContactAxis::str() const {
    return fmt::format("InteractionContactAxis(a=({}, {}, {:.3f}), b=({}, {}, {:.3f}), length={:.3f})", polyline_a, segment_a, t_a, polyline_b, segment_b, t_b, segment.length());
}

} // namespace wood_session
