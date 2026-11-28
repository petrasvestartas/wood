#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_grid;

/// The L-shaped floor, its first bay alone, and one bay framed seven ways, side by side.
int main() {

    Grids grids(
        {
            {"floor_l_plan", Plan::l_shaped_floor(), Framing::l_shaped_floor()},
            {"isolated_bay", Plan::l_shaped_floor().bay(0), Framing::l_shaped_floor()},
            {"slab_head", Plan::square_bay(), Framing::slab_on_heads()},
            {"beam_head", Plan::double_bay(), Framing::beams_on_heads()},
            {"node_flush", Plan::square_bay(), Framing::girders_and_purlins()},
            {"node_through", Plan::square_bay(), Framing::through_columns()},
            {"purlin_stacked", Plan::square_bay(), Framing::stacked_purlins()},
            {"round_columns", Plan::square_bay(), Framing::round_columns()},
        },
        3500.0,
        "templates_grid_1_bay_framing"
    );
    std::cout << grids << std::endl;
    grids.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Single-floor bay isolation and pure-timber framing modes side by side: a five-bay L floor beside bay 0 isolated via Plan::bay(0); a point-supported CLT slab on columns with a single chamfered timber head; post-and-beam with in-line chamfered timber heads; girder-and-purlin with flush columns and CLT strips; continuous through-columns with beams butting to column faces; continuous purlins stacked on lowered girders; and glulam framing on round timber columns.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_1_bay_framing --parallel 6 && ./build/templates_grid_1_bay_framing && ../bash/publish-scene.sh --target templates_grid_1_bay_framing

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
