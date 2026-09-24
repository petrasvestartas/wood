#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> XS = {9140.0, 8530.0, 9140.0};
const std::vector<double> YS = {7600.0, 7600.0};
const std::vector<Polyline> FOOTPRINT = {Polyline({Point(0.0, 0.0, 0.0), Point(26810.0, 0.0, 0.0), Point(26810.0, 7600.0, 0.0), Point(17670.0, 7600.0, 0.0), Point(17670.0, 15200.0, 0.0), Point(0.0, 15200.0, 0.0), Point(0.0, 0.0, 0.0)})}; // an L: the far corner bay left out
const std::vector<double> ELEVATIONS = {0.0, 4500.0, 8300.0, 12100.0};
const wood_grid::Framing FRAMING{.system = 2, .span = 0, .spacing = 3000.0, .node = 0, .deck = 175.0, .wall = 175.0, .facade = true, .profiles = {.column = profile_rectangle(365.0, 365.0), .girder = profile_rectangle(365.0, 365.0), .purlin = profile_rectangle(265.0, 265.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid");
    wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a three storey orthogonal building: non-uniform bays, an L-shaped footprint, purlin on girder floors with the girders on the x lines and purlin stations across every bay, facade walls under the perimeter members; every element under the storey_k group it caps or stands in, contacts over the whole scene because a column stands on the deck of the storey below. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid --parallel 4 && ./build/templates_grid && ../bash/publish-scene.sh --target templates_grid

|||||||| WORKFLOW ||||||||
examples/templates_grid.cpp
 |
 |-- Pattern::orthogonal(XS, YS) -> plan lines; Building::from_footprint(FOOTPRINT, ELEVATIONS, pattern)   src/templates/grid.h
 |-- to_session(wood_session, FRAMING): roles, columns, stations, joint cuts, elements under storey_k    src/templates/grid_joints.h
 |-- instance_by_key() when INSTANCES
 |-- compute_contacts(0)
 '-- pb_dump(pb_path("live"))                                                       data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
