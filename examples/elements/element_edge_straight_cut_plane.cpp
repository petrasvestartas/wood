#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A beam cut by a plane.
int main() {

    WoodSession scene("element_edge_straight_cut_plane");

    const std::shared_ptr<EdgeStraightElement> beam = EdgeStraightElement::square(
        Polyline({Point(0.0, 0.0, 0.0), Point(1600.0, 0.0, 0.0)}), 100.0, "beam"
    );
    scene.add(beam);

    const std::shared_ptr<JointCutPlaneElement> cut = std::make_shared<JointCutPlaneElement>(
        Plane::from_point_normal({1300.0, 0.0, 0.0}, {-1.0, 0.0, 0.5}), 500.0, "cut"
    );
    scene.add(cut);
    scene.add_interaction(cut, beam, cut->interaction(0));

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        beam->name,
        beam->model_geometry_brep().face_count(),
        beam->model_geometry_brep().is_solid(),
        beam->model_geometry_brep().volume_exact(),
        beam->element_geometry_brep().volume_exact()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A beam cut by a plane: a JointCutPlaneElement is a joint, and the beam keeps the side the plane's normal points to.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_straight_cut_plane --parallel 6 && ./build/element_edge_straight_cut_plane && ../bash/publish-scene.sh --target element_edge_straight_cut_plane

|||||||| TIME ||||||||
2.00 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
