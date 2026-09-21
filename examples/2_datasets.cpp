#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::hex_block_rossiniere};   // config::Dataset::<name> autocompletes

int main() {

    WoodSession from_yaml = WoodSession::yaml_load(DATASET);
    std::cout << from_yaml << " distance " << from_yaml.settings.distance << " search " << from_yaml.settings.search_type << "\n";

    WoodSession from_obj = WoodSession::obj_load(DATASET);
    std::cout << from_obj << " plates from the obj alone, default settings\n";

    WoodSession from_pb = WoodSession::pb_load(config::session_pb(0));
    std::cout << from_pb << " from a session file\n";

    from_yaml.add_to_tree(true, true, false, false);
    from_yaml.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the three ways a scene is loaded: a dataset yml (settings, obj and sidecars), an obj alone (plates, default settings), a session .pb (everything, including the interactions).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 2_datasets --parallel 4 && ./build/2_datasets && ../bash/publish-scene.sh --target 2_datasets

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
