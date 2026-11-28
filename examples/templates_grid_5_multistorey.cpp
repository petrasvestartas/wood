#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_grid;

/// Eight multi-storey stacks, side by side.
int main() {

    Grids grids(
        {
            {"five_bay", Stack::l_shaped_two_storey, 12000.0},
            {"shifted_floors", Stack::shifted_floors, 24000.0},
            {"tapered_tower", Stack::tapered_tower, 21000.0},
            {"twisted_tower", Stack::twisted_tower, 25000.0},
            {"setback_tower", Stack::setback_tower, 30000.0},
            {"cylindrical_rotunda", Stack::rotunda, 28000.0},
            {"oblique_flatiron", Stack::flatiron, 30000.0},
            {"core_and_facade", Stack::core_and_facades, 30000.0},
        },
        11000.0,
        "templates_grid_5_multistorey"
    );
    std::cout << grids << std::endl;
    grids.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Eight multi-storey timber building topologies built with wood_grid::Stack: (1) a two-storey L-frame with in-line chamfered column heads and 45-degree notched flush CLT cassette decks; (2) five storeys of shifted cantilevered floor boxes over a plumb column grid; (3) a six-storey tapered tower with seamless continuous leaning columns; (4) a seven-storey 42-degree twisting tower with seamless helicoidal round timber columns; (5) a six-storey cascading stepped-terrace building around a central atrium; (6) a four-storey 16-sector cylindrical ring-and-radial rotunda around an open oculus; (7) a four-storey chamfered pentagonal flatiron building with diagonal facade cuts; and (8) a three-storey hybrid frame with pinwheel CLT core walls and corner X-braced facades.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_5_multistorey --parallel 6 && ./build/templates_grid_5_multistorey && ../bash/publish-scene.sh --target templates_grid_5_multistorey

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
