#include "wood_session.h"
#include "../src/session.h"

#include <iostream>

const char* DATASET = "data/floor_model.pb";

int main() {
    const wood_session::WoodSession scene = wood_session::WoodSession::pb_load(DATASET);
    std::cout << scene << "\n";
    scene.pb_dump(wood_session::pb_path("live"));
    return 0;
}

/*
description: load a .pb into a WoodSession -> print it -> dump it back to "live.pb".

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_io -j8 && ./build/main_io
cloudflare: ../bash/publish-scene.sh --target main_io
view: https://petrasvestartas.github.io/session/
*/
