#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A butterfly key inlaid in the top of two beams end to end.
int main() {

    WoodSession scene("element_joint_beam_butterfly_key");

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
    const std::shared_ptr<JointBeam> key = JointBeam::butterfly_key(
        *first,
        *second,
        *contact,
        0.0,   // top: flush with the beams' top
        600.0, // length
        150.0, // head_length
        100.0, // head_width
        50.0,  // neck_width
        60.0,  // depth
        60.0,  // end_depth
        60.0   // pocket_depth
    );
    key->name = "butterfly_key";
    scene.add(key);
    scene.add_interaction(key, first, key->interaction(0));
    scene.add_interaction(key, second, key->interaction(1));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A butterfly key inlaid in the top of two beams end to end.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_butterfly_key --parallel 6 && ./build/element_joint_beam_butterfly_key && ../bash/publish-scene.sh --target element_joint_beam_butterfly_key

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
