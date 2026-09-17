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
    for (const std::shared_ptr<Plate>& plate : internal::load_plates(name))
        scene.add(plate);
    scene.compute_joints(face_to_face);
    pb_dump(scene, "live");
}

int main() {
    
    run("annen_box", 200);
    run("annen_box_pair", 200);
    run("annen_grid_small", 200);
    run("annen_grid_full_arch", 0);
    run("annen_corner", 0);
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Compute joints between wood elements in a dataset.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/main_joint_types && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target main_joint_types

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/