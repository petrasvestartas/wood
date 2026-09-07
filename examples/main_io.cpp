#include "wood_session.h"
#include "../src/session.h"

#include <iostream>

const char* DATASET = "data/floor_model.pb";

int main() {
    const wood_session::WoodSession scene = wood_session::WoodSession::load(DATASET);
    std::cout << scene << "\n";
    scene.pb_dump("live");
    return 0;
}

/*
description: load a .pb into a WoodSession -> dump it back to "live.pb".
run: cd ~/code/code_cpp/wood_research/wood && cmake --build build --target main_io -j8 && ./build/main_io
cloudflare: cd ~/code/code_cpp/wood_research/wood && ../bash/publish-scene.sh --target main_io   ->   https://petrasvestartas.github.io/session/
*/
