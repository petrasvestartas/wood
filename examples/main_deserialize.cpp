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

    session_cpp::Session session(DATASET);
    const std::shared_ptr<session_cpp::TreeNode> inputs = session.add_group("Inputs");
    wood_session::add_solids(session, inputs, elements.plates);
    wood_session::add_solids(session, inputs, elements.columns);
    wood_session::add_solids(session, inputs, elements.solids);
    wood_session::pb_dump(session, "live");
    return 0;
}
