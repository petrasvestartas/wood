#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

/// The square floor with the model's definitions, or in compas_tf's parity mode with --compas.
int main(int argc, char** argv) {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor = argc > 1 && std::string(argv[1]) == "--compas" ? wood_floor::Floor::compas_parity(plan, wood_floor::FloorSizes{}) : wood_floor::Floor(plan, wood_floor::FloorSizes{});
    std::cout << floor.check().str() << std::endl;
    WoodSession session("templates_floor_2_column_model");
    const std::shared_ptr<TreeNode> group = session.add_group("column_model");

    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const std::shared_ptr<Column> column = wood_floor::add_column_model(session, floor, 0, group).column;
    const std::shared_ptr<Support> support = session.supports().front();

    const Mesh& base = support->element_geometry_mesh();
    const Mesh& stock = column->element_geometry_mesh();
    std::cout << fmt::format("support volume {:.6f} closed {}", compute_volume(base), base.is_closed()) << std::endl;
    std::cout << fmt::format("stock   volume {:.6f} faces {} closed {}", compute_volume(stock), stock.number_of_faces(), stock.is_closed()) << std::endl;

    const Mesh& carved = column->model_geometry_mesh();
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << fmt::format("carved  volume {:.6f} faces {} closed {} in {:.1f} ms", compute_volume(carved), carved.number_of_faces(), carved.is_closed(), ms) << std::endl;

    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the timber floor, port of compas_tf example_model_2_column_model: the support of corner 0, the Sherpa Power Base L 140 C built from its datasheet dimensions, and the column standing on it, a 220 square in the corner frame from the support's column foot (the head plate top at 150 less the 12 the head plate is let into the column end) to the floor at 3500, its outer corner on the bay corner, with the head 120 wider on the two bay sides over the top 730 built into the same solid. The support joint lets the head plate into the column end and drills the three column screws; the six column cutters of quarter 0, lifted to the floor, carve the head, their middle level at the outer rib bottoms (compas_tf's 1.65 tsections level with --compas). Prints the floor's report and the volumes of the support, the stock and the carved column.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_2_column_model --parallel 6 && ./build/templates_floor_2_column_model && ../bash/publish-scene.sh --target templates_floor_2_column_model

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
