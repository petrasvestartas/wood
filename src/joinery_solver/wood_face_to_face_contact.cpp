#include "pch.h"
#include "wood_face_to_face_contact.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// FaceContact - JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json FaceContact::jsondump() const {
    return nlohmann::ordered_json{
        {"guid", guid},
        {"face_a", face_a},
        {"face_b", face_b},
        {"type", static_cast<int>(type)},
        {"area", area.jsondump()},
    };
}

FaceContact FaceContact::jsonload(const nlohmann::json& data) {

    FaceContact contact;
    contact.guid   = data.value("guid", std::string());
    contact.face_a = data.value("face_a", 0);
    contact.face_b = data.value("face_b", 0);
    contact.type   = static_cast<ContactType>(data.value("type", -1));
    if (data.contains("area"))
        contact.area = Polyline::jsonload(data["area"]);

    return contact;
}

} // namespace wood_session
