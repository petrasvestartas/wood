#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// b_0, the boundary family: a joint on one plate's side face alone, its four slices standing outside the face.
int main() {

    WoodSession scene("element_joint_plate_b_0");

    // a 300 x 200 plate 40 thick
    const Polyline bottom({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 200.0, 0.0}, {0.0, 200.0, 0.0}, {0.0, 0.0, 0.0}});
    const std::shared_ptr<Plate> plate = std::make_shared<Plate>(bottom, bottom.transformed(Xform::translation(0.0, 0.0, 40.0)), "plate");
    scene.add(plate);

    // the border contact of its side face 2, the joint oriented on it with the plate alone, added and passed to the plate
    const std::shared_ptr<InteractionContactFace> contact = WoodSession::compute_border_contact(*plate, 2);
    const std::shared_ptr<JointPlate> joint = JointPlate::b_0();
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
The boundary family on one plate: a 300 x 200 plate 40 thick, the border contact of its side face 2 (WoodSession::compute_border_contact, 2024's border_to_face: the average of the face's two side edges and two thin rectangles across the thickness around it), and JointPlate::b_0 oriented on it with the plate alone, as the solver makes it for an adjacency row pairing a plate with itself on a face; its four slice rectangles, 0.25 and 16 either side of the face's middle, stand 6 out of the face and a millimetre past the plate's faces, exactly as the 2025 reference writes them, and cut nothing from the plate; the joint is shown so the slices read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_b_0 --parallel 6 && ./build/element_joint_plate_b_0 && ../bash/publish-scene.sh --target element_joint_plate_b_0

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
