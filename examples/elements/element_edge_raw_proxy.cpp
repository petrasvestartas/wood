#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Round proxy, registered laser-scanned point cloud, reconstructed log mesh and bifurcated tree fork.
int main() {

    WoodSession scene("element_edge_raw_proxy");

    // 1. Low-poly round proxy beam before stock attachment
    const std::shared_ptr<EdgeRawElement> proxy = EdgeRawElement::round(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 1000.0}), 62.0, 0.2, "proxy"
    );
    scene.add(proxy);

    // 2. Same round proxy with its scanned RGB point cloud attached by raw_xform
    const std::shared_ptr<EdgeRawElement> scanned = EdgeRawElement::round(
        Line::from_points({420.0, 0.0, 0.0}, {420.0, 0.0, 1000.0}), 62.0, 0.2, "scanned"
    );
    scanned->attach_stock("raw_log_3");
    scanned->use_raw_mesh = false;
    scanned->show_pointcloud = true;
    scene.add(scanned);

    // 3. Reconstructed raw trunk mesh active on the same axis
    const std::shared_ptr<EdgeRawElement> log = EdgeRawElement::from_stock(
        Line::from_points({840.0, 0.0, 0.0}, {840.0, 0.0, 1000.0}), 62.0, "raw_log_3", 0.0, true, "log"
    );
    scene.add(log);

    // 4. Bifurcated Rossiniere spruce tree fork with stem and secondary branch axes
    const std::shared_ptr<EdgeRawElement> fork = EdgeRawElement::from_stock(
        Line::from_points({1260.0, 0.0, 0.0}, {1260.0, 0.0, 1100.0}), 65.0, "raw_fork_0", 0.0, false, "fork"
    );
    scene.add(fork);

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four stages of a raw timber member side by side: a round proxy beam, the proxy with its laser-scanned RGB point cloud registered by raw_xform, the closed trunk mesh, and a bifurcated tree fork carrying its secondary branch axis.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_proxy --parallel 6 && ./build/element_edge_raw_proxy && ../bash/publish-scene.sh --target element_edge_raw_proxy

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
