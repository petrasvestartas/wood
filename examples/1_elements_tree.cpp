#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::vector<double> XS = {4000.0};
const std::vector<double> YS = {3000.0};
const std::vector<double> ELEVATIONS = {0.0, 3700.0}; // column 3000 + head 300 + beam 200 + deck 200, the datum is the deck underside
const double GAP = 2000.0;
const wood_grid::Framing FRAMING{.system = 1, .span = 0, .node = 0, .deck = 200.0, .head = 300.0, .reach = 200.0, .profiles = {.column = profile_rectangle(200.0, 200.0), .girder = profile_rectangle(200.0, 200.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("elements_tree");

    for (int i = 0; i < 3; i++) {

        const Xform shift = Xform::translation(i * (XS[0] + 2.0 * FRAMING.reach + GAP), 0.0, 0.0);
        const std::vector<Polyline> footprint = {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, XS[0], YS[0]).transformed(shift)};
        const wood_grid::Building building = wood_grid::Building::from_footprint(footprint, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS).transformed(shift));
        const std::shared_ptr<TreeNode> branch = wood_session.add_group(fmt::format("bay_{}", i));

        for (const std::shared_ptr<Element>& element : building.to_elements(FRAMING, 0))
            wood_session.add(element, branch);
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(1);

    std::cout << wood_session;
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the floor bay of 1_elements_flat built on the grid template three times side by side, each bay a branch of the tree root; compute_contacts(1) pairs elements only inside the same branch, so no contact crosses from one bay to another. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances: five definitions, column, head, edge girder, edge beam and deck.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_elements_tree --parallel 4 && ./build/1_elements_tree && ../bash/publish-scene.sh --target 1_elements_tree

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
