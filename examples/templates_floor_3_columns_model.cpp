#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const wood_floor::FloorGuide SIZES{
    .size_column_head = 220.0,
    .size_column_head_chamfer = 120.0,
    .size_outer_ribs = 100.0,
    .size_inner_ribs = 60.0,
    .size_inner_beams = 60.0,
    .size_wedge = 240.0,
    .height = 650.0,
    .rise = 453.0,
    .size_oculus = 1000.0,
};

int main() {

    WoodSession session("templates_floor_3_columns_model");
    const std::shared_ptr<TreeNode> root = session.add_group("columns_model");

    for (int i = 0; i < 4; i++) {
        const std::string suffix = fmt::format("_{}", i);
        const std::shared_ptr<TreeNode> group = wood_floor::add_group(session, "column_model" + suffix, root);
        const std::shared_ptr<Column> column = wood_floor::add_column_model(session, wood_floor::FloorGuide::rectangle(3000.0, 3000.0, i, SIZES), group, suffix);
        std::cout << fmt::format("{} carved volume {:.3f} closed {}", column->name, compute_volume(column->model_geometry_mesh()), column->model_geometry_mesh().is_closed()) << std::endl;
    }

    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber floor, port of compas_tf example_model_3_columns_model: the column model of step 2, its support, the support joint and the carved column head, built in place at the four bay corners, one group per column with its index on every name. Prints the carved volume of each column, the four the same.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_3_columns_model --parallel 6 && ./build/templates_floor_3_columns_model && ../bash/publish-scene.sh --target templates_floor_3_columns_model

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
