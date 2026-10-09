#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The side-to-top designs ts_e_p, an upright plate on the top of a base plate, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ts_e_p");

    const std::vector<std::shared_ptr<JointPlate>> joints = {
        JointPlate::ts_e_p_0(),
        JointPlate::ts_e_p_1(),
        JointPlate::ts_e_p_2(),
        JointPlate::ts_e_p_3(),
    };

    for (size_t i = 0; i < joints.size(); i++) {
        const double x = 700.0 * (i % 3);
        const double y = -700.0 * (i / 3);
        const std::shared_ptr<TreeNode> group = scene.add_group(joints[i]->parameters.library);

        const std::shared_ptr<Plate> base = Plate::from_rectangle(
            {x, y, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            400.0,
            400.0,
            40.0,
            fmt::format("base_{}", i)
        );
        const std::shared_ptr<Plate> upright = Plate::from_rectangle(
            {x + 180.0, y + 75.0, 40.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
            250.0,
            250.0,
            40.0,
            fmt::format("upright_{}", i)
        );
        scene.add(base, group);
        scene.add(upright, group);

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
        joints[i]->orient(contact, {base, upright});
        scene.add(joints[i], group);
        scene.add_interaction(joints[i], upright, joints[i]->interaction(0));
        scene.add_interaction(joints[i], base, joints[i]->interaction(1));

        // drawn apart, so both sides of the joint read
        upright->place(Xform::translation(0.0, 0.0, 200.0));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The side-to-top family ts_e_p, one design per pair: ts_e_p_0 to ts_e_p_3, each an upright 250 x 250 plate standing in the middle of a 400 x 400 base, both 40 thick; the joint is oriented on their face contact and passed to each plate with add_interaction, tenons on the upright and mortises through the base; the upright is lifted 200 afterwards so both read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ts_e_p --parallel 6 && ./build/element_joint_plate_ts_e_p && ../bash/publish-scene.sh --target element_joint_plate_ts_e_p

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
