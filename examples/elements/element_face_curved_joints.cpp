#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The points of a quarter arc in the xz plane, radius r about the origin, at y.
static std::vector<Point> arc(double r, double y) {

    std::vector<Point> points;
    for (int k = 0; k <= 6; k++) {
        const double angle = 0.5 * Tolerance::PI * static_cast<double>(k) / 6.0;
        points.push_back(Point(r * std::cos(angle), y, r * std::sin(angle)));
    }

    return points;
}

/// A strip of a barrel vault from y0 to y1, 120 thick.
static std::shared_ptr<FaceCurvedElement> strip(double y0, double y1, const std::string& name) {
    return std::make_shared<FaceCurvedElement>(
        NurbsCurve::create_interpolated(arc(2000.0, y0)),
        NurbsCurve::create_interpolated(arc(2000.0, y1)),
        NurbsCurve::create_interpolated(arc(1880.0, y0)),
        NurbsCurve::create_interpolated(arc(1880.0, y1)),
        name
    );
}

/// Two curved faces joined along their long edge by pins.
int main() {

    WoodSession scene("element_face_curved_joints");

    const std::shared_ptr<FaceCurvedElement> a = strip(0.0, 600.0, "a");
    const std::shared_ptr<FaceCurvedElement> b = strip(600.0, 1200.0, "b");
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
Two curved faces joined: pins centred across the contact of their long edges, bored into both.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_face_curved_joints --parallel 6 && ./build/element_face_curved_joints && ../bash/publish-scene.sh --target element_face_curved_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
