#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A session of several variable edges: a joist on a beam.
int main() {

    WoodSession scene("element_edge_variable_session");

    for (const std::shared_ptr<EdgeVariableElement>& edge : EdgeVariableElement::pair_tee())
        scene.add(edge);

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A session of several variable edges: the joist and the beam of a tee, each added to the scene.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_variable_session --parallel 6 && ./build/element_edge_variable_session && ../bash/publish-scene.sh --target element_edge_variable_session

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
