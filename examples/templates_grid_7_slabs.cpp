#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_grid;

/// Seven section-active slab systems after Engel, side by side.
int main() {

    Grids grids(
        {
            {"one_way_girder_slab", Plan::one_way_girder_floor(), Framing::one_way_girder_slab()},
            {"three_sided_slab", Plan::three_sided_floor(), Framing::three_sided_slab()},
            {"ribbed_cantilever_slab", Plan::ribbed_floor(), Framing::ribbed_cantilever_slab()},
            {"t_beam_slab", Plan::t_beam_floor(), Framing::t_beam_slab()},
            {"mushroom_drop_slab", Plan::mushroom_floor(), Framing::mushroom_drop_slab()},
            {"waffle_coffered_slab", Plan::waffle_floor(), Framing::waffle_coffered_slab()},
            {"diagonal_waffle_slab", Plan::diagonal_waffle_floor(), Framing::diagonal_waffle_slab()},
            {"umbrella_canopies", Stack::umbrella_canopies, 15500.0},
        },
        7000.0,
        "templates_grid_7_slabs"
    );
    std::cout << grids << std::endl;
    grids.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Eight section-active structural slab systems after Heino Engel (Structure Systems, §3.4): (1) a four-sided cantilevered one-way CLT slab carried on two parallel haunched glulam girders; (2) a three-sided U-beam supported CLT slab with one free cantilevered open edge; (3) a one-way closely ribbed floor whose open-ended joists cantilever past two inset primary girders beneath a thin top deck; (4) an integral T-beam floor (`profile_t`) cantilevering over two inset primary girders; (5) a two-way cantilevered CLT mushroom slab (`Pilzdecke`) on inset columns with flared capitals and square shear drop-panels; (6) an orthogonal two-way coffered waffle slab on four inset columns with solid shear drop-panels; (7) a 45-degree diagonal coffered waffle slab with diamond column shear drop-panels; and (8) four staggered single-column cantilevered umbrella canopies on base shoes, tapered shafts and chamfered capitals.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_7_slabs --parallel 6 && ./build/templates_grid_7_slabs && ../bash/publish-scene.sh --target templates_grid_7_slabs

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
