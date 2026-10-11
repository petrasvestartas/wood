#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// tt_e_p_custom: your own outlines on two stacked plates, a hidden butterfly key pocket milled into each, on the pairs square and turned 30 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_tt_e_p_custom");

    // a hidden butterfly key in the joint's unit box: x across the contact, y along the plates' normal from the lower plate's
    // bottom (-0.5) through the contact (0) to the upper plate's top (0.5), z along the contact; each plate is milled a bow-tie
    // pocket half its thickness deep from the contact face, and a loose key of the same outline locks the two together
    const auto bow_tie = [](double y) {
        return Polyline({{-0.3, y, -0.15}, {0.0, y, -0.06}, {0.3, y, -0.15}, {0.3, y, 0.15}, {0.0, y, 0.06}, {-0.3, y, 0.15}, {-0.3, y, -0.15}});
    };
    const std::vector<Polyline> male = {bow_tie(-0.25), bow_tie(0.0)};
    const std::vector<Polyline> female = {bow_tie(0.0), bow_tie(0.25)};

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
        const std::shared_ptr<JointPlate> joint = JointPlate::tt_e_p_custom(male, female);
        joint->orient(contact, {lower, upper});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, lower, joint->interaction(0));
        scene.add_interaction(joint, upper, joint->interaction(1));

        // drawn apart 300 up, so both sides read
        const Vector apart(0.0, 0.0, 300.0);
        upper->place(Xform::translation(apart[0], apart[1], apart[2]));
        joint->place(Xform::translation(apart[0] * 0.5, apart[1] * 0.5, apart[2] * 0.5));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
tt_e_p_custom: your own outlines on two stacked plates, a hidden butterfly key pocket milled into each, on the pairs square and turned 30 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_tt_e_p_custom --parallel 6 && ./build/element_joint_plate_tt_e_p_custom && ../bash/publish-scene.sh --target element_joint_plate_tt_e_p_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
