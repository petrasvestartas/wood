#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const double SIDE = 6000.0;
const int NX = 4;
const int NY = 3;
const std::vector<double> ELEVATIONS = {0.0, 4000.0};
const int SPAN = 0; // 0 girders on the x lines only, the other two families carry nothing; -1 every side a beam
const wood_grid::Framing FRAMING{.system = 1, .span = SPAN, .node = 0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_triangular");
    wood_grid::Building::from_footprint({}, ELEVATIONS, wood_grid::Pattern::triangular(SIDE, NX, NY)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a triangular plan: three families of lines at 0, 60 and 120 degrees over four by three rhombi, every bounded triangle a bay, girders on the x lines with the decks spanning between them and edge members round the boundary; SPAN -1 turns every side into a beam meeting its neighbours in mitres on six-sided heads. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_triangular --parallel 4 && ./build/templates_grid_triangular && ../bash/publish-scene.sh --target templates_grid_triangular

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
