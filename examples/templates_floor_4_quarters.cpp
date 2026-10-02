#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const bool DUMP = true; // write name, volume and box centre of every element to data/output/pb/floor_4_quarters.txt, the parity record against compas_tf

/// Every element under the group as `name volume cx cy cz`, the box centre in world coordinates, the parity record against compas_tf.
void dump(const WoodSession& session, const std::string& path) {

    std::ofstream file(path);

    for (const std::shared_ptr<Element>& element : *session.objects.elements) {
        const Mesh& mesh = element->model_geometry_mesh();
        const AABB box = AABB::from_mesh(mesh);
        file << fmt::format("{} {:.6f} {:.6f} {:.6f} {:.6f}\n", element->name, compute_volume(mesh), box.cx, box.cy, box.cz);
    }
}

/// The square floor with the model's definitions, or in compas_tf's parity mode with --compas.
int main(int argc, char** argv) {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor = argc > 1 && std::string(argv[1]) == "--compas" ? wood_floor::Floor::compas_parity(plan, wood_floor::FloorSizes{}) : wood_floor::Floor(plan, wood_floor::FloorSizes{});
    std::cout << floor.check().str() << std::endl;
    WoodSession session("templates_floor_4_quarters");
    const std::shared_ptr<TreeNode> root = session.add_group("quarters_model");

    for (size_t q = 0; q < 4; q++)
        wood_floor::add_quarter_model(session, floor.quarter(q), wood_floor::add_group(session, fmt::format("quarter_model_{}", q), root));

    std::cout << fmt::format("{} elements: {} plates, {} variable beams", session.objects.elements->size(), session.plates().size(), session.beam_variables().size()) << std::endl;
    session.pb_dump(pb_path("live"));

    if constexpr (DUMP)
        dump(session, std::filesystem::path(pb_path("floor_4_quarters")).replace_extension(".txt").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the timber floor, port of compas_tf example_model_4_quarters: the four quarters of the bay, each built in place at its corner from the floor's shared planes and lifted to the floor at 3500, in the groups beds (one per row), tsections, outer_ribs, inner_ribs, wedges_inner_beams and inner_beams with the quarter index on every name. Ribs and inner beams are variable beams, every other member a plate. Prints the floor's report; DUMP writes name, volume and box centre of every element, compared against compas_tf with --compas.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_4_quarters --parallel 6 && ./build/templates_floor_4_quarters && ../bash/publish-scene.sh --target templates_floor_4_quarters

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
