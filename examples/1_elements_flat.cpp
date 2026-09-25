#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::vector<double> XS = {4000.0};
const std::vector<double> YS = {3000.0};
const std::vector<Polyline> FOOTPRINT = {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 4000.0, 3000.0)};
const std::vector<double> ELEVATIONS = {0.0, 3700.0}; // column 3000 + head 300 + beam 200 + deck 200, the datum is the deck underside
const wood_grid::Framing FRAMING{.system = 1, .span = 0, .node = 0, .deck = 200.0, .head = 300.0, .reach = 200.0, .profiles = {.column = profile_rectangle(200.0, 200.0), .girder = profile_rectangle(200.0, 200.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("elements_flat");
    wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);

    std::cout << wood_session;
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
one floor bay built on the grid template - a one-bay footprint over one storey, post and beam with the girders on the x sides: four columns, four heads, two edge girders running through, two edge beams butting against their sides and a deck resting on the member tops; compute_contacts(0) pairs every element with every other and finds 20 contacts: column on head 4, girder and beam undersides on the flat head tops 8, beam ends on girder sides 4, girder and beam tops under the deck 4; the heads stop under the members, so no head touches the deck. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_elements_flat --parallel 4 && ./build/1_elements_flat && ../bash/publish-scene.sh --target 1_elements_flat

|||||||| WORKFLOW ||||||||
examples/1_elements_flat.cpp
 |
 |-- Pattern::orthogonal(XS, YS) -> plan lines; Building::from_footprint(FOOTPRINT, ELEVATIONS, pattern)   src/templates/grid/grid.h
 |-- to_session(wood_session, FRAMING): roles, columns, joint cuts, elements under storey_0            src/templates/grid/grid_joints.h
 |-- instance_by_key() when INSTANCES
 |-- compute_contacts(0)                                                            coplanar face overlaps, cut beam ends included
 '-- pb_dump(pb_path("live"))                                                       data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
