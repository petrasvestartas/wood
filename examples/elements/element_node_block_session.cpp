#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A session of several blocks: two stacked.
int main() {

    WoodSession scene("element_node_block_session");

    for (const std::shared_ptr<NodeBlockElement>& block : NodeBlockElement::pair_stacked())
        scene.add(block);

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A session of several blocks: a block on a block, each added to the scene.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_node_block_session --parallel 6 && ./build/element_node_block_session && ../bash/publish-scene.sh --target element_node_block_session

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
