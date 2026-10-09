#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The top-to-top designs tt_e_p, two stacked plates joined by pins, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_tt_e_p");

    const std::vector<std::shared_ptr<JointPlate>> joints = {
        JointPlate::tt_e_p_0(8.0),
        JointPlate::tt_e_p_1(8.0),
        JointPlate::tt_e_p_2(6, 60.0, 8.0),
        JointPlate::tt_e_p_3(60.0, 8.0),
        JointPlate::tt_e_p_4(60.0, 8.0),
        JointPlate::tt_e_p_5(60.0, 8.0),
    };

    for (size_t i = 0; i < joints.size(); i++) {
        const double x = 700.0 * (i % 3);
        const double y = -700.0 * (i / 3);
        const std::shared_ptr<TreeNode> group = scene.add_group(joints[i]->parameters.library);

        const std::shared_ptr<Plate> lower = Plate::from_rectangle(
            {x, y, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            400.0,
            300.0,
            40.0,
            fmt::format("lower_{}", i)
        );
        const std::shared_ptr<Plate> upper = Plate::from_rectangle(
            {x + 150.0, y, 40.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            400.0,
            300.0,
            40.0,
            fmt::format("upper_{}", i)
        );
        scene.add(lower, group);
        scene.add(upper, group);

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
        joints[i]->orient(contact, {lower, upper});
        scene.add(joints[i], group);
        scene.add_interaction(joints[i], lower, joints[i]->interaction(0));
        scene.add_interaction(joints[i], upper, joints[i]->interaction(1));

        // drawn apart, so both sides of the joint read
        upper->place(Xform::translation(0.0, 0.0, 150.0));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The top-to-top family tt_e_p, one design per pair: two 400 x 300 plates 40 thick, the upper one 150 along, its bottom on the lower one's top; the joint is oriented on their face contact and passed to each plate with add_interaction, drilling both plates along its lines; the upper plate is lifted 150 afterwards so the holes read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_tt_e_p --parallel 6 && ./build/element_joint_plate_tt_e_p && ../bash/publish-scene.sh --target element_joint_plate_tt_e_p

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
