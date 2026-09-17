#include "wood_session.h"

using namespace wood_session;

// globals::Dataset::Face::<name> for SEARCH = face_to_face, ::Cross::<name> for cross_joint.
const std::string DATASET = globals::Dataset::inplane_hexshell;
const SearchType SEARCH = SearchType::face_to_face;  // face_to_face | cross_joint | face_to_face_then_cross

int main() {
    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_joints(SEARCH);
    wood_session.add_joints();
    wood_session.write("live");
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
cmake --build build --config Release --parallel && ./build/3_joint_detection && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 3_joint_detection

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
