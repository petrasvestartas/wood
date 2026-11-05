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

/// A curved face cut by a plane.
int main() {

    WoodSession scene("element_face_curved_cut_plane");

    const std::shared_ptr<FaceCurvedElement> face = strip(0.0, 600.0, "strip");
    scene.add(face);

    const std::shared_ptr<JointCutPlaneElement> cut = std::make_shared<JointCutPlaneElement>(
        Plane::from_point_normal({1400.0, 0.0, 1400.0}, {-1.0, 0.0, 0.0}), 1200.0, "cut"
    );
    scene.add(cut);
    scene.add_interaction(cut, face, cut->interaction(0));

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
A curved face cut by a plane: a JointCutPlaneElement on the strip, which keeps the side the normal points to.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_face_curved_cut_plane --parallel 6 && ./build/element_face_curved_cut_plane && ../bash/publish-scene.sh --target element_face_curved_cut_plane

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
