#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A wedge connector between two beams side by side: a prism let into both, pinned across, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_wedge");

    const std::shared_ptr<BeamVariable> left = BeamVariable::between(
        Polyline::rectangle({0.0, -200.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        Polyline::rectangle({1600.0, -200.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        "left"
    );
    const std::shared_ptr<BeamVariable> right = BeamVariable::between(
        Polyline::rectangle({0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        Polyline::rectangle({1600.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        "right"
    );
    scene.add(left);
    scene.add(right);

    // the wedge from their contact, added, and passed to each beam in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointBeam> wedge = JointBeam::wedge(
        *left,
        *right,
        *contact,
        300.0,
        133.0
    );
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
Two 1600 long beams 200 wide and 300 deep side by side, touching on their long faces; JointBeam::wedge on their face contact stops 300 short of each end of the contact's top edge, its triangular WEDGE_PROFILE hangs below that edge, a pocket 133 deep is cut under each slanted face into each beam, and pins run across through the wedge every 320; the wedge is added and passed to each beam with add_interaction, its part and pins nested under it; the right beam is moved 300 off afterwards so the pockets read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_wedge --parallel 6 && ./build/element_joint_beam_wedge && ../bash/publish-scene.sh --target element_joint_beam_wedge

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
