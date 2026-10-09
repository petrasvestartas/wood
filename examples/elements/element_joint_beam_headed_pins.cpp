#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Headed pins through a beam into the end of another, in each PinLayout.
int main() {

    WoodSession scene("element_joint_beam_headed_pins");

    const std::array<PinLayout, 3> layouts = {PinLayout::corners, PinLayout::vertical, PinLayout::horizontal};

    for (size_t i = 0; i < layouts.size(); i++) {
        const double x = 800.0 * i;
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("layout_{}", i));

        const std::shared_ptr<BeamVariable> beam = BeamVariable::between(
            Polyline::rectangle({x, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 120.0, 300.0),
            Polyline::rectangle({x + 600.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 120.0, 300.0),
            fmt::format("beam_{}", i)
        );
        const std::shared_ptr<BeamVariable> joist = BeamVariable::between(
            Polyline::rectangle({x + 250.0, 120.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 100.0, 300.0),
            Polyline::rectangle({x + 250.0, 720.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 100.0, 300.0),
            fmt::format("joist_{}", i)
        );
        scene.add(beam, group);
        scene.add(joist, group);

        // the pins from their contact, added, and passed to each member in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(beam, joist);
        const std::shared_ptr<JointBeam> pins = JointBeam::headed_pins(
            *beam,
            *joist,
            *contact,
            layouts[i],
            3,
            20.0,
            0.0,
            5.0
        );
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
Three T joints, each a 120 wide, 300 deep beam with a 100 wide joist ending on its side; JointBeam::headed_pins on their face contact drives pins 5 in radius and 200 long from the beam's far face through it into the joist end, pre-drilled, laid out by PinLayout: corners, a vertical column of 3 and a horizontal row of 3, 20 in from the contact's edges; the pins are added and passed to each member with add_interaction.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_headed_pins --parallel 6 && ./build/element_joint_beam_headed_pins && ../bash/publish-scene.sh --target element_joint_beam_headed_pins

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
