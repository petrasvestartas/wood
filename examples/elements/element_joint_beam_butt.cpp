#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A butt of two beams whose axes meet at a node: a post on a sill, and a corner where the sill runs on to the post's far face.
int main() {

    WoodSession scene("element_joint_beam_butt");

    // two 150 x 150 beams on axes meeting at a node: a tee, the post standing on the middle of the sill, and a corner, the post on its end
    for (const double x : {0.0, 1600.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(x == 0.0 ? "tee" : "corner");
        const double node = x == 0.0 ? 0.0 : 600.0;
        const std::shared_ptr<Beam> sill = std::make_shared<Beam>(Polyline({{x - 600.0, 0.0, 0.0}, {x + 600.0, 0.0, 0.0}}), 75.0, "sill");
        const std::shared_ptr<Beam> post = std::make_shared<Beam>(Polyline({{x + node, 0.0, 0.0}, {x + node, 0.0, 800.0}}), 75.0, "post");
        scene.add(sill, group);
        scene.add(post, group);

        // the butt, added, and passed to the running sill and the butting post
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
A butt of two 150 x 150 beams whose axes meet at a node: in the tee the post is cut on the sill's top face and stands flush on it; in the corner the post is cut the same way and the sill runs on to the post's far face, so the corner closes flush.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_butt --parallel 6 && ./build/element_joint_beam_butt && ../bash/publish-scene.sh --target element_joint_beam_butt

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
