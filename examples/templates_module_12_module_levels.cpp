#include "wood_session.h"
#include "src/templates/module/module_building.h"

using namespace session_cpp;
using namespace wood_session;

/// Modules stacked two high on the decks of a skeleton, a deck every second storey.
int main() {

    wood_module::ModuleBuildingGuide guide;
    guide.rooms = {1, 1, 1, 1};
    guide.storeys = 4;
    guide.modules[0].openings[2] = {{500.0, 1500.0, 300.0, 2200.0}};
    guide.modules[0].openings[3] = {{800.0, 1000.0, 0.0, 2100.0}};
    guide.post = 240.0;
    guide.beam = 400.0;
    guide.deck = 180.0;
    guide.bay = 2;
    guide.level = 2;
    guide.coupler = 27.0;

    wood_module::ModuleBuilding building(guide, "module_levels");
    std::cout << building << std::endl;
    building.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 12 of the timber modules: a skeleton that takes the load of every two storeys. Modules carry each other two high on a deck, the deck hands them to 240 x 400 beams and 240 x 240 posts, so the lowest module of a tall building carries no more than one above it; the four modules of a bay are coupled by a plate at their start ends.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_12_module_levels --parallel 6 && ./build/templates_module_12_module_levels && ../bash/publish-scene.sh --target templates_module_12_module_levels

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
