#include "pch.h"
#include "wood_interaction_contact_face.h"
#include "interaction_contact_face.pb.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactFace - Constructors
// ═══════════════════════════════════════════════════════════════════════════

InteractionContactFace::InteractionContactFace(
    int face_a,
    int face_b,
    ContactType type,
    Polyline polygon,
    std::array<Line, 2> lines,
    std::array<Polyline, 4> volumes)
    : face_a(face_a),
      face_b(face_b),
      type(type),
      polygon(std::move(polygon)),
      lines(std::move(lines)),
      volumes(std::move(volumes))
{}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionContactFace - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionContactFace::kind() const {
    return "face";
}

void InteractionContactFace::flip() {

    std::swap(face_a, face_b);
    std::swap(lines[0], lines[1]);
    std::swap(volumes[0], volumes[2]);
    std::swap(volumes[1], volumes[3]);
}

std::shared_ptr<InteractionContact> InteractionContactFace::flipped() const {

    const std::shared_ptr<InteractionContactFace> out = std::make_shared<InteractionContactFace>(*this);
    out->flip();

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
    for (const auto& line : lines)
        if (!proto.add_lines()->ParseFromString(line.pb_dumps()))
            throw std::runtime_error("Invalid contact line");
    for (const auto& volume : volumes)
        if (!proto.add_volumes()->ParseFromString(volume.pb_dumps()))
            throw std::runtime_error("Invalid contact volume");
    proto.set_face_a(face_a);
    proto.set_face_b(face_b);
    proto.set_type(static_cast<int>(type));
    if (!proto.mutable_polygon()->ParseFromString(polygon.pb_dumps()))
        throw std::runtime_error("Failed to parse Polyline protobuf data");

    return proto.SerializeAsString();
}

InteractionContactFace InteractionContactFace::interaction_data_loads(const std::string& data) {

    wood_proto::InteractionContactFace proto;
    if (!proto.ParseFromString(data))
        throw std::runtime_error("Failed to parse InteractionContactFace protobuf data");

    InteractionContactFace contact;
    if (proto.lines_size() > 2 || proto.volumes_size() > 4)
        throw std::runtime_error("Invalid face contact dimensions");
    for (int i = 0; i < proto.lines_size(); ++i)
        contact.lines[i] = Line::pb_loads(proto.lines(i).SerializeAsString());
    for (int i = 0; i < proto.volumes_size(); ++i)
        contact.volumes[i] = Polyline::pb_loads(proto.volumes(i).SerializeAsString());
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
    return fmt::format(
        "InteractionContactFace(face_a={}, face_b={}, type={}, polygon={}, lines={}, volumes={})\n",
        face_a,
        face_b,
        to_string(type),
        polygon.point_count(),
        lines.size(),
        volumes.size()
    );
}

} // namespace wood_session
