#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// top_side_annen_tenons.
int main() {

    WoodSession scene("element_joint_plate_top_side_annen_tenons");

    // a 400 x 400 base and a 250 x 250 upright on its middle, both 40 thick: upright and leaning to 60 degrees, the second pair 800 along y, each in its group
    for (const double angle : {90.0, 60.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<FacePlanarElement>, 2> pair = FacePlanarElement::pair_top_side(angle);
        const std::shared_ptr<FacePlanarElement> base = pair[0];
        const std::shared_ptr<FacePlanarElement> upright = pair[1];
        for (const std::shared_ptr<FacePlanarElement>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 90.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        const std::shared_ptr<InteractionContactFace> contact =
            scene.compute_face_contact(base, upright);
        const std::shared_ptr<JointPlateElement> joint = JointPlateElement::top_side_annen_tenons(30.0);
        joint->orient(contact, {base, upright});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, upright, joint->interaction(0));
        scene.add_interaction(joint, base, joint->interaction(1));

        const Vector apart =
            (upright->polylines[0].center() - base->polylines[0].center()).normalized() * 120.0;
        upright->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
top_side_annen_tenons: the through-tenons of Christopher Robeller's 2017 Annen plugin on an upright plate standing on a base plate, the middle 80% of its edge in 4m + 1 segments with a tenon on every fourth, two tenons on the 250 mm edge with 30 mm tabs, through slots in the base, on the pairs upright and leaning to 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_top_side_annen_tenons --parallel 6 && ./build/element_joint_plate_top_side_annen_tenons && ../bash/publish-scene.sh --target element_joint_plate_top_side_annen_tenons

|||||||| TIME ||||||||
0.52 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
