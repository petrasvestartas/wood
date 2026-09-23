#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const double SIDE = 4000.0;
const int NX = 3;
const int NY = 2;
const std::vector<double> HEIGHTS = {4000.0, 3600.0};
const double STRUCTURAL_SYSTEM = 2.0; // purlin on girder: every side ties, so side 0 and its opposite carry the girders
const double SPACING = 3000.0; // max purlin spacing, two purlins per cell
const double ANGLE = 10.0;
const wood_grid::Dimensions DIMENSIONS{.column = 240.0, .head = 300.0, .reach = 400.0, .beam = 240.0, .purlin = 200.0, .deck = 160.0, .wall = 120.0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    wood_grid::Grid grid = wood_grid::Grid::from_plan(wood_grid::create_hexagonal(SIDE, NX, NY), HEIGHTS);
    grid.update_default_face_attributes({{"structural_system", STRUCTURAL_SYSTEM}});
    wood_grid::compute_faces(grid, ANGLE);
    wood_grid::compute_spans(grid);
    wood_grid::compute_members(grid, ANGLE);
    wood_grid::compute_purlins(grid, SPACING);
    wood_grid::compute_supports(grid);

    WoodSession wood_session("templates_grid_hex");

    std::vector<std::shared_ptr<TreeNode>> storeys;
    for (size_t storey = 0; storey < HEIGHTS.size(); storey++)
        storeys.push_back(wood_session.add_group(fmt::format("storey_{}", storey)));
    const auto branch = [&](const std::optional<double>& storey) { return storeys[static_cast<size_t>(storey.value_or(0.0))]; };

    for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"column", 1.0}}))
        wood_session.add(wood_grid::to_column(grid, edge, DIMENSIONS), branch(grid.graph.edge_attribute(edge, "storey")));

    for (const std::string& node : grid.graph.vertices_where({{"head", 1.0}}))
        wood_session.add(wood_grid::to_head(grid, node, DIMENSIONS), branch(grid.graph.vertex_attribute(node, "storey")));

    for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"beam", 1.0}}))
        wood_session.add(wood_grid::to_beam(grid, edge, DIMENSIONS), branch(grid.graph.edge_attribute(edge, "storey")));

    for (const size_t face : grid.faces_where({{"floor", 1.0}})) {
        wood_session.add(wood_grid::to_deck(grid, face, DIMENSIONS), branch(grid.face_attribute(face, "storey")));
        for (const std::shared_ptr<Beam>& purlin : wood_grid::to_purlins(grid, face, DIMENSIONS))
            wood_session.add(purlin, branch(grid.face_attribute(face, "storey")));
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a hexagonal plan over two storeys: three-valent nodes with hexagonal heads, one through girder per node with the other two lines butting against its sides, two purlins per cell between its girder sides, storey_k groups and contacts over the whole scene. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_hex --parallel 4 && ./build/templates_grid_hex && ../bash/publish-scene.sh --target templates_grid_hex

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
