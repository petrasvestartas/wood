#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// side_side_rotated_keys.
int main() {

    WoodSession scene("element_joint_plate_side_side_rotated_keys");
    scene.settings.rotated_joint_as_average = true;
    scene.settings.all_treated_as_rotated = true;

    for (const double angle : {0.0, 20.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_rotated(angle);
        const std::shared_ptr<Plate> left = pair[0];
        const std::shared_ptr<Plate> right = pair[1];
        for (const std::shared_ptr<Plate>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 0.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        const std::shared_ptr<InteractionContactFace> contact =
            scene.compute_face_contact(left, right);
        const std::shared_ptr<JointPlate> joint = JointPlate::side_side_rotated_keys();
        joint->orient(contact, {left, right}, scene.settings);
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, left, joint->interaction(0));
        scene.add_interaction(joint, right, joint->interaction(1));

        const Vector apart =
            (right->polylines[0].center() - left->polylines[0].center()).normalized() * 150.0;
        right->place(Xform::translation(apart[0], apart[1], apart[2]));
        joint->place(Xform::translation(apart[0] * 0.5, apart[1] * 0.5, apart[2] * 0.5));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
side_side_rotated_keys: the diamond pockets of two plates folded along a shared edge, their contact read as rotated, and the key that fills both, on the pairs side by side and twisted 20 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_side_side_rotated_keys --parallel 6 && ./build/element_joint_plate_side_side_rotated_keys && ../bash/publish-scene.sh --target element_joint_plate_side_side_rotated_keys

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
