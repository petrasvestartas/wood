#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::vector<Polyline> FOOTPRINT = {Polyline({Point(0.0, 0.0, 0.0), Point(21031.2, 0.0, 0.0), Point(21031.2, 45720.0, 0.0), Point(42062.4, 45720.0, 0.0), Point(42062.4, 67056.0, 0.0), Point(0.0, 67056.0, 0.0), Point(0.0, 0.0, 0.0)})}; // Branch3D topology T1, an L of 138 by 220 ft
const std::vector<double> XS = wood_grid::compute_bays(42062.4, 3505.2); // 11.5 ft, the CLT strip width
const std::vector<double> YS = wood_grid::compute_bays(67056.0, 4572.0); // 15 ft, the rest last
const std::vector<Polyline> CORES = {Polyline::rectangle(Point(14935.2, 45720.0, 0.0), X, Y, 6096.0, 9144.0), Polyline::rectangle(Point(11582.4, 9144.0, 0.0), X, Y, 3048.0, 6096.0)};
const std::vector<double> ELEVATIONS = {0.0, 3657.6, 7315.2, 10972.8}; // three of Branch's twelve storeys
const int CAPITAL = 1; // head under the deck: 0 conical, a frustum from the column to reach; 1 stepped, a capital under a drop panel
const wood_grid::Framing FRAMING{.system = 0, .span = 1, .edge = false, .node = 0, .deck = 291.4, .wall = 250.0, .head = 300.0, .reach = 600.0, .capital = CAPITAL, .panel = 3505.2};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_point_supported");
    wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS), CORES).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Branch3D's point supported plate on the grid template: an L footprint over a tight column grid of 11.5 by 15 ft with two cores, no beams at all, every column ending in a head under the deck, stepped by CAPITAL as a capital under a drop panel (0 a conical head), the CLT in strips as wide as the column spacing running along the y lines, so their joints fall on the column lines, core walls in pinwheels with deck holes, no column inside or on a core. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_point_supported --parallel 4 && ./build/templates_grid_point_supported && ../bash/publish-scene.sh --target templates_grid_point_supported

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
