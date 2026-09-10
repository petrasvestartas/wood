#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int DATASET = 20;                 // globals::DATASET_NAMES
const bool CONTACTS_OR_JOINTS = true;   // face contacts between any elements, or joints between plates

int main() {
    WoodSession scene = WoodSession::yaml_load(globals::DATASET_NAMES[DATASET]);
    Session view(globals::DATASET_NAMES[DATASET]);
    add_outlines(view, scene);
    if constexpr (CONTACTS_OR_JOINTS) {
        scene.compute_contacts();
        add_contacts_by_type(view, scene.contacts());
    } else {
        scene.compute_joints(face_to_face);
        add_joints_by_type(view, scene.joints());
    }
    pb_dump(view, "live");
    return 0;
}

/*
description: one dataset by index -> face contacts between its elements, or the joints the solver makes between its plates, each on the graph edge of its pair -> drawn as the elements' bottom and top outlines with one colored ring per contact or joint type.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target 2_contact_detection -j8 && ./build/2_contact_detection
cloudflare: ../bash/publish-scene.sh --target 2_contact_detection
view: https://petrasvestartas.github.io/session/
*/
