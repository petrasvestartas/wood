#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The beam element: a polyline axis with a radius, and with a profile.
int main() {

    WoodSession scene("element_beam");

    scene.add(std::make_shared<Beam>(Polyline({Point(0.0, 0.0, 0.0), Point(1000.0, 0.0, 0.0)}), 60.0, "radius"));
    const Polyline profile = Polyline::rectangle(Point(-60.0, -100.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 120.0, 200.0);
    scene.add(std::make_shared<Beam>(Polyline({Point(0.0, 400.0, 0.0), Point(600.0, 400.0, 0.0), Point(1000.0, 700.0, 0.0)}), std::vector<Polyline>{profile}, std::vector<Vector>{}, "profile"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The beam element, two ways: a straight axis with a 60 radius, and a two-segment axis swept by a 120 x 200 rectangular profile.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_beam --parallel 6 && ./build/element_beam && ../bash/publish-scene.sh --target element_beam

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
