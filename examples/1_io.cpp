#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int SESSION = 0;   // globals::SESSION_NAMES
const std::string DATASET = globals::Dataset::hex_block_rossiniere;  // globals::Dataset::<name> autocompletes

int main() {
    const Session session = Session::pb_load(globals::session_pb(SESSION));
    const WoodSession scene = WoodSession::from_session(session);
    pb_dump(scene, globals::SESSION_NAMES[SESSION]);

    const WoodSession plates = WoodSession::yaml_load(DATASET);
    pb_dump(plates, "live");
    return 0;
}

/*
description: a session .pb by index deserialized as a plain Session, then handed to from_session, which keeps its tree, graph and xforms and remakes every generic Element as the plate, column or solid its element_type names; then a dataset .yml by index -> the obj it names, as plates.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target 1_io -j8 && ./build/1_io
cloudflare: ../bash/publish-scene.sh --target 1_io
view: https://petrasvestartas.github.io/session/
*/
