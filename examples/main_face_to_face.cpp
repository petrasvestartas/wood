#include "wood_session.h"
#include "wood_face_to_face.h"
#include "wood_element.h"
#include "../src/session.h"
#include "../src/polyline.h"

#include <fmt/core.h>
#include <filesystem>
#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════════
// Choose
// ═══════════════════════════════════════════════════════════════════════════

const bool PLATES_OR_BLOCKS = false;

// ═══════════════════════════════════════════════════════════════════════════
// Plates — plates built in code
// ═══════════════════════════════════════════════════════════════════════════
static std::vector<wood_session::WoodElement> plates() {
    const double lift = 174.0;
    const double width = 1000.0;
    const double height = 500.0;
    const double thickness = 15.0;
    const session_cpp::Vector x(1.0, 0.0, 0.0);
    const session_cpp::Vector y(0.0, 1.0, 0.0);

    const session_cpp::Vector fold(0.0, height, -lift);
    const double fold_length = fold.magnitude();

    std::vector<wood_session::WoodElement> elements;

    elements.emplace_back(
        session_cpp::Polyline::rectangle(session_cpp::Point(-500, 0, 0), x, y, width, height),
        session_cpp::Polyline::rectangle(session_cpp::Point(-500, 0, -thickness), x, y, width, height));
    elements.emplace_back(
        session_cpp::Polyline::rectangle(session_cpp::Point(-500, -500, 0), x, y, width, height),
        session_cpp::Polyline::rectangle(session_cpp::Point(-500, -500, -thickness), x, y, width, height)
    );

    elements.emplace_back(
        session_cpp::Polyline::rectangle(session_cpp::Point(1000, 0, 0), x, y, width, height),
        session_cpp::Polyline::rectangle(session_cpp::Point(1000, 0, -thickness), x, y, width, height));
    elements.emplace_back(
        session_cpp::Polyline::rectangle(session_cpp::Point(1000, -500, lift), x, fold, width, fold_length),
        session_cpp::Polyline::rectangle(session_cpp::Point(1000, -500, lift - thickness), x, fold, width, fold_length)
    );

    return elements;
}

// ═══════════════════════════════════════════════════════════════════════════
// Blocks — the compas_tf floor system, as closed polylines
// ═══════════════════════════════════════════════════════════════════════════

static std::vector<wood_session::BlockElement> blocks() {

    const std::filesystem::path pb = internal::session_data_dir() / "floor_model.pb";

    if (!std::filesystem::exists(pb)) {
        fmt::print(stderr, "not found: {}\ngenerate it with:\n" "  cd data/face_to_face_detection && .venv/bin/python step_to_pb.py\n", pb.string());
        return {};
    }

    session_cpp::Session session = session_cpp::Session::pb_load(pb.string());
    std::vector<wood_session::BlockElement> elements;
    for (const std::vector<session_cpp::Polyline>& loops : session.select_by_type<session_cpp::Polyline>())
        elements.emplace_back(loops);

    return elements;
}

// ═══════════════════════════════════════════════════════════════════════════
// Run — the scene itself is wood_session::write_element_and_contacts
// ═══════════════════════════════════════════════════════════════════════════

int main() {

    wood_session::globals::reset_defaults();

    if constexpr (PLATES_OR_BLOCKS) {
        std::vector<wood_session::WoodElement> elements = plates();
        const std::vector<wood_session::FaceContact> contacts = wood_session::face_contacts(elements);
        const std::vector<wood_session::WoodJoint> joints = get_connection_zones(elements, face_to_face);
        wood_session::write_element_and_contacts("wood - face to face contacts", elements, contacts);
    } else {
        std::vector<wood_session::BlockElement> elements = blocks();
        const std::vector<wood_session::FaceContact> contacts = wood_session::face_contacts(elements);
        wood_session::write_element_and_contacts("wood - face to face contacts", elements, contacts);
    }

    return 0;
}
