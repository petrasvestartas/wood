#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A butterfly key inlaid in the top of two beams end to end.
int main() {

    WoodSession scene("element_joint_beam_butterfly_key");

    const std::array<std::shared_ptr<BeamVariable>, 2> beams = BeamVariable::pair_end_to_end();
    const std::shared_ptr<BeamVariable> first = beams[0];
    const std::shared_ptr<BeamVariable> second = beams[1];
    scene.add(first);
    scene.add(second);

    const std::shared_ptr<InteractionContactFace> contact =
        scene.compute_face_contact(first, second);
    const std::shared_ptr<JointBeam> key = JointBeam::butterfly_key(
        *first, *second, *contact, 0.0, 600.0, 150.0, 100.0, 50.0, 60.0, 60.0, 60.0
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
