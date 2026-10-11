#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ts_e_p_3: the parametric tenons of an upright plate standing on a base plate, a tenon per four divisions, through their mortises in the base, on the pairs upright and leaning to 60 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ts_e_p_3");

    // a 400 x 400 base and a 250 x 250 upright on its middle, both 40 thick: upright and leaning to 60 degrees, the second pair 800 along y, each in its group
    for (const double angle : {90.0, 60.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_top_side(angle);
        const std::shared_ptr<Plate> base = pair[0];
        const std::shared_ptr<Plate> upright = pair[1];
        for (const std::shared_ptr<Plate>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 90.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
        const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_3();
        joint->orient(contact, {base, upright});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, upright, joint->interaction(0));
        scene.add_interaction(joint, base, joint->interaction(1));

        // drawn apart 120 along the line between the two plates' centres, so both sides read
        const Vector apart = (upright->polylines[0].center() - base->polylines[0].center()).normalized() * 120.0;
        upright->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ts_e_p_3: the parametric tenons of an upright plate standing on a base plate, a tenon per four divisions, through their mortises in the base, on the pairs upright and leaning to 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ts_e_p_3 --parallel 6 && ./build/element_joint_plate_ts_e_p_3 && ../bash/publish-scene.sh --target element_joint_plate_ts_e_p_3

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
