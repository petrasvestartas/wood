#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// corner_tenons: the parametric chamfered tenons and through mortises of a floor and a wall plate mitred at a right angle, on the pairs square and folded to 60 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_corner_tenons");

    // a 300 x 400 floor and a 300 high wall, both 40 thick, mitred on the bisector: square and folded to 60 degrees, the second pair 800 along y, each in its group
    for (const double angle : {90.0, 60.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_folded(angle);
        const std::shared_ptr<Plate> floor = pair[0];
        const std::shared_ptr<Plate> wall = pair[1];
        for (const std::shared_ptr<Plate>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 90.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(floor, wall);
        const std::shared_ptr<JointPlate> joint = JointPlate::corner_tenons();
        joint->orient(contact, {floor, wall});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, wall, joint->interaction(0));
        scene.add_interaction(joint, floor, joint->interaction(1));

        // drawn apart 141 along the line between the two plates' centres, so both sides read
        const Vector apart = (wall->polylines[0].center() - floor->polylines[0].center()).normalized() * 141.4;
        wall->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
corner_tenons: the parametric chamfered tenons and through mortises of a floor and a wall plate mitred at a right angle, on the pairs square and folded to 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_corner_tenons --parallel 6 && ./build/element_joint_plate_corner_tenons && ../bash/publish-scene.sh --target element_joint_plate_corner_tenons

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
