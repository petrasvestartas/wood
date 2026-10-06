#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The block element: lofted between loops, and from a closed mesh.
int main() {

    WoodSession scene("element_block");

    const Polyline bottom = Polyline::rectangle(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 300.0, 300.0);
    const Polyline top = Polyline::rectangle(Point(50.0, 50.0, 250.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 200.0, 200.0);
    scene.add(std::make_shared<Block>(std::vector<Polyline>{bottom, top}, "loops"));

    const Polyline base = bottom.translated(Vector(500.0, 0.0, 0.0));
    scene.add(std::make_shared<Block>(Mesh::loft({base}, {base.translated(Vector(0.0, 0.0, 250.0))}, true), "mesh"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The block element, two ways: lofted between a 300 square and a 200 square 250 above it, and from a closed mesh, a 300 cube-like box.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_block --parallel 6 && ./build/element_block && ../bash/publish-scene.sh --target element_block

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
