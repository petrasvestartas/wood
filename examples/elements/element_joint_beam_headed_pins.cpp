#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Headed pins through a beam into the end of another, in each PinLayout.
int main() {

    WoodSession scene("element_joint_beam_headed_pins");

    const std::array<PinLayout, 3> layouts = {
        PinLayout::corners, PinLayout::vertical, PinLayout::horizontal
    };

    for (size_t i = 0; i < layouts.size(); i++) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("layout_{}", i));

        const std::array<std::shared_ptr<BeamVariable>, 2> beams = BeamVariable::pair_tee();
        const std::shared_ptr<BeamVariable> beam = beams[0];
        const std::shared_ptr<BeamVariable> joist = beams[1];
        beam->place(Xform::translation(800.0 * i, 0.0, 0.0));
        joist->place(Xform::translation(800.0 * i, 0.0, 0.0));
        scene.add(beam, group);
        scene.add(joist, group);

        const std::shared_ptr<InteractionContactFace> contact =
            scene.compute_face_contact(beam, joist);
        const std::shared_ptr<JointBeam> pins =
            JointBeam::headed_pins(*beam, *joist, *contact, layouts[i], 3, 30.0, 0.0, 5.0);
        pins->name = fmt::format("pins_{}", i);
        scene.add(pins, group);
        scene.add_interaction(pins, beam, pins->interaction(0));
        scene.add_interaction(pins, joist, pins->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Headed pins through a beam into the end of another, in each PinLayout.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_headed_pins --parallel 6 && ./build/element_joint_beam_headed_pins && ../bash/publish-scene.sh --target element_joint_beam_headed_pins

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
