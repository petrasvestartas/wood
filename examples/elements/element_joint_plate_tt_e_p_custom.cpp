#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// tt_e_p_custom: your own outlines on two stacked plates, a hidden butterfly key pocket milled into each, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_tt_e_p_custom");

    // two 400 x 300 plates 40 thick, the upper one turned about the vertical, its bottom on the lower one's top, so their contact is
    // an irregular polygon, its centre apart from its polylabel
    const std::shared_ptr<Plate> lower = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 300.0, 40.0, "lower");
    const std::shared_ptr<Plate> upper = Plate::from_rectangle({150.0, -60.0, 40.0}, {0.8, 0.6, 0.0}, {-0.6, 0.8, 0.0}, 400.0, 300.0, 40.0, "upper");
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

    // drawn apart along the plates' normal, the direction the upper plate drops onto the lower one, so the outlines on both read
    upper->place(Xform::translation(0.0, 0.0, 150.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The top-to-top design tt_e_p_custom on one pair: two 400 x 300 plates 40 thick, the upper one turned about the vertical, its bottom on the lower one's top, so their contact is an irregular octagon. Your own outlines in the joint's unit box: a bow-tie pair per plate, from the contact face to half the plate's thickness, milled as a pocket into each plate; a loose butterfly key of the same outline, set in both pockets, locks the plates against sliding and pulling apart and stays hidden between them. The joint is oriented on the face contact and passed to each plate with add_interaction; the upper plate is lifted 150 afterwards so both pockets read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_tt_e_p_custom --parallel 6 && ./build/element_joint_plate_tt_e_p_custom && ../bash/publish-scene.sh --target element_joint_plate_tt_e_p_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
