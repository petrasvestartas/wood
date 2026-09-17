#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int DATASET = 19;                 // globals::DATASET_NAMES

int main() {
    WoodSession wood_session = WoodSession::yaml_load(globals::DATASET_NAMES[DATASET]);
    wood_session.compute_contacts();
    wood_session.add_to_tree(true, true, true, false);
    wood_session.pb_dump(pb_path("live").string());
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Compute contacts between wood elements in a dataset.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/2_contact_detection && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 2_contact_detection

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
