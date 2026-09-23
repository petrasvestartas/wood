#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> RADII = {4000.0, 8000.0, 12000.0}; // an atrium in the middle, no many-valent centre node
const int SECTORS = 12;
const std::vector<double> HEIGHTS = {4000.0};
const double STRUCTURAL_SYSTEM = 2.0; // purlin on girder
const double SPAN = 0.0; // side 0 of every sector is a ray: the rays carry the girders, the purlins run round
const double SPACING = 2000.0; // max purlin spacing
const double ANGLE = 10.0;
const wood_grid::Dimensions DIMENSIONS{.column = 240.0, .head = 300.0, .reach = 400.0, .beam = 240.0, .purlin = 200.0, .deck = 160.0, .wall = 120.0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    wood_grid::Grid grid = wood_grid::Grid::from_plan(wood_grid::create_radial(RADII, SECTORS), HEIGHTS);
    grid.update_default_face_attributes({{"structural_system", STRUCTURAL_SYSTEM}, {"span", SPAN}});
    wood_grid::compute_faces(grid, ANGLE);
    wood_grid::compute_spans(grid);
    wood_grid::compute_members(grid, ANGLE);
    wood_grid::compute_purlins(grid, SPACING);
    wood_grid::compute_supports(grid);

    WoodSession wood_session("templates_grid_radial");

    for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"column", 1.0}}))
        wood_session.add(wood_grid::to_column(grid, edge, DIMENSIONS));

    for (const std::string& node : grid.graph.vertices_where({{"head", 1.0}}))
        wood_session.add(wood_grid::to_head(grid, node, DIMENSIONS));

    for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"beam", 1.0}}))
        wood_session.add(wood_grid::to_beam(grid, edge, DIMENSIONS));

    for (const size_t face : grid.faces_where({{"floor", 1.0}})) {
        wood_session.add(wood_grid::to_deck(grid, face, DIMENSIONS));
        for (const std::shared_ptr<Beam>& purlin : wood_grid::to_purlins(grid, face, DIMENSIONS))
            wood_session.add(purlin);
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on a radial plan: two rings of twelve sectors round an atrium, girders on the rays, purlins between them across each sector, heads shaped by the lines that meet at each node, mitred girders where two rays run straight on. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_radial --parallel 4 && ./build/templates_grid_radial && ../bash/publish-scene.sh --target templates_grid_radial

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
