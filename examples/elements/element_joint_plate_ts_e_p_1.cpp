#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ts_e_p_1: the two Annen tenons of an upright plate standing on a base plate, through two mortises in the base, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ts_e_p_1");

    // a 400 x 400 base plate and a 250 x 250 upright standing in the middle of its top face, both 40 thick
    const std::shared_ptr<Plate> base = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 400.0, 40.0, "base");
    const std::shared_ptr<Plate> upright = Plate::from_rectangle({180.0, 75.0, 40.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 250.0, 250.0, 40.0, "upright");
    scene.add(base);
    scene.add(upright);

    // the joint from their contact, added, and passed to each plate in its target order: the upright first, the male of a top-side pair
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
    const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_1();
    joint->orient(contact, {base, upright});
    scene.add(joint);
    scene.add_interaction(joint, upright, joint->interaction(0));
    scene.add_interaction(joint, base, joint->interaction(1));

    // drawn apart along the base's normal, the direction the upright drops onto the base, so the tenons and the mortises both read
    upright->place(Xform::translation(0.0, 0.0, 120.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The top-side design ts_e_p_1 on one pair: a 400 x 400 base plate and a 250 x 250 upright standing in the middle of its top face, both 40 thick; the joint is oriented on their face contact and passed to each plate with add_interaction, the upright its male side and the base its female, the two fixed tenons of the Annen project on the upright's bottom edge, one run with the full-height edge marker, each the base's thickness deep, and two mortises through the base, a hole on each of its faces, 2024's literals with no parameters; the joint owns no piece and stays hidden; the upright is lifted 120 afterwards, along the base's normal, so the tenons and the mortises both read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ts_e_p_1 --parallel 6 && ./build/element_joint_plate_ts_e_p_1 && ../bash/publish-scene.sh --target element_joint_plate_ts_e_p_1

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
