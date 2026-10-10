#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// tt_e_p_0: one pin hole at the centre of the contact of two stacked plates, drilled through both, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_tt_e_p_0");

    // two 400 x 300 plates 40 thick, the upper one turned about the vertical, its bottom on the lower one's top, so their contact is
    // an irregular polygon, its centre apart from its polylabel
    const std::shared_ptr<Plate> lower = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 300.0, 40.0, "lower");
    const std::shared_ptr<Plate> upper = Plate::from_rectangle({150.0, -60.0, 40.0}, {0.8, 0.6, 0.0}, {-0.6, 0.8, 0.0}, 400.0, 300.0, 40.0, "upper");
    scene.add(lower);
    scene.add(upper);

    // the joint from their face contact, added, and passed to each plate in its target order, an 8 mm pin
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
    const std::shared_ptr<JointPlate> joint = JointPlate::tt_e_p_0(8.0);
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
The top-to-top design tt_e_p_0 on one pair: two 400 x 300 plates 40 thick, the upper one turned about the vertical, its x axis (0.8, 0.6), its corner at (150, -60), its bottom on the lower one's top, so their contact is an irregular octagon; the joint is oriented on their face contact and passed to each plate with add_interaction, one pin hole of radius 8 at the centre of the contact, drilled one plate thickness into each plate, a cylinder through each; the joint owns no piece and stays hidden; the upper plate is lifted 150 afterwards, along the plates' normal, so the holes in both read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_tt_e_p_0 --parallel 6 && ./build/element_joint_plate_tt_e_p_0 && ../bash/publish-scene.sh --target element_joint_plate_tt_e_p_0

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
