#include "wood_session.h"
#include "src/templates/timber_floor/floor_product.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// The joints between cross-laminated panels and the shear connections of timber and concrete.
int main() {

    const std::vector<double> layers = {40.0, 20.0, 40.0, 20.0, 40.0};
    FloorProducts floors({
        {"joint_cover", FloorProduct::cross_laminated(layers, 1)},
        {"joint_grout", FloorProduct::cross_laminated(layers, 2)},
        {"joint_keys", FloorProduct::cross_laminated(layers, 3)},
        {"joint_blocks", FloorProduct::cross_laminated(layers, 4)},
        {"concrete_stepped", FloorProduct::composite_stepped()},
        {"concrete_micro", FloorProduct::composite_micro()},
        {"concrete_notched", FloorProduct::composite_notched()},
        {"concrete_lugs", FloorProduct::composite_lugs()},
        {"concrete_wave", FloorProduct::composite_wave()},
        {"concrete_crossed", FloorProduct::composite_crossed()},
        {"concrete_locks", FloorProduct::composite_locks()},
        {"concrete_bars", FloorProduct::composite_bars()},
        {"concrete_plates", FloorProduct::composite_plates()},
        {"concrete_glued", FloorProduct::composite_glued()},
    }, "timber_floor_shear");
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 7 of the timber floors: the joints between two cross-laminated panels, a cover board and a strip, grout, wedge keys and steel shear blocks, and ten ways to hold concrete to a timber floor: boards of two depths, small notches, deep notches with pins, lugs in pockets of the joists, a wave-shaped recess in veneer lumber, crossed pins, flat steel locks, V-shaped bars, perforated plates on edge, and glue alone.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_7_shear --parallel 6 && ./build/templates_timber_floor_7_shear && ../bash/publish-scene.sh --target templates_timber_floor_7_shear

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
