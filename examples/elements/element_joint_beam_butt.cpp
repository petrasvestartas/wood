#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A butt of two beams whose axes meet at a node.
int main() {

    WoodSession scene("element_joint_beam_butt");

    // two 150 x 150 beams on axes meeting at a node: top_side, the post standing on the middle of the sill, and top_side at an end, the post on the sill's end
    for (const double x : {0.0, 1600.0}) {
        const std::shared_ptr<TreeNode> group =
            scene.add_group(x == 0.0 ? "top_side" : "top_side_end");
        const double node = x == 0.0 ? 0.0 : 600.0;
        const std::shared_ptr<Beam> sill = std::make_shared<Beam>(
            Polyline({{x - 600.0, 0.0, 0.0}, {x + 600.0, 0.0, 0.0}}), 75.0, "sill"
        );
        const std::shared_ptr<Beam> post = std::make_shared<Beam>(
            Polyline({{x + node, 0.0, 0.0}, {x + node, 0.0, 800.0}}), 75.0, "post"
        );
        scene.add(sill, group);
        scene.add(post, group);

        for (const std::shared_ptr<Beam>& beam : {sill, post}) {
            std::shared_ptr<Polyline> axis = std::make_shared<Polyline>(beam->axis);
            axis->linecolor = Color(0.85f, 0.1f, 0.1f);
            axis->width = 4.0;
            scene.add_polyline(axis, group);
        }

        const std::shared_ptr<JointBeam> butt = JointBeam::butt(*sill, *post);
        scene.add(butt, group);
        scene.add_interaction(butt, sill, butt->interaction(0));
        scene.add_interaction(butt, post, butt->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A butt of two 150 x 150 beams whose axes meet at a node: in top_side the post is cut on the sill's top face and stands flush on it; at the end the post is cut the same way and the sill runs on to the post's far face, so the corner closes flush.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_butt --parallel 6 && ./build/element_joint_beam_butt && ../bash/publish-scene.sh --target element_joint_beam_butt

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
