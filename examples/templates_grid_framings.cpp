#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double BAY = 9000.0;
const double GAP = 3000.0;
const std::vector<double> ELEVATIONS = {0.0, 4500.0};
const std::vector<double> DROPS = {0.0, 203.2, 640.0}; // girder tops below the datum: flush with the purlins, hung 8 in as Branch3D, stacked under the purlins
const wood_grid::Framing FRAMING{.system = 2, .span = 0, .spacing = 3000.0, .node = 1, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_framings");

    for (size_t i = 0; i < DROPS.size(); i++) {

        const Xform shift = Xform::translation(i * (BAY + GAP), 0.0, 0.0);
        const std::vector<Polyline> footprint = {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, BAY, BAY).transformed(shift)};
        const wood_grid::Building building = wood_grid::Building::from_footprint(footprint, ELEVATIONS, wood_grid::Pattern::orthogonal({BAY}, {BAY}).transformed(shift));
        wood_grid::Framing framing = FRAMING;
        framing.drop = DROPS[i];
        const std::shared_ptr<TreeNode> branch = wood_session.add_group(fmt::format("bay_{}", i));

        for (const std::shared_ptr<Element>& element : building.to_elements(framing, 0))
            wood_session.add(element, branch);
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(1);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The three purlin on girder joints side by side, one bay each: flush, where the purlin is cut by the girder side over its full depth; hung, where the girder top sits 8 in lower and the purlin bears on the part of its side above the girder top; stacked, where the girder drops a whole purlin depth and the purlin runs over it bearing on its top; compute_contacts(1) pairs elements inside each bay only. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_framings --parallel 4 && ./build/templates_grid_framings && ../bash/publish-scene.sh --target templates_grid_framings

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
