#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::vector<double> XS = {4000.0, 4000.0};
const std::vector<double> YS = {3000.0, 3000.0};
const std::vector<Polyline> FOOTPRINT = {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 8000.0, 6000.0)};
const std::vector<double> ELEVATIONS = {0.0, 3000.0, 6000.0, 9000.0}; // three storeys of column 2800 + head 200, each datum the top of the heads, members and deck
const wood_grid::Framing FRAMING{.system = 1, .span = -1, .node = 0, .deck = 200.0, .reach = 200.0, .profiles = {.column = profile_rectangle(200.0, 200.0), .girder = profile_rectangle(200.0, 200.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("elements_tree");
    const wood_grid::Building building = wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS));

    // one branch per storey, one twig per kind of element under it
    for (size_t storey = 0; storey + 1 < ELEVATIONS.size(); storey++) {
        const std::shared_ptr<TreeNode> branch = wood_session.add_group(fmt::format("storey_{}", storey));
        std::map<std::string, std::shared_ptr<TreeNode>> kinds;
        for (const std::shared_ptr<Element>& element : building.to_elements(FRAMING, storey)) {
            if (!kinds.count(element->name)) {
                kinds[element->name] = std::make_shared<TreeNode>(element->name + "s");
                wood_session.add(kinds[element->name], branch);
            }
            wood_session.add(element, kinds[element->name]);
        }
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_face_contacts(0);

    std::cout << wood_session;
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
two by two bays of the 1_elements_flat framing stacked three storeys high, a beam on every grid line between the column heads (span -1), so the heads meet every case - two beams at the corners, three on the edges, four at the centre - and every deck sits between four beams with its corners cut by the heads; each storey a branch of the tree root and every kind of element a twig under it: columns, heads, beams and decks; each column stands on the head below it, so compute_face_contacts(0) pairs every element with every other across the storeys. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances: four definitions, column, head, beam and deck.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_elements_tree --parallel 4 && ./build/1_elements_tree && ../bash/publish-scene.sh --target 1_elements_tree

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
