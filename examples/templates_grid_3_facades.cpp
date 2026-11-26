#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

/// Four timber facades, side by side.
int main() {

    Grids grids(
        {
            {"curtain_mullion", Plan::curtain_wall(), Framing::curtain_wall()},
            {"punched_openings", Plan::punched_wall(), Framing::punched_wall()},
            {"braced_facade", Plan::braced_wall(), Framing::braced_wall()},
            {"gable_facade", Plan::gable_wall(), Framing::gable_wall()},
        },
        5000.0,
        "templates_grid_3_facades"
    );
    std::cout << grids << std::endl;
    grids.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Vertical timber facade grids built with the same planar Grid engine (height = 0): a 4x3 mullion-and-transom curtain wall with continuous vertical mullions and T-butted rails; a punched facade with window and door voids and flush CLT spandrel panels; a 4x3 heavy-timber facade with co-planar X-bracing in the outer bays; and a pitched pentagonal gable wall with chevron bracing clipped cleanly to the roof rake.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_3_facades --parallel 6 && ./build/templates_grid_3_facades && ../bash/publish-scene.sh --target templates_grid_3_facades

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
