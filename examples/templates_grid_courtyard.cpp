#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<Polyline> FOOTPRINT = {
    Polyline({Point(0.0, 0.0, 0.0), Point(36576.0, 0.0, 0.0), Point(36576.0, 27432.0, 0.0), Point(0.0, 27432.0, 0.0), Point(0.0, 0.0, 0.0)}),
    Polyline({Point(12192.0, 9144.0, 0.0), Point(12192.0, 18288.0, 0.0), Point(24384.0, 18288.0, 0.0), Point(24384.0, 9144.0, 0.0), Point(12192.0, 9144.0, 0.0)}),
}; // the outer ring counter-clockwise, the courtyard clockwise
const std::vector<double> XS = wood_grid::compute_bays(36576.0, 9144.0);
const std::vector<double> YS = wood_grid::compute_bays(27432.0, 9144.0);
const std::vector<double> ELEVATIONS = {0.0, 3657.6, 7315.2, 10972.8};
const wood_grid::Framing FRAMING{.system = 1, .span = 0, .node = 2, .facade = true};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_courtyard");
    wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a courtyard footprint: an outer ring with a clockwise hole over three storeys of 30 ft bays, girders on the x lines, columns through the levels with the decks notched round them, edge members and facade walls on the outer ring and on the courtyard ring alike, no deck over the courtyard. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_courtyard --parallel 4 && ./build/templates_grid_courtyard && ../bash/publish-scene.sh --target templates_grid_courtyard

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
