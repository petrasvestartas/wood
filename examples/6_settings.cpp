#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::annen_box};

int main() {

    WoodSession scene = WoodSession::obj_load(DATASET);

    Settings& s = scene.settings;
    s.joint_parameters[1 * 3 + 0] = 200;   // ss_e_op family: division length
    s.joint_parameters[1 * 3 + 2] = 10;    // ss_e_op family: joint id
    s.joint_parameters[2 * 3 + 2] = 20;    // ts_e_p family: joint id
    s.joint_volume_extension = {0, 0, 10};
    s.joint_scale = {1.0, 1.0, 1.0};
    s.distance = 0.1;
    s.angle = 0.11;

    scene.compute_features(face_to_face);
    for (const FeaturePlate& joint : scene.get_plate_features())
        std::cout << fmt::format("{} divisions {} length {:.0f}\n", joint.name, joint.divisions, joint.length);

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    std::cout << "the file carries the settings: division length " << back.settings.joint_parameters[3] << "\n";

    scene.add_to_tree();
    scene.pb_dump(pb_path("live").string());

    return 0;
}

/*
description: the solver settings as a value on the scene: joint ids and division lengths per family, volume extension, scale and tolerances set in code, the solve, and the settings read back from the file.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target 6_settings --parallel 4 && ./build/6_settings
cloudflare: ../bash/publish-scene.sh --target 6_settings
view: https://petrasvestartas.github.io/session/
*/
