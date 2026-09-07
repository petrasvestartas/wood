#include "wood_session.h"
#include "../src/session.h"

#include <fmt/core.h>

#include <filesystem>

const char* DATASET = "floor_model";

int main() {
    const std::filesystem::path pb = internal::session_data_dir() / (std::string(DATASET) + ".pb");
    if (!std::filesystem::exists(pb)) {
        fmt::print(stderr, "not found: {}\ngenerate it with:\n  cd data/face_to_face_detection && .venv/bin/python model_to_pb.py\n", pb.string());
        return 1;
    }

    const wood_session::SessionElements elements = wood_session::load_elements(pb);
    fmt::print("{}: {} plates, {} columns, {} solids\n",
               DATASET, elements.plates.size(), elements.columns.size(), elements.solids.size());

    size_t faces = 0;
    for (const wood_session::WoodElement& plate : elements.plates) faces += plate.polylines.size();
    for (const wood_session::WoodColumn& column : elements.columns) faces += column.polylines.size();
    for (const wood_session::BlockElement& solid : elements.solids) faces += solid.polylines.size();
    fmt::print("{} face outlines\n", faces);

    session_cpp::Session session(DATASET);
    const std::shared_ptr<session_cpp::TreeNode> inputs = session.add_group("Inputs");
    wood_session::add_solids(session, inputs, elements.plates);
    wood_session::add_solids(session, inputs, elements.columns);
    wood_session::add_solids(session, inputs, elements.solids);
    fmt::print("wrote {}\n", wood_session::pb_dump(session, "live").string());
    return 0;
}
