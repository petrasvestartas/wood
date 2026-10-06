#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A block lofted between a bottom and a top loop.
int main() {

    WoodSession scene("element_block");

    const Polyline bottom = Polyline::rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 300.0);
    const Polyline top = Polyline::rectangle({50.0, 50.0, 250.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 200.0, 200.0);
    scene.add(std::make_shared<Block>(std::vector<Polyline>{bottom, top}, "block"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A block lofted between a 300 square and a 200 square 250 above it.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_block --parallel 6 && ./build/element_block && ../bash/publish-scene.sh --target element_block

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
