#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Pins centred across the contact of two stacked blocks, bored into both.
int main() {

    WoodSession scene("element_joint_beam_centred_pins");

    const std::array<std::shared_ptr<NodeBlockElement>, 2> blocks = NodeBlockElement::pair_stacked();
    const std::shared_ptr<NodeBlockElement> lower = blocks[0];
    const std::shared_ptr<NodeBlockElement> upper = blocks[1];
    scene.add(lower);
    scene.add(upper);

    const std::shared_ptr<InteractionContactFace> contact =
        scene.compute_face_contact(lower, upper);
    const std::shared_ptr<JointBeamElement> pins =
        JointBeamElement::centred_pins(*lower, *upper, *contact, 8.0, 160.0, 50.0);
    pins->name = "pins";
    scene.add(pins);
    scene.add_interaction(pins, lower, pins->interaction(0));
    scene.add_interaction(pins, upper, pins->interaction(1));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Pins centred across the contact of two stacked blocks, bored into both.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_centred_pins --parallel 6 && ./build/element_joint_beam_centred_pins && ../bash/publish-scene.sh --target element_joint_beam_centred_pins

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
