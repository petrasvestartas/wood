#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// side_side_rotated_miter: the four slices of two plates folded along a shared edge, their contact read as rotated, on the pairs side by side and twisted 20 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_side_side_rotated_miter");
    scene.settings.rotated_joint_as_average = true;
    scene.settings.all_treated_as_rotated = true;

    // two 300 x 400 blocks 100 thick end to end, the right one twisted about the seam's normal: side by side and twisted 20 degrees, the second pair 800 along y, each in its group
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

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
        const std::shared_ptr<JointPlate> joint = JointPlate::side_side_rotated_miter();
        joint->orient(contact, {left, right}, scene.settings);
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, left, joint->interaction(0));
        scene.add_interaction(joint, right, joint->interaction(1));

        // drawn apart 150 along the line between the two plates' centres, so both sides read
        const Vector apart = (right->polylines[0].center() - left->polylines[0].center()).normalized() * 150.0;
        right->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
side_side_rotated_miter: the four slices of two plates folded along a shared edge, their contact read as rotated, on the pairs side by side and twisted 20 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_side_side_rotated_miter --parallel 6 && ./build/element_joint_plate_side_side_rotated_miter && ../bash/publish-scene.sh --target element_joint_plate_side_side_rotated_miter

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
