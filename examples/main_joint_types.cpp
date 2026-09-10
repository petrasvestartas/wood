#include "wood_session.h"
#include "../src/session.h"

using namespace session_cpp;
using namespace wood_session;

static void run(const std::string& name, const double division) {
    globals::reset_defaults();
    if (!internal::plates_exist(name))
        return;
    if (division > 0)
        globals::JOINTS_PARAMETERS_AND_TYPES[1*3+0] = division;
    globals::JOINTS_PARAMETERS_AND_TYPES[1*3+2] = 10;
    globals::JOINTS_PARAMETERS_AND_TYPES[2*3+2] = 20;

    WoodSession scene(globals::DATA_SET_INPUT_NAME);
    for (const WoodElement& element : internal::load_plates(name))
        scene.add(std::make_shared<WoodElement>(element));
    scene.compute_joints(face_to_face);
    pb_dump(scene, globals::DATA_SET_INPUT_NAME);
}

int main() {
    run("annen_corner", 0);
    run("annen_box", 200);
    run("annen_box_pair", 200);
    run("annen_grid_small", 200);
    run("annen_grid_full_arch", 0);
    return 0;
}

/*
description: five annen datasets with ss_e_op (11) and ts_e_p (20) joints -> data/output/pb/<name>.pb each.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_joint_types -j8 && ./build/main_joint_types
cloudflare: ../bash/publish-scene.sh data/output/pb/annen_corner.pb
view: https://petrasvestartas.github.io/session/
*/
