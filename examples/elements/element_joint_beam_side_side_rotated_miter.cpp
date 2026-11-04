#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// side_side_rotated_miter on beams.
int main() {

    WoodSession scene("element_joint_beam_side_side_rotated_miter");
    scene.settings.joint_parameters[17] = JointPlate::library_id("side_side_rotated_miter");

    // two 150 x 150 beams 1200 long, meeting at 60 degrees
    const std::array<std::shared_ptr<EdgeStraightElement>, 2> pair = EdgeStraightElement::pair_end_to_end(60.0);
    const std::shared_ptr<EdgeStraightElement> first = pair[0];
    const std::shared_ptr<EdgeStraightElement> second = pair[1];
    for (const std::shared_ptr<EdgeStraightElement>& beam : pair) {
        scene.add(beam);
    }

    const InteractionContactAxis contact(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}), 1.0, 0.0, 0, 0, 0, 0
    );
    const std::shared_ptr<JointBeam> joint =
        JointBeam::from_contact(*first, *second, contact, 600.0, 0.91, 0, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart: the second beam moved off the first
    second->place(Xform::translation(0.0, 300.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
side_side_rotated_miter on beams: a miter trimming two extended beams at their corner, on a pair meeting at 60 degrees, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_side_side_rotated_miter --parallel 6 && ./build/element_joint_beam_side_side_rotated_miter && ../bash/publish-scene.sh --target element_joint_beam_side_side_rotated_miter

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
