#include "wood_session.h"
#include "src/templates/plaited/plaited.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 3600.0;
const int CELLS = 6;
const double THICKNESS = 220.0;
const double BEVEL = 0.48;

/// Joseph Abeille's 1699 flat stone vault (volta piana): 36 interlocking hexahedral blocks on a 6x6 bay.
int main() {

    const Mesh floor_mesh = wood_plaited::flat_quad_mesh(SPAN, SPAN, CELLS, CELLS, 0.0, THICKNESS);
    wood_plaited::PlaitedStereotomy vault(
        floor_mesh,
        wood_plaited::PlaitedPattern::Abeille,
        THICKNESS,
        BEVEL
    );

    std::cout << fmt::format(
        "plaited flat floor: {} blocks, {} contacts\n",
        vault.blocks().size(),
        vault.graph.number_of_edges()
    );
    vault.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Joseph Abeille's 1699 flat stone vault (volta piana) over a 3.6 x 3.6 m bay: 36 truncated hexahedral blocks whose opposite bevel planes alternate inward and outward by 0.48 rad, flush around the square perimeter and coffered in Leonardo's four-bar weave on the intrados, with 60 coplanar contacts.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_plaited_1_flat_floor --parallel 6 && ./build/templates_plaited_1_flat_floor && ../bash/publish-scene.sh --target templates_plaited_1_flat_floor

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
