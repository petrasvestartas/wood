#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> XS(4, 6000.0);
const std::vector<double> YS(3, 6000.0);
const double SKEW = 30.0; // degrees the y lines lean towards x
const std::vector<double> ELEVATIONS = {0.0, 4000.0, 8000.0};
const wood_grid::Framing FRAMING{.system = 2, .span = 0, .spacing = 2500.0, .node = 1};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_skewed");
    wood_grid::Building::from_footprint({}, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS, SKEW)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a skewed plan: four by three bays whose y lines lean 30 degrees, every bounded cell a bay over two storeys, girders on the x lines and purlin stations parallel to the leaning cross lines, columns flush with the datum so every member butts obliquely into a column face and the decks rest over all. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_skewed --parallel 4 && ./build/templates_grid_skewed && ../bash/publish-scene.sh --target templates_grid_skewed

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
