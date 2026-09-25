#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const Polyline BOTTOM = Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 30000.0, 20000.0);
const Polyline TOP = Polyline::rectangle(Point(1500.0, 1500.0, 14400.0), X, Y, 27000.0, 17000.0); // every face leans inwards
const std::vector<double> ELEVATIONS = {0.0, 3600.0, 7200.0, 10800.0, 14400.0};
const wood_grid::Pattern PATTERN = wood_grid::Pattern::orthogonal(wood_grid::compute_bays(30000.0, 6000.0), wood_grid::compute_bays(20000.0, 5000.0));
const wood_grid::Framing FRAMING{.system = 2, .span = 0, .spacing = 3000.0, .node = 0, .taper = 30.0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_solid_taper");
    wood_grid::Building::from_solid(Mesh::loft({BOTTOM}, {TOP}), ELEVATIONS, PATTERN).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow A on a tapered massing: a loft between two rectangles sliced at four storeys, every level's section filled with the same orthogonal pattern, the interior columns vertical and the perimeter columns inclined to follow the moving section within the taper angle, decks and edge members on each level's own section, purlin stations across every bay. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_solid_taper --parallel 4 && ./build/templates_grid_solid_taper && ../bash/publish-scene.sh --target templates_grid_solid_taper

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
