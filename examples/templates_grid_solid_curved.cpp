#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const double RADIUS = 15000.0;
const double HEIGHT = 15200.0;
const int SECTORS = 16;
const double FACET = 10.0; // degrees per facet of the curved face; the sections take their corners from the rays whatever the facet
const std::vector<double> ELEVATIONS = {0.0, 3800.0, 7600.0, 11400.0, 15200.0};
const wood_grid::Pattern PATTERN = wood_grid::Pattern::radial({5000.0, 10000.0, RADIUS}, SECTORS);
const wood_grid::Framing FRAMING{.system = 2, .span = 0, .spacing = 2500.0, .node = 0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_solid_curved");
    wood_grid::Building::from_solid(wood_grid::to_mesh(BRep::create_cylinder(RADIUS, HEIGHT), FACET), ELEVATIONS, PATTERN).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow A on a curved massing: a BRep cylinder meshed at FACET degrees, sliced at four storeys, every section resampled so its corners are the points where the radial rays cross it (the facet corners between rays dropped), so the rim columns stand on the rays however the cylinder is faceted, girders on the rays running through the ring nodes, the chords as purlin rows with stations between them, edge members mitred at the facet corners, heads shaped by the lines at each node. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_solid_curved --parallel 4 && ./build/templates_grid_solid_curved && ../bash/publish-scene.sh --target templates_grid_solid_curved

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
