#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The plate element: from its two outlines, and from a rectangle.
int main() {

    WoodSession scene("element_plate");

    const Polyline bottom({Point(0.0, 0.0, 0.0), Point(600.0, 0.0, 0.0), Point(500.0, 400.0, 0.0), Point(0.0, 300.0, 0.0), Point(0.0, 0.0, 0.0)});
    scene.add(std::make_shared<Plate>(bottom, bottom.translated(Vector(0.0, 0.0, 40.0)), "outlines"));
    scene.add(Plate::from_rectangle(Point(800.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 600.0, 400.0, 40.0, "rectangle"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The plate element, two ways: from its bottom outline and the same outline 40 above, any closed polygon, and Plate::from_rectangle, a 600 x 400 rectangle 40 thick from an origin and two axes.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_plate --parallel 6 && ./build/element_plate && ../bash/publish-scene.sh --target element_plate

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
