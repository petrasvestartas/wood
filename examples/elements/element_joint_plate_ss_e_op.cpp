#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The out-of-plane designs ss_e_op, a floor and a wall plate at a corner, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_op");

    const std::vector<std::shared_ptr<JointPlate>> joints = {
        JointPlate::ss_e_op_3(),
        JointPlate::ss_e_op_4(),
        JointPlate::ss_e_op_5(),
    };

    for (size_t i = 0; i < joints.size(); i++) {
        const double x = 900.0 * (i % 3);
        const double y = -900.0 * (i / 3);
        const std::shared_ptr<TreeNode> group = scene.add_group(joints[i]->parameters.library);

        // a floor and a wall meeting at a right angle, their side faces mitred on one plane
        const Polyline floor_bottom({{x, y, 0.0}, {x + 300.0, y, 0.0}, {x + 300.0, y + 400.0, 0.0}, {x, y + 400.0, 0.0}, {x, y, 0.0}});
        const Polyline floor_top({{x, y, 40.0}, {x + 260.0, y, 40.0}, {x + 260.0, y + 400.0, 40.0}, {x, y + 400.0, 40.0}, {x, y, 40.0}});
        const Polyline wall_bottom({{x + 300.0, y, 0.0}, {x + 300.0, y + 400.0, 0.0}, {x + 300.0, y + 400.0, 300.0}, {x + 300.0, y, 300.0}, {x + 300.0, y, 0.0}});
        const Polyline wall_top({{x + 260.0, y, 40.0}, {x + 260.0, y + 400.0, 40.0}, {x + 260.0, y + 400.0, 300.0}, {x + 260.0, y, 300.0}, {x + 260.0, y, 40.0}});
        const std::shared_ptr<Plate> floor = std::make_shared<Plate>(floor_bottom, floor_top, fmt::format("floor_{}", i));
        const std::shared_ptr<Plate> wall = std::make_shared<Plate>(wall_bottom, wall_top, fmt::format("wall_{}", i));
        scene.add(floor, group);
        scene.add(wall, group);

        // the joint from their contact, added, and passed to each plate in its target order: the wall first, the male of an out-of-plane pair
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(floor, wall);
        joints[i]->orient(contact, {floor, wall});
        scene.add(joints[i], group);
        scene.add_interaction(joints[i], wall, joints[i]->interaction(0));
        scene.add_interaction(joints[i], floor, joints[i]->interaction(1));

        // drawn apart, so both sides of the joint read
        wall->place(Xform::translation(100.0, 0.0, 100.0));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The side-to-side out-of-plane family ss_e_op, designs ss_e_op_3 to ss_e_op_5, one per pair: a 300 x 400 floor plate and a 300 high wall plate, both 40 thick, meeting at a right angle on a mitred side face; the joint is oriented on their face contact and passed to each plate with add_interaction, the wall its male side and the floor its female, fingers on both edges; the wall is moved 100 out and up afterwards so both read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_op --parallel 6 && ./build/element_joint_plate_ss_e_op && ../bash/publish-scene.sh --target element_joint_plate_ss_e_op

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
