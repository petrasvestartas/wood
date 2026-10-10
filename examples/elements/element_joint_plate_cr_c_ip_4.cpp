#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cr_c_ip_4: the milled half-lap of two upright plates crossing at their middles, with one vertical drill through its centre, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cr_c_ip_4");

    // two upright 400 x 200 plates 40 thick crossing at their middles, the first along x and the second along y
    const std::shared_ptr<Plate> first = Plate::from_rectangle({0.0, 20.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "first");
    const std::shared_ptr<Plate> second = Plate::from_rectangle({180.0, -200.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "second");
    scene.add(first);
    scene.add(second);

    // the joint from their cross contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactCross> contact = scene.compute_cross_contact(first, second);
    const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_4();
    joint->orient(contact, {first, second});
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart along the cross's depth, the direction the plates slide into each other, so both half-laps read
    second->place(Xform::translation(0.0, 0.0, 250.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The cross design cr_c_ip_4 on one pair, on its default shift 0.5: two upright 400 x 200 plates 40 thick crossing at their middles, the first along x and the second along y; the joint is oriented on their cross contact (compute_cross_contact) and passed to each plate with add_interaction, the five rings of the milled half-lap of cr_c_ip_2, their bottom sides extended 0.15, and one vertical drill down the centre of the lap, a line along the cross's depth bored through both plates as a drill, kept where 2024 declared it (2024 offset it too, reading past its arrays); the joint owns no piece and stays hidden; the second plate is lifted 250 afterwards, along the cross's depth, so both half-laps and the drill hole read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cr_c_ip_4 --parallel 6 && ./build/element_joint_plate_cr_c_ip_4 && ../bash/publish-scene.sh --target element_joint_plate_cr_c_ip_4

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
