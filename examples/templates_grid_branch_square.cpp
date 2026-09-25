#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const int SYSTEM = 1; // Branch3D's three structural methods: 0 plate on columns, 1 post and beam, 2 purlin on girder
const double SIDE = 18288.0; // 60 ft
const std::vector<double> XS = wood_grid::compute_bays(SIDE, 4572.0);
const std::vector<double> YS = wood_grid::compute_bays(SIDE, 4572.0);
const std::vector<double> ELEVATIONS = {0.0, 3657.6, 7315.2};
const std::vector<wood_grid::Framing> FRAMINGS = {
    wood_grid::Framing{.system = 0, .edge = false, .node = 0, .deck = 365.1, .head = 300.0, .reach = 600.0, .panel = 3505.2},
    wood_grid::Framing{.system = 1, .span = 0, .node = 2, .deck = 189.8, .profiles = {.girder = profile_rectangle(220.0, 520.0), .edge_girder = profile_rectangle(220.0, 440.0), .edge_beam = profile_rectangle(220.0, 280.0)}},
    wood_grid::Framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 189.8, .profiles = {.girder = profile_rectangle(220.0, 520.0), .purlin = profile_rectangle(220.0, 400.0)}},
};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_branch_square");
    wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, SIDE, SIDE)}, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)).to_session(wood_session, FRAMINGS[SYSTEM]);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Branch3D's 60 ft square over 15 ft bays in its three structural methods by SYSTEM: plate on columns with heads under the CLT strips and no members, post and beam with girders on the x lines and the deck spanning to them, purlin on girder with the girders running y hung 8 in and purlin rows at 10 ft; under the two framed methods the columns run through the two storeys with the decks notched round them. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_branch_square --parallel 4 && ./build/templates_grid_branch_square && ../bash/publish-scene.sh --target templates_grid_branch_square

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
