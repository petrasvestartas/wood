#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

/// The colonnade office in four stages, side by side.
int main() {

    Grids grids(
        {
            {"stage_1_colonnade", Stack::colonnade_office, 40000.0},
            {"stage_2_spandrels", Stack::glazed_office, 40000.0},
            {"stage_3_fin_screen", Plan::fin_screen(), Framing::fin_screen(), 40000.0},
            {"stammhaus_headquarters", Stack::screened_office, 40000.0},
        },
        12000.0,
        "templates_grid_4_developed_facade"
    );
    std::cout << grids << std::endl;
    grids.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four-stage tectonic development of a five-storey timber headquarters inspired by the Blumer-Lehmann Stammhaus in Gossau: (1) primary glulam structure with an eight-bay recessed ground colonnade, four cantilevered upper CLT slabs, and a projecting flat timber roof cornice; (2) recessed ground storefront arcade and upper ribbon window mullions; (3) the isolated two-layer developed timber screen (`Framing::fin_screen`, `system = 3` on `Plan::facade`) pairing 760 mm solid horizontal timber spandrel bands in the inner layer with continuous syncopated vertical glulam fins in the outer layer; and (4) the complete four-sided headquarters assembly.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_4_developed_facade --parallel 6 && ./build/templates_grid_4_developed_facade && ../bash/publish-scene.sh --target templates_grid_4_developed_facade

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
