#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::vector<Polyline> FOOTPRINT = {Polyline({Point(0.0, 0.0, 0.0), Point(21945.6, 0.0, 0.0), Point(21945.6, 45720.0, 0.0), Point(43891.2, 45720.0, 0.0), Point(43891.2, 67056.0, 0.0), Point(0.0, 67056.0, 0.0), Point(0.0, 0.0, 0.0)})}; // Branch3D residential L, 72 by 220 ft
const std::vector<double> XS = wood_grid::compute_bays(43891.2, 9144.0); // 30 ft bays, the rest last
const std::vector<double> YS = wood_grid::compute_bays(67056.0, 9144.0);
const std::vector<Polyline> CORES = {Polyline::rectangle(Point(7467.6, 19354.8, 0.0), X, Y, 7010.4, 7010.4)};
const std::vector<double> HEIGHTS(3, 3657.6); // three of Branch's twelve storeys
const wood_grid::Profiles GLULAM{.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0), .edge_girder = profile_rectangle(300.0, 720.0), .edge_beam = profile_rectangle(260.0, 600.0)};
const wood_grid::Framing FRAMING{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 189.8, .wall = 250.0, .profiles = GLULAM};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_branch_residential");
    wood_grid::Building::from_footprint(FOOTPRINT, HEIGHTS, wood_grid::Pattern::orthogonal(XS, YS), CORES).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Branch3D's residential preset on the grid template: an L footprint over 30 ft bays with the rest as the last bay, girders running y hung 8 in below the datum, purlin rows on the cross lines and stations at 10 ft, columns through the levels with the decks notched round them, a core as four pinwheel walls with a deck hole and the girders and purlins that reach it cut at its outer faces; no column inside or on the core. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_branch_residential --parallel 4 && ./build/templates_grid_branch_residential && ../bash/publish-scene.sh --target templates_grid_branch_residential

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
