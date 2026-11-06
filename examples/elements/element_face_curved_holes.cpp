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

/// A curved face with a hole through its thickness.
int main() {

    WoodSession scene("element_face_curved_holes");

    const std::shared_ptr<FaceCurvedElement> face = strip(0.0, 600.0, "strip");
    scene.add(face);

    const double turn = 0.4 * Tolerance::PI;
    const Vector out(std::cos(turn), 0.0, std::sin(turn));
    const Point foot(0.0, 300.0, 0.0);
    const std::shared_ptr<JointElement> hole =
        JointElement::drill(Line::from_points(foot + out * 2060.0, foot + out * 1820.0), 30.0);
    hole->name = "hole";
    hole->is_visible = false;
    scene.add(hole);
    scene.add_interaction(hole, face, hole->interaction(0));

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        face->name,
        face->model_geometry_brep().face_count(),
        face->model_geometry_brep().is_solid(),
        face->model_geometry_brep().volume_exact(),
        face->element_geometry_brep().volume_exact()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A curved face with a hole: a drill is a joint, its interaction bores the strip through its thickness.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_face_curved_holes --parallel 6 && ./build/element_face_curved_holes && ../bash/publish-scene.sh --target element_face_curved_holes

|||||||| TIME ||||||||
4.30 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
