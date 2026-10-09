#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The in-plane designs ss_e_ip, two plates side by side in one plane, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_ip");

    const std::vector<std::shared_ptr<JointPlate>> joints = {
        JointPlate::ss_e_ip_0(),
        JointPlate::ss_e_ip_1(),
        JointPlate::ss_e_ip_2(),
        JointPlate::ss_e_ip_3(),
        JointPlate::ss_e_ip_4(),
        JointPlate::ss_e_ip_5(),
    };

    for (size_t i = 0; i < joints.size(); i++) {
        const double x = 900.0 * (i % 3);
        const double y = -900.0 * (i / 3);
        const std::shared_ptr<TreeNode> group = scene.add_group(joints[i]->parameters.library);

        const std::shared_ptr<Plate> left = Plate::from_rectangle(
            {x, y, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            300.0,
            400.0,
            40.0,
            fmt::format("left_{}", i)
        );
        const std::shared_ptr<Plate> right = Plate::from_rectangle(
            {x + 300.0, y, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            300.0,
            400.0,
            40.0,
            fmt::format("right_{}", i)
        );
        scene.add(left, group);
        scene.add(right, group);

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
        joints[i]->orient(contact, {left, right});
        scene.add(joints[i], group);
        scene.add_interaction(joints[i], left, joints[i]->interaction(0));
        scene.add_interaction(joints[i], right, joints[i]->interaction(1));

        // drawn apart, so both sides of the joint read
        right->place(Xform::translation(150.0, 0.0, 0.0));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The side-to-side in-plane family ss_e_ip, one design per pair: two 300 x 400 plates 40 thick meeting edge to edge in one plane; the joint is oriented on their face contact and passed to each plate with add_interaction, the edge of each cut to the joint's profile; the right plate is moved 150 away afterwards so both read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip --parallel 6 && ./build/element_joint_plate_ss_e_ip && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
