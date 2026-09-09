#include "wood_session.h"
#include "../src/session.h"

const char* DATASET = "data/floor_model.pb";
const char* DATASET_YAML = "data/hexboxes.yml";

int main() {
    const std::shared_ptr<session_cpp::Session> session = session_cpp::Session::pb_load(DATASET);
    const wood_session::WoodSession scene = wood_session::WoodSession::from_session(session);
    wood_session::pb_dump(*scene.to_session(), "live");

    const wood_session::WoodSession plates = wood_session::WoodSession::yaml_load(DATASET_YAML);
    const std::shared_ptr<session_cpp::Session> plates_session = plates.to_session();
    wood_session::pb_dump(*plates_session, "hex_boxes");
    return 0;
}

/*
description: load a .pb session -> convert to WoodSession -> dump to "live.pb"; load a dataset .yml (obj + txt files it names) as a WoodSession -> its session -> dump to "hexboxes.pb".

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_io -j8 && ./build/main_io
cloudflare: ../bash/publish-scene.sh --target main_io
view: https://petrasvestartas.github.io/session/
*/
