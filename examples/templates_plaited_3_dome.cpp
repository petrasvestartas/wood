#include "wood_session.h"
#include "src/templates/plaited/plaited.h"

using namespace session_cpp;
using namespace wood_session;

const double DOME_RADIUS = 3000.0;
const int FREQUENCY = 3;
const double CLIP_RATIO = 0.40;
const double THICKNESS = 180.0;
const double BEVEL = 0.36;

/// Doubly curved geodesic LeonardoStar dome: planar pentagon-hexagon facets subdivided into ortho-kite voussoirs on a level circular base.
int main() {

    const Mesh dome_mesh = wood_plaited::geodesic_dome_mesh(DOME_RADIUS, FREQUENCY, CLIP_RATIO);
    wood_plaited::PlaitedStereotomy dome(
        dome_mesh,
        wood_plaited::PlaitedPattern::LeonardoStar,
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
        "plaited dome: {} kite voussoirs, {} contacts\n",
        dome.blocks().size(),
        dome.graph.number_of_edges()
    );
    dome.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Doubly curved Goldberg geodesic dome tiled by universal LeonardoStar ortho-kite voussoirs: exact tangent-plane pentagons and hexagons clipped to a level horizontal springing ring on z = 0, with full coplanar side-face contact around five- and six-ray intrados star coffers.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_plaited_3_dome --parallel 6 && ./build/templates_plaited_3_dome && ../bash/publish-scene.sh --target templates_plaited_3_dome

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
