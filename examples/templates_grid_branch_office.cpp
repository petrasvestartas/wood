#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const int VARIANT = 1; // Branch3D's framing sweep: 0 beams spanning x, 1 purlins spanning x at 10 ft, 2 purlins spanning y at 10 ft, 3 purlins spanning y at 20 ft
const double WIDTH = 45720.0; // 150 ft
const double DEPTH = 54864.0; // 180 ft
const std::vector<double> XS = wood_grid::compute_bays(WIDTH, 9144.0);
const std::vector<double> YS = wood_grid::compute_bays(DEPTH, VARIANT < 2 ? 6096.0 : 9144.0);
const std::vector<Polyline> CORES = {Polyline::rectangle(Point(15240.0, 27432.0, 0.0), X, Y, 9144.0, 9144.0), Polyline::rectangle(Point(18288.0, 14020.8, 0.0), X, Y, 6096.0, 3048.0)};
const std::vector<double> ELEVATIONS = {0.0, 4267.2, 8534.4, 12801.6}; // three of Branch's six storeys
const std::vector<wood_grid::Framing> FRAMINGS = {
    wood_grid::Framing{.system = 1, .span = 0, .node = 2, .deck = 241.8, .wall = 250.0, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 720.0)}},
    wood_grid::Framing{.system = 2, .span = 0, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 241.8, .wall = 250.0, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 720.0), .purlin = profile_rectangle(240.0, 520.0)}},
    wood_grid::Framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 241.8, .wall = 250.0, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 720.0), .purlin = profile_rectangle(260.0, 640.0)}},
    wood_grid::Framing{.system = 2, .span = 1, .spacing = 6096.0, .node = 2, .drop = 203.2, .deck = 241.8, .wall = 250.0, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 720.0), .purlin = profile_rectangle(260.0, 640.0)}},
};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_branch_office");
    wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, WIDTH, DEPTH)}, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS), CORES).to_session(wood_session, FRAMINGS[VARIANT]);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Branch3D's office preset on the grid template: a 150 by 180 ft rectangle with a stair core and a lift core, four framings by VARIANT over 30 ft columns lines, columns through the levels with the decks notched round them, cores as pinwheel walls with deck holes and every girder and purlin reaching them cut at their outer faces, no column inside or on a core. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_branch_office --parallel 4 && ./build/templates_grid_branch_office && ../bash/publish-scene.sh --target templates_grid_branch_office

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
