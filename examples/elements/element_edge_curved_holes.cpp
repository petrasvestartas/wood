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

/// A curved edge with a hole through its width.
int main() {

    WoodSession scene("element_edge_curved_holes");

    const std::shared_ptr<EdgeCurvedElement> edge = arch(0.0, 2400.0, 700.0, 0.0, "arch");
    scene.add(edge);

    const std::shared_ptr<JointElement> hole =
        JointElement::drill(Line::from_points({1200.0, 120.0, 700.0}, {1200.0, -120.0, 700.0}), 30.0);
    hole->name = "hole";
    hole->is_visible = false;
    scene.add(hole);
    scene.add_interaction(hole, edge, hole->interaction(0));

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        edge->name,
        edge->model_geometry_brep().face_count(),
        edge->model_geometry_brep().is_solid(),
        edge->model_geometry_brep().volume(),
        edge->element_geometry_brep().volume()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A curved edge with a hole: a drill is a joint, its interaction bores the arch at its crown.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_curved_holes --parallel 6 && ./build/element_edge_curved_holes && ../bash/publish-scene.sh --target element_edge_curved_holes

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
