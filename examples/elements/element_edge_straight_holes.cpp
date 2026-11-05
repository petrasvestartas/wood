#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A beam with a hole through its width: a drill is a joint, what it does an interaction.
int main() {

    WoodSession scene("element_edge_straight_holes");

    const std::shared_ptr<EdgeStraightElement> beam = EdgeStraightElement::square(
        Polyline({Point(0.0, 0.0, 0.0), Point(1600.0, 0.0, 0.0)}), 100.0, "beam"
    );
    scene.add(beam);

    const std::shared_ptr<JointElement> hole =
        JointElement::drill(Line::from_points({800.0, 0.0, 150.0}, {800.0, 0.0, -150.0}), 30.0);
    hole->name = "hole";
    hole->is_visible = false;
    scene.add(hole);
    scene.add_interaction(hole, beam, hole->interaction(0));

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
A beam with a hole: a JointElement::drill added to the session, its interaction bores the beam. The hole is part of the beam, no element of its own.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_straight_holes --parallel 6 && ./build/element_edge_straight_holes && ../bash/publish-scene.sh --target element_edge_straight_holes

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
