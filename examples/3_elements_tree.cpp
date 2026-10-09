#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> XS = {4000.0, 4000.0, 4000.0};
const std::vector<double> YS = {3000.0, 3000.0};
const std::vector<Polyline> FOOTPRINT = {Polyline({
    Point(0.0, 0.0, 0.0), 
    Point(12000.0, 0.0, 0.0), 
    Point(12000.0, 3000.0, 0.0), 
    Point(8000.0, 3000.0, 0.0), 
    Point(8000.0, 6000.0, 0.0), 
    Point(0.0, 6000.0, 0.0), 
    Point(0.0, 0.0, 0.0)})
}; // an L: three by two bays, the far corner bay left open
const std::vector<double> ELEVATIONS = {0.0, 3000.0, 6000.0}; // two storeys of column 2800 + head 200, each datum the top of the heads, members and deck
const wood_grid::Framing FRAMING{
    .system = 1, 
    .span = -1, 
    .node = 0, 
    .deck = 200.0, 
    .reach = 200.0, 
    .profiles = {
        .column = profile_rectangle(200.0, 200.0), 
        .girder = profile_rectangle(200.0, 200.0)
    }};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

/// The L of five bays, two storeys of post and beam, its tree a level per storey and a group per kind.
int main() {

    const wood_grid::Building building = wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS));
    wood_grid::Grid grid(building, FRAMING, "elements_tree");

    if constexpr (INSTANCES)
        grid.instance_by_key();

    std::cout << grid;
    grid.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
an L of five bays - three by two with the far corner bay left open - post and beam with 200 square columns and beams, stacked two storeys high, a beam on every grid line between the column heads (span -1), so the heads meet every case - two beams at the outer corners, three on the edges, four inside and at the re-entrant corner, every head chamfered between each two neighbouring beams - and every deck sits between four beams with its corners cut by the heads; the Grid's tree has a branch per level and a twig per kind under it, plan_<l>, columns_<l>, heads_<l>, beams_<l> and decks_<l>, the ground decks under level_0; each column stands on the head below it, so the Grid's contacts pair every element with every other across the storeys. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances: four definitions, column, head, beam and deck.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 3_elements_tree --parallel 6 && ./build/3_elements_tree && ../bash/publish-scene.sh --target 3_elements_tree

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
