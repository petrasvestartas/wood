#include "wood_session.h"
#include "src/templates/timber_floor/floor_product.h"
#include "src/templates/timber_floor/timber_floor_guide.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// The floors of producers: Schneider, Lignatur, Lignotrend and Novatop.
int main() {

    FloorProducts floors({
        {"clt_box_closed", FloorProduct::clt_box(0)},
        {"clt_box_open", FloorProduct::clt_box(1, 220.0)},
        {"clt_box_roof", FloorProduct::clt_box(2, 220.0)},
        {"lignatur_box", FloorProduct::lignatur_box()},
        {"lignatur_surface", FloorProduct::lignatur_surface()},
        {"lignotrend_rib", FloorProduct::lignotrend_rib()},
        {"lignotrend_block", FloorProduct::lignotrend_block()},
        {"novatop_element", FloorProduct::novatop()},
        {"novatop_open", FloorProduct::novatop_open()},
    }, "timber_floor_producers", 4, LAYERS_HEAVY);
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the timber floors: the floors of four producers, samples 1200 long with the sections of their approvals, under one layer of 110 for all that lies on them. Best wood Schneider CLT BOX, three glulam ribs per 1200 between cross-laminated flanges 60, closed, open above, and the roof's two ribs; Lignatur box 200 wide, and surface element of four cells per 1000; Lignotrend Rippe Q3 and Block Q3, 625 wide, of boards lying on each other; Novatop Element, ribs 27 both ways between panels 27, and Open.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_6_producers --parallel 6 && ./build/templates_timber_floor_6_producers && ../bash/publish-scene.sh --target templates_timber_floor_6_producers

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
