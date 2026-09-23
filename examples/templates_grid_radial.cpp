#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> RADII = {4000.0, 8000.0, 12000.0}; // an atrium in the middle, no many-valent centre node
const int SECTORS = 12;
const std::vector<double> HEIGHTS = {4000.0, 4000.0};
const int SPAN = 0; // 0 girders on the rays, chords as edge beams on the rings; -1 every line a beam
const wood_grid::Framing FRAMING{.system = 1, .span = SPAN, .node = 0, .profiles = {.column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 240.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_radial");
    wood_grid::Building::from_footprint({}, HEIGHTS, wood_grid::Pattern::radial(RADII, SECTORS)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a radial plan with an empty footprint, every bounded cell a bay: two rings of twelve sectors round an atrium over two storeys, girders on the rays running through the ring nodes, the inner and outer chords as edge beams mitred at the ring corners, heads shaped by the lines that meet at each node. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_radial --parallel 4 && ./build/templates_grid_radial && ../bash/publish-scene.sh --target templates_grid_radial

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
