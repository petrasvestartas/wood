#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A wedge connector between two beams side by side: a prism let into both, pinned across, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_wedge");

    const std::array<std::shared_ptr<EdgeVariableElement>, 2> beams = EdgeVariableElement::pair_side_by_side();
    const std::shared_ptr<EdgeVariableElement> left = beams[0];
    const std::shared_ptr<EdgeVariableElement> right = beams[1];
    scene.add(left);
    scene.add(right);

    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointBeamElement> wedge =
        JointBeamElement::wedge(*left, *right, *contact, 300.0, 133.0);
    wedge->name = "wedge";
    scene.add(wedge);
    scene.add_interaction(wedge, left, wedge->interaction(0));
    scene.add_interaction(wedge, right, wedge->interaction(1));

    // drawn apart: the right beam moved off, its pocket and the wedge in the left one in view
    right->place(Xform::translation(0.0, 300.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A wedge connector between two beams side by side: a prism let into both, pinned across, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_wedge --parallel 6 && ./build/element_joint_beam_wedge && ../bash/publish-scene.sh --target element_joint_beam_wedge

|||||||| TIME ||||||||
1.48 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
