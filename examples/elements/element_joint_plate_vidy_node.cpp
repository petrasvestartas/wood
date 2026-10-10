#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// One Vidy node: two wall plates and two roof plates meeting at one edge, joined by the linked tenons ss_e_op_5 and their shadow joint.
int main() {

    // the four plates of the first node of vidy_corner, its walls 6 and 7, its roof plates 9 and 8, with the dataset's settings
    const WoodSession corner = WoodSession::yaml_load(config::Dataset::vidy_corner);
    WoodSession scene("element_joint_plate_vidy_node");
    scene.settings = corner.settings;
    const std::vector<std::shared_ptr<Plate>> plates = corner.plates();
    const std::array<std::string, 4> names = {"wall_0", "wall_1", "roof_0", "roof_1"};
    const std::array<int, 4> node = {6, 7, 9, 8};
    for (size_t i = 0; i < 4; i++) {
        const std::shared_ptr<Plate> plate = std::make_shared<Plate>(*plates[node[i]]);
        plate->name = names[i];
        scene.add(plate);
    }

    // the node as a three-valence group: the Vidy instruction 1, then the two walls and the two roof plates in the sidecar's order
    scene.three_valence = {{1}, {0, 1, 2, 3}};

    // every joint the four plates need, added and passed to each plate with add_interaction
    scene.compute_features();

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
One Vidy node: two wall plates and two roof plates meeting at one edge, joined by the linked tenons ss_e_op_5 and their shadow joint.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_vidy_node --parallel 6 && ./build/element_joint_plate_vidy_node && ../bash/publish-scene.sh --target element_joint_plate_vidy_node

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
