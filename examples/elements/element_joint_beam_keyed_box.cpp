#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// keyed_box: two plates face to face, held by a box through a mortise in both and locked by two wedge keys, drawn before its last step, the plates together and the second key lifted out.
int main() {

    WoodSession scene("element_joint_beam_keyed_box");

    // two 40 thick plates standing face to face, 320 x 400
    const double thickness = 40.0;
    const Vector across(0.0, 1.0, 0.0);
    const Vector up(0.0, 0.0, 1.0);
    const Point corner(0.0, -160.0, -200.0);
    const std::shared_ptr<FacePlanarElement> first = FacePlanarElement::from_rectangle(
        corner - Vector(thickness, 0.0, 0.0), across, up, 320.0, 400.0, thickness, "first"
    );
    const std::shared_ptr<FacePlanarElement> second =
        FacePlanarElement::from_rectangle(corner, across, up, 320.0, 400.0, thickness, "second");
    scene.add(first);
    scene.add(second);

    const std::shared_ptr<InteractionContactFace> contact =
        scene.compute_face_contact(first, second);
    const std::shared_ptr<JointBeamElement> joint = JointBeamElement::keyed_box(*first, *second, *contact);
    joint->name = "keyed_box";
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    const std::shared_ptr<JointBeamElement> key_second =
        scene.get_element_by_name<JointBeamElement>("keyed_box_part_2");

    // the plates together on the box, the second key lifted out: it only goes in once the plates are seated
    key_second->place(Xform::translation(0.0, 0.0, 220.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
keyed_box: two plates face to face, held by a box through a mortise in both and locked by two wedge keys. The first key stops the box on the second plate; the second, driven in once the plates meet, pinches them together. Drawn before its last step, the plates together and the second key lifted out.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_keyed_box --parallel 6 && ./build/element_joint_beam_keyed_box && ../bash/publish-scene.sh --target element_joint_beam_keyed_box

|||||||| TIME ||||||||
1.16 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
