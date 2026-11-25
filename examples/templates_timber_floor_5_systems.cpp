#include "wood_session.h"
#include "src/templates/timber_floor/floor_product.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// The floors of the timber floor chapter that every carpenter makes: joists, ribs, boxes and solid ones.
int main() {

    FloorProducts floors({
        {"joists_exposed", FloorProduct::joists(0)},
        {"joists_partly_exposed", FloorProduct::joists(1)},
        {"joists_clad", FloorProduct::joists(2)},
        {"joists_trimmed", FloorProduct::trimmed()},
        {"ribbed_above", FloorProduct::ribbed(false)},
        {"ribbed_below", FloorProduct::ribbed(true)},
        {"box", FloorProduct::box()},
        {"beams_butted", FloorProduct::planks(false)},
        {"beams_comb", FloorProduct::planks(true)},
        {"glulam_elements", FloorProduct::glulam_elements()},
        {"boards_dowelled", FloorProduct::laminated(false)},
        {"boards_nailed", FloorProduct::laminated(true)},
        {"cross_laminated_3", FloorProduct::cross_laminated({40.0, 40.0, 40.0})},
        {"cross_laminated_5", FloorProduct::cross_laminated({40.0, 20.0, 40.0, 20.0, 40.0})},
        {"cross_laminated_7", FloorProduct::cross_laminated({30.0, 40.0, 30.0, 40.0, 30.0, 40.0, 30.0})},
        {"veneer", FloorProduct::veneer()},
    }, "timber_floor_systems");
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the timber floors: the sixteen floors of the floor chapter no one producer makes, samples 2400 long: joists exposed, partly exposed over boards between them and clad on battens, joists trimmed round an opening, ribs glued to a panel over or under them and between two, solid beams butted under a board and with a comb of tongues, glulam elements on edge with a strip in their joint, boards on edge held by dowels and by nails, cross-laminated timber of three, five and seven layers, and a veneer panel.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_5_systems --parallel 6 && ./build/templates_timber_floor_5_systems && ../bash/publish-scene.sh --target templates_timber_floor_5_systems

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
