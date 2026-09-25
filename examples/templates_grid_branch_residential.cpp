#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::vector<Polyline> FOOTPRINT = {Polyline({Point(0.0, 0.0, 0.0), Point(21945.6, 0.0, 0.0), Point(21945.6, 45720.0, 0.0), Point(43891.2, 45720.0, 0.0), Point(43891.2, 67056.0, 0.0), Point(0.0, 67056.0, 0.0), Point(0.0, 0.0, 0.0)})}; // Branch3D residential L, 72 by 220 ft
const int VARIANT = 0; // 0 Branch's residential preset: purlins on girders over 30 ft bays and one core; 1 Branch's topology T2: post and beam over 25 by 15 ft bays with its stair and lift cores
const std::vector<double> XS = wood_grid::compute_bays(43891.2, VARIANT == 0 ? 9144.0 : 7620.0); // whole bays, the rest last
const std::vector<double> YS = wood_grid::compute_bays(67056.0, VARIANT == 0 ? 9144.0 : 4572.0);
const std::vector<std::vector<Polyline>> CORES = {
    {Polyline::rectangle(Point(7467.6, 19354.8, 0.0), X, Y, 7010.4, 7010.4)},
    {Polyline::rectangle(Point(15849.6, 45720.0, 0.0), X, Y, 6096.0, 9144.0), Polyline::rectangle(Point(12496.8, 9144.0, 0.0), X, Y, 3048.0, 6096.0)},
};
const std::vector<double> ELEVATIONS = {0.0, 3657.6, 7315.2, 10972.8}; // three of Branch's twelve storeys
const wood_grid::Profiles GLULAM{.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0), .edge_girder = profile_rectangle(300.0, 720.0), .edge_beam = profile_rectangle(260.0, 600.0)};
const std::vector<wood_grid::Framing> FRAMINGS = {
    wood_grid::Framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 189.8, .wall = 250.0, .profiles = GLULAM},
    wood_grid::Framing{.system = 1, .span = 0, .node = 2, .deck = 189.8, .wall = 250.0, .profiles = GLULAM},
};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_branch_residential");
    wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS), CORES[VARIANT]).to_session(wood_session, FRAMINGS[VARIANT]);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Branch3D's residential L on the grid template, by VARIANT: the residential preset over 30 ft bays with girders running y hung 8 in below the datum, purlin rows on the cross lines and stations at 10 ft, and one core; or the topology T2 post and beam over 25 by 15 ft bays with Branch's stair and lift cores, where a pattern crossing near a core corner keeps its own column and the cores never pull one onto their walls. Columns run through the levels with the decks notched round them, cores are pinwheel walls with deck holes, every girder and purlin reaching a core is cut at its outer face, and no column stands inside, on or against a core wall. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_branch_residential --parallel 4 && ./build/templates_grid_branch_residential && ../bash/publish-scene.sh --target templates_grid_branch_residential

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
