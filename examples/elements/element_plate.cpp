#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A plate from its bottom and top outlines.
int main() {

    WoodSession scene("element_plate");

    const Polyline bottom({Point(0.0, 0.0, 0.0), Point(600.0, 0.0, 0.0), Point(500.0, 400.0, 0.0), Point(0.0, 300.0, 0.0), Point(0.0, 0.0, 0.0)});
    scene.add(std::make_shared<Plate>(bottom, bottom.translated(Vector(0.0, 0.0, 40.0)), "plate"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A plate from two polylines: its bottom outline, any closed polygon, and the same outline 40 above as its top.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_plate --parallel 6 && ./build/element_plate && ../bash/publish-scene.sh --target element_plate

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
