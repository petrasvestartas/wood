#include "wood_session.h"
using namespace session_cpp;
using namespace wood_session;

static void run(const std::string& name, const double division) {

    config::reset_defaults();

    if (!config::plates_exist(name))
        return;

    WoodSession scene = WoodSession::obj_load(name);
    if (division > 0)
        scene.settings.joint_parameters[1*3+0] = division;
    scene.settings.joint_parameters[1*3+2] = 10;
    scene.settings.joint_parameters[2*3+2] = 20;
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