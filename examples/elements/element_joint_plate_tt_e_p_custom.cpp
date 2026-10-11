#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// tt_e_p_custom: your own outlines on two stacked plates, a hidden butterfly key pocket milled into each, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_tt_e_p_custom");

    // two 400 x 300 plates 40 thick stacked, the upper one turned about the vertical, so their contact is an irregular polygon
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_stacked();
    const std::shared_ptr<Plate> lower = pair[0];
    const std::shared_ptr<Plate> upper = pair[1];
    scene.add(lower);
    scene.add(upper);

    // a hidden butterfly key in the joint's unit box: x across the contact, y along the plates' normal from the lower plate's
    // bottom (-0.5) through the contact (0) to the upper plate's top (0.5), z along the contact; each plate is milled a bow-tie
    // pocket half its thickness deep from the contact face, and a loose key of the same outline locks the two together
    const auto bow_tie = [](double y) {
        return Polyline({{-0.3, y, -0.15}, {0.0, y, -0.06}, {0.3, y, -0.15}, {0.3, y, 0.15}, {0.0, y, 0.06}, {-0.3, y, 0.15}, {-0.3, y, -0.15}});
    };
    const std::vector<Polyline> male = {bow_tie(-0.25), bow_tie(0.0)};
    const std::vector<Polyline> female = {bow_tie(0.0), bow_tie(0.25)};

    // the joint from their face contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
    const std::shared_ptr<JointPlate> joint = JointPlate::tt_e_p_custom(male, female);
    joint->orient(contact, {lower, upper});
    scene.add(joint);
    scene.add_interaction(joint, lower, joint->interaction(0));
    scene.add_interaction(joint, upper, joint->interaction(1));

    // drawn apart along the plates' normal, the key half way, the direction the upper plate drops onto the lower one, so the outlines on both read
    upper->place(Xform::translation(0.0, 0.0, 300.0));
    joint->place(Xform::translation(0.0, 0.0, 150.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
tt_e_p_custom: your own outlines on two stacked plates, a hidden butterfly key pocket milled into each, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_tt_e_p_custom --parallel 6 && ./build/element_joint_plate_tt_e_p_custom && ../bash/publish-scene.sh --target element_joint_plate_tt_e_p_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
