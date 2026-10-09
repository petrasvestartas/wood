#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A joint that drills: a hole along an axis through a block.
int main() {

    WoodSession scene("element_joint_drill");

    const Polyline loop = Polyline::rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 300.0);
    const std::shared_ptr<Block> block = std::make_shared<Block>(std::vector<Polyline>{loop, loop.translated({0.0, 0.0, 200.0})}, "block");
    scene.add(block);

    const std::shared_ptr<Joint> drill = Joint::drill(Line::from_points({150.0, 150.0, 250.0}, {150.0, 150.0, -50.0}), 25.0);
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
A 300 square block 200 high and Joint::drill along a vertical axis through its middle, 25 in radius; the drill is added hidden and passed to the block with add_interaction, which takes the hole away.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_drill --parallel 6 && ./build/element_joint_drill && ../bash/publish-scene.sh --target element_joint_drill

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
