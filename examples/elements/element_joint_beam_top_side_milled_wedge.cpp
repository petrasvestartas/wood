#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// top_side_milled_wedge on beams: the milled wedge, on a pair at 60 degrees, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_top_side_milled_wedge");
    scene.settings.joint_parameters[8] = JointPlateElement::library_id("top_side_milled_wedge");

    // two 150 x 150 beams 1200 long, meeting at 60 degrees
    const std::array<std::shared_ptr<EdgeStraightElement>, 2> pair = EdgeStraightElement::pair_tee(60.0);
    const std::shared_ptr<EdgeStraightElement> first = pair[0];
    const std::shared_ptr<EdgeStraightElement> second = pair[1];
    for (const std::shared_ptr<EdgeStraightElement>& beam : pair) {
        scene.add(beam);
    }

    const InteractionContactAxis contact(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}), 0.5, 0.0, 0, 0, 0, 0
    );
    const std::shared_ptr<JointBeamElement> joint =
        JointBeamElement::from_contact(*first, *second, contact, 500.0, 0.91, 1, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart: the second beam moved off the first
    second->place(Xform::translation(0.0, 0.0, 300.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
top_side_milled_wedge on beams: the milled wedge, on a pair at 60 degrees, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_top_side_milled_wedge --parallel 6 && ./build/element_joint_beam_top_side_milled_wedge && ../bash/publish-scene.sh --target element_joint_beam_top_side_milled_wedge

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
