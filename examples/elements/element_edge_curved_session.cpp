#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A planar glulam arch from x0 to x1, height rise at its middle, at y.
static std::shared_ptr<EdgeCurvedElement>
arch(double x0, double x1, double rise, double y, const std::string& name) {

    std::vector<Point> points;
    for (int i = 0; i <= 8; i++) {
        const double u = static_cast<double>(i) / 8.0;
        points.emplace_back(x0 + (x1 - x0) * u, y, rise * 4.0 * u * (1.0 - u));
    }

    return EdgeCurvedElement::single_curved(
        NurbsCurve::create_interpolated(points), 140.0, 240.0, 6, 32, name
    );
}

/// A session of several curved edges.
int main() {

    WoodSession scene("element_edge_curved_session");

    scene.add(arch(0.0, 2400.0, 700.0, 0.0, "arch_0"));
    scene.add(arch(0.0, 2400.0, 700.0, 900.0, "arch_1"));
    scene.add(arch(0.0, 2400.0, 700.0, 1800.0, "arch_2"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A session of several curved edges: three glulam arches side by side.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_curved_session --parallel 6 && ./build/element_edge_curved_session && ../bash/publish-scene.sh --target element_edge_curved_session

|||||||| TIME ||||||||
0.78 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
