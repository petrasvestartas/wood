#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A timber-frame wall from lines meeting at nodes: sill, head, three posts and two braces, every member butted flat on another.
int main() {

    WoodSession scene("butt_frame");

    // 140 x 140 beams on axes drawn node to node: a 3000 wide, 2400 high wall
    const double radius = 70.0;
    const std::shared_ptr<Beam> sill =
        std::make_shared<Beam>(Polyline({{0.0, 0.0, 0.0}, {3000.0, 0.0, 0.0}}), radius, "sill");
    const std::shared_ptr<Beam> head = std::make_shared<Beam>(
        Polyline({{0.0, 0.0, 2400.0}, {3000.0, 0.0, 2400.0}}), radius, "head"
    );
    const std::shared_ptr<Beam> left_post = std::make_shared<Beam>(
        Polyline({{0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}}), radius, "left_post"
    );
    const std::shared_ptr<Beam> middle_post = std::make_shared<Beam>(
        Polyline({{1500.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}}), radius, "middle_post"
    );
    const std::shared_ptr<Beam> right_post = std::make_shared<Beam>(
        Polyline({{3000.0, 0.0, 0.0}, {3000.0, 0.0, 2400.0}}), radius, "right_post"
    );
    const std::shared_ptr<Beam> left_brace = std::make_shared<Beam>(
        Polyline({{0.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}}), radius, "left_brace"
    );
    const std::shared_ptr<Beam> right_brace = std::make_shared<Beam>(
        Polyline({{3000.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}}), radius, "right_brace"
    );
    for (const std::shared_ptr<Beam>& beam :
         {sill, head, left_post, middle_post, right_post, left_brace, right_brace}) {
        scene.add(beam);

        std::shared_ptr<Polyline> axis = std::make_shared<Polyline>(beam->axis);
        axis->linecolor = Color(0.85f, 0.1f, 0.1f);
        axis->width = 4.0;
        scene.add_polyline(axis, scene.group_named("input"));
    }

    // the posts on the sill and under the head, which run on over the corner posts: butts read from the drawn members
    const std::vector<std::array<std::shared_ptr<Beam>, 2>> posts = {
        {sill, left_post},
        {sill, middle_post},
        {sill, right_post},
        {head, left_post},
        {head, middle_post},
        {head, right_post},
    };
    for (const std::array<std::shared_ptr<Beam>, 2>& pair : posts) {
        const std::shared_ptr<JointBeam> butt = JointBeam::butt(*pair[0], *pair[1]);
        butt->name = fmt::format("{}_{}_{}", butt->name, pair[0]->name, pair[1]->name);
        scene.add(butt);
        scene.add_interaction(butt, pair[0], butt->interaction(0));
        scene.add_interaction(butt, pair[1], butt->interaction(1));
    }

    const std::vector<std::array<std::shared_ptr<Beam>, 2>> braces = {
        {sill, left_brace},
        {left_post, left_brace},
        {head, left_brace},
        {middle_post, left_brace},
        {sill, right_brace},
        {right_post, right_brace},
        {head, right_brace},
        {middle_post, right_brace},
    };
    for (const std::array<std::shared_ptr<Beam>, 2>& pair : braces) {
        const std::shared_ptr<JointBeam> top_side = JointBeam::butt_top_side(*pair[0], *pair[1]);
        top_side->name = fmt::format("{}_{}_{}", top_side->name, pair[0]->name, pair[1]->name);
        scene.add(top_side);
        scene.add_interaction(top_side, pair[0], top_side->interaction(0));
        scene.add_interaction(top_side, pair[1], top_side->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A timber-frame wall drawn as lines meeting at nodes, each line a 140 x 140 beam: the posts are cut flat on the sill's top face and the head's bottom face, the sill and the head run on to the corner posts' outer faces, and each brace is cut flat on the sill, the head and the posts it meets.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 20_butt_frame --parallel 6 && ./build/20_butt_frame && ../bash/publish-scene.sh --target 20_butt_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
