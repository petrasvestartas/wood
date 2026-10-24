#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// border, the boundary family: a joint on one plate's side face alone, its four slices standing outside the face.
int main() {

    WoodSession scene("element_joint_plate_border");

    const Polyline bottom(
        {{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 200.0, 0.0}, {0.0, 200.0, 0.0}, {0.0, 0.0, 0.0}
        }
    );
    const std::shared_ptr<Plate> plate = std::make_shared<Plate>(
        bottom, bottom.transformed(Xform::translation(0.0, 0.0, 40.0)), "plate"
    );
    scene.add(plate);

    const std::shared_ptr<InteractionContactFace> contact =
        WoodSession::compute_border_contact(*plate, 2);
    const std::shared_ptr<JointPlate> joint = JointPlate::border();
    joint->orient(contact, {plate});
    joint->is_visible = true;
    scene.add(joint);
    scene.add_interaction(joint, plate, joint->interaction(0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
border, the boundary family: a joint on one plate's side face alone, its four slices standing outside the face.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_border --parallel 6 && ./build/element_joint_plate_border && ../bash/publish-scene.sh --target element_joint_plate_border

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
