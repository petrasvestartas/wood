#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> XS = {4000.0};
const std::vector<double> YS = {3000.0};
const std::vector<double> HEIGHTS = {3700.0}; // column 3000 + head 300 + beam 200 + deck 200, the node is the deck top
const double STRUCTURAL_SYSTEM = 1.0; // post and beam: girders on two sides, the deck spans between them
const bool LONGEST = true; // girders on the long sides, along x
const double ANGLE = 10.0;
const double GAP = 2000.0;
const wood_grid::Dimensions DIMENSIONS{.column = 200.0, .head = 300.0, .reach = 200.0, .beam = 200.0, .purlin = 200.0, .deck = 200.0, .wall = 100.0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("elements_tree");

    for (int i = 0; i < 3; i++) {

        const Mesh plan = wood_grid::create_orthogonal(XS, YS).transformed(Xform::translation(i * (XS[0] + 2 * DIMENSIONS.reach + GAP), 0.0, 0.0));
        wood_grid::Grid grid = wood_grid::Grid::from_plan(plan, HEIGHTS);
        grid.update_default_face_attributes({{"structural_system", STRUCTURAL_SYSTEM}});
        wood_grid::compute_faces(grid, ANGLE);
        wood_grid::compute_spans(grid, LONGEST);
        wood_grid::compute_members(grid, ANGLE);
        wood_grid::compute_supports(grid);

        const std::shared_ptr<TreeNode> branch = wood_session.add_group(fmt::format("bay_{}", i));

        for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"column", 1.0}}))
            wood_session.add(wood_grid::to_column(grid, edge, DIMENSIONS), branch);

        for (const std::string& node : grid.graph.vertices_where({{"head", 1.0}}))
            wood_session.add(wood_grid::to_head(grid, node, DIMENSIONS), branch);

        for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"beam", 1.0}}))
            wood_session.add(wood_grid::to_beam(grid, edge, DIMENSIONS), branch);

        for (const size_t face : grid.faces_where({{"floor", 1.0}}))
            wood_session.add(wood_grid::to_deck(grid, face, DIMENSIONS), branch);
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(1);

    std::cout << wood_session;
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the floor bay of 1_elements_flat built on the grid template three times side by side, each bay a branch of the tree root; compute_contacts(1) pairs elements only inside the same branch, so no contact crosses from one bay to another. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_elements_tree --parallel 4 && ./build/1_elements_tree && ../bash/publish-scene.sh --target 1_elements_tree

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
