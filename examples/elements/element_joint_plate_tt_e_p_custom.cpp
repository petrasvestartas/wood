#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// tt_e_p_custom: your own rectangles in the joint's unit box on the faces of two stacked plates, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_tt_e_p_custom");

    // two 400 x 300 plates 40 thick, the upper one turned about the vertical, its bottom on the lower one's top, so their contact is
    // an irregular polygon, its centre apart from its polylabel
    const std::shared_ptr<Plate> lower = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 300.0, 40.0, "lower");
    const std::shared_ptr<Plate> upper = Plate::from_rectangle({150.0, -60.0, 40.0}, {0.8, 0.6, 0.0}, {-0.6, 0.8, 0.0}, 400.0, 300.0, 40.0, "upper");
    scene.add(lower);
    scene.add(upper);

    // the outlines in the joint's unit box, mapped onto the contact: x across the contact, y along the plates' normal, from the
    // lower plate's bottom at y = -0.5 through the contact at y = 0 to the upper plate's top at y = 0.5, z along the contact;
    // a pair per side, the lower plate's on its bottom and its top (face 0, face 1), the upper plate's on its bottom and its top,
    // here a rectangle over the middle half of the contact on each; as 2024 kept a custom pair, the outlines carry the
    // fabrication type nothing, and a top-top contact has no edge to merge them into, so they stay features and cut nothing
    const Polyline bottom({{-0.25, -0.5, -0.25}, {0.25, -0.5, -0.25}, {0.25, -0.5, 0.25}, {-0.25, -0.5, 0.25}, {-0.25, -0.5, -0.25}});
    const Polyline middle({{-0.25, 0.0, -0.25}, {0.25, 0.0, -0.25}, {0.25, 0.0, 0.25}, {-0.25, 0.0, 0.25}, {-0.25, 0.0, -0.25}});
    const Polyline top({{-0.25, 0.5, -0.25}, {0.25, 0.5, -0.25}, {0.25, 0.5, 0.25}, {-0.25, 0.5, 0.25}, {-0.25, 0.5, -0.25}});
    const std::vector<Polyline> male = {bottom, middle};
    const std::vector<Polyline> female = {middle, top};

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
The top-to-top design tt_e_p_custom on one pair: two 400 x 300 plates 40 thick, the upper one turned about the vertical, its x axis (0.8, 0.6), its corner at (150, -60), its bottom on the lower one's top, so their contact is an irregular octagon; the joint is oriented on their face contact and passed to each plate with add_interaction, your own outlines in its unit box, the box mapped onto the contact: x across it, y along the plates' normal from the lower plate's bottom to the upper plate's top, z along it; a rectangle over the middle half of the contact on each face of each plate, kept pair by pair as the 2024 library kept a custom design, of fabrication type nothing; a top-top contact has no edge to merge a rectangle into, so the outlines stay features and cut nothing; the joint owns no piece and stays hidden; the upper plate is lifted 150 afterwards, along the plates' normal, so the outlines on both read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_tt_e_p_custom --parallel 6 && ./build/element_joint_plate_tt_e_p_custom && ../bash/publish-scene.sh --target element_joint_plate_tt_e_p_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
