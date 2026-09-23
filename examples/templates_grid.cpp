#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> XS = {9140.0, 8530.0, 9140.0};
const std::vector<double> YS = {7600.0, 7600.0};
const std::vector<double> HEIGHTS = {4500.0, 3800.0, 3800.0};
const size_t VOID_BAY = 5; // far corner bay, top 0: an L-shaped footprint
const double STRUCTURAL_SYSTEM = 2.0; // purlin on girder: girders on the short sides, purlins between them
const double SPACING = 3000.0; // max purlin spacing
const double ANGLE = 10.0;
const wood_grid::Dimensions DIMENSIONS{.column = 365.0, .head = 400.0, .reach = 400.0, .beam = 365.0, .purlin = 265.0, .deck = 175.0, .wall = 175.0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    Mesh plan = wood_grid::create_orthogonal(XS, YS);
    plan.set_face_attribute(VOID_BAY, "top", 0.0);
    for (const std::pair<size_t, size_t>& edge : plan.edges_on_boundary())
        plan.set_edge_attribute(edge, "wall", 1.0);

    const std::vector<size_t> loop = *plan.face_vertices(VOID_BAY);
    for (size_t i = 0; i < loop.size(); i++)
        plan.set_edge_attribute({loop[i], loop[(i + 1) % loop.size()]}, "wall", 1.0);

    wood_grid::Grid grid = wood_grid::Grid::from_plan(plan, HEIGHTS);
    grid.update_default_face_attributes({{"structural_system", STRUCTURAL_SYSTEM}});
    wood_grid::compute_faces(grid, ANGLE);
    wood_grid::compute_spans(grid);
    wood_grid::compute_members(grid, ANGLE);
    wood_grid::compute_purlins(grid, SPACING);
    wood_grid::compute_supports(grid);

    WoodSession wood_session("templates_grid");

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

    for (const size_t face : grid.faces_where({{"wall", 1.0}}))
        wood_session.add(wood_grid::to_wall(grid, face, DIMENSIONS), branch(grid.face_attribute(face, "storey")));

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a three storey orthogonal building: non-uniform bays, one corner bay left void for an L-shaped footprint, purlin on girder floors, walls on the footprint boundary; every element under the storey_k group it caps or stands in, contacts over the whole scene because a column stands on the deck of the storey below. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid --parallel 4 && ./build/templates_grid && ../bash/publish-scene.sh --target templates_grid

|||||||| WORKFLOW ||||||||
examples/templates_grid.cpp
 |
 |-- create_orthogonal(XS, YS) -> plan mesh; "top" 0 on the void bay, "wall" 1 on the footprint edges
 |-- Grid::from_plan(plan, HEIGHTS)                                                  src/templates/grid.h
 |-- compute_faces, compute_spans, compute_members, compute_purlins, compute_supports
 |-- to_column, to_head, to_beam, to_deck, to_purlins, to_wall -> add(element, storey_k)
 |-- instance_by_key() when INSTANCES
 |-- compute_contacts(0)
 '-- pb_dump(pb_path("live"))                                                       data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
