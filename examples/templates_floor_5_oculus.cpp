#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const bool DUMP = true; // write name, volume and box centre of every element to data/output/pb/floor_5_oculus.txt, the parity record against compas_tf

const wood_floor::FloorGuide GUIDE{.size_grid_x = 3000.0, .size_grid_y = 3000.0, .size_oculus = 1000.0, .sizes = wood_floor::FloorSizes{}};

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

    WoodSession session("templates_floor_5_oculus");
    const std::shared_ptr<TreeNode> group = session.add_group("oculus");
    wood_floor::add_oculus_model(session, GUIDE, group);

    std::cout << fmt::format("{} elements: {} plates, {} variable beams", session.objects.elements->size(), session.plates().size(), session.beam_variables().size()) << std::endl;
    session.pb_dump(pb_path("live"));

    if constexpr (DUMP)
        dump(session, std::filesystem::path(pb_path("floor_5_oculus")).replace_extension(".txt").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the timber floor, port of compas_tf example_model_5_oculus: the oculus of the bay from the FloorGuide lifted to the floor at 3500, its four boundary beams as variable beams, the four bottom wedges and the inner plate as plates. DUMP writes name, volume and box centre of every element, compared against compas_tf.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_5_oculus --parallel 6 && ./build/templates_floor_5_oculus && ../bash/publish-scene.sh --target templates_floor_5_oculus

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
