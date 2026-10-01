#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

const wood_floor::FloorGuide GUIDE{
    .size_grid_x = 3000.0,
    .size_grid_y = 3000.0,
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

    WoodSession session("templates_floor_2_column_model");
    const std::shared_ptr<TreeNode> group = session.add_group("column_model");

    const std::shared_ptr<Support> support = wood_floor::to_support(GUIDE);
    const std::shared_ptr<Column> column = wood_floor::to_column(GUIDE, *support);
    session.add(support, group);
    session.add(column, group);

    const Mesh& base = support->element_geometry_mesh();
    const Mesh& stock = column->element_geometry_mesh();
    std::cout << fmt::format("support volume {:.6f} closed {}", compute_volume(base), base.is_closed()) << std::endl;
    std::cout << fmt::format("stock   volume {:.6f} faces {} closed {}", compute_volume(stock), stock.number_of_faces(), stock.is_closed()) << std::endl;

    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const std::shared_ptr<Joint> joint = Joint::support(*support, *column);
    session.add(joint, group);
    session.add_joint(joint);

    for (const std::shared_ptr<Joint>& cutter : wood_floor::to_column_cutters(GUIDE, *column)) {
        session.add(cutter, group);
        session.add_joint(cutter);
    }

    const Mesh& carved = column->model_geometry_mesh();
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << fmt::format("carved  volume {:.6f} faces {} closed {} in {:.1f} ms", compute_volume(carved), carved.number_of_faces(), carved.is_closed(), ms) << std::endl;

    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the timber floor, port of compas_tf example_model_2_column_model: the support of one quarter, the Sherpa Power Base L 140 C built from its datasheet dimensions, and the column standing on it, a 220 square from the support's column foot (the head plate top at 150 less the 12 the head plate is let into the column end) to the floor at 3500, its outer corner on the grid corner, with the head 120 wider on the two bay sides over the top 730 built into the same solid. The support joint lets the head plate into the column end and drills the three column screws; the six column cutters of the guide, lifted to the floor, carve the head. Prints the volumes of the support, the stock and the carved column, the figures compared against compas_tf.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_2_column_model --parallel 6 && ./build/templates_floor_2_column_model && ../bash/publish-scene.sh --target templates_floor_2_column_model

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
