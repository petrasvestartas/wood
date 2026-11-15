#include "wood_session.h"
#include "src/templates/plaited/plaited.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 5200.0;
const double RISE = 1650.0;
const double DEPTH = 3400.0;
const int COURSES = 12;
const int RINGS = 6;
const double THICKNESS = 230.0;
const double BEVEL = 0.46;

/// Single-curved plaited barrel arch (Arco di Abeille): 72 full-contact voussoirs springing from a horizontal base.
int main() {

    const Mesh arch_mesh = wood_plaited::arch_quad_mesh(SPAN, RISE, DEPTH, COURSES, RINGS);
    wood_plaited::PlaitedStereotomy arch(
        arch_mesh,
        wood_plaited::PlaitedPattern::Abeille,
        THICKNESS,
        BEVEL,
        0.0,
        false,
        false,
        true,
        0.0,
        true
    );

    std::cout << fmt::format(
        "plaited arch: {} voussoirs, {} contacts\n",
        arch.blocks().size(),
        arch.graph.number_of_edges()
    );
    arch.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Single-curved plaited barrel arch (12 courses by 6 rings = 72 voussoirs): full-depth shared bevel planes ensure tight face-to-face contact across every joint from extrados to woven intrados, with flush vertical arch elevations and flat horizontal ground springing beds.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_plaited_2_arch --parallel 6 && ./build/templates_plaited_2_arch && ../bash/publish-scene.sh --target templates_plaited_2_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
