#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A small timber house from plates drawn on their mid-planes: a floor, four walls on the footprint and a roof slab, every plate butted flat on another.
int main() {

    WoodSession scene("butt_house");

    // the 6000 x 4000 footprint, the floor and the roof 200 thick on z = 0 and z = 2800, the walls 120 thick standing on the footprint's sides
    const std::array<Point, 4> footprint = {Point(0.0, 0.0, 0.0), Point(6000.0, 0.0, 0.0), Point(6000.0, 4000.0, 0.0), Point(0.0, 4000.0, 0.0)};
    const std::shared_ptr<Plate> floor = Plate::from_rectangle({0.0, 0.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "floor");
    const std::shared_ptr<Plate> roof = Plate::from_rectangle({0.0, 0.0, 2700.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "roof");
    std::array<std::shared_ptr<Plate>, 4> walls;
    for (size_t i = 0; i < 4; i++)
        walls[i] = Plate::from_line(Line::from_points(footprint[i], footprint[(i + 1) % 4]), {0.0, 0.0, 1.0}, 2800.0, 120.0, fmt::format("wall_{}", i));
    scene.add(floor);
    scene.add(roof);
    for (const std::shared_ptr<Plate>& wall : walls)
        scene.add(wall);

    // the butts, running plate first: the long walls run on past the short ones at the corners, the floor and the roof run on under and over every wall
    const std::vector<std::array<std::shared_ptr<Plate>, 2>> pairs = {
        {walls[0], walls[1]}, {walls[0], walls[3]}, {walls[2], walls[1]}, {walls[2], walls[3]},
        {floor, walls[0]}, {floor, walls[1]}, {floor, walls[2]}, {floor, walls[3]},
        {roof, walls[0]}, {roof, walls[1]}, {roof, walls[2]}, {roof, walls[3]},
    };
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
A small timber house drawn on mid-planes: a floor and a roof slab 200 thick and four 120 thick walls standing on the 6000 x 4000 footprint. The short walls are cut flat on the long walls' inner faces and the long walls run on to their outer faces; the walls are cut flat on the floor's top and the roof's bottom, and the floor and the roof run on to the walls' outer faces, so the outside closes flush.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 21_butt_house --parallel 6 && ./build/21_butt_house && ../bash/publish-scene.sh --target 21_butt_house

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
