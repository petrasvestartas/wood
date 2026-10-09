#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A bow-tie key across the end-to-end contact of two beams, let into both, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_tie");

    const std::shared_ptr<BeamVariable> first = BeamVariable::between(
        Polyline::rectangle({0.0, -100.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        Polyline::rectangle({1000.0, -100.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        "first"
    );
    const std::shared_ptr<BeamVariable> second = BeamVariable::between(
        Polyline::rectangle({1000.0, -100.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        Polyline::rectangle({2000.0, -100.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        "second"
    );
    scene.add(first);
    scene.add(second);

    // the key from their contact, added, and passed to each beam in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(first, second);
    const std::shared_ptr<JointBeam> tie = JointBeam::tie(*first, *second, *contact);
    tie->name = "tie";
    scene.add(tie);
    scene.add_interaction(tie, first, tie->interaction(0));
    scene.add_interaction(tie, second, tie->interaction(1));

    // drawn apart: the second beam moved off, its pocket and the key in the first one in view
    second->place(Xform::translation(400.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two 1000 long beams 200 wide and 300 deep end to end; JointBeam::tie on their face contact makes an 800 long bow-tie key in four pieces, 200 long, 40 wide heads and a 20 wide neck, 58.5 deep and 138.5 below the contact's top edge, with a pocket 80 deep in each beam; the key is added and passed to each beam with add_interaction; the second beam is moved 400 off afterwards so the pockets read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_tie --parallel 6 && ./build/element_joint_beam_tie && ../bash/publish-scene.sh --target element_joint_beam_tie

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
