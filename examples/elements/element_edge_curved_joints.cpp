#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Half an arch, side -1 or 1 of the crown at the origin's x, its end cut flat at the crown.
static std::shared_ptr<EdgeCurvedElement> half_arch(double side, const std::string& name) {

    std::vector<Point> points;
    for (int i = 0; i <= 8; i++) {
        const double u = static_cast<double>(i) / 8.0;
        points.emplace_back(side * 2400.0 * u, 0.0, 700.0 * (1.0 - u * u));
    }

    const std::shared_ptr<EdgeCurvedElement> half = EdgeCurvedElement::single_curved(
        NurbsCurve::create_interpolated(points), 140.0, 240.0, 6, 32, name
    );
    half->extension = 60.0;
    half->cuts = {Plane::from_point_normal({0.0, 0.0, 700.0}, {side, 0.0, 0.0})};

    return half;
}

/// Two curved edges joined at the crown by pins.
int main() {

    WoodSession scene("element_edge_curved_joints");

    const std::shared_ptr<EdgeCurvedElement> a = half_arch(-1.0, "a");
    const std::shared_ptr<EdgeCurvedElement> b = half_arch(1.0, "b");
    scene.add(a);
    scene.add(b);

    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(a, b);
    const std::shared_ptr<JointBeamElement> pins =
        JointBeamElement::centred_pins(*a, *b, *contact, 8.0, 160.0, 50.0);
    pins->name = "pins";
    scene.add(pins);
    scene.add_interaction(pins, a, pins->interaction(0));
    scene.add_interaction(pins, b, pins->interaction(1));

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        a->name,
        a->model_geometry_brep().face_count(),
        a->model_geometry_brep().is_solid(),
        a->model_geometry_brep().volume(),
        a->element_geometry_brep().volume()
    );
    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        b->name,
        b->model_geometry_brep().face_count(),
        b->model_geometry_brep().is_solid(),
        b->model_geometry_brep().volume(),
        b->element_geometry_brep().volume()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two curved edges joined: pins centred across the contact of their cut ends at the crown, bored into both.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_curved_joints --parallel 6 && ./build/element_edge_curved_joints && ../bash/publish-scene.sh --target element_edge_curved_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
