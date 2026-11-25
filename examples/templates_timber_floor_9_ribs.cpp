#include "wood_session.h"
#include "src/templates/timber_floor/floor_product.h"
#include "src/templates/timber_floor/timber_floor_guide.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// More floors of producers: cross-laminated ribs, boxes and voids, veneer lumber ribs, dowelled layers and concrete on glulam ribs.
int main() {

    FloorProducts floors({
        {"clt_rib", FloorProduct::cross_laminated_ribs(false, 120.0, 140.0, 280.0, 625.0, 2500.0)},
        {"clt_rib_box", FloorProduct::cross_laminated_ribs(true, 100.0, 120.0, 300.0, 600.0, 2400.0)},
        {"void_panel", FloorProduct::void_panel()},
        {"lvl_rib", FloorProduct::lvl_rib(false)},
        {"lvl_rib_strips", FloorProduct::lvl_rib(true)},
        {"dowelled_layers", FloorProduct::dowelled_layers()},
        {"concrete_ribs", FloorProduct::composite_ribs()},
    }, "timber_floor_ribs", 4, LAYERS_HEAVY);
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 9 of the timber floors: more floors of producers, samples: cross-laminated 120 on glulam ribs 140 x 280 every 625 (Hasslacher), a box of two plates 100 and ribs 120 x 300 every 600 (Derix), a void panel of two plates 80 and core strips 186 x 120 with 372 between them (Derix), veneer lumber ribs 51 x 357 every 735 under a panel 27, open and with a strip 300 x 36 under each (Stora Enso), five layers of boards and squared timber held by upright beech dowels 20 (as Holz100 and Nur-Holz), and glulam ribs 240 x 280, paired, under concrete 100 let 40 into their notches with two pins each (Cree, ZECH).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_9_ribs --parallel 6 && ./build/templates_timber_floor_9_ribs && ../bash/publish-scene.sh --target templates_timber_floor_9_ribs

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
