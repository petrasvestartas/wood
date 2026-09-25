#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<Polyline> FOOTPRINT = {Polyline({Point(0.0, 0.0, 0.0), Point(13716.0, 13716.0, 0.0), Point(36576.0, 13716.0, 0.0), Point(36576.0, 27432.0, 0.0), Point(0.0, 27432.0, 0.0), Point(0.0, 0.0, 0.0)})}; // Branch3D preset P19: one diagonal side
const std::vector<double> XS = wood_grid::compute_bays(36576.0, 9144.0);
const std::vector<double> YS = wood_grid::compute_bays(27432.0, 9144.0);
const std::vector<double> ELEVATIONS = {0.0, 3657.6, 7315.2, 10972.8};
const wood_grid::Profiles GLULAM{.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0), .edge_girder = profile_rectangle(300.0, 720.0), .edge_beam = profile_rectangle(260.0, 600.0)};
const wood_grid::Framing FRAMING{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .profiles = GLULAM};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_pentagon");
    wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Branch3D's pentagon preset on the grid template: an orthogonal 30 ft grid clipped by a diagonal side, girders running y hung 8 in below the datum, purlin rows on the cross lines and stations at 10 ft spaced over the unclipped cells, columns through the levels; the edge member on the diagonal runs through and every girder, purlin and deck meeting it ends on an oblique cut. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_pentagon --parallel 4 && ./build/templates_grid_pentagon && ../bash/publish-scene.sh --target templates_grid_pentagon

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
