#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::vector<Polyline> FOOTPRINT = {Polyline({Point(0.0, 0.0, 0.0), Point(45720.0, 0.0, 0.0), Point(45720.0, 21336.0, 0.0), Point(18288.0, 21336.0, 0.0), Point(18288.0, 48768.0, 0.0), Point(45720.0, 48768.0, 0.0), Point(45720.0, 70104.0, 0.0), Point(0.0, 70104.0, 0.0), Point(0.0, 0.0, 0.0)})}; // Branch3D topology T9, a U of 150 by 230 ft
const std::vector<double> XS = wood_grid::compute_bays(45720.0, 9144.0);
const std::vector<double> YS = wood_grid::compute_bays(70104.0, 9144.0);
const std::vector<Polyline> CORES = {Polyline::rectangle(Point(12192.0, 48768.0, 0.0), X, Y, 6096.0, 9144.0), Polyline::rectangle(Point(15240.0, 15240.0, 0.0), X, Y, 3048.0, 6096.0)}; // the first notches the inner corner of the courtyard
const std::vector<double> ELEVATIONS = {0.0, 4876.8, 9753.6, 14630.4}; // three of Branch's eight storeys
const wood_grid::Profiles GLULAM{.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0), .edge_girder = profile_rectangle(300.0, 720.0), .edge_beam = profile_rectangle(260.0, 600.0)};
const wood_grid::Framing FRAMING{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .wall = 250.0, .profiles = GLULAM};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_branch_institutional");
    wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS), CORES).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Branch3D's institutional preset on the grid template: a U footprint over 30 ft bays with two cores, one of them notching the inner corner of the courtyard, girders running y hung 8 in below the datum, purlin rows on the cross lines and stations at 10 ft, columns through the levels with the decks notched round them, core walls in pinwheels with the members that reach them cut at their outer faces. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_branch_institutional --parallel 4 && ./build/templates_grid_branch_institutional && ../bash/publish-scene.sh --target templates_grid_branch_institutional

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
