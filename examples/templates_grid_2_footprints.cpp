#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_grid;

/// Seven floor patterns and footprints, side by side.
int main() {

    Grids grids(
        {
            {"orthogonal", Plan::large_l_shaped_floor(), Framing::large_l_shaped_floor()},
            {"skewed", Plan::skewed_floor(), Framing::skewed_floor()},
            {"radial", Plan::ring_floor(), Framing::ring_floor()},
            {"triangular", Plan::triangular_floor(), Framing::triangular_floor()},
            {"hexagonal", Plan::hexagonal_floor(), Framing::hexagonal_floor()},
            {"irregular", Plan::irregular_floor(), Framing::irregular_floor()},
            {"courtyard", Plan::courtyard_floor(), Framing::courtyard_floor()},
        },
        8000.0,
        "templates_grid_2_footprints"
    );
    std::cout << grids << std::endl;
    grids.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Single-floor planar grid patterns and footprints side by side: an L-plan with uneven bays, girders, purlins and CLT strips; a 30-degree skewed grid with beveled T-section perimeter girders; a 12-sector concentric glulam ring pavilion around an open central oculus with square-ended radial girders and T-butted ring beams; an equilateral triangular diagrid with six-way mitered nodes; a honeycomb hexagonal floor with three-way Y-miters on round timber columns; a five-sided irregular footprint with continuous girders and T-butted cross-beams; and a courtyard ring with continuous through-columns.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_2_footprints --parallel 6 && ./build/templates_grid_2_footprints && ../bash/publish-scene.sh --target templates_grid_2_footprints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
