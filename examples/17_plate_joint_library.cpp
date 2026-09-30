#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession scene("17_plate_joint_library");
    const std::vector<std::shared_ptr<JointPlate>> joints = {
        JointPlate::ts_e_p_0(),
        JointPlate::ts_e_p_1(),
        JointPlate::ts_e_p_2(8, 0.5),
        JointPlate::ts_e_p_3(8, 0.5),
        JointPlate::ts_e_p_3(16, 0.25),
        JointPlate::ts_e_p_5(4)
    };

    for (size_t i = 0; i < joints.size(); ++i) {
        const double offset = i * 600.0;
        const std::shared_ptr<Plate> bottom = Plate::from_rectangle({offset, 0, 0}, {1, 0, 0}, {0, 1, 0}, 400, 300, 40);
        const std::shared_ptr<Plate> upright = Plate::from_rectangle({offset + 150, 0, 40}, {0, 1, 0}, {0, 0, 1}, 300, 250, 40);
        const std::shared_ptr<TreeNode> group = scene.add_group(joints[i]->parameters.library + " / " + std::to_string(i));
        scene.add(bottom, group);
        scene.add(upright, group);
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(bottom, upright);
        joints[i]->orient(contact, {bottom, upright});
        scene.add(joints[i], group);
        scene.add_joint(joints[i]);
    }

    scene.pb_dump(pb_path("17_plate_joint_library"));
    scene.pb_dump(pb_path("live"));

    return 0;
}

/*
description: named plate joint constructors, with different parameters on the same contact.
directory: cd /home/petras/code/code_cpp/wood_research/wood
run: buildslot ~/.local/bin/cmake --build build --target 17_plate_joint_library --parallel 4 && tools/run_guarded.sh -t 10 -m 4 -- build/17_plate_joint_library
cloudflare: ../bash/publish-scene.sh "$PWD/data/output/pb/17_plate_joint_library.pb" --no-notify
view: https://petrasvestartas.github.io/session/
*/
