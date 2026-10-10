#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// tt_e_p_5: pin holes every 60 along the largest rectangle inscribed in the contact of two stacked plates, scaled 0.95, drilled through both, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_tt_e_p_5");

    // two 400 x 300 plates 40 thick stacked, the upper one turned about the vertical, so their contact is an irregular polygon
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_stacked();
    const std::shared_ptr<Plate> lower = pair[0];
    const std::shared_ptr<Plate> upper = pair[1];
    scene.add(lower);
    scene.add(upper);

    // the joint from their face contact, added, and passed to each plate in its target order, 8 mm pins every 60 along the largest rectangle inscribed in the contact, scaled 0.95
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
    const std::shared_ptr<JointPlate> joint = JointPlate::tt_e_p_5(60.0, 0.95, 8.0);
    joint->orient(contact, {lower, upper});
    scene.add(joint);
    scene.add_interaction(joint, lower, joint->interaction(0));
    scene.add_interaction(joint, upper, joint->interaction(1));

    // drawn apart along the plates' normal, the direction the upper plate drops onto the lower one, so the holes in both read
    upper->place(Xform::translation(0.0, 0.0, 150.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The top-to-top design tt_e_p_5 on one pair: two 400 x 300 plates 40 thick, the upper one turned about the vertical, its x axis (0.8, 0.6), its corner at (150, -60), its bottom on the lower one's top, so their contact is an irregular octagon; the joint is oriented on their face contact and passed to each plate with add_interaction, the largest empty rectangle 2024 inscribed in the contact, inset by 1 - 0.95 of its shorter extent, and a pin hole of radius 8 every 60 along its edges, each drilled one plate thickness into each plate, a cylinder through each; the joint owns no piece and stays hidden; the upper plate is lifted 150 afterwards, along the plates' normal, so the holes in both read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_tt_e_p_5 --parallel 6 && ./build/element_joint_plate_tt_e_p_5 && ../bash/publish-scene.sh --target element_joint_plate_tt_e_p_5

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
