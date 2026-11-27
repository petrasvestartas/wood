#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Two scanned tree forks carrying a swept raw spruce girder.
int main() {

    WoodSession scene("element_edge_raw_session");

    const std::shared_ptr<TreeNode> forks = scene.group_named("forks");
    const std::shared_ptr<TreeNode> logs = scene.group_named("logs");

    const std::shared_ptr<EdgeRawElement> left = EdgeRawElement::from_stock(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 1100.0}), 65.0, "raw_fork_0", -90.0, false, "fork_left"
    );
    left->cuts = {
        Plane::from_point_normal({0.0, 0.0, 40.0}, {0.0, 0.0, 1.0}),
        Plane::from_point_normal({0.0, 0.0, 960.0}, {0.0, 0.0, -1.0}),
    };
    scene.add(left, forks);

    const std::shared_ptr<EdgeRawElement> right = EdgeRawElement::from_stock(
        Line::from_points({1450.0, 0.0, 0.0}, {1450.0, 0.0, 1200.0}), 65.0, "raw_fork_2", 90.0, false, "fork_right"
    );
    right->cuts = {
        Plane::from_point_normal({1450.0, 0.0, 40.0}, {0.0, 0.0, 1.0}),
        Plane::from_point_normal({1450.0, 0.0, 960.0}, {0.0, 0.0, -1.0}),
    };
    scene.add(right, forks);

    const std::shared_ptr<EdgeRawElement> girder = EdgeRawElement::from_stock(
        Line::from_points({-200.0, 0.0, 1015.0}, {1650.0, 0.0, 1015.0}), 55.0, "raw_log_1", 90.0, true, "girder"
    );
    scene.add(girder, logs);

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A portal of Rossiniere raw timber: two bifurcated spruce forks turned inward and trimmed flat at base and crown, carrying a naturally swept log girder.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_session --parallel 6 && ./build/element_edge_raw_session && ../bash/publish-scene.sh --target element_edge_raw_session

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
