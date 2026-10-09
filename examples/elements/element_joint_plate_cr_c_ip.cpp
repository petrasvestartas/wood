#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The cross designs cr_c_ip, two upright plates crossing each other, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cr_c_ip");

    const std::vector<std::shared_ptr<JointPlate>> joints = {
        JointPlate::cr_c_ip_0(),
        JointPlate::cr_c_ip_1(),
        JointPlate::cr_c_ip_2(),
        JointPlate::cr_c_ip_3(),
        JointPlate::cr_c_ip_4(),
        JointPlate::cr_c_ip_5(),
    };

    for (size_t i = 0; i < joints.size(); i++) {
        const double x = 700.0 * (i % 3);
        const double y = -700.0 * (i / 3);
        const std::shared_ptr<TreeNode> group = scene.add_group(joints[i]->parameters.library);

        const std::shared_ptr<Plate> first = Plate::from_rectangle(
            {x, y + 20.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0},
            400.0,
            200.0,
            40.0,
            fmt::format("first_{}", i)
        );
        const std::shared_ptr<Plate> second = Plate::from_rectangle(
            {x + 180.0, y - 200.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
            400.0,
            200.0,
            40.0,
            fmt::format("second_{}", i)
        );
        scene.add(first, group);
        scene.add(second, group);

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactCross> contact = scene.compute_cross_contact(first, second);
        joints[i]->orient(contact, {first, second});
        scene.add(joints[i], group);
        scene.add_interaction(joints[i], first, joints[i]->interaction(0));
        scene.add_interaction(joints[i], second, joints[i]->interaction(1));

        // drawn apart, so both sides of the joint read
        second->place(Xform::translation(0.0, 0.0, 250.0));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The cross family cr_c_ip, one design per pair: two upright 400 x 200 plates 40 thick crossing at their middles; the joint is oriented on their cross contact (compute_cross_contact) and passed to each plate with add_interaction, a slot from the top of one and from the bottom of the other; the second plate is lifted 250 afterwards so both slots read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cr_c_ip --parallel 6 && ./build/element_joint_plate_cr_c_ip && ../bash/publish-scene.sh --target element_joint_plate_cr_c_ip

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
