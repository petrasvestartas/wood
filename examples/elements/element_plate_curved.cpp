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

/// A curved plate: a strip of a barrel vault, 120 thick between the outer and the inner arc, 600 wide between its two meridians.
int main() {

    WoodSession scene("element_plate_curved");

    const std::shared_ptr<PlateCurved> plate = std::make_shared<PlateCurved>(
        NurbsCurve::create_interpolated(arc(2000.0, 0.0)),
        NurbsCurve::create_interpolated(arc(2000.0, 600.0)),
        NurbsCurve::create_interpolated(arc(1880.0, 0.0)),
        NurbsCurve::create_interpolated(arc(1880.0, 600.0))
    );
    scene.add(plate);

    std::cout << scene << std::endl;
    std::cout << fmt::format(
        "{} faces, solid {}, volume {:.0f} mm3\n",
        plate->element_geometry_brep().face_count(),
        plate->element_geometry_brep().is_solid(),
        plate->element_geometry_brep().volume()
    );
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A curved plate, a strip of a barrel vault: its outer face the ruled surface between two quarter arcs of radius 2000 interpolated through seven points, 600 apart, its inner face between the arcs of radius 1880, 120 in; the two side faces along the arcs and the two caps at the springing and at the crown close it into one solid of six exact NURBS patches, every edge shared by two faces. The volume printed is the tessellated solid's, within a per cent of the quarter annulus times the width.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_plate_curved --parallel 6 && ./build/element_plate_curved && ../bash/publish-scene.sh --target element_plate_curved

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
