#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const double WIDTH = 30000.0;
const double DEPTH = 18000.0;
const double HEIGHT = 12000.0;
const std::vector<double> ELEVATIONS = {0.0, 4000.0, 8000.0, 12000.0};
const wood_grid::Pattern PATTERN = wood_grid::Pattern::orthogonal(wood_grid::compute_bays(WIDTH, 6000.0), wood_grid::compute_bays(DEPTH, 6000.0));
const wood_grid::Framing FRAMING{.system = 1, .span = 1, .node = 1};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_solid_box");
    const Mesh massing = Mesh::create_box(WIDTH, DEPTH, HEIGHT).transformed(Xform::translation(WIDTH / 2.0, DEPTH / 2.0, HEIGHT / 2.0));
    wood_grid::Building::from_solid(massing, ELEVATIONS, PATTERN).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow A on the simplest massing, a box: sliced at three storeys, every section the same rectangle filled with the same 6 m pattern, girders on the y lines with the decks spanning between them, columns flush with the datum so every member butts into a column face and the deck rests over all; the result equals the footprint workflow of the same rectangle. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_solid_box --parallel 4 && ./build/templates_grid_solid_box && ../bash/publish-scene.sh --target templates_grid_solid_box

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
