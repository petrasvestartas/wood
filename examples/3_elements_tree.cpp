#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> XS = {4000.0, 4000.0, 4000.0};
const std::vector<double> YS = {3000.0, 3000.0};
const std::vector<Polyline> FOOTPRINT = {Polyline({
    Point(0.0, 0.0, 0.0), 
    Point(12000.0, 0.0, 0.0), 
    Point(12000.0, 3000.0, 0.0), 
    Point(8000.0, 3000.0, 0.0), 
    Point(8000.0, 6000.0, 0.0), 
    Point(0.0, 6000.0, 0.0), 
    Point(0.0, 0.0, 0.0)})
}; // an L: three by two bays, the far corner bay left open
const std::vector<double> ELEVATIONS = {0.0, 3000.0, 6000.0}; // two storeys of column 2800 + head 200, each datum the top of the heads, members and deck
const wood_grid::Framing FRAMING{
    .system = 1, 
    .span = -1, 
    .node = 0, 
    .deck = 200.0, 
    .reach = 200.0, 
    .profiles = {
        .column = profile_rectangle(200.0, 200.0), 
        .girder = profile_rectangle(200.0, 200.0)
    }};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("elements_tree");
    const wood_grid::Building building = wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS));

    // storey_0, storey_1 under the root; under each a group per kind holding its elements
    for (size_t storey = 0; storey + 1 < ELEVATIONS.size(); storey++) {
 
        const std::shared_ptr<TreeNode> tree_storey = wood_session.add_group(fmt::format("storey_{}", storey));
        const std::vector<std::shared_ptr<Element>> elements = building.to_elements(FRAMING, storey);

        for (const std::string kind : {"column", "head", "beam", "deck"}) {
            const std::shared_ptr<TreeNode> tree_type = std::make_shared<TreeNode>(kind);
            wood_session.add(tree_type, tree_storey);

            for (const std::shared_ptr<Element>& element : elements)
                if (element->name == kind)
                    wood_session.add(element, tree_type);
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
an L of five bays - three by two with the far corner bay left open - post and beam with 200 square columns and beams, stacked two storeys high, a beam on every grid line between the column heads (span -1), so the heads meet every case - two beams at the outer corners, three on the edges, four inside and at the re-entrant corner, every head chamfered between each two neighbouring beams - and every deck sits between four beams with its corners cut by the heads; each storey a branch of the tree root and every kind of element a twig under it: columns, heads, beams and decks; each column stands on the head below it, so compute_face_contacts(0) pairs every element with every other across the storeys. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances: four definitions, column, head, beam and deck.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 3_elements_tree --parallel 4 && ./build/3_elements_tree && ../bash/publish-scene.sh --target 3_elements_tree

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
