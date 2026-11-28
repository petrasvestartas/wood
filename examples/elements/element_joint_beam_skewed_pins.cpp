#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// Two pins leaning 45 degrees, driven through a joist's top into the beam its end stands on.
int main() {

    WoodSession scene("element_joint_beam_skewed_pins");

    const std::array<std::shared_ptr<EdgeVariableElement>, 2> beams = EdgeVariableElement::pair_tee();
    const std::shared_ptr<EdgeVariableElement> beam = beams[0];
    const std::shared_ptr<EdgeVariableElement> joist = beams[1];
    scene.add(beam);
    scene.add(joist);

    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(joist, beam);
    const std::shared_ptr<JointBeamElement> pins = JointBeamElement::skewed_pins(*joist, *beam, *contact, 5.0, 80.0);
    pins->name = "pins";
    scene.add(pins);
    scene.add_interaction(pins, joist, pins->interaction(0));
    scene.add_interaction(pins, beam, pins->interaction(1));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two pins leaning 45 degrees, driven through a joist's top down across the contact into the beam its end stands on, bored into both.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_skewed_pins --parallel 6 && ./build/element_joint_beam_skewed_pins && ../bash/publish-scene.sh --target element_joint_beam_skewed_pins

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
