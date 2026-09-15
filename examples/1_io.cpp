#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int SESSION = 0;   // globals::SESSION_NAMES
const std::string DATASET = globals::Dataset::hex_block_rossiniere;  // globals::Dataset::<name> autocompletes

int main() {
    const Session session = Session::pb_load(globals::session_pb(SESSION));
    const WoodSession scene = WoodSession::from_session(session);
    std::cout << scene << std::endl;
    pb_dump(scene, globals::SESSION_NAMES[SESSION]);

    const WoodSession scene_plates = WoodSession::yaml_load(DATASET);
    std::cout << scene_plates << std::endl;
    pb_dump(scene_plates, "live"); // Live will be seen in the viewer.

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
WoodSession serialization example.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/1_io && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 1_io

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/