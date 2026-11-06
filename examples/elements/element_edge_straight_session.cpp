#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A session of several edges: two columns carrying a beam.
int main() {

    WoodSession scene("element_edge_straight_session");

    const std::shared_ptr<TreeNode> columns = scene.group_named("columns");
    const std::shared_ptr<TreeNode> beams = scene.group_named("beams");

    for (size_t i = 0; i < 2; i++) {
        const double x = 3000.0 * i;
        scene.add(
            EdgeStraightElement::corner_square(
                Line::from_points({x, 0.0, 0.0}, {x, 0.0, 3000.0}),
                Plane({x - 110.0, -110.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}),
                220.0,
                fmt::format("column_{}", i)
            ),
            columns
        );
    }

    scene.add(
        EdgeStraightElement::square(
            Polyline({Point(-300.0, 0.0, 3110.0), Point(3300.0, 0.0, 3110.0)}), 110.0, "beam"
        ),
        beams
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A session of several edges: two columns and the beam on them, each added to its group, read back by name.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_straight_session --parallel 6 && ./build/element_edge_straight_session && ../bash/publish-scene.sh --target element_edge_straight_session

|||||||| TIME ||||||||
0.52 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
