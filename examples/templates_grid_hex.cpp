#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const double SIDE = 4000.0;
const int NX = 3;
const int NY = 2;
const std::vector<double> ELEVATIONS = {0.0, 4000.0, 7600.0};
const wood_grid::Framing FRAMING{.span = -1, .node = 0, .profiles = {.column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 400.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_hex");
    wood_grid::Building::from_footprint({}, ELEVATIONS, wood_grid::Pattern::hexagonal(SIDE, NX, NY)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a hexagonal plan over two storeys, every line a beam: three-valent interior nodes where three equal beams meet in V mitres on hexagonal heads, edge beams round the boundary, six hexagonal decks per level, storey_k groups and contacts over the whole scene. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_hex --parallel 4 && ./build/templates_grid_hex && ../bash/publish-scene.sh --target templates_grid_hex

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
