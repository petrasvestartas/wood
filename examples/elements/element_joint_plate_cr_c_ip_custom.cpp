#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cr_c_ip_custom: your own rectangles in the joint's unit box, an unequal slot into each of two upright plates crossing at their middles, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cr_c_ip_custom");

    // two upright 400 x 200 plates 40 thick crossing at their middles, the first along x and the second along y
    const std::shared_ptr<Plate> first = Plate::from_rectangle({0.0, 20.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "first");
    const std::shared_ptr<Plate> second = Plate::from_rectangle({180.0, -200.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "second");
    scene.add(first);
    scene.add(second);

    // the outlines in the joint's unit box, mapped onto the cross: one plate's thickness along x, the other's along y and the
    // cross's depth along z, its middle at z = 0, as 2024's library drew the family; a pair per side, the male's on the faces at
    // y = -0.5 and y = 0.5, the first plate's on this pair, the male of a cross, and the female's on the faces at x = -0.5 and
    // x = 0.5, the second's; as 2024 merged a custom pair, a closed rectangle of five points is clipped into the plate's outline:
    // here the slots split the depth unequally, the male's from the first plate's top edge, where cr_c_ip_0 starts its slot, to
    // z = -0.4, 60 deep, and the female's from the second plate's bottom edge to the same z = -0.4, 140 deep
    const std::vector<Polyline> male = {
        Polyline({{0.5, -0.5, -1.0}, {-0.5, -0.5, -1.0}, {-0.5, -0.5, -0.4}, {0.5, -0.5, -0.4}, {0.5, -0.5, -1.0}}),
        Polyline({{0.5, 0.5, -1.0}, {-0.5, 0.5, -1.0}, {-0.5, 0.5, -0.4}, {0.5, 0.5, -0.4}, {0.5, 0.5, -1.0}}),
    };
    const std::vector<Polyline> female = {
        Polyline({{-0.5, 0.5, 1.0}, {-0.5, -0.5, 1.0}, {-0.5, -0.5, -0.4}, {-0.5, 0.5, -0.4}, {-0.5, 0.5, 1.0}}),
        Polyline({{0.5, 0.5, 1.0}, {0.5, -0.5, 1.0}, {0.5, -0.5, -0.4}, {0.5, 0.5, -0.4}, {0.5, 0.5, 1.0}}),
    };

    // the joint from their cross contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactCross> contact = scene.compute_cross_contact(first, second);
    const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_custom(male, female);
    joint->orient(contact, {first, second});
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart along the cross's depth, the direction the plates slide into each other, so both slots read
    second->place(Xform::translation(0.0, 0.0, 250.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The cross design cr_c_ip_custom on one pair: two upright 400 x 200 plates 40 thick crossing at their middles, the first along x and the second along y; the joint takes your own outlines in its unit box, the first plate's thickness along x, the second's along y and the cross's depth along z, is oriented on their cross contact (compute_cross_contact) and passed to each plate with add_interaction; as the 2024 library kept a custom pair, the outlines carry the fabrication type nothing and only a closed rectangle of five points, or a line of two, is merged into the plate: here the male pair, the first plate's on a cross, cuts a slot 60 deep down from its top edge and the female pair cuts one 140 deep up from the second plate's bottom edge, the cross's depth split unequally where cr_c_ip_0 halves it; the joint owns no piece and stays hidden; the second plate is lifted 250 afterwards, along the cross's depth, so both slots read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cr_c_ip_custom --parallel 6 && ./build/element_joint_plate_cr_c_ip_custom && ../bash/publish-scene.sh --target element_joint_plate_cr_c_ip_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
