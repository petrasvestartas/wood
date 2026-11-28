#include "wood_session.h"
#include "src/templates/module/spatial_module.h"

using namespace laika_cpp;
using namespace wood_session;

/// Three closed modules of one size: timber frame, solid plates, and a timber frame under a solid ceiling.
int main() {

    wood_module::SpatialModuleGuide framed;
    framed.openings[0] = {{900.0, 1200.0, 900.0, 2100.0}, {3900.0, 1200.0, 900.0, 2100.0}};
    framed.openings[2] = {{500.0, 1500.0, 300.0, 2200.0}};
    framed.openings[3] = {{800.0, 1000.0, 0.0, 2100.0}};

    wood_module::SpatialModuleGuide solid = framed;
    solid.solid_walls = true;
    solid.solid_floor = true;
    solid.solid_ceiling = true;
    solid.pin_spacing = 600.0;

    wood_module::SpatialModuleGuide mixed = framed;
    mixed.solid_ceiling = true;

    wood_module::ModuleTypes types({{"framed", framed}, {"solid", solid}, {"mixed", mixed}});
    std::cout << types << std::endl;
    types.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 7 of the timber modules: three closed modules 6000 x 2950 in plan, 2500 clear, on a 3000 grid less a 50 joint. framed: a floor box of 80 x 160 ribs at 625 between two 27 boards, 214 thick, walls of 60 x 120 studs at 625 with a 15 sheet on each face, 150 thick, a ceiling box of 80 x 120 ribs between two 15 boards. solid: cross-laminated plates, the floor 140, the walls 100, the ceiling 80, pinned along every edge from the outside at 600. mixed: the framed module under a solid ceiling 80 thick. In each the walls stand on the floor and carry the ceiling, the long walls over the whole length, the ends between them.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_7_module_types --parallel 6 && ./build/templates_module_7_module_types && ../bash/publish-scene.sh --target templates_module_7_module_types

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
