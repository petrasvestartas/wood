#include "wood_session.h"
#include "src/templates/module/module_building.h"

using namespace laika_cpp;
using namespace wood_session;

/// Modules hung between the beams of a skeleton, every one carrying only itself.
int main() {

    wood_module::ModuleBuildingGuide guide;
    guide.rooms = {1, 1, 1};
    guide.storeys = 3;
    guide.modules[0].solid_walls = true;
    guide.modules[0].wall_panel = 60.0;
    guide.modules[0].openings[2] = {{500.0, 1500.0, 300.0, 2200.0}};
    guide.modules[0].openings[3] = {{800.0, 1000.0, 0.0, 2100.0}};
    guide.post = 200.0;

    wood_module::ModuleBuilding building(guide, "module_rack");
    std::cout << building << std::endl;
    building.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 10 of the timber modules: nine modules in a rack. Posts 200 x 200 over the full height stand in every joint between modules, a 200 x 320 beam per storey between the two of a row, an 80 x 80 ledger along the foot of both its faces; a module slides in lengthwise onto the elastic strips on two ledgers, 20 clear of the beams, so the modules are all alike, their walls 60 thick, and no module carries another.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_10_module_rack --parallel 6 && ./build/templates_module_10_module_rack && ../bash/publish-scene.sh --target templates_module_10_module_rack

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
