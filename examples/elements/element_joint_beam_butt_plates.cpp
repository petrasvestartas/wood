#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Butts of plates drawn on their mid-planes: two walls standing on a floor and meeting at a corner, every face flush.
int main() {

    WoodSession scene("element_joint_beam_butt_plates");

    // a 1000 x 800 floor and two 600 high walls, all 40 thick, each centred on its line: the floor on z = 0, the walls on y = 0 and x = 0
    const std::shared_ptr<Plate> floor = Plate::from_rectangle({0.0, 0.0, -20.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 1000.0, 800.0, 40.0, "floor");
    const std::shared_ptr<Plate> long_wall = Plate::from_rectangle({0.0, 20.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 1000.0, 600.0, 40.0, "long_wall");
    const std::shared_ptr<Plate> short_wall = Plate::from_rectangle({-20.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 800.0, 600.0, 40.0, "short_wall");
    scene.add(floor);
    scene.add(long_wall);
    scene.add(short_wall);

    // the mid-plane polygons as drawn, red over the solids
    for (const std::shared_ptr<Plate>& plate : {floor, long_wall, short_wall}) {
        std::vector<Point> points;
        for (size_t i = 0; i < plate->polylines[0].point_count(); i++)
            points.push_back(plate->polylines[0][i] + (plate->polylines[1][i] - plate->polylines[0][i]) * 0.5);
        std::shared_ptr<Polyline> outline = std::make_shared<Polyline>(points);
        outline->linecolor = Color(0.85f, 0.1f, 0.1f);
        outline->width = 4.0;
        scene.add_polyline(outline);
    }

    // the walls butt on the floor, which runs on under their outer faces; the short wall butts on the long one, which runs on to its outer face
    const std::array<std::array<std::shared_ptr<Plate>, 2>, 3> pairs = {{{floor, long_wall}, {floor, short_wall}, {long_wall, short_wall}}};
    for (const std::array<std::shared_ptr<Plate>, 2>& pair : pairs) {
        const std::shared_ptr<JointBeam> butt = JointBeam::butt(*pair[0], *pair[1]);
        butt->name = fmt::format("butt_{}_{}", pair[0]->name, pair[1]->name);
        scene.add(butt);
        scene.add_interaction(butt, pair[0], butt->interaction(0));
        scene.add_interaction(butt, pair[1], butt->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Butts of 40 thick plates drawn on their mid-planes, so they overlap where they meet: the walls are cut on the floor's top face and the floor runs on to their outer faces; the short wall is cut on the long wall's inner face and the long wall runs on to the short wall's outer face.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_butt_plates --parallel 6 && ./build/element_joint_beam_butt_plates && ../bash/publish-scene.sh --target element_joint_beam_butt_plates

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
