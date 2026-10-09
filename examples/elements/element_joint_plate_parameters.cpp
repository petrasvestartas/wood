#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// One design, ts_e_p_3, with its divisions and shift changed, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_parameters");

    const std::vector<std::shared_ptr<JointPlate>> joints = {
        JointPlate::ts_e_p_3(8, 0.5),
        JointPlate::ts_e_p_3(16, 0.5),
        JointPlate::ts_e_p_3(24, 0.5),
        JointPlate::ts_e_p_3(16, 0.0),
        JointPlate::ts_e_p_3(16, 0.25),
        JointPlate::ts_e_p_3(16, 1.0),
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
The parameters of a library design: ts_e_p_3 on the same upright and base six times, the first row with 8, 16 and 24 divisions at shift 0.5, two, four and six tenons (a tenon per four divisions, at least eight), the second with 16 divisions at shift 0, 0.25 and 1, the tenon sides leaning one way at 0, straight at 0.5 and the other way at 1, dovetails that lock the upright.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_parameters --parallel 6 && ./build/element_joint_plate_parameters && ../bash/publish-scene.sh --target element_joint_plate_parameters

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
