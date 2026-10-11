#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// rotated_custom: your own outlines, a half-lap step along the whole seam, the male's tongue in the female's rebate, on the pair side by side, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_rotated_custom");
    scene.settings.rotated_joint_as_average = true;
    scene.settings.all_treated_as_rotated = true;

    // a step along the whole seam in the joint's unit box: x across the seam, y through the thickness, z along the joint line; the male's
    // edge moved half a thickness across the seam, a straight profile and its end line on each face, and its upper half milled off over
    // that strip, the female's lower half milled off over the same strip, so the male's tongue fills the female's rebate
    const auto edge = [](double y) {
        return Polyline({{-0.5, y, 0.5}, {-0.5, y, -0.5}});
    };
    const auto strip = [](double x0, double y) {
        return Polyline({{x0, y, 0.6}, {-0.5, y, 0.6}, {-0.5, y, -0.6}, {x0, y, -0.6}, {x0, y, 0.6}});
    };
    const std::vector<Polyline> male = {edge(-0.5), edge(0.5), edge(-0.5), edge(0.5), strip(0.0, 0.0), strip(0.0, 0.5)};
    const std::vector<Polyline> female = {strip(0.1, -0.5), strip(0.1, 0.0)};

    // two 300 x 400 blocks 100 thick end to end, the right one twisted about the seam's normal: side by side; twisted plates take no step, their faces not parallel to carry it
    for (const double angle : {0.0}) {
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
        const std::shared_ptr<JointPlate> joint = JointPlate::rotated_custom(male, female);
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
rotated_custom: your own outlines, a half-lap step along the whole seam, the male's tongue in the female's rebate, on the pair side by side, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_rotated_custom --parallel 6 && ./build/element_joint_plate_rotated_custom && ../bash/publish-scene.sh --target element_joint_plate_rotated_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
