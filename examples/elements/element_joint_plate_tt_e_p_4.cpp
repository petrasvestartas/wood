#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// tt_e_p_4: a lattice of pin holes 60 apart inside the contact of two stacked plates offset 20 inward, drilled through both, on the pairs square and turned 30 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_tt_e_p_4");

    // two 400 x 300 plates 40 thick stacked, the upper one shifted: square and turned 30 degrees, the second pair 800 along y, each in its group
    for (const double angle : {0.0, 30.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_stacked(angle);
        const std::shared_ptr<Plate> lower = pair[0];
        const std::shared_ptr<Plate> upper = pair[1];
        for (const std::shared_ptr<Plate>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 0.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
        const std::shared_ptr<JointPlate> joint = JointPlate::tt_e_p_4(60.0, 20.0, 8.0);
        joint->orient(contact, {lower, upper});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, lower, joint->interaction(0));
        scene.add_interaction(joint, upper, joint->interaction(1));

        // drawn apart 150 up, so both sides read
        const Vector apart(0.0, 0.0, 150.0);
        upper->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
tt_e_p_4: a lattice of pin holes 60 apart inside the contact of two stacked plates offset 20 inward, drilled through both, on the pairs square and turned 30 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_tt_e_p_4 --parallel 6 && ./build/element_joint_plate_tt_e_p_4 && ../bash/publish-scene.sh --target element_joint_plate_tt_e_p_4

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
