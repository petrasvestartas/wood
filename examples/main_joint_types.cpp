#include "wood_session.h"
using namespace session_cpp;
using namespace wood_session;

static void run(const std::string& name, const double division) {

    config::reset_defaults();

    if (!config::plates_exist(name))
        return;

    if (division > 0)
        config::JOINTS_PARAMETERS_AND_TYPES[1*3+0] = division;
    config::JOINTS_PARAMETERS_AND_TYPES[1*3+2] = 10;
    config::JOINTS_PARAMETERS_AND_TYPES[2*3+2] = 20;

    WoodSession scene = WoodSession::obj_load(name);
    scene.compute_joints(face_to_face);
    scene.add_to_tree();
    scene.pb_dump(pb_path("live").string());
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