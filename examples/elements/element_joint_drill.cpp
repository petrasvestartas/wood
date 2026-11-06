#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A joint that drills: a hole along an axis through a block.
int main() {

    WoodSession scene("element_joint_drill");

    const std::shared_ptr<NodeBlockElement> block = NodeBlockElement::box({0.0, 0.0, 0.0}, 300.0, 300.0, 200.0);
    scene.add(block);

    const std::shared_ptr<JointElement> drill =
        JointElement::drill(Line::from_points({150.0, 150.0, 250.0}, {150.0, 150.0, -50.0}), 25.0);
    drill->name = "drill";
    drill->is_visible = false;
    scene.add(drill);
    scene.add_interaction(drill, block, drill->interaction(0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A joint that drills: a hole along an axis through a block.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_drill --parallel 6 && ./build/element_joint_drill && ../bash/publish-scene.sh --target element_joint_drill

|||||||| TIME ||||||||
0.81 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
