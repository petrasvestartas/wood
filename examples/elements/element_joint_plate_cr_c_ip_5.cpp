#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cr_c_ip_5: the Brussels half-lap of two upright plates crossing at their middles, its bottom sides extended unequally, a 50 mm bit down the centre and a 10 mm bit below the lap, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cr_c_ip_5");

    // two upright 400 x 200 plates 40 thick crossing at their middles, the first along x and the second along y
    const std::shared_ptr<Plate> first = Plate::from_rectangle({0.0, 20.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "first");
    const std::shared_ptr<Plate> second = Plate::from_rectangle({180.0, -200.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "second");
    scene.add(first);
    scene.add(second);

    // the joint from their cross contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactCross> contact = scene.compute_cross_contact(first, second);
    const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_5();
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
The cross design cr_c_ip_5 on one pair, on its default shift 0.5: two upright 400 x 200 plates 40 thick crossing at their middles, the first along x and the second along y; the joint is oriented on their cross contact (compute_cross_contact) and passed to each plate with add_interaction, the five rings of the milled half-lap, their bottom sides extended 0.27 on their first segment and shortened 0.075 on their third as 2024 built the Brussels sports tower (data/cross_brussels_sports_tower.yml, id 35), a vertical 50 mm bit down the centre of the lap and a horizontal 10 mm bit below it, each bored through both plates; the joint owns no piece and stays hidden; the second plate is lifted 250 afterwards, along the cross's depth, so both half-laps and the bits read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cr_c_ip_5 --parallel 6 && ./build/element_joint_plate_cr_c_ip_5 && ../bash/publish-scene.sh --target element_joint_plate_cr_c_ip_5

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
