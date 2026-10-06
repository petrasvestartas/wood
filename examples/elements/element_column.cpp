#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A column: a rectangular section swept along its axis.
int main() {

    WoodSession scene("element_column");

    const Polyline section = Polyline::rectangle(Point(-100.0, -150.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 200.0, 300.0);
    scene.add(std::make_shared<Column>(Line::from_points(Point(0.0, 0.0, 0.0), Point(0.0, 0.0, 3500.0)), section, "column"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A column: a 200 x 300 rectangle at the base of a 3500 axis, swept along it. The axis and the section are its features.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_column --parallel 6 && ./build/element_column && ../bash/publish-scene.sh --target element_column

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
