#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

/// Eight section-active slab, portal frame and Vierendeel systems after Engel, side by side.
int main() {

    Grids grids(
        {
            {"mushroom_drop_slab", Plan::mushroom_floor(), Framing::mushroom_drop_slab()},
            {"ribbed_cantilever_slab", Plan::ribbed_floor(), Framing::ribbed_cantilever_slab()},
            {"waffle_coffered_slab", Plan::waffle_floor(), Framing::waffle_coffered_slab()},
            {"umbrella_canopies", Stack::umbrella_canopies, 15500.0},
            {"tapered_portal_hall", Stack::tapered_portal_hall, 19200.0},
            {"shear_vierendeel", Stack::shear_vierendeel, 24000.0},
            {"concentric_vierendeel", Stack::concentric_vierendeel, 24000.0},
            {"mega_transfer_tower", Stack::mega_transfer_tower, 20000.0},
        },
        8000.0,
        "templates_grid_7_frames_and_slabs"
    );
    std::cout << grids << std::endl;
    grids.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Eight section-active slab, rigid portal frame and multi-panel Vierendeel systems after Heino Engel (Structure Systems, Chapter 3): (1) a cantilevered two-way CLT mushroom slab on inset columns with two-stage flared capitals and square shear drop-panels; (2) a one-way closely ribbed slab cantilevering past two inset primary girder lines; (3) a four-sided cantilevered two-way coffered waffle slab on four inset columns with solid shear drop-panels; (4) four staggered single-column cantilevered umbrella canopies on Sherpa base shoes, tapered shafts and chamfered capitals; (5) a three-storey coupled two-hinged portal hall with base shoes, tapered legs, rigid knees and tapered corridor cantilevers; (6) a double-cantilevered eight-panel Vierendeel elevation with shear-gradated bay widths; (7) four biaxial intersecting story-deep Vierendeel frames spanning a 24 m hall on eight perimeter columns; and (8) a six-storey mega-frame alternating full-span five-panel Vierendeel transfer storeys with lighter office floors.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_7_frames_and_slabs --parallel 6 && ./build/templates_grid_7_frames_and_slabs && ../bash/publish-scene.sh --target templates_grid_7_frames_and_slabs

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
