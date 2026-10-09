#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A beam: a rectangular profile swept along a polyline axis.
int main() {

    WoodSession scene("element_beam");

    const Polyline axis({{0.0, 0.0, 0.0}, {600.0, 0.0, 0.0}, {1000.0, 300.0, 0.0}});
    const Polyline profile = Polyline::rectangle(
        {-60.0, -100.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        120.0,
        200.0
    );
    scene.add(
        std::make_shared<Beam>(
            axis,
            std::vector<Polyline>{profile},
            std::vector<Vector>{},
            "beam"
        )
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A beam: a 120 x 200 rectangular profile swept along a two-segment polyline axis.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_beam --parallel 6 && ./build/element_beam && ../bash/publish-scene.sh --target element_beam

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
