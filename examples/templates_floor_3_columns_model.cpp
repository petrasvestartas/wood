#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The square floor with the model's definitions, or in compas_tf's parity mode with --compas.
int main(int argc, char** argv) {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor = argc > 1 && std::string(argv[1]) == "--compas" ? wood_floor::Floor::compas_parity(plan, wood_floor::FloorSizes{}) : wood_floor::Floor(plan, wood_floor::FloorSizes{});
    std::cout << floor.check().str() << std::endl;
    WoodSession session("templates_floor_3_columns_model");
    const std::shared_ptr<TreeNode> root = session.add_group("columns_model");

    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<TreeNode> group = wood_floor::add_group(session, fmt::format("column_model_{}", q), root);
        const std::shared_ptr<Column> column = wood_floor::add_column_model(session, floor, q, group).column;
        std::cout << fmt::format("{} carved volume {:.3f} closed {}", column->name, compute_volume(column->model_geometry_mesh()), column->model_geometry_mesh().is_closed()) << std::endl;
    }

    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber floor, port of compas_tf example_model_3_columns_model: the column model of step 2 built in place at each of the four bay corners from that corner's frame and its quarter's cutters, no rotation applied, one group per column with its index on every name. Prints the floor's report and the carved volume of each column, the four the same on the square.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_3_columns_model --parallel 6 && ./build/templates_floor_3_columns_model && ../bash/publish-scene.sh --target templates_floor_3_columns_model

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
