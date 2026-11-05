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

/// A session of several curved faces.
int main() {

    WoodSession scene("element_face_curved_session");

    scene.add(strip(0.0, 600.0, "strip_0"));
    scene.add(strip(600.0, 1200.0, "strip_1"));
    scene.add(strip(1200.0, 1800.0, "strip_2"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A session of several curved faces: three strips of a barrel vault side by side.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_face_curved_session --parallel 6 && ./build/element_face_curved_session && ../bash/publish-scene.sh --target element_face_curved_session

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
