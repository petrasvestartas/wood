#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const bool DUMP = true; // write name, volume and box centre of every element to data/output/pb/floor_4_quarters.txt, the parity record against compas_tf

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

/// Every element under the group as `name volume cx cy cz`, the box centre in world coordinates, the parity record against compas_tf.
void dump(const WoodSession& session, const std::string& path) {

    std::ofstream file(path);

    for (const std::shared_ptr<Element>& element : *session.objects.elements) {
        const Mesh& mesh = element->model_geometry_mesh();
        const AABB box = AABB::from_mesh(mesh);
        file << fmt::format("{} {:.6f} {:.6f} {:.6f} {:.6f}\n", element->name, compute_volume(mesh), box.cx, box.cy, box.cz);
    }
}

int main() {

    WoodSession session("templates_floor_4_quarters");
    const std::shared_ptr<TreeNode> root = session.add_group("quarters_model");

    for (int i = 0; i < 4; i++) {
        const std::string suffix = fmt::format("_{}", i);
        const std::shared_ptr<TreeNode> group = wood_floor::add_group(session, "quarter_model" + suffix, root);
        wood_floor::add_quarter_model(session, GUIDE, Xform::rotation_z(i * 90.0, true), group, suffix);
    }

    std::cout << fmt::format("{} elements: {} plates, {} variable beams", session.objects.elements->size(), session.plates().size(), session.beam_variables().size()) << std::endl;
    session.pb_dump(pb_path("live"));

    if constexpr (DUMP)
        dump(session, std::filesystem::path(pb_path("floor_4_quarters")).replace_extension(".txt").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the timber floor, port of compas_tf example_model_4_quarters: one quarter of the bay from the FloorGuide, lifted to the floor at 3500, in the groups beds (one per row), tsections, outer_ribs, inner_ribs, wedges_inner_beams and inner_beams, placed four times by quarter turns about the bay centre with the quarter index on every name. Ribs and inner beams are variable beams, every other member a plate. DUMP writes name, volume and box centre of every element, compared against compas_tf.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_4_quarters --parallel 6 && ./build/templates_floor_4_quarters && ../bash/publish-scene.sh --target templates_floor_4_quarters

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
