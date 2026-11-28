#include "wood_session.h"
#include "src/templates/module/module_building.h"

using namespace laika_cpp;
using namespace wood_session;

/// Modules in pairs on the decks of a skeleton, a deck per storey.
int main() {

    wood_module::ModuleBuildingGuide guide;
    guide.rooms = {2, 2};
    guide.storeys = 3;
    guide.modules[0].solid_walls = true;
    guide.modules[0].wall_panel = 60.0;
    guide.modules[0].openings[2] = {{500.0, 1500.0, 300.0, 2200.0}};
    guide.modules[0].openings[3] = {{800.0, 1000.0, 0.0, 2100.0}};
    guide.post = 240.0;
    guide.deck = 140.0;
    guide.bay = 2;

    wood_module::ModuleBuilding building(guide, "module_frame");
    std::cout << building << std::endl;
    building.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 11 of the timber modules: a skeleton with a deck per storey, two modules side by side in every bay, open to each other. Posts 240 x 240 over the full height, 240 x 320 beams along and across between them, a 140 solid deck on the beams, notched round the posts; the modules stand on battens and elastic strips on the decks and carry only themselves, as in the reuse of an existing frame.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_11_module_frame --parallel 6 && ./build/templates_module_11_module_frame && ../bash/publish-scene.sh --target templates_module_11_module_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
