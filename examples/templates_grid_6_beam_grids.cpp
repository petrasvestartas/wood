#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

/// Seven section-active beam and grillage systems after Engel, side by side.
int main() {

    Grids grids(
        {
            {"built_up_profiles", Plan::double_bay(), Framing::built_up_profiles()},
            {"belly_spans", Plan::five_span_beams(), Framing::belly_beams()},
            {"haunched_cantilever", Plan::cantilever_hall(), Framing::haunched_cantilever()},
            {"gerber_hall", Plan::gerber_hall(), Framing::gerber_hall()},
            {"diagonal_grillage", Plan::diagonal_floor(), Framing::diagonal_floor()},
            {"concentric_lantern", Plan::concentric_floor(), Framing::concentric_floor()},
            {"ramified_floor", Plan::ramified_floor(), Framing::ramified_floor()},
        },
        7000.0,
        "templates_grid_6_beam_grids"
    );
    std::cout << grids << std::endl;
    grids.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Seven section-active beam and beam-grid systems after Heino Engel (Structure Systems, Chapter 3): (1) built-up timber I-beam purlins and hollow box edge beams on rectangular glulam girders; (2) five simple spans with parabolic midspan belly depth increase under stacked purlins; (3) continuous multi-span girders with support haunches over inset columns and tapered balanced cantilevers; (4) an articulated Gerber hall whose paired-column girders taper to zero-moment hinges supporting a parabolic drop-in span; (5) a homogeneous 45-degree diagonal square grillage restrained at the corners by a continuous orthogonal perimeter rim; (6) a gradated four-ring chiral concentric lantern grillage with cantilevered corners on perimeter fin columns; and (7) a three-way ramified branching beam floor on six perimeter round timber columns.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_6_beam_grids --parallel 6 && ./build/templates_grid_6_beam_grids && ../bash/publish-scene.sh --target templates_grid_6_beam_grids

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
