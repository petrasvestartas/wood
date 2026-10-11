#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// rotated_mitre on beams: a mitre trimming two extended beams meeting at a right-angled corner, on the pairs meeting square and at 60 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_beam_rotated_mitre");
    scene.settings.joint_parameters[17] = JointPlate::library_id("rotated_mitre");

    // two 150 x 150 beams 1200 long: meeting square and at 60 degrees, the second pair 2000 along x, each in its group
    for (const double angle : {90.0, 60.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Beam>, 2> pair = Beam::pair_end_to_end(angle);
        const std::shared_ptr<Beam> first = pair[0];
        const std::shared_ptr<Beam> second = pair[1];
        for (const std::shared_ptr<Beam>& beam : pair) {
            beam->place(Xform::translation(angle == 90.0 ? 0.0 : 2000.0, 0.0, 0.0));
            beam->name = fmt::format("{}_{:g}", beam->name, angle);
            scene.add(beam, group);
        }

        // the joint from their axis contact, added and passed to each beam
        const InteractionContactAxis contact(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}), 1.0, 0.0, 0, 0, 0, 0);
        const std::shared_ptr<JointBeam> joint = JointBeam::from_contact(*first, *second, contact, 600.0, 0.91, 0, scene.settings);
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, first, joint->interaction(0));
        scene.add_interaction(joint, second, joint->interaction(1));

        // drawn apart: the second beam moved off the first
        second->place(Xform::translation(0.0, 300.0, 0.0));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
rotated_mitre on beams: a mitre trimming two extended beams meeting at a right-angled corner, on the pairs meeting square and at 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_rotated_mitre --parallel 6 && ./build/element_joint_beam_rotated_mitre && ../bash/publish-scene.sh --target element_joint_beam_rotated_mitre

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
