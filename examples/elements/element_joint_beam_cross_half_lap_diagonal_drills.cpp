#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cross_half_lap_diagonal_drills on beams: two diagonal drills, on a pair crossing at 60 degrees, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_cross_half_lap_diagonal_drills");
    scene.settings.joint_parameters[11] = JointPlate::cross_half_lap_id(CrossDrills::two_diagonal);

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
    const std::shared_ptr<JointBeam> joint =
        JointBeam::from_contact(*first, *second, contact, 500.0, 0.91, 1, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart: the second beam moved down off the first, the way it comes out of its notch
    second->place(Xform::translation(0.0, 0.0, -300.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
cross_half_lap_diagonal_drills on beams: two diagonal drills, on a pair crossing at 60 degrees, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_cross_half_lap_diagonal_drills --parallel 6 && ./build/element_joint_beam_cross_half_lap_diagonal_drills && ../bash/publish-scene.sh --target element_joint_beam_cross_half_lap_diagonal_drills

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
