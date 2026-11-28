#include "wood_session.h"
#include "src/templates/module/module_building.h"

using namespace laika_cpp;
using namespace wood_session;

/// Two rows of modules facing each other over a corridor of planar panels.
int main() {

    wood_module::ModuleBuildingGuide guide;
    guide.rooms = {1, 2, 1};
    guide.storeys = 2;
    guide.corridor = 2000.0;
    guide.modules[0].openings[2] = {{500.0, 1500.0, 300.0, 2200.0}};
    guide.modules[0].openings[3] = {{800.0, 1000.0, 0.0, 2100.0}};
    guide.modules[0].side_posts = 1;
    guide.coupler = 27.0;

    wood_module::ModuleBuilding building(guide, "module_corridor");
    std::cout << building << std::endl;
    building.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 13 of the timber modules: two storeys of two rows of modules, their doors to a corridor 2000 wide between them. The corridor is no module: solid panels 160 thick, one per module across, lie on 80 x 80 ledgers on the ends of the two modules facing each other, the last ones the corridor's roof. The middle modules of a row are open to each other.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_13_module_corridor --parallel 6 && ./build/templates_module_13_module_corridor && ../bash/publish-scene.sh --target templates_module_13_module_corridor

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
