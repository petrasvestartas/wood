#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Butterfly key pocket outline at depth y of the unit box.
static Polyline bow_tie(double y) {

    return Polyline({
        {-0.3, y, -0.15},
        {0.0, y, -0.06},
        {0.3, y, -0.15},
        {0.3, y, 0.15},
        {0.0, y, 0.06},
        {-0.3, y, 0.15},
        {-0.3, y, -0.15},
    });
}

/// top_top_custom.
int main() {

    WoodSession scene("element_joint_plate_top_top_custom");

    const std::vector<Polyline> male = {bow_tie(-0.25), bow_tie(0.0)};
    const std::vector<Polyline> female = {bow_tie(0.0), bow_tie(0.25)};

    // two 400 x 300 plates 40 thick stacked, the upper one shifted: square and turned 30 degrees, the second pair 500 along y, each in its group
    for (const double angle : {0.0, 30.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<FacePlanarElement>, 2> pair = FacePlanarElement::pair_stacked(angle);
        const std::shared_ptr<FacePlanarElement> lower = pair[0];
        const std::shared_ptr<FacePlanarElement> upper = pair[1];
        for (const std::shared_ptr<FacePlanarElement>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 0.0 ? 0.0 : 500.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        const std::shared_ptr<InteractionContactFace> contact =
            scene.compute_face_contact(lower, upper);
        const std::shared_ptr<JointPlateElement> joint = JointPlateElement::top_top_custom(male, female);
        joint->orient(contact, {lower, upper});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, lower, joint->interaction(0));
        scene.add_interaction(joint, upper, joint->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
top_top_custom: your own outlines on two stacked plates, a hidden butterfly key pocket milled into each, on the pairs square and turned 30 degrees.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_top_top_custom --parallel 6 && ./build/element_joint_plate_top_top_custom && ../bash/publish-scene.sh --target element_joint_plate_top_top_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
