#include "wood_session.h"
#include "src/templates/timber_floor/floor_product.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// The floors of producers: Schneider, Lignatur, Lignotrend, Novatop and Kielsteg.
int main() {

    FloorProducts floors({
        {"clt_box_closed", FloorProduct::clt_box(0)},
        {"clt_box_open", FloorProduct::clt_box(1, 220.0)},
        {"clt_box_roof", FloorProduct::clt_box(2, 220.0)},
        {"lignatur_box", FloorProduct::lignatur_box()},
        {"lignatur_surface", FloorProduct::lignatur_surface()},
        {"lignatur_silence", FloorProduct::lignatur_surface(240.0, 1)},
        {"lignatur_shell", FloorProduct::lignatur_surface(200.0, 2)},
        {"lignotrend_rib", FloorProduct::lignotrend_rib()},
        {"lignotrend_block", FloorProduct::lignotrend_block()},
        {"novatop_element", FloorProduct::novatop()},
        {"novatop_open", FloorProduct::novatop_open()},
        {"kielsteg", FloorProduct::kielsteg()},
        {"kielsteg_deep", FloorProduct::kielsteg(730.0, 80.0, 134.0, 10.0)},
    }, "timber_floor_producers");
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the timber floors: the floors of five producers, samples 2400 long with the sections of their approvals. Best wood Schneider CLT BOX, three glulam ribs per 1200 between cross-laminated flanges 60, closed, open above, and the roof's two ribs; Lignatur box 200 wide, surface element of four cells per 1000, with the blocks of silence, and the shell; Lignotrend Rippe Q3 and Block Q3, 625 wide, of boards lying on each other; Novatop Element, ribs 27 both ways between panels 27, and Open; Kielsteg 280 and 730, chord timbers half a pitch apart above and below and webs bent between their joints.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_6_producers --parallel 6 && ./build/templates_timber_floor_6_producers && ../bash/publish-scene.sh --target templates_timber_floor_6_producers

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
