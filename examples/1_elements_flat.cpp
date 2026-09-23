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
const wood_grid::Dimensions DIMENSIONS{.column = 200.0, .head = 300.0, .reach = 200.0, .beam = 200.0, .purlin = 200.0, .deck = 200.0, .wall = 100.0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    wood_grid::Grid grid = wood_grid::Grid::from_plan(wood_grid::create_orthogonal(XS, YS), HEIGHTS);
    grid.update_default_face_attributes({{"structural_system", STRUCTURAL_SYSTEM}});
    wood_grid::compute_faces(grid, ANGLE);
    wood_grid::compute_spans(grid, LONGEST);
    wood_grid::compute_members(grid, ANGLE);
    wood_grid::compute_supports(grid);

    WoodSession wood_session("elements_flat");

    for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"column", 1.0}}))
        wood_session.add(wood_grid::to_column(grid, edge, DIMENSIONS));

    for (const std::string& node : grid.graph.vertices_where({{"head", 1.0}}))
        wood_session.add(wood_grid::to_head(grid, node, DIMENSIONS));

    for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"beam", 1.0}}))
        wood_session.add(wood_grid::to_beam(grid, edge, DIMENSIONS));

    for (const size_t face : grid.faces_where({{"floor", 1.0}}))
        wood_session.add(wood_grid::to_deck(grid, face, DIMENSIONS));

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);

    std::cout << wood_session;
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
one floor bay built on the grid template - a one-bay plan over one storey, rules that mark columns, heads, girders and beams on the graph, then four columns, four heads, two girders running through, two beams butting against them and a deck - added with no parent, so every element sits under the tree root; compute_contacts(0) pairs every element with every other and finds 20 contacts: column on head 4, girder and beam undersides on the flat head tops 8, beam ends on girder sides 4, girder and beam tops under the deck 4; the heads stop under the beams, so no head touches the deck. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_elements_flat --parallel 4 && ./build/1_elements_flat && ../bash/publish-scene.sh --target 1_elements_flat

|||||||| WORKFLOW ||||||||
examples/1_elements_flat.cpp
 |
 |-- create_orthogonal(XS, YS) -> plan mesh; Grid::from_plan(plan, HEIGHTS)         src/templates/grid.h
 |-- compute_faces, compute_spans, compute_members, compute_supports              attributes on the kernel Graph
 |-- edges_where / vertices_where / faces_where -> to_column, to_head, to_beam, to_deck
 |-- instance_by_key() when INSTANCES
 |-- compute_contacts(0)                                                            coplanar face overlaps, cut beam ends included
 '-- pb_dump(pb_path("live"))                                                       data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
