#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A support: a steel column base on a plane.
int main() {

    WoodSession scene("element_support");

    scene.add(std::make_shared<Support>(Plane::xy_plane(), "support"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A support on the xy plane with the manufacturer's dimensions: base plate with anchors, tube, head plate and column screws.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_support --parallel 6 && ./build/element_support && ../bash/publish-scene.sh --target element_support

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
