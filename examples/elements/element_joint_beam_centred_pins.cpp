#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Pins centred across the contact of two stacked blocks, bored into both.
int main() {

    WoodSession scene("element_joint_beam_centred_pins");

    const Polyline lower_loop = Polyline::rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 400.0);
    const Polyline upper_loop = Polyline::rectangle({50.0, 50.0, 200.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 300.0);
    const std::shared_ptr<Block> lower = std::make_shared<Block>(std::vector<Polyline>{lower_loop, lower_loop.translated({0.0, 0.0, 200.0})}, "lower");
    const std::shared_ptr<Block> upper = std::make_shared<Block>(std::vector<Polyline>{upper_loop, upper_loop.translated({0.0, 0.0, 200.0})}, "upper");
    scene.add(lower);
    scene.add(upper);

    // the pins from their contact, added, and passed to each block in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
    const std::shared_ptr<JointBeam> pins = JointBeam::centred_pins(
        *lower,
        *upper,
        *contact,
        8.0,
        160.0,
        50.0
    );
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
A 300 square block 200 high on a 400 square block 200 high; JointBeam::centred_pins on their face contact stands a pin 8 in radius and 160 long at each corner of the contact inset by 50, half in each block, and bores both; the pins are added and passed to each block with add_interaction.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_centred_pins --parallel 6 && ./build/element_joint_beam_centred_pins && ../bash/publish-scene.sh --target element_joint_beam_centred_pins

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
