#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A cutting plane.
int main() {

    WoodSession scene("element_joint_cut_plane");

    scene.add(std::make_shared<JointCutPlaneElement>(
        Plane::from_point_normal({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}), 1000.0, "cut"
    ));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A cutting plane, drawn as a square of its size. What it cuts keeps the side its normal points to.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_cut_plane --parallel 6 && ./build/element_joint_cut_plane && ../bash/publish-scene.sh --target element_joint_cut_plane

|||||||| TIME ||||||||
3.50 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
