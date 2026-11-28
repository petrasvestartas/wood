#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// cross_slot on beams: the plain slot, on a pair crossing at 60 degrees.
int main() {

    WoodSession scene("element_joint_beam_cross_slot");
    scene.settings.joint_parameters[11] = JointPlateElement::library_id("cross_slot");

    // two 150 x 150 beams 1200 long, crossing at 60 degrees
    const std::array<std::shared_ptr<EdgeStraightElement>, 2> pair = EdgeStraightElement::pair_crossing(60.0);
    const std::shared_ptr<EdgeStraightElement> first = pair[0];
    const std::shared_ptr<EdgeStraightElement> second = pair[1];
    for (const std::shared_ptr<EdgeStraightElement>& beam : pair) {
        scene.add(beam);
    }

    const InteractionContactAxis contact(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}), 0.5, 0.5, 0, 0, 0, 0
    );
    const std::shared_ptr<JointBeamElement> joint =
        JointBeamElement::from_contact(*first, *second, contact, 500.0, 0.91, 1, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
cross_slot on beams: the plain slot, on a pair crossing at 60 degrees.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_cross_slot --parallel 6 && ./build/element_joint_beam_cross_slot && ../bash/publish-scene.sh --target element_joint_beam_cross_slot

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
